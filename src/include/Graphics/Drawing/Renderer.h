/****
 * Renderer.h
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

#include "../Renderer.h"
#include "Reader.h"

namespace Graphics
{
namespace Drawing
{
/**
 * @brief A drawing contains a compact list of drawing commands, like a virtual plotter.
 */
class Renderer : public MultiRenderer
{
public:
	Renderer(const Location& location, const DrawingObject& drawing) : MultiRenderer(location), reader(drawing)
	{
	}

protected:
	void renderDone(const Object*) override
	{
		this->object.reset();
	}

	const Object* getNextObject() override
	{
		object.reset(reader.readObject());
		return object.get();
	}

private:
	Drawing::Reader reader;
	std::unique_ptr<Object> object;
};

} // namespace Drawing
} // namespace Graphics
