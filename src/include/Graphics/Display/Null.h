/****
 * Null.h
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

#include <Graphics/AbstractDisplay.h>
#include <Graphics/AddressWindow.h>

namespace Graphics
{
namespace Display
{
/**
 * @brief Null display device, discards data
 * 
 * Used for testing performance and algorithms.
 */
class NullDevice : public AbstractDisplay
{
public:
	NullDevice(uint16_t width = 240, uint16_t height = 320, PixelFormat format = PixelFormat::RGB565)
		: nativeSize(width, height), pixelFormat(format)
	{
	}

	bool begin()
	{
		return true;
	}

	bool begin(uint16_t width, uint16_t height, PixelFormat format)
	{
		nativeSize = Size{width, height};
		pixelFormat = format;
		return true;
	}

	/* Device */

	String getName() const override
	{
		return F("Null Display Device");
	}
	Size getNativeSize() const override
	{
		return nativeSize;
	}
	bool setOrientation(Orientation orientation) override
	{
		this->orientation = orientation;
		return true;
	}

	bool setScrollMargins(uint16_t top, uint16_t bottom) override
	{
		(void)top;
		(void)bottom;
		return true;
	}

	bool scroll(int16_t y) override
	{
		(void)y;
		return true;
	}

	/* RenderTarget */

	Size getSize() const override
	{
		return rotate(nativeSize, orientation);
	}

	PixelFormat getPixelFormat() const override
	{
		return pixelFormat;
	}

	Surface* createSurface(size_t bufferSize = 0) override;

private:
	friend class NullSurface;

	Size nativeSize{};
	PixelFormat pixelFormat{};
	AddressWindow addrWindow{};
};

} // namespace Display
} // namespace Graphics
