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

#include "../MipiDisplay.h"

namespace Graphics
{
namespace Display
{
class ILI9341 : public MipiDisplay
{
public:
	static constexpr Size resolution{240, 320};

	ILI9341(HSPI::Controller& spi, Size screenSize = resolution) : MipiDisplay(spi, resolution, screenSize)
	{
		setDefaultAddressMode(Mipi::DCS_ADDRESS_MODE_MIRROR_X);
	}

	uint16_t readNvMemStatus();

	/* Device */

	String getName() const override
	{
		return F("ILI9341");
	}

protected:
	bool initialise() override;
};

} // namespace Display
} // namespace Graphics
