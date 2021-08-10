/**
 * Drawing.h
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

#include "../Object.h"
#include "Writer.h"

namespace Graphics
{
namespace Drawing
{
class Target
{
public:
	Target(DrawingObject& drawing) : writer(drawing.getStream())
	{
	}

	Target(Stream& stream) : writer(stream)
	{
	}

	bool render(const Object& object, const Rect& location);

	void flush()
	{
		writer.flush();
	}

private:
	Writer writer;
};

} // namespace Drawing

using DrawingTarget = Drawing::Target;

} // namespace Graphics
