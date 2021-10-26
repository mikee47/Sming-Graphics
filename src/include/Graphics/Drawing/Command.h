/****
 * Command.h
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

#include <WString.h>

namespace Graphics
{
namespace Drawing
{
/**
 * @brief Drawing operations
 *
 * pt1 is cursor position, set by `move` command
 * pt2 is new position, set by parameters
 *
 * Commands may interpret these as corners of a rectangle (e.g. ellipse)
 *
 * For example:
 *
 * 		reset
 * 		xabs  12
 * 		yabs  100
 * 		move
 * 		xrel  100
 * 		color yellow
 *		width 3
 * 		line
 * 
 * 		x = -5
 * 		color = red
 * 		width = 3
 * 		line
 *
 */

// Command, arg, description
#define GRAPHICS_DRAWING_COMMAND_MAP(XX)                                                                               \
	XX(reset, "", "Reset registers to default")                                                                        \
	XX(push, "", "Push all registers to stack")                                                                        \
	XX(pop, "", "Pop all registers from stack")                                                                        \
	XX(storePen, "id", "Store penColor and width to slot")                                                             \
	XX(storeBrush, "id", "Store brushColor to slot")                                                                   \
	XX(incx, "pt2", "++x2")                                                                                            \
	XX(decx, "pt2", "--x2")                                                                                            \
	XX(incy, "pt2", "++y2")                                                                                            \
	XX(decy, "pt2", "--y2")                                                                                            \
	XX(move, "pt1, pt2", "Set pt2 = pt1")                                                                              \
	XX(setPixel, "pt2", "Set pixel colour")                                                                            \
	XX(line, "pt1, pt2, penId", "Draw line")                                                                           \
	XX(lineto, "pt1, pt2, penId", "Draw line then set pt1 = pt2")                                                      \
	XX(drawRect, "pt1, pt2, radius, penId", "Draw rect with optional rounded corners")                                 \
	XX(fillRect, "pt1, pt2, radius, brushId", "Fill rect with optional rounded corners")                               \
	XX(drawEllipse, "pt1, pt2, penId", "Draw ellipse within rectangle")                                                \
	XX(fillEllipse, "pt1, pt2, brushId", "Fill ellipse within rectangle")                                              \
	XX(drawArc, "pt1, pt2, startAngle, endAngle, penId", "Draw arc within rectangle from startAngle -> endAngle")      \
	XX(fillArc, "pt1, pt2, startAngle, endAngle, brushId", "Fill arc within rectangle from startAngle -> endAngle")    \
	XX(drawCircle, "pt2, radius, penId", "Draw circle centred at pt2 with radius")                                     \
	XX(fillCircle, "pt2, radius, brushId", "Draw circle centred at pt2 with radius")                                   \
	XX(beginSub, "id", "Start a subroutine")                                                                           \
	XX(endSub, "", "End a subroutine")                                                                                 \
	XX(call, "id", "Call a subroutine")                                                                                \
	XX(drawText, "id", "Draw text asset from offset to length")

enum class Command : uint8_t {
#define XX(cmd, args, desc) cmd,
	GRAPHICS_DRAWING_COMMAND_MAP(XX)
#undef XX
};

} // namespace Drawing
} // namespace Graphics

String toString(Graphics::Drawing::Command cmd);
