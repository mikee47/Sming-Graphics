/****
 * Registers.h
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

#include "../Types.h"

namespace Graphics
{
namespace Drawing
{
// name, type, default
#define GRAPHICS_DRAWING_REGISTER_LIST(XX)                                                                             \
	XX(x1, int16_t, 0)                                                                                                 \
	XX(y1, int16_t, 0)                                                                                                 \
	XX(x2, int16_t, 0)                                                                                                 \
	XX(y2, int16_t, 0)                                                                                                 \
	XX(penColor, Color, Color::White)                                                                                  \
	XX(brushColor, Color, Color::Black)                                                                                \
	XX(penWidth, uint16_t, 1)                                                                                          \
	XX(radius, uint16_t, 0)                                                                                            \
	XX(startAngle, uint16_t, 0)                                                                                        \
	XX(angle, int16_t, 0)                                                                                              \
	XX(brushId, AssetID, 0)                                                                                            \
	XX(penId, AssetID, 0)                                                                                              \
	XX(textId, AssetID, 0)                                                                                             \
	XX(id, AssetID, 0)                                                                                                 \
	XX(length, uint16_t, 0xffff)                                                                                       \
	XX(offset, uint16_t, 0)                                                                                            \
	XX(style, FontStyles, 0)                                                                                           \
	XX(fontId, AssetID, 0)

enum class OpCode : uint8_t {
	store,
	add,
	sub,
	execute,
};

String toString(OpCode opcode);

struct Registers {
#define XX(name, type, def) type name{def};
	GRAPHICS_DRAWING_REGISTER_LIST(XX)
#undef XX

	Point pt1() const
	{
		return Point{x1, y1};
	}

	Point pt2() const
	{
		return Point{x2, y2};
	}

	Rect rect() const
	{
		return Rect{pt1(), pt2()};
	}

	uint16_t endAngle() const
	{
		return startAngle + angle;
	}

	template <typename T> void update(uint8_t index, OpCode opcode, T value)
	{
		assert(index * sizeof(T) < sizeof(Registers));
		auto& reg = reinterpret_cast<T*>(this)[index];
		switch(opcode) {
		case OpCode::store:
			reg = value;
			break;
		case OpCode::add:
			reg += value;
			break;
		case OpCode::sub:
			reg -= value;
			break;
		case OpCode::execute:
			assert(false);
			break;
		}
	}

	static String nameAt(uint8_t offset);
};

} // namespace Drawing
} // namespace Graphics
