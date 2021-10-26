/****
 * AddressWindow.h
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

namespace Graphics
{
/**
 * @brief Manages a rectangular area of display memory with position information
 * 
 * Accesses to display memory is controlled by first setting an active Address Window.
 * This is a rectangular area into which following writes (or reads) will store data.
 * 
 * Although the display hardware usually manages this some operations require tracking the
 * position within the driver.
 */
struct AddressWindow {
	// Last access mode for window
	enum class Mode {
		none,
		write,
		read,
	};

	Rect bounds{};		///< y and h are updated by seek()
	uint16_t column{0}; ///< Relative x position within window
	Rect initial{};
	Mode mode{};

	AddressWindow()
	{
	}

	AddressWindow(const Rect& rect) : bounds(rect), initial(rect)
	{
	}

	void reset()
	{
		column = 0;
		bounds = initial;
	}

	bool setMode(Mode mode)
	{
		if(this->mode == mode) {
			return false;
		}
		this->mode = mode;
		reset();
		return true;
	}

	AddressWindow& operator=(const Rect& rect)
	{
		initial = rect;
		mode = Mode::none;
		reset();
		return *this;
	}

	/**
	 * @brief Get remaining pixels in window from current position
	 */
	size_t getPixelCount() const
	{
		return bounds.w * bounds.h - column;
	}

	uint16_t seek(uint16_t count)
	{
		if(bounds.h == 0) {
			return 0;
		}
		auto pos = column;
		uint16_t res{0};
		column += count;
		while(column >= bounds.w && bounds.h != 0) {
			column -= bounds.w;
			++bounds.y;
			--bounds.h;
			res += bounds.w;
		}
		return res + column - pos;
	}

	Point pos() const
	{
		return Point(left(), top());
	}

	uint16_t left() const
	{
		return bounds.left() + column;
	}

	uint16_t top() const
	{
		return bounds.top();
	}

	uint16_t right() const
	{
		return bounds.right();
	}

	uint16_t bottom() const
	{
		return bounds.bottom();
	}
};

} // namespace Graphics
