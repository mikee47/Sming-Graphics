/**
 * Types.cpp
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

#include "include/Graphics/Types.h"
#include <Data/CStringArray.h>

namespace
{
#ifndef M_PI
constexpr float M_PI{3.14159265358979};
#endif

} // namespace

String toString(Graphics::Orientation orientation)
{
	return String(unsigned(orientation) * 90);
}

String toString(Graphics::Align align)
{
	switch(align) {
	case Graphics::Align::Near:
		return F("near");
	case Graphics::Align::Centre:
		return F("centre");
	case Graphics::Align::Far:
		return F("far");
	default:
		return nullptr;
	}
}

String toString(Graphics::Origin origin)
{
	DEFINE_FSTR_LOCAL(strings, "E\0"
							   "NE\0"
							   "N\0"
							   "NW\0"
							   "W\0"
							   "SW\0"
							   "S\0"
							   "SE\0"
							   "Centre")
	return CStringArray(strings)[unsigned(origin)];
}

String toString(Graphics::FontStyle style)
{
	switch(style) {
#define XX(name, desc)                                                                                                 \
	case Graphics::FontStyle::name:                                                                                    \
		return F(#name);
		GRAPHICS_FONT_STYLE(XX)
#undef XX
	default:
		return nullptr;
	}
}

namespace Graphics
{
uint16_t normaliseAngle(int angle)
{
	if(angle >= 360) {
		return unsigned(angle) % 360;
	}
	while(angle < 0) {
		angle += 360;
	}
	return angle;
}

Origin degreesToOrigin(uint16_t angle)
{
	return Origin((normaliseAngle(angle) + 23) / 45);
}

String Size::toString() const
{
	return Point(*this).toString();
}

String Rect::toString() const
{
	char buf[32];
	m_snprintf(buf, sizeof(buf), "%d, %d, %u, %u", x, y, w, h);
	return buf;
}

String Region::toString() const
{
	String s;
	for(auto& r : rects) {
		if(!r) {
			continue;
		}
		if(s) {
			s += ", ";
		}
		s += '(';
		s += r.toString();
		s += ')';
	}
	return s;
}

String Location::toString() const
{
	String s;
	s += source.toString();
	s += " -> ";
	s += dest.toString();
	s += " @";
	s += pos.toString();
	return s;
}

} // namespace Graphics
