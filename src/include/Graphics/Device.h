/****
 * Device.h
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

#include "Types.h"
#include "Surface.h"

namespace Graphics
{
/**
 * @brief A physical display device
 */
class Device
{
public:
	virtual ~Device()
	{
	}

	/**
	 * @brief Get name of display
	 */
	virtual String getName() const = 0;

	/**
	 * @brief Set display orientation
	 */
	virtual bool setOrientation(Orientation orientation) = 0;

	/**
	 * @brief Get physical size of display
	 * @retval Size Dimensions for NORMAL orientation
	 */
	virtual Size getNativeSize() const = 0;

	/**
	 * @brief Get current display orientation
	 */
	Orientation getOrientation()
	{
		return orientation;
	}

	/**
	 * @brief Set margins for hardware scrolling
	 * @param top Number of fixed pixels at top of screen
	 * @param bottom Number of fixed pixels at bottom of screen
	 * 
	 * Area between top/bottom can be scrolled using `scroll()` method.
	 */
	virtual bool setScrollMargins(uint16_t top, uint16_t bottom) = 0;

	/**
	 * @brief Scroll region of display up or down using hardware scrolling
	 * @param y Number of lines to scroll. Positive values scroll content down, negative values scroll up.
	 */
	virtual bool scroll(int16_t y) = 0;

protected:
	Orientation orientation{};
};

} // namespace Graphics
