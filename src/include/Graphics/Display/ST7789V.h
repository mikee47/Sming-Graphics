/****
 * ST7789V.h
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
 * @author: October 2021 - Slavey Karadzhov <slav@attachix.com>
 *
 ****/

#pragma once

#include "../MipiDisplay.h"

namespace Graphics
{
namespace Display
{
class ST7789V : public MipiDisplay
{
public:
	static constexpr Size resolution{240, 320};

	ST7789V(HSPI::Controller& spi, Size screenSize = resolution) : MipiDisplay(spi, resolution, screenSize)
	{
	}

	uint16_t readNvMemStatus();

	/* Device */

	String getName() const override
	{
		return F("ST7789V");
	}

protected:
	bool initialise() override;
};

} // namespace Display
} // namespace Graphics
