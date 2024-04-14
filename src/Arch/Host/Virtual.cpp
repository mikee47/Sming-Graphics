/**
 * Virtual.cpp
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming-Graphics Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author: May 2021 - mikee47 <mike@sillyhouse.net>
 *
 ****/

#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS

#include <hostlib/sockets.h>
#include <Graphics/Display/Virtual.h>
#include <Graphics/DisplayList.h>
#include <Platform/System.h>
#include <hostlib/threads.h>
#include <hostlib/CommandLine.h>
#include <queue>

namespace Graphics
{
namespace Display
{
namespace
{
constexpr uint8_t BYTES_PER_PIXEL{3};

union Command {
	struct SetSize {
		static constexpr uint8_t command{0};
		Size size;
	};
	struct CopyPixels {
		static constexpr uint8_t command{1};
		Rect source;
		Point dest;
	};
	struct Scroll {
		static constexpr uint8_t command{2};
		Rect area;
		Point shift;
		bool wrapx;
		bool wrapy;
		PackedColor fill;
	};
	struct Fill {
		static constexpr uint8_t command{3};
		Rect r;
		PackedColor color;
	};
	struct SetScrollMargins {
		static constexpr uint8_t command{4};
		uint16_t top;
		uint16_t bottom;
	};
	struct SetScrollOffset {
		static constexpr uint8_t command{5};
		uint16_t offset;
	};
};

struct ReadPixelInfo {
	ReadBuffer buffer;
	size_t bytesToRead;
	ReadStatus* status;
	Surface::ReadCallback callback;
	void* param;

	static void transferCallback(void* param)
	{
		auto info = new ReadPixelInfo(*static_cast<ReadPixelInfo*>(param));
		System.queueCallback(
			[](void* param) {
				auto info = static_cast<ReadPixelInfo*>(param);
				info->readComplete();
				// memset(info, 0, sizeof(*info));
				delete info;
			},
			info);
	}

	void readComplete()
	{
		// debug_i("readComplete()");
		if(buffer.format != PixelFormat::BGR24) {
			auto src = buffer.data.get();
			auto dst = src;
			for(unsigned i = 0; i < bytesToRead; i += BYTES_PER_PIXEL) {
				PixelBuffer buf;
				buf.bgra32.b = *src++;
				buf.bgra32.g = *src++;
				buf.bgra32.r = *src++;
				dst += writeColor(dst, buf.color, buffer.format);
			}
			bytesToRead = dst - buffer.data.get();
		}
		if(status != nullptr) {
			*status = ReadStatus{bytesToRead, buffer.format, true};
		}
		if(callback) {
			callback(buffer, bytesToRead, param);
		}
		buffer.data.release();
	}
};

class CommandList : public DisplayList
{
public:
	class Queue : private std::queue<CommandList*>
	{
	public:
		void push(CommandList& list)
		{
			mutex.lock();
			queue::push(&list);
			mutex.unlock();
		}

		CommandList* pop()
		{
			mutex.lock();
			CommandList* list{nullptr};
			if(!queue::empty()) {
				list = queue::front();
				queue::pop();
			}
			mutex.unlock();
			return list;
		}

	private:
		CMutex mutex;
	};

	using DisplayList::DisplayList;

	enum class State {
		idle,
		pending,
		running,
		failed,
	};

	bool isBusy() const
	{
		return state != State::idle;
	}

	template <typename T> bool writeCommand(const T& param)
	{
		return DisplayList::writeCommand(uint8_t(T::command), &param, sizeof(param));
	}

	void prepare(Callback callback = nullptr, void* param = nullptr)
	{
		// debug_i("%p %s(), offset %u, size %u", this, __FUNCTION__, offset, size);
		assert(state == CommandList::State::idle);
		state = CommandList::State::pending;
		DisplayList::prepare(callback, param);
	}

	void execute()
	{
		// debug_i("%p %s(), offset %u, size %u", this, __FUNCTION__, offset, size);
		assert(state == CommandList::State::pending);
		state = CommandList::State::running;
		assert(offset == 0);
	}

	void wait()
	{
		while(!callback && state != CommandList::State::idle) {
			// sched_yield();
		}
	}

	void complete()
	{
		// debug_i("%p %s(), offset %u, size %u", this, __FUNCTION__, offset, size);
		if(callback) {
			System.queueCallback(callback, param);
		}
		state = CommandList::State::idle;
	}

	volatile State state{};
};

} // namespace

class Virtual::NetworkThread : public CThread
{
public:
	NetworkThread(Virtual& screen, const String& ipaddr, uint16_t port)
		: CThread("VirtualScreen", 1), screen(screen), addr(ipaddr.c_str(), port)
	{
		CThread::execute();
	}

	void terminate()
	{
		terminated = true;
		sem.post();
		join();
	}

	void transfer(CommandList& list)
	{
		assert(list.state == CommandList::State::pending);
		queue.push(list);
		sem.post();
		list.wait();
	}

protected:
	struct Header {
		static constexpr uint32_t packetMagic{0x3facbe5a};
		static constexpr uint32_t touchMagic{0x3facbe5b};
		uint32_t magic;
		uint32_t len;

