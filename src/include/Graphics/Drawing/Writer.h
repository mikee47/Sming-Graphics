/****
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

#include "../Asset.h"
#include "../Stream.h"
#include "Command.h"
#include "Registers.h"

namespace Graphics
{
namespace Drawing
{
class Writer
{
public:
	Writer(Print& stream) : buffer(stream)
	{
		reset();
	}

	void reset()
	{
		active = Registers{};
		write(Command::reset);
	}

	void setPenColor(Color color)
	{
		setRegister(active.penColor, color);
	}

	void setPenWidth(uint16_t width)
	{
		setRegister(active.penWidth, width);
	}

	void setPen(const Pen& pen)
	{
		setPenColor(pen.getColor());
		setPenWidth(pen.width);
	}

	void setBrushColor(Color color)
	{
		setRegister(active.brushColor, color);
	}

	void setBrush(const Brush& brush)
	{
		setBrushColor(brush.getColor());
	}

	void moveto(Point pt)
	{
		setRegister(active.x2, pt.x);
		setRegister(active.y2, pt.y);
		write(Command::move);
	}

	void moveto(int16_t x, int16_t y)
	{
		moveto({x, y});
	}

	void setPixel(Point pt)
	{
		setpos(pt);
		write(Command::setPixel);
	}

	void line(Point pt)
	{
		setpos(pt);
		write(Command::line);
	}

	void lineto(Point pt)
	{
		setpos(pt);
		write(Command::lineto);
		active.x1 = active.x2;
		active.y1 = active.y2;
	}

	void drawArc(Point pt, uint16_t startAngle, uint16_t endAngle, bool filled = false)
	{
		setpos(pt);
		setRegister(active.startAngle, startAngle);
		setRegister(active.angle, endAngle - startAngle);
		write(filled ? Command::fillArc : Command::drawArc);
	}

	void fillArc(Point pt, uint16_t startAngle, uint16_t endAngle)
	{
		drawArc(pt, startAngle, endAngle, true);
	}

	void drawRect(Point pt, uint16_t radius = 0)
	{
		setpos(pt);
		setRegister(active.radius, radius);
		write(Command::drawRect);
	}

	void fillRect(Point pt, uint16_t radius = 0)
	{
		setpos(pt);
		setRegister(active.radius, radius);
		write(Command::fillRect);
	}

	void drawCircle(Point pt, uint16_t radius)
	{
		setpos(pt);
		setRegister(active.radius, radius);
		write(Command::drawCircle);
	}

	void fillCircle(Point pt, uint16_t radius)
	{
		setpos(pt);
		setRegister(active.radius, radius);
		write(Command::fillCircle);
	}

	void drawEllipse(Point pt)
	{
		setpos(pt);
		write(Command::drawEllipse);
	}

	void fillEllipse(Point pt)
	{
		setpos(pt);
		write(Command::fillEllipse);
	}

	void beginSub(uint16_t id)
	{
		setRegister(active.id, id);
		write(Command::beginSub);
		++subIndex;
	}

	void endSub()
	{
		assert(subIndex != 0);
		write(Command::endSub);
		--subIndex;
	}

	void flush()
	{
		buffer.flush();
	}

private:
	template <typename T> uint8_t getRegisterOffset(T& reg)
	{
		return reinterpret_cast<uint8_t*>(&reg) - reinterpret_cast<uint8_t*>(&active);
	}

	void setRegister(Color& reg, Color color);
	void setRegister(uint16_t& reg, uint16_t value);

	void setRegister(int16_t& reg, int16_t value)
	{
		setRegister(reinterpret_cast<uint16_t&>(reg), value);
	}

	void setpos(Point pt);
	void write(OpCode op, uint8_t off, uint32_t value);
	void write(Command cmd);

	void write(const void* data, size_t length)
	{
		buffer.write(data, length);
	}

	WriteStream buffer;
	Registers active;
	Point pt1{};
	Point pt2{};
	uint16_t subIndex{0};
};

} // namespace Drawing
} // namespace Graphics
