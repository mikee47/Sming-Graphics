/**
 * Null.cpp
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
 * @author: June 2021 - mikee47 <mike@sillyhouse.net>
 *
 * This code contains some reworked code from the Adafruit_ILI9341 display driver.
 * 
 * See https://github.com/adafruit/Adafruit_ILI9341
 *
 ****/

#include <Graphics/Display/Null.h>
#include <Platform/System.h>

namespace Graphics
{
namespace Display
{
class NullSurface : public Surface
{
public:
	NullSurface(NullDevice& device, uint16_t bufferSize) : device(device), bufferSize(bufferSize)
	{
		buffer.reset(new uint8_t[bufferSize]);
	}

	Type getType() const
	{
		return Type::Device;
	}

	Stat stat() const override
	{
		return Stat{
			.used = 0,
			.available = 0xffff,
		};
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
		return true;
	}

	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) override
	{
		available = bufferSize;
		return (available >= minBytes) ? buffer.get() : nullptr;
	}

	void commit(uint16_t length) override
	{
		(void)length;
		assert(length <= bufferSize);
	}

	bool blockFill(const void* data, uint16_t length, uint32_t repeat) override
	{
		(void)data;
		(void)length;
		(void)repeat;
		return true;
	}

	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length) override
	{
		(void)data;
		(void)offset;
		(void)length;
		return true;
	}

	bool setPixel(PackedColor color, Point pt) override
	{
		(void)color;
		(void)pt;
		return true;
	}

	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param)
	{
		auto& addrWindow = device.addrWindow;
		auto sz = addrWindow.bounds.size();
		size_t pixelCount = (sz.w * sz.h) - addrWindow.column;
		uint8_t bpp = std::max(getBytesPerPixel(buffer.format), getBytesPerPixel(getPixelFormat()));
		pixelCount = std::min(pixelCount, buffer.size() / bpp);
		assert(pixelCount != 0);
		if(pixelCount == 0) {
			return 0;
		}

		addrWindow.seek(pixelCount);

		if(buffer.format == PixelFormat::None) {
			buffer.format = getPixelFormat();
		}

		auto length = pixelCount * getBytesPerPixel(buffer.format);
		os_get_random(buffer.data.get(), length);
		if(status != nullptr) {
			*status = ReadStatus{length, buffer.format, true};
		}

		if(callback) {
			System.queueCallback([&]() { callback(buffer, length, param); });
		}

		return pixelCount;
	}

	void reset() override
	{
	}

	bool present(PresentCallback callback, void* param) override
	{
		return System.queueCallback(callback, param);
	}

private:
	NullDevice& device;
	std::unique_ptr<uint8_t[]> buffer;
	size_t bufferSize;
};

Surface* NullDevice::createSurface(size_t bufferSize)
{
	return new NullSurface(*this, bufferSize ?: 512U);
}

} // namespace Display
} // namespace Graphics
