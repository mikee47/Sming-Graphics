/**
 * Drawing.cpp
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

#include "../include/Graphics/Drawing/Writer.h"
#include "../include/Graphics/Drawing/Header.h"

namespace Graphics
{
namespace Drawing
{
void Writer::setRegister(Color& reg, Color color)
{
	if(reg == color) {
		return;
	}
	auto offset = getRegisterOffset(reg);
	PixelBuffer src{color};
	PixelBuffer dst{reg};
	if(src.packed.value == dst.packed.value) {
		// Just alpha has changed
		write(OpCode::store, offset + 3, src.packed.alpha);
	} else {
		write(OpCode::store, offset, uint32_t(color));
	}
	reg = color;
}

void Writer::setRegister(uint16_t& reg, uint16_t value)
{
	if(reg == value) {
		return;
	}
	OpCode opcode;
	uint16_t arg;
	auto offset = getRegisterOffset(reg);
	if(offset >= 16) {
		opcode = OpCode::store;
		arg = value;
	} else if(value > reg) {
		opcode = OpCode::add;
		arg = value - reg;
	} else {
		opcode = OpCode::sub;
		arg = reg - value;
	}
	write(opcode, offset, arg);
	reg = value;
}

void Writer::setpos(Point pt)
{
	int off = pt.x - active.x2;
	if(abs(off) == 1) {
		write(off < 0 ? Command::decx : Command::incx);
		active.x2 = pt.x;
	} else {
		setRegister(active.x2, pt.x);
	}
	off = pt.y - active.y2;
	if(abs(off) == 1) {
		write(off < 0 ? Command::decy : Command::incy);
		active.y2 = pt.y;
	} else {
		setRegister(active.y2, pt.y);
	}
}

void Writer::write(OpCode op, uint8_t off, uint32_t value)
{
	Header::Type type;
	if(value > 0xffff) {
		type = Header::Type::uint32;
	} else if(value > 0xff || off >= 16) {
		type = Header::Type::uint16;
	} else {
		type = Header::Type::uint8;
	}
	auto shift = uint8_t(type);
	Header hdr{{uint8_t(uint8_t(off) >> shift), type, op, value}};
	write(&hdr, 1 + (1 << shift));
}

void Writer::write(Command cmd)
{
	Header hdr{};
	hdr.opcode = OpCode::execute;
	hdr.cmd = cmd;
	write(&hdr, 1);
}

} // namespace Drawing
} // namespace Graphics
