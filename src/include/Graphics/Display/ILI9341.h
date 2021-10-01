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

#include "Mipi.h"

namespace Graphics
{
namespace Display
{
class ILI9341Surface;

class ILI9341 : public Mipi::Base
{
public:
	static constexpr Size nativeSize{240, 320};

	using Mipi::Base::Base;

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin = PIN_NONE,
			   uint32_t clockSpeed = 4000000) override;

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

protected:
	friend class ILI9341Surface;
};

} // namespace Display
} // namespace Graphics
