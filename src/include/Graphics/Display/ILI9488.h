/****
 * ILI9488.h
 *
 * Copyright 2023 mikee47 <mike@sillyhouse.net>
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

#include "../MipiDisplay.h"

namespace Graphics
{
namespace Display
{
class ILI9488 : public MipiDisplay
{
public:
	static constexpr Size resolution{320, 480};

	ILI9488(HSPI::Controller& spi, Size screenSize = resolution) : MipiDisplay(spi, resolution, screenSize)
	{
		setDefaultAddressMode(Mipi::DCS_ADDRESS_MODE_MIRROR_X);
	}

	uint16_t readNvMemStatus();

	/* Device */

	String getName() const override
	{
		return F("ILI9488");
	}

	PixelFormat getPixelFormat() const override
	{
		// Only 18-bit color supported in SPI mode
		return PixelFormat::RGB24;
	}

protected:
	bool initialise() override;
};

} // namespace Display
} // namespace Graphics
