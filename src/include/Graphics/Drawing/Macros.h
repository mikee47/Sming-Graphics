/****
 * Macros.h
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

#include "Header.h"

/*
 * Set of macros to allow construction of drawings as uint8_t[] arrays:
 *
 *   DEFINE_FSTR_ARRAY(myDrawing, uint8_t,
 *     GDRAW_RESET()
 *     GDRAW_PENWIDTH(3)
 *     GDRAW_XABS(10)
 *     GDRAW_YABS(10)
 *     GDRAW_COLOR(Color::Red)
 *     GDRAW_CIRCLE(50)
 *     GDRAW_XREL(100)
 *     GDRAW_COLOR(Color::Green)
 *     GDRAW_FILLCIRCLE(50)
 *   )
 */

#define GDRAW_FIELD(value, shift) uint8_t(uint8_t(Graphics::Drawing::value) << shift)
#define GDRAW_OPCODE(opcode) GDRAW_FIELD(opcode, 6)
#define GDRAW_TYPE(type) GDRAW_FIELD(Header::type, 4)
#define GDRAW_CMD(cmd) GDRAW_OPCODE(OpCode::execute) | GDRAW_TYPE(Type::uint8) | uint8_t(Graphics::Drawing::cmd),
#define GDRAW_UINT16(value) uint8_t(value), uint8_t((value) >> 8),
#define GDRAW_INT16(value) uint8_t(value), uint8_t(uint16_t(value) >> 8),
#define GDRAW_UINT32(value) uint8_t(value), uint8_t((value) >> 8), uint8_t((value) >> 16), uint8_t((value) >> 24),
#define GDRAW_MAKE_UINT32(w1, w2) (uint32_t(w2) << 16) | uint32_t(w1)
#define GDRAW_REGDEF(reg, size) uint8_t(offsetof(Graphics::Drawing::Registers, reg) / size)
#define GDRAW_REG_UINT8(opcode, reg, value)                                                                            \
	GDRAW_OPCODE(opcode) | GDRAW_TYPE(Type::uint8) | GDRAW_REGDEF(reg, 1), uint8_t(value),
#define GDRAW_REG_UINT16(opcode, reg, value)                                                                           \
	GDRAW_OPCODE(opcode) | GDRAW_TYPE(Type::uint16) | GDRAW_REGDEF(reg, 2), GDRAW_UINT16(value)
#define GDRAW_REG_INT16(reg, value)                                                                                    \
	((value) < 0 ? GDRAW_OPCODE(OpCode::sub) : GDRAW_OPCODE(OpCode::add)) | GDRAW_TYPE(Type::uint16) |                 \
		GDRAW_REGDEF(reg, 2),                                                                                          \
		GDRAW_INT16(value)
#define GDRAW_REG_UINT32(opcode, reg, value)                                                                           \
	GDRAW_OPCODE(opcode) | GDRAW_TYPE(Type::uint32) | GDRAW_REGDEF(reg, 4), GDRAW_UINT32(value)
