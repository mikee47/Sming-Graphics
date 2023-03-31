/****
 * Touch.h
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
class Device;

class Touch
{
public:
	struct State {
		Point pos;
		uint16_t pressure;
	};

	struct Calibration {
		Point origin{0, 0};
		Point num{1, 1};
		Point den{1, 1};

		Point translate(Point pt)
		{
			IntPoint p(pt);
			p -= origin;
			p *= num;
			p /= den;
			return Point(p);
		}
	};

	/**
     * @brief Callback function
     */
	using Callback = Delegate<void()>;

	Touch(Device* device = nullptr) : device(device)
	{
	}

	/**
	 * @brief Set display orientation
	 */
	virtual bool setOrientation(Orientation orientation);

	/**
	 * @brief Get physical size of display
	 * @retval Size Dimensions for NORMAL orientation
	 */
	virtual Size getNativeSize() const = 0;

	/**
     * @brief Get current state
     */
	virtual State getState() const = 0;

	/**
     * @brief Register callback to be invoked when state changes
     */
	virtual void setCallback(Callback callback)
	{
		this->callback = callback;
	}

	/**
	 * @brief Get native dimensions for current orientation
	 */
	Size getSize() const
	{
		auto size = getNativeSize();
		if(orientation == Orientation::deg90 || orientation == Orientation::deg270) {
			std::swap(size.w, size.h);
		}
		return size;
	}

	/**
	 * @brief Get current display orientation
	 */
	Orientation getOrientation()
	{
		return orientation;
	}

	void setCalibration(const Calibration& cal)
	{
		calibration = cal;
	}

	/**
	 * @brief Translate position into screen co-ordinates
	 */
	Point translate(Point rawPos)
	{
		return calibration.translate(rawPos);
	}

protected:
	Device* device;
	Orientation orientation{};
	Calibration calibration;
	Callback callback;
};

} // namespace Graphics

String toString(Graphics::Touch::State state);
