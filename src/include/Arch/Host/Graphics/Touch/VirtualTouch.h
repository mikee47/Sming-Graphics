/****
 * VirtualTouch.h
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
 * @author: May 2021 - mikee47 <mike@sillyhouse.net>
 *
 ****/

#pragma once

#include <Graphics/Touch.h>

namespace Graphics
{
namespace Display
{
class Virtual;
}

class VirtualTouch : public Touch
{
public:
	VirtualTouch(Display::Virtual& display);

	bool begin()
	{
		return true;
	}

	void end()
	{
	}

	/* Touch */

	Size getNativeSize() const override;

	State getState() const override
	{
		return state;
	}

private:
	State state;
};

} // namespace Graphics