#define GDRAW_RESET() GDRAW_CMD(Command::reset)
#define GDRAW_SAVE() GDRAW_CMD(Command::push)
#define GDRAW_RESTORE() GDRAW_CMD(Command::pop)
#define GDRAW_XREL(cx) GDRAW_REG_INT16(x2, cx)
#define GDRAW_YREL(cy) GDRAW_REG_INT16(y2, cy)
#define GDRAW_XABS(x_) GDRAW_REG_UINT16(OpCode::store, x2, x_)
#define GDRAW_YABS(y_) GDRAW_REG_UINT16(OpCode::store, y2, y_)
#define GDRAW_ID(id_) GDRAW_REG_UINT16(OpCode::store, id, id_)
#define GDRAW_SELECT_PEN(id) GDRAW_REG_UINT16(OpCode::store, penId, id)
#define GDRAW_SELECT_BRUSH(id) GDRAW_REG_UINT16(OpCode::store, brushId, id)
#define GDRAW_SELECT_TEXT(id) GDRAW_REG_UINT16(OpCode::store, textId, id)
#define GDRAW_OFFSET_LENGTH(off, len) GDRAW_REG_UINT32(OpCode::store, length, GDRAW_MAKE_UINT32(len, off))
#define GDRAW_PEN_COLOR(col) GDRAW_REG_UINT32(OpCode::store, penColor, uint32_t(col))
#define GDRAW_PEN_WIDTH(width) GDRAW_REG_UINT16(OpCode::store, penWidth, width)
#define GDRAW_STORE_PEN(id) GDRAW_ID(id) GDRAW_CMD(Command::storePen)
#define GDRAW_BRUSH_COLOR(col) GDRAW_REG_UINT32(OpCode::store, brushColor, uint32_t(col))
#define GDRAW_STORE_BRUSH(id) GDRAW_ID(id) GDRAW_CMD(Command::storeBrush)
#define GDRAW_END_ANGLE(value) GDRAW_REG_UINT16(OpCode::store, endAngle, value) GDRAW_CMD(Command::angle)
#define GDRAW_MOVE() GDRAW_CMD(Command::move)
#define GDRAW_LINE() GDRAW_CMD(Command::line)
#define GDRAW_LINE_TO() GDRAW_CMD(Command::lineto)
#define GDRAW_RADIUS(value) GDRAW_REG_UINT16(OpCode::store, radius, value)
#define GDRAW_RECT(radius) GDRAW_RADIUS(radius) GDRAW_CMD(Command::drawRect)
#define GDRAW_FILL_RECT(radius) GDRAW_RADIUS(radius) GDRAW_CMD(Command::fillRect)
#define GDRAW_ELLIPSE() GDRAW_CMD(Command::drawEllipse)
#define GDRAW_FILL_ELLIPSE() GDRAW_CMD(Command::fillEllipse)
#define GDRAW_START_ANGLE(angle) GDRAW_REG_UINT16(OpCode::store, startAngle, value)
#define GDRAW_ANGLE(angle) GDRAW_REG_UINT16(OpCode::store, angle, value)
#define GDRAW_ARC(startAngle) GDRAW_START_ANGLE(startAngle) GDRAW_CMD(Command::drawArc)
#define GDRAW_FILL_ARC(startAngle) GDRAW_START_ANGLE(startAngle) GDRAW_CMD(Command::fillArc)
#define GDRAW_CIRCLE(radius) GDRAW_RADIUS(radius) GDRAW_CMD(Command::drawCircle)
#define GDRAW_FILL_CIRCLE(radius) GDRAW_RADIUS(radius) GDRAW_CMD(Command::fillCircle)
#define GDRAW_BEGIN_SUB(id) GDRAW_ID(id) GDRAW_CMD(Command::beginSub)
#define GDRAW_END_SUB() GDRAW_CMD(Command::endSub)
#define GDRAW_CALL(id) GDRAW_ID(id) GDRAW_CMD(Command::call)
#define GDRAW_RESOURCE(id, dataType, kind)                                                                             \
	GDRAW_ID(id)                                                                                                       \
	GDRAW_OPCODE(OpCode::store) | GDRAW_TYPE(Type::resource) | GDRAW_FIELD(Header::dataType, 2) |                      \
		GDRAW_FIELD(Header::LengthSize::uint8, 1) | GDRAW_FIELD(Header::kind, 0)
#define GDRAW_DEFINE_CHARS(id, len, ...) GDRAW_RESOURCE(id, DataType::charArray, ResourceKind::text), len, __VA_ARGS__,
#define GDRAW_DRAW_CHARS(len, ...)                                                                                     \
	GDRAW_DEFINE_CHARS(0, len, __VA_ARGS__) GDRAW_SELECT_TEXT(0) GDRAW_CMD(Command::drawText)
#define GDRAW_DRAW_TEXT(id) GDRAW_SELECT_TEXT(id) GDRAW_CMD(Command::drawText)
#define GDRAW_FONT_STYLE(font_id, style_)                                                                              \
	GDRAW_REG_UINT32(OpCode::store, style, GDRAW_MAKE_UINT32(uint32_t(style_), font_id))
