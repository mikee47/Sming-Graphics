/**
 * ILI9341.h
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

#pragma once

#include "../SpiDisplay.h"

namespace Graphics
{
namespace Display
{
class ILI9341Surface;

class ILI9341 : public SpiDisplay
{
public:
	static constexpr Size nativeSize{240, 320};

	using SpiDisplay::SpiDisplay;

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin = PIN_NONE,
			   uint32_t clockSpeed = 4000000);

	uint32_t readRegister(uint8_t cmd, uint8_t byteCount);

	uint32_t readDisplayId();
	uint32_t readDisplayStatus();
	uint8_t readPowerMode();
	uint8_t readMADCTL();
	uint8_t readPixelFormat();
	uint8_t readImageFormat();
	uint8_t readSignalMode();
	uint8_t readSelfDiag();
	uint16_t readNvMemStatus();

	/* Device */

	String getName() const override
	{
		return F("ILI9341");
	}

	Size getNativeSize() const override
	{
		return nativeSize;
	}

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

	Surface* createSurface(size_t bufferSize = 0) override;

private:
	friend class ILI9341Surface;

	static bool transferBeginEnd(HSPI::Request& request);

	uint8_t dcPin{PIN_NONE};
	bool dcState{};
};

} // namespace Display
} // namespace Graphics
