/**
 * Mipi.h
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
 ****/

#pragma once

#include "../SpiDisplay.h"
#include <Digital.h>

namespace Graphics
{
namespace Display
{
namespace Mipi
{
/* Defines for Mobile Industry Processor Interface (MIPI)  */

/* MIPI Display Serial Interface (DSI) commands. See: https://en.wikipedia.org/wiki/Display_Serial_Interface */
enum SerialInterfaceCommand {
	DSI_V_SYNC_START = 0x01,
	DSI_V_SYNC_END = 0x11,
	DSI_H_SYNC_START = 0x21,
	DSI_H_SYNC_END = 0x31,

	DSI_COMPRESSION_MODE = 0x07,
	DSI_END_OF_TRANSMISSION = 0x08,

	DSI_COLOR_MODE_OFF = 0x02,
	DSI_COLOR_MODE_ON = 0x12,
	DSI_SHUTDOWN_PERIPHERAL = 0x22,
	DSI_TURN_ON_PERIPHERAL = 0x32,

	DSI_GENERIC_SHORT_WRITE_0_PARAM = 0x03,
	DSI_GENERIC_SHORT_WRITE_1_PARAM = 0x13,
	DSI_GENERIC_SHORT_WRITE_2_PARAM = 0x23,

	DSI_GENERIC_READ_REQUEST_0_PARAM = 0x04,
	DSI_GENERIC_READ_REQUEST_1_PARAM = 0x14,
	DSI_GENERIC_READ_REQUEST_2_PARAM = 0x24,

	DSI_DCS_SHORT_WRITE = 0x05,
	DSI_DCS_SHORT_WRITE_PARAM = 0x15,

	DSI_DCS_READ = 0x06,
	DSI_EXECUTE_QUEUE = 0x16,

	DSI_SET_MAXIMUM_RETURN_PACKET_SIZE = 0x37,

	DSI_NULL_PACKET = 0x09,
	DSI_BLANKING_PACKET = 0x19,
	DSI_GENERIC_LONG_WRITE = 0x29,
	DSI_DCS_LONG_WRITE = 0x39,

	DSI_PICTURE_PARAMETER_SET = 0x0a,
	DSI_COMPRESSED_PIXEL_STREAM = 0x0b,

	DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20 = 0x0c,
	DSI_PACKED_PIXEL_STREAM_YCBCR24 = 0x1c,
	DSI_PACKED_PIXEL_STREAM_YCBCR16 = 0x2c,

	DSI_PACKED_PIXEL_STREAM_30 = 0x0d,
	DSI_PACKED_PIXEL_STREAM_36 = 0x1d,
	DSI_PACKED_PIXEL_STREAM_YCBCR12 = 0x3d,

