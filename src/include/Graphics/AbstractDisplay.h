/**
 * AbstractDisplay.h
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
 *
 ****/

#pragma once

#include "Device.h"

namespace Graphics
{

/**
 * Use this class as a base for all types of display drivers.
 */
class AbstractDisplay : public Device, public RenderTarget
{
public:
	virtual ~AbstractDisplay()
	{
	}
};

} // namespace Graphics
