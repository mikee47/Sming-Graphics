/**
 * Registers.cpp
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

#include "../include/Graphics/Drawing/Registers.h"

namespace Graphics
{
namespace Drawing
{
String Registers::nameAt(uint8_t offset)
{
	assert(offset < sizeof(Registers));
	switch(offset) {
#define XX(name, type, def)                                                                                            \
	case offsetof(Registers, name):                                                                                    \
		return F(#name);
		GRAPHICS_DRAWING_REGISTER_LIST(XX)
#undef XX
	default:
		String s = nameAt(ALIGNDOWN4(offset));
		s += '[';
		s += offset & 3;
		s += ']';
		return s;
	}
}

String toString(OpCode opcode)
{
	switch(opcode) {
	case OpCode::store:
		return F("STORE");
	case OpCode::add:
		return F("ADD");
	case OpCode::sub:
		return F("SUB");
	case OpCode::execute:
		return F("EXECUTE");
	default:
		assert(false);
		return nullptr;
	}
}

} // namespace Drawing
} // namespace Graphics