		Header() : magic(0), len(0)
		{
		}

		Header(uint32_t len) : magic(packetMagic), len(len)
		{
		}
	};

	void* thread_routine() override
	{
		CommandList* list{nullptr};

		while(!terminated) {
			if(!socket.active()) {
				debug_i("[VS] Connecting...");
				if(socket.connect(addr)) {
					debug_i("[VS] Connected to %s", socket.addr().text().c_str());
				}
				continue;
			}

			if(list == nullptr) {
				if(sem.timedwait(100000)) {
					list = queue.pop();
				} else if(socket.available()) {
					uint8_t buffer[16];
					readPacket(buffer, sizeof(buffer), false);
				}
				continue;
			}

			if(execute(*list)) {
				list = nullptr;
			}
		}

		socket.close();

		return nullptr;
	}

	bool execute(CommandList& list)
	{
		list.execute();
		if(!sendPacket(list.getContent(), list.used())) {
			debug_e("[EEEEEEEEEEEEK]");
			return false;
		}

		using Code = DisplayList::Code;
		DisplayList::Entry entry;
		size_t SMING_UNUSED offset{0};
		while(list.readEntry(entry)) {
			// host_printf("%p @ %u: %s(0x%x, %u)\r\n", static_cast<DisplayList*>(&list), offset,
			// 			toString(entry.code).c_str(), entry.data, entry.length);
			offset = list.readOffset();
			switch(entry.code) {
			case Code::writeDataBuffer:
				sendPacket(entry.data, entry.length);
				break;
			case Code::readStart:
			case Code::read: {
				size_t len = readPacket(entry.data, entry.length);
				if(len != entry.length) {
					debug_w("[DL] Read got %u, expected %u", len, entry.length);
					return false;
				}
				break;
			}
			case Code::callback:
				entry.callback(entry.data);
				break;
			default:;
			}
		}

		list.complete();
		return true;
	}

	bool sendPacket(const void* data, size_t size)
	{
		// host_printf("[VS] sendPacket %u\r\n", size);

		Header hdr{size};
		if(socket.send(&hdr, sizeof(hdr)) == int(sizeof(hdr))) {
			if(socket.send(data, size) == int(size)) {
				return true;
			}
		}
		debug_e("[VS] Error sending packet");
		socket.close();
		return false;
	}

	size_t readPacket(void* buffer, size_t length, bool waitingForReply = true)
	{
		for(;;) {
			Header hdr{};
			int res = socket.recv(&hdr, sizeof(hdr));
			if(res != int(sizeof(hdr))) {
				debug_e("[VS] Header read failed");
				break;
			}
			if(hdr.magic != hdr.packetMagic && hdr.magic != hdr.touchMagic) {
				debug_e("[VS] Bad magic");
				break;
			}
			if(hdr.len > length) {
				debug_e("[VS] Read buffer too small, have %u require %u", length, hdr.len);
				break;
			}
			res = socket.recv(buffer, hdr.len);
			if(res != int(hdr.len)) {
				debug_e("[VS] Data read failed");
				break;
			}
			if(hdr.magic == hdr.touchMagic) {
				interrupt_begin();
				screen.handleTouch(buffer, hdr.len);
				interrupt_end();
				if(waitingForReply) {
					continue;
				}
			}
			// host_printf("[VS] readPacket %u\r\n", hdr.len);
			return hdr.len;
		}
		socket.close();
		return 0;
	}

private:
	Virtual& screen;
	CSockAddr addr;
	CSocket socket;
	CSemaphore sem; // Signals state change
	CommandList::Queue queue;
	volatile bool terminated{false};
};

class VirtualSurface : public Surface
{
public:
	VirtualSurface(Virtual& device, size_t bufferSize) : device(device), list(device.addrWindow, bufferSize)
	{
	}

	Type getType() const
	{
		return Type::Device;
	}

	Stat stat() const override
	{
		return Stat{
			.used = list.used(),
			.available = list.freeSpace(),
		};
	}

	void reset() override
	{
		list.reset();
	}

	Size getSize() const override
	{
		return device.getSize();
	}

	PixelFormat getPixelFormat() const override
	{
		return device.getPixelFormat();
	}

	bool setAddrWindow(const Rect& rect) override
	{
		device.addrWindow = rect;
		return list.setAddrWindow(rect);
	}

	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) override
	{
		return list.getBuffer(minBytes, available);
	}

	void commit(uint16_t length) override
	{
		list.commit(length);
	}

	bool blockFill(const void* data, uint16_t length, uint32_t repeat) override
	{
		return list.blockFill(data, length, repeat);
	}

	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length) override
	{
		return list.writeDataBuffer(data, offset, length);
	}

