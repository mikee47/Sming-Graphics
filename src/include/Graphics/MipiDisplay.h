/****
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

#include "SpiDisplay.h"
#include "Mipi.h"

namespace Graphics
{
class MipiSurface;

class MipiDisplay : public SpiDisplay
{
public:
	static const SpiDisplayList::Commands commands;

	using SpiDisplay::SpiDisplay;

	MipiDisplay(HSPI::Controller& spi, Size resolution, Size screenSize)
		: SpiDisplay(spi), resolution(resolution), nativeSize(screenSize)
	{
	}

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin = PIN_NONE,
			   uint32_t clockSpeed = 4000000, ExecuteDone callback = nullptr);

	using HSPI::Device::getSpeed;

	uint32_t readRegister(uint8_t cmd, uint8_t byteCount);

	uint32_t readDisplayId()
	{
		return readRegister(Mipi::DCS_GET_DISPLAY_ID, 4) >> 8;
	}

	uint32_t readDisplayStatus()
	{
		return readRegister(Mipi::DCS_GET_DISPLAY_STATUS, 4);
	}

	uint8_t readPowerMode()
	{
		return readRegister(Mipi::DCS_GET_POWER_MODE, 1);
	}

	uint8_t readMADCTL()
	{
		return readRegister(Mipi::DCS_GET_ADDRESS_MODE, 1);
	}

	uint8_t readPixelFormat()
	{
		return readRegister(Mipi::DCS_GET_PIXEL_FORMAT, 1);
	}

	uint8_t readImageFormat()
	{
		return readRegister(Mipi::DCS_GET_DISPLAY_MODE, 1);
	}

	uint8_t readSignalMode()
	{
		return readRegister(Mipi::DCS_GET_DISPLAY_MODE, 1);
	}

	uint8_t readSelfDiag()
	{
		return readRegister(Mipi::DCS_GET_DIAGNOSTIC_RESULT, 1);
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

	Size getResolution() const
	{
		return resolution;
	}

	bool setScrollMargins(uint16_t top, uint16_t bottom) override;
	bool scroll(int16_t y) override;

	/* Device */

	bool setOrientation(Orientation orientation) override;

	/* RenderTarget */

	Size getSize() const override
	{
		return rotate(nativeSize, orientation);
	}

	PixelFormat getPixelFormat() const override
	{
		return PixelFormat::RGB565;
	}

	// Used by Surface to adjust for screen orientation
	Point getAddrOffset() const
	{
		return addrOffset;
	}

	Surface* createSurface(size_t bufferSize = 0) override;

	uint16_t getScrollOffset() const
	{
		return scrollOffset;
	}

protected:
	/**
	 * @brief Perform display-specific initialisation
	 * @retval bool true on success, false on failure
	 */
	virtual bool initialise() = 0;

	/**
	 * @brief Called by implementation to send fixed initialisation sequences
	 */
	void sendInitData(const FSTR::ObjectBase& data)
	{
		SpiDisplay::execute(commands, data);
	}

	/**
	 * @brief Set default address mode setting
	 *
	 * The display may be attached to the controller in various ways,
	 * resulting in a flipped or rotated display.
	 *
	 * Changing the default mode allows this to be corrected.
	 *
	 * For example, if the display is flipped horizontally, use:
	 * 
	 * 		setDefaultAddressMode(Graphics::Mipi::DCS_ADDRESS_MODE_MIRROR_X);
	 *
	 * Flag values may be combined, for example:
	 *
	 * 		setDefaultAddressMode(
	 * 			Graphics::Mipi::DCS_ADDRESS_MODE_MIRROR_X |
	 * 			Graphics::Mipi::DCS_ADDRESS_MODE_MIRROR_Y
	 * 		);
	 *
	 */
	void setDefaultAddressMode(uint8_t mode)
	{
		mode |= Mipi::DCS_ADDRESS_MODE_BGR;
		if(mode == defaultAddressMode) {
			return;
		}

		defaultAddressMode = mode;
		if(isReady()) {
			setOrientation(orientation);
		}
	}

	Size resolution{};  ///< Controller resolution
	Size nativeSize{};  ///< Size of attached screen
	Point addrOffset{}; ///< Display orientation may require adjustment to address window position
	uint8_t defaultAddressMode{Mipi::DCS_ADDRESS_MODE_BGR};

private:
	static bool transferBeginEnd(HSPI::Request& request);

	uint8_t dcPin{PIN_NONE};
	bool dcState{};
	uint16_t scrollOffset{0};
};

class MipiSurface : public Graphics::Surface
{
public:
	MipiSurface(MipiDisplay& display, size_t bufferSize);

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
		Rect r = rect;
		r.y -= display.getScrollOffset();
		r += display.getAddrOffset();
		while(r.y < 0) {
			r.y += display.getResolution().h;
		}
		r.y %= display.getResolution().h;
		return displayList.setAddrWindow(r);
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

	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param) override;
	bool render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer) override;
	bool present(PresentCallback callback, void* param) override;

protected:
	MipiDisplay& display;
	SpiDisplayList displayList;
};

} // namespace Graphics