	DSI_PACKED_PIXEL_STREAM_16 = 0x0e,
	DSI_PACKED_PIXEL_STREAM_18 = 0x1e,
	DSI_PIXEL_STREAM_3BYTE_18 = 0x2e,
	DSI_PACKED_PIXEL_STREAM_24 = 0x3e,
};

/* MIPI DSI Peripheral-to-Processor transaction types */
enum SerialTransactionType {
	DSI_RX_ACKNOWLEDGE_AND_ERROR_REPORT = 0x02,
	DSI_RX_END_OF_TRANSMISSION = 0x08,
	DSI_RX_GENERIC_SHORT_READ_RESPONSE_1BYTE = 0x11,
	DSI_RX_GENERIC_SHORT_READ_RESPONSE_2BYTE = 0x12,
	DSI_RX_GENERIC_LONG_READ_RESPONSE = 0x1a,
	DSI_RX_DCS_LONG_READ_RESPONSE = 0x1c,
	DSI_RX_DCS_SHORT_READ_RESPONSE_1BYTE = 0x21,
	DSI_RX_DCS_SHORT_READ_RESPONSE_2BYTE = 0x22,
};

/* MIPI Display Command Set (DCS).
 *
 * The Display Command Set (DCS) is a set of common commands for controlling the display device,
 * and their format is specified by the DSI standard. It defines registers that can be addressed and what their operation is.
 * It includes basic commands such as sleep, enable, and invert display.
 *
 * See: https://en.wikipedia.org/wiki/Display_Serial_Interface
 * See: https://www.mipi.org/specifications/display-command-set
 */
enum DisplayCommandSet {
	DCS_NOP = 0x00,
	DCS_SOFT_RESET = 0x01,
	DCS_GET_COMPRESSION_MODE = 0x03,
	DCS_GET_DISPLAY_ID = 0x04,
	DCS_GET_ERROR_COUNT_ON_DSI = 0x05,
	DCS_GET_RED_CHANNEL = 0x06,
	DCS_GET_GREEN_CHANNEL = 0x07,
	DCS_GET_BLUE_CHANNEL = 0x08,
	DCS_GET_DISPLAY_STATUS = 0x09,
	DCS_GET_POWER_MODE = 0x0A,
	DCS_GET_ADDRESS_MODE = 0x0B,
	DCS_GET_PIXEL_FORMAT = 0x0C,
	DCS_GET_DISPLAY_MODE = 0x0D,
	DCS_GET_SIGNAL_MODE = 0x0E,
	DCS_GET_DIAGNOSTIC_RESULT = 0x0F,
	DCS_ENTER_SLEEP_MODE = 0x10,
	DCS_EXIT_SLEEP_MODE = 0x11,
	DCS_ENTER_PARTIAL_MODE = 0x12,
	DCS_ENTER_NORMAL_MODE = 0x13,
	DCS_GET_IMAGE_CHECKSUM_RGB = 0x14,
	DCS_GET_IMAGE_CHECKSUM_CT = 0x15,
	DCS_EXIT_INVERT_MODE = 0x20,
	DCS_ENTER_INVERT_MODE = 0x21,
	DCS_SET_GAMMA_CURVE = 0x26,
	DCS_SET_DISPLAY_OFF = 0x28,
	DCS_SET_DISPLAY_ON = 0x29,
	DCS_SET_COLUMN_ADDRESS = 0x2A,
	DCS_SET_PAGE_ADDRESS = 0x2B,
	DCS_WRITE_MEMORY_START = 0x2C,
	DCS_WRITE_LUT = 0x2D,
	DCS_READ_MEMORY_START = 0x2E,
	DCS_SET_PARTIAL_ROWS = 0x30, /* MIPI DCS 1.02 - DCS_SET_PARTIAL_AREA before that */
	DCS_SET_PARTIAL_COLUMNS = 0x31,
	DCS_SET_SCROLL_AREA = 0x33,
	DCS_SET_TEAR_OFF = 0x34,
	DCS_SET_TEAR_ON = 0x35,
	DCS_SET_ADDRESS_MODE = 0x36,
	DCS_SET_SCROLL_START = 0x37,
	DCS_EXIT_IDLE_MODE = 0x38,
	DCS_ENTER_IDLE_MODE = 0x39,
	DCS_SET_PIXEL_FORMAT = 0x3A,
	DCS_WRITE_MEMORY_CONTINUE = 0x3C,
	DCS_SET_3D_CONTROL = 0x3D,
	DCS_READ_MEMORY_CONTINUE = 0x3E,
	DCS_GET_3D_CONTROL = 0x3F,
	DCS_SET_VSYNC_TIMING = 0x40,
	DCS_SET_TEAR_SCANLINE = 0x44,
	DCS_GET_SCANLINE = 0x45,
	DCS_SET_DISPLAY_BRIGHTNESS = 0x51,  /* MIPI DCS 1.3 */
	DCS_GET_DISPLAY_BRIGHTNESS = 0x52,  /* MIPI DCS 1.3 */
	DCS_WRITE_CONTROL_DISPLAY = 0x53,   /* MIPI DCS 1.3 */
	DCS_GET_CONTROL_DISPLAY = 0x54,		/* MIPI DCS 1.3 */
	DCS_WRITE_POWER_SAVE = 0x55,		/* MIPI DCS 1.3 */
	DCS_GET_POWER_SAVE = 0x56,			/* MIPI DCS 1.3 */
	DCS_SET_CABC_MIN_BRIGHTNESS = 0x5E, /* MIPI DCS 1.3 */
	DCS_GET_CABC_MIN_BRIGHTNESS = 0x5F, /* MIPI DCS 1.3 */
	DCS_READ_DDB_START = 0xA1,
	DCS_READ_PPS_START = 0xA2,
	DCS_READ_DDB_CONTINUE = 0xA8,
	DCS_READ_PPS_CONTINUE = 0xA9,
};

/* MIPI DCS pixel formats */
enum DcsPixelFormat {
	DCS_PIXEL_FMT_24BIT = 7,
	DCS_PIXEL_FMT_18BIT = 6,
	DCS_PIXEL_FMT_16BIT = 5,
	DCS_PIXEL_FMT_12BIT = 3,
	DCS_PIXEL_FMT_8BIT = 2,
	DCS_PIXEL_FMT_3BIT = 1,
};

const SpiDisplayList::Commands commands{
	.setColumn = DCS_SET_COLUMN_ADDRESS,
	.setRow = DCS_SET_PAGE_ADDRESS,
	.readStart = DCS_READ_MEMORY_START,
	.read = DCS_READ_MEMORY_CONTINUE,
	.writeStart = DCS_WRITE_MEMORY_START,
};

class Base : public SpiDisplay
{
public:
	using SpiDisplay::SpiDisplay;

