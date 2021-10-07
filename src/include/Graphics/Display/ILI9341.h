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

class ILI9341 : public Mipi::Base
{
public:
	using Mipi::Base::Base;

	ILI9341(HSPI::Controller& spi, Size screenSize={320,240}): Mipi::Base(spi, screenSize)
	{
	}

	uint16_t readNvMemStatus();

	/* Device */

	String getName() const override
	{
		return F("ILI9341");
	}

	bool setOrientation(Orientation orientation) override;

	/* RenderTarget */

	PixelFormat getPixelFormat() const override
	{
		return PixelFormat::RGB565;
	}

	Surface* createSurface(size_t bufferSize = 0) override;

protected:
	bool initialise() override;
};

} // namespace Display
} // namespace Graphics
