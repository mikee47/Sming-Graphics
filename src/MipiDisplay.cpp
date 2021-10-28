#include "include/Graphics/MipiDisplay.h"
#include <Digital.h>
#include <Platform/System.h>

namespace Graphics
{
namespace
{
// Reading GRAM returns one byte per pixel for R/G/B (only top 6 bits are used, bottom 2 are clear)
static constexpr size_t READ_PIXEL_SIZE{3};

/**
 * @brief Manages completion of a `readDataBuffer` operation
 *
 * Performs format conversion, invokes callback (if provided) then releases shared buffer.
 *
 * @note Data is read back in RGB24 format, but written in RGB565.
 */
struct ReadPixelInfo {
	ReadBuffer buffer;
	size_t bytesToRead;
	ReadStatus* status;
	Surface::ReadCallback callback;
	void* param;

	static void IRAM_ATTR transferCallback(void* param)
	{
		System.queueCallback(taskCallback, param);
	}

	static void taskCallback(void* param)
	{
		auto info = static_cast<ReadPixelInfo*>(param);
		info->readComplete();
	}

	void readComplete()
	{
		if(buffer.format != PixelFormat::RGB24) {
			auto srcptr = &buffer.data[buffer.offset];
			auto dstptr = srcptr;
			if(buffer.format == PixelFormat::RGB565) {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb565.r = *srcptr++ >> 3;
					buf.rgb565.g = *srcptr++ >> 2;
					buf.rgb565.b = *srcptr++ >> 3;
					*dstptr++ = buf.u8[1];
					*dstptr++ = buf.u8[0];
				}
			} else {
				for(size_t i = 0; i < bytesToRead; i += READ_PIXEL_SIZE) {
					PixelBuffer buf;
					buf.rgb24.r = *srcptr++;
					buf.rgb24.g = *srcptr++;
					buf.rgb24.b = *srcptr++;
					dstptr += writeColor(dstptr, buf.color, buffer.format);
				}
			}
			bytesToRead = dstptr - &buffer.data[buffer.offset];
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

} // namespace

using namespace Mipi;

/*
 * MipiDisplay
 */

const SpiDisplayList::Commands MipiDisplay::commands{
	.setColumn = DCS_SET_COLUMN_ADDRESS,
	.setRow = DCS_SET_PAGE_ADDRESS,
	.readStart = DCS_READ_MEMORY_START,
	.read = DCS_READ_MEMORY_CONTINUE,
	.writeStart = DCS_WRITE_MEMORY_START,
};

uint32_t MipiDisplay::readRegister(uint8_t cmd, uint8_t byteCount)
{
	HSPI::Request req;
	req.setCommand8(cmd);
	req.dummyLen = (byteCount > 2);
	req.in.set32(0, byteCount);
	execute(req);
	return req.in.data32;
}

bool MipiDisplay::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin, uint32_t clockSpeed)
{
	if(!SpiDisplay::begin(pinSet, chipSelect, resetPin, clockSpeed)) {
		return false;
	}

	this->dcPin = dcPin;
	pinMode(dcPin, OUTPUT);
	digitalWrite(dcPin, HIGH);
	dcState = true;
	onTransfer(transferBeginEnd);

	return initialise() && setOrientation(orientation);
}

Surface* MipiDisplay::createSurface(size_t bufferSize)
{
	return new MipiSurface(*this, bufferSize ?: 512U);
}

bool MipiDisplay::setOrientation(Orientation orientation)
{
	uint8_t mode = defaultAddressMode;
	switch(orientation) {
	case Orientation::deg0:
		addrOffset = Point{};
		break;
	case Orientation::deg90:
		mode ^= DCS_ADDRESS_MODE_MIRROR_X | DCS_ADDRESS_MODE_SWAP_XY;
		addrOffset = Point{};
		break;
	case Orientation::deg180:
		mode ^= DCS_ADDRESS_MODE_MIRROR_X | DCS_ADDRESS_MODE_MIRROR_Y;
		addrOffset = Point(0, resolution.h - nativeSize.h);
		break;
	case Orientation::deg270:
		mode ^= DCS_ADDRESS_MODE_SWAP_XY | DCS_ADDRESS_MODE_MIRROR_Y;
		addrOffset = Point(resolution.h - nativeSize.h, 0);
		break;
	default:
		return false;
	}

	SpiDisplayList list(commands, addrWindow, 16);
	list.writeCommand(DCS_SET_ADDRESS_MODE, mode, 1);
	execute(list);
	this->orientation = orientation;
	return true;
}

// Protected methods

bool IRAM_ATTR MipiDisplay::transferBeginEnd(HSPI::Request& request)
{
	if(request.busy) {
		auto device = reinterpret_cast<MipiDisplay*>(request.device);
		auto newState = (request.cmdLen == 0);
		if(device->dcState != newState) {
			digitalWrite(device->dcPin, newState);
			device->dcState = newState;
		}
	}
	return true;
}

bool MipiDisplay::setScrollMargins(uint16_t top, uint16_t bottom)
{
	// TFA + VSA + BFA == 320
	if(top + bottom > resolution.h) {
		return false;
	}
	uint16_t data[]{
		swapBytes(top),
		swapBytes(resolution.h - (top + bottom)),
		swapBytes(bottom),
	};
	SpiDisplayList list(commands, addrWindow, 16);
	list.writeCommand(Mipi::DCS_SET_SCROLL_AREA, data, sizeof(data));
	execute(list);
	return true;
}

bool MipiDisplay::scroll(int16_t y)
{
	y = scrollOffset - y;
	while(y < 0) {
		y += resolution.h;
	}
	y %= resolution.h;
	SpiDisplayList list(commands, addrWindow, 16);
	list.writeCommand(Mipi::DCS_SET_SCROLL_START, swapBytes(y), 2);
	execute(list);
	scrollOffset = y;
	return true;
}

/*
 * MipiSurface
 */

MipiSurface::MipiSurface(MipiDisplay& display, size_t bufferSize)
	: display(display), displayList(MipiDisplay::commands, display.getAddressWindow(), bufferSize)
{
}

/*
	 * So far tested only on ILI9341 displays. Other MIPI displays look very similar.
	 *
	 * The ILI9341 is fussy when reading GRAM:
	 *
	 *  - Pixels are read in RGB24 format, but written in RGB565.
	 * 	- The RAMRD command resets the read position to the start of the address window
	 *    so is used only for the first packet
	 *  - Second and subsequent packets use the RAMRD_CONT command
	 *  - Pixels must not be split across SPI packets so each packet can be for a maximum of 63 bytes (21 pixels)
	 */
int MipiSurface::readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param)
{
	// RAM read transactions must be in multiples of 3 bytes
	static constexpr size_t packetPixelBytes{63};

	auto pixelCount = (buffer.size() - buffer.offset) / READ_PIXEL_SIZE;
	if(pixelCount == 0) {
		debug_w("[readDataBuffer] pixelCount == 0");
		return 0;
	}
	auto& addrWindow = display.getAddressWindow();
	if(addrWindow.bounds.h == 0) {
		debug_w("[readDataBuffer] addrWindow.bounds.h == 0");
		return 0;
	}

	constexpr size_t hdrsize = DisplayList::codelen_readStart + DisplayList::codelen_read +
							   DisplayList::codelen_callback + sizeof(ReadPixelInfo);
	if(!displayList.require(hdrsize)) {
		debug_w("[readDataBuffer] no space");
		return -1;
	}
	if(!displayList.canLockBuffer()) {
		return -1;
	}
	if(buffer.format == PixelFormat::None) {
		buffer.format = PixelFormat::RGB24;
	}
	size_t maxPixels = (addrWindow.bounds.w * addrWindow.bounds.h) - addrWindow.column;
	pixelCount = std::min(maxPixels, pixelCount);
	ReadPixelInfo info{buffer, pixelCount * READ_PIXEL_SIZE, status, callback, param};
	if(status != nullptr) {
		*status = ReadStatus{};
	}

	auto bufptr = &buffer.data[buffer.offset];
	if(addrWindow.mode == AddressWindow::Mode::read) {
		displayList.readMem(bufptr, info.bytesToRead);
	} else {
		auto len = std::min(info.bytesToRead, packetPixelBytes);
		displayList.readMem(bufptr, len);
		if(len < info.bytesToRead) {
			displayList.readMem(bufptr + len, info.bytesToRead - len);
		}
	}
	addrWindow.seek(pixelCount);

	info.buffer.data.addRef();
	if(!displayList.writeCallback(info.transferCallback, &info, sizeof(info))) {
		debug_e("[DL] CALLBACK NO SPACE");
	}

	displayList.lockBuffer(buffer.data);
	return pixelCount;
}

bool MipiSurface::render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer)
{
	// Small fills can be handled without using a renderer
	auto isSmall = [](const Rect& r) -> bool {
		constexpr size_t maxFillPixels{32};
		return (r.w * r.h) <= maxFillPixels;
	};

	switch(object.kind()) {
	case Object::Kind::FilledRect: {
		// Handle small transparent fills using display list
		auto obj = reinterpret_cast<const FilledRectObject&>(object);
		if(obj.radius != 0 || !obj.brush.isTransparent() || !isSmall(obj.rect)) {
			break;
		}
		auto color = obj.brush.getPackedColor(PixelFormat::RGB565);
		constexpr size_t bytesPerPixel{2};
		Rect absRect = obj.rect + location.topLeft();
		if(!absRect.clip(getSize())) {
			return true;
		}
		// debug_i("[ILI] HWBLEND (%s), %s", absRect.toString().c_str(), toString(color).c_str());
		return displayList.fill(absRect, color, bytesPerPixel, FillInfo::callbackRGB565);
	}
	default:;
	}

	return Surface::render(object, location, renderer);
}

bool MipiSurface::present(PresentCallback callback, void* param)
{
	if(displayList.isBusy()) {
		debug_e("displayList BUSY, surface %p", this);
		return true;
	}
	if(displayList.isEmpty()) {
		// debug_d("displayList EMPTY, surface %p", this);
		return false;
	}
	display.execute(displayList, callback, param);
	return true;
}

} // namespace Graphics