	Base(HSPI::Controller& spi, Size screenSize) : SpiDisplay(spi), nativeSize(screenSize)
	{
	}

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin = PIN_NONE,
			   uint32_t clockSpeed = 4000000);

	using HSPI::Device::getSpeed;

	uint32_t readRegister(uint8_t cmd, uint8_t byteCount);

	uint32_t readDisplayId()
	{
		return readRegister(DCS_GET_DISPLAY_ID, 4) >> 8;
	}

	uint32_t readDisplayStatus()
	{
		return readRegister(DCS_GET_DISPLAY_STATUS, 4);
	}

	uint8_t readPowerMode()
	{
		return readRegister(DCS_GET_POWER_MODE, 1);
	}

	uint8_t readMADCTL()
	{
		return readRegister(DCS_GET_ADDRESS_MODE, 1);
	}

	uint8_t readPixelFormat()
	{
		return readRegister(DCS_GET_PIXEL_FORMAT, 1);
	}

	uint8_t readImageFormat()
	{
		return readRegister(DCS_GET_DISPLAY_MODE, 1);
	}

	uint8_t readSignalMode()
	{
		return readRegister(DCS_GET_DISPLAY_MODE, 1);
	}

	uint8_t readSelfDiag()
	{
		return readRegister(DCS_GET_DIAGNOSTIC_RESULT, 1);
	}

	/**
	 * @brief Sets the screen size. Must be called before calling ::begin()
	 */
	void setNativeSize(Size screenSize)
	{
		nativeSize = screenSize;
	}

	Size getNativeSize() const override
	{
		return nativeSize;
	}

	/* Device */
	// bool setOrientation(Orientation orientation) override;

	/* RenderTarget */
	Size getSize() const override
	{
		return rotate(nativeSize, orientation);
	}

	// Surface* createSurface(size_t bufferSize = 0) override;

protected:
	/**
	 * @brief Perform display-specific initialisation
	 * @retval bool true on success, false on failure
	 */
	virtual bool initialise() = 0;

	Size nativeSize{0, 0};

private:
	static bool transferBeginEnd(HSPI::Request& request);

	uint8_t dcPin{PIN_NONE};
	bool dcState{};
};

class Surface : public Graphics::Surface
{
public:
	Surface(Base& display, size_t bufferSize)
		: display(display), displayList(commands, display.getAddressWindow(), bufferSize)
	{
	}

	Type getType() const
	{
		return Type::Device;
	}

	Stat stat() const override
	{
		return Stat{
			.used = displayList.used(),
			.available = displayList.freeSpace(),
		};
	}

	void reset() override
	{
		displayList.reset();
	}

	Size getSize() const override
	{
		return display.getSize();
	}

	PixelFormat getPixelFormat() const override
	{
		return display.getPixelFormat();
	}

	bool setAddrWindow(const Rect& rect) override
	{
		return displayList.setAddrWindow(rect);
	}

	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) override
	{
		return displayList.getBuffer(minBytes, available);
	}

	void commit(uint16_t length) override
	{
		displayList.commit(length);
	}

	bool blockFill(const void* data, uint16_t length, uint32_t repeat) override
	{
		return displayList.blockFill(data, length, repeat);
	}

	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length) override
	{
		return displayList.writeDataBuffer(data, offset, length);
	}

	bool setPixel(PackedColor color, Point pt) override
	{
		return displayList.setPixel(color, 2, pt);
	}

	//	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param) override;

	bool render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer) override
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

		return Graphics::Surface::render(object, location, renderer);
	}

	bool present(PresentCallback callback, void* param) override
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

protected:
	Base& display;
	SpiDisplayList displayList;
};

} // namespace Mipi
} // namespace Display
} // namespace Graphics
