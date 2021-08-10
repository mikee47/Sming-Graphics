/**
 * Touch.cpp
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

#include "include/Graphics/Touch.h"
#include "include/Graphics/Device.h"

String toString(Graphics::Touch::State state)
{
	String s;
	s += toString(state.pos);
	s += ", ";
	s += state.pressure;
	return s;
}

namespace Graphics
{
bool Touch::setOrientation(Orientation orientation)
{
	if(device != nullptr && !device->setOrientation(orientation)) {
		return false;
	}
	this->orientation = orientation;
	return true;
}

} // namespace Graphics