	bool setPixel(PackedColor color, Point pt) override
	{
		return list.setPixel(color, 3, pt);
	}

	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param)
	{
		if(buffer.format == PixelFormat::None) {
			buffer.format = PixelFormat::BGR24;
		}
		if(status != nullptr) {
			*status = ReadStatus{};
		}
		uint8_t bpp = std::max(getBytesPerPixel(buffer.format), BYTES_PER_PIXEL);

		auto& addrWindow = device.addrWindow;
		auto sz = addrWindow.bounds.size();
		size_t pixelCount = (sz.w * sz.h) - addrWindow.column;
		if(pixelCount == 0) {
			return 0;
		}
		constexpr size_t hdrsize =
			DisplayList::codelen_readStart + DisplayList::codelen_callback + sizeof(ReadPixelInfo);
		if(!list.require(hdrsize)) {
			debug_w("[readDataBuffer] no space");
			return -1;
		}
		if(!list.canLockBuffer()) {
			return -1;
		}

		pixelCount = std::min(pixelCount, (buffer.size() - buffer.offset) / bpp);
		size_t bytesToRead = pixelCount * BYTES_PER_PIXEL;
		assert(buffer.offset + bytesToRead <= buffer.data.size());
		if(!list.readMem(&buffer.data[buffer.offset], bytesToRead)) {
			return -1;
		}
		addrWindow.seek(pixelCount);
		ReadPixelInfo info{buffer, bytesToRead, status, callback, param};
		list.writeCallback(info.transferCallback, &info, sizeof(info));
		list.lockBuffer(buffer.data);
		buffer.data.addRef();
		return pixelCount;
	}

	bool render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer) override
	{
		if(device.mode == Virtual::Mode::Normal) {
			switch(object.kind()) {
			case Object::Kind::FilledRect: {
				// Handle small transparent fills using display list
				auto obj = static_cast<const FilledRectObject&>(object);
				if(obj.blender || obj.radius != 0 || !obj.brush.isTransparent()) {
					break;
				}
				Rect absRect = obj.rect + location.topLeft();
				if(!absRect.clip(getSize())) {
					return true;
				}
				Command::Fill cmd{absRect, obj.brush.getPackedColor(PixelFormat::BGRA32)};
				return list.writeCommand(cmd);
			}
			case Object::Kind::Copy: {
				auto obj = static_cast<const CopyObject&>(object);
				Command::CopyPixels cmd{obj.source, obj.dest};
				return list.writeCommand(cmd);
			}
			case Object::Kind::Scroll: {
				auto obj = static_cast<const ScrollObject&>(object);
				Command::Scroll cmd{obj.area, obj.shift, obj.wrapx, obj.wrapy, pack(obj.fill, PixelFormat::BGR24)};
				return list.writeCommand(cmd);
			}

			default:;
			}
		}

		return Surface::render(object, location, renderer);
	}

	bool present(PresentCallback callback, void* param) override
	{
		if(list.isBusy()) {
			debug_e("displayList BUSY, surface %p", this);
			return true;
		}
		if(list.isEmpty()) {
			debug_d("displayList EMPTY, surface %p", this);
			return false;
		}
		list.prepare(callback, param);
		device.thread->transfer(list);
		return true;
	}

private:
	Virtual& device;
	CommandList list;
};

Virtual::Virtual()
{
}

Virtual::~Virtual()
{
}

bool Virtual::begin(uint16_t width, uint16_t height)
{
	auto params = commandLine.getParameters();
	auto addr = params.find("vsaddr");
	auto port = params.find("vsport");
	if(!addr || !port) {
		host_printf("Virtual screen requires vsaddr and vsport command-line parameters\r\n");
		return false;
	}

	return begin(addr.getValue(), port.getValue().toInt(), width, height);
}

bool Virtual::begin(const String& ipaddr, uint16_t port, uint16_t width, uint16_t height)
{
	if(thread) {
		thread->terminate();
		thread.reset();
	}

	thread = std::make_unique<NetworkThread>(*this, ipaddr, port);

	nativeSize = Size{width, height};
	return sizeChanged();
}

bool Virtual::sizeChanged()
{
	CommandList list(addrWindow, 32);
	list.writeCommand(Command::SetSize{getSize()});
	list.prepare();
	thread->transfer(list);
	return true;
}

bool Virtual::setOrientation(Orientation orientation)
{
	this->orientation = orientation;
	return sizeChanged();
}

bool Virtual::setScrollMargins(uint16_t top, uint16_t bottom)
{
	if(top + bottom >= nativeSize.h) {
		debug_e("[VS] setScrollMargins(%u, %u) invalid parameters", top, bottom);
		return false;
	}

	debug_d("[VS] setScrollMargins(%u, %u)", top, bottom);

	CommandList list(addrWindow, 32);
	list.writeCommand(Command::SetScrollMargins{top, bottom});
	list.prepare();
	thread->transfer(list);

	return true;
}

void Virtual::setScrollOffset(uint16_t line)
{
	CommandList list(addrWindow, 32);
	list.writeCommand(Command::SetScrollOffset{line});
	list.prepare();
	thread->transfer(list);
}

Surface* Virtual::createSurface(size_t bufferSize)
{
	return new VirtualSurface(*this, bufferSize ?: 512U);
}

} // namespace Display
} // namespace Graphics
