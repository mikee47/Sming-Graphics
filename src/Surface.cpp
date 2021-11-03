/**
 * Surface.cpp
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

#include "include/Graphics/Surface.h"
#include "include/Graphics/Renderer.h"
#include <Platform/System.h>

namespace Graphics
{
bool Surface::render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer)
{
	// Small fills can be handled without using a renderer
	auto isSmall = [](const Rect& r) -> bool {
		constexpr size_t maxFillPixels{16};
		return (r.w * r.h) <= maxFillPixels;
	};

	// Handled any immediate (simple) drawing
	switch(object.kind()) {
	case Object::Kind::Point: {
		auto obj = static_cast<const PointObject&>(object);
		if(obj.brush.isTransparent()) {
			break;
		}
		auto pixelFormat = getPixelFormat();
		Point pt = obj.point + location.topLeft();
		PackedColor cl{};
		if(obj.brush.isSolid()) {
			cl = obj.brush.getPackedColor(pixelFormat);
		} else {
			Brush brush(obj.brush);
			brush.setPixelFormat(pixelFormat);
			brush.writePixel(Location{location, {}, obj.point}, &cl);
		}
		return setPixel(cl, pt);
	}

	case Object::Kind::FilledRect: {
		// Draw solid filled non-rounded rectangles
		auto obj = static_cast<const FilledRectObject&>(object);
		if(obj.blender || obj.radius != 0 || obj.brush.isTransparent()) {
			break;
		}
		if(!obj.brush.isSolid() && !isSmall(obj.rect)) {
			break;
		}
		return fillSmallRect(obj.brush, location, obj.rect);
	}

	case Object::Kind::Line: {
		// Draw horizontal or vertical lines
		auto obj = static_cast<const LineObject&>(object);
		if(obj.pen.isTransparent()) {
			break;
		}
		Point pt1 = obj.pt1;
		Point pt2 = obj.pt2;
		Rect r;
		if(pt1.x == pt2.x) {
			if(pt1.y > pt2.y) {
				std::swap(pt1.y, pt2.y);
			}
			r = Rect(pt1, obj.pen.width, 1 + pt2.y - pt1.y);
		} else if(obj.pt1.y == obj.pt2.y) {
			if(pt1.x > pt2.x) {
				std::swap(pt1.x, pt2.x);
			}
			r = Rect(pt1, 1 + pt2.x - pt1.x, obj.pen.width);
		} else {
			break;
		}
		if(!obj.pen.isSolid() || !isSmall(r)) {
			break;
		}
		return fillSmallRect(obj.pen, location, r);
	}

	case Object::Kind::ScrollMargins: {
		auto obj = reinterpret_cast<const ScrollMarginsObject&>(object);
		return setScrollMargins(obj.top, obj.bottom);
	}

	case Object::Kind::ScrollOffset: {
		auto obj = reinterpret_cast<const ScrollOffsetObject&>(object);
		return setScrollOffset(obj.offset);
	}

	default:; // Continue to use renderer
	}

	Location loc{location, location.size()};
	renderer.reset(object.createRenderer(loc));
	return true;
}

bool Surface::render(const Object& object, const Rect& location)
{
	std::unique_ptr<Renderer> renderer;
	auto res = render(object, location, renderer);
	return res && execute(renderer);
}

bool Surface::fillSmallRect(const Brush& brush, const Rect& location, const Rect& rect)
{
	auto pixelFormat = getPixelFormat();
	Rect absRect = rect + location.topLeft();
	if(!absRect.clip(location)) {
		return true;
	}
	if(brush.isSolid()) {
		return fillRect(brush.getPackedColor(pixelFormat), absRect);
	}
	if(!setAddrWindow(absRect)) {
		return false;
	}
	size_t pixelCount = rect.w * rect.h;
	size_t bufSize = pixelCount * getBytesPerPixel(pixelFormat);
	uint16_t avail;
	auto buf = getBuffer(bufSize, avail);
	if(buf == nullptr) {
		return false;
	}
	Brush fill = brush;
	fill.setPixelFormat(pixelFormat);
	fill.writePixels(Location{location, {}, rect.topLeft()}, buf, pixelCount);
	commit(bufSize);
	return true;
}

bool Surface::fillRect(PackedColor color, const Rect& rect)
{
	if(!setAddrWindow(rect)) {
		return false;
	}
	return blockFill(color, rect.w * rect.h);
}

bool Surface::drawHLine(PackedColor color, uint16_t x0, uint16_t x1, uint16_t y, uint16_t w)
{
	if(x0 > x1) {
		std::swap(x0, x1);
	}
	return fillRect(color, Rect(x0, y, 1 + x1 - x0, w));
}

bool Surface::drawVLine(PackedColor color, uint16_t x, uint16_t y0, uint16_t y1, uint16_t w)
{
	if(y0 > y1) {
		std::swap(y0, y1);
	}
	return fillRect(color, Rect(x, y0, w, 1 + y1 - y0));
}

bool Surface::writePixels(const void* data, uint16_t length)
{
	uint16_t available;
	auto buf = getBuffer(length, available);
	if(buf == nullptr) {
		return false;
	}
	memcpy(buf, data, length);
	commit(length);
	return true;
}

} // namespace Graphics
