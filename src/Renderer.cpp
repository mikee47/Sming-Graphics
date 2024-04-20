/**
 * Renderer.cpp
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

#include "include/Graphics/Renderer.h"
#include "include/Graphics/Surface.h"
#include <Platform/Timers.h>

#ifdef ENABLE_GRAPHICS_DEBUG
#define debug_g(fmt, ...) debug_i(fmt, ##__VA_ARGS__)
#else
#define debug_g debug_none
#endif

#ifndef M_PI
constexpr float M_PI{3.14159265358979};
#endif

namespace Graphics
{
/* PointList */

bool PointList::render(Surface& surface)
{
	object.brush.setPixelFormat(surface.getPixelFormat());

	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		auto pt = get();
		if(pt == nullptr) {
			break;
		}

		object.point = *pt;
		if(!surface.render(object, bounds, renderer)) {
			return false;
		}
		next();
	}

	reset();
	return true;
}

/* RectList */

bool RectList::render(Surface& surface)
{
	object.brush.setPixelFormat(surface.getPixelFormat());

	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		auto rect = get();
		if(rect == nullptr) {
			break;
		}

		object.rect = *rect;
		if(!surface.render(object, bounds, renderer)) {
			return false;
		}
		next();
	}

	reset();
	return true;
}

/* MultiRenderer */

bool MultiRenderer::execute(Surface& surface)
{
	while(true) {
		if(renderer) {
			if(!surface.execute(renderer)) {
				return false;
			}
			renderDone(object);
			object = nullptr;
		}

		if(object == nullptr) {
			object = getNextObject();
			if(object == nullptr) {
				// Render complete
				return true;
			}
		}

		debug_g("[RENDER] %s -> %s", object->toString().c_str(), location.toString().c_str());

		if(!surface.render(*object, location.dest, renderer)) {
			// Render couldn't be started, try again with another surface
			return false;
		}
		if(!renderer) {
			renderDone(object);
			object = nullptr;
		}
	}
}

/*
 * GfxLineRenderer
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */

void GfxLineRenderer::init()
{
	steep = abs(y1 - y0) > abs(x1 - x0);
	if(steep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if(x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	xaddr = x0 - 1;

	dx = x1 - x0;
	dy = abs(y1 - y0);
	err = dx / 2;
	ystep = (y0 < y1) ? 1 : -1;
}

bool GfxLineRenderer::execute(Surface& surface)
{
	pen.setPixelFormat(surface.getPixelFormat());

	for(; x0 <= x1; x0++) {
		Point pt = steep ? Point(y0, x0) : Point(x0, y0);
		if(!surface.setPixel(pen.getPackedColor(pt), pt)) {
			return false;
		}
		err -= dy;
		if(err < 0) {
			y0 += ystep;
			err += dx;
		}
	}

	return true;
}

/*
 * LineRenderer
 *
 * See http://enchantia.com/graphapp/
 *
 * Run-length slice line drawing:
 *
 * This is based on Bresenham's line-slicing algorithm, which is faster than the traditional Bresenham's line drawing algorithm,
 * and better suited for drawing filled rectangles instead of individual pixels.
 *
 * It essentially reverses the ordinary Bresenham's logic; instead of keeping an error term which counts along the
 * direction of travel (the major axis), it keeps an error term perpendicular to the major axis, to determine when
 * to step to the next run of pixels.
 *
 * See Michael Abrash's Graphics Programming Black Book on-line at http://www.ddj.com/articles/2001/0165/0165f/0165f.htm
 * chapter 36 (and 35 and 37) for more details.
 *
 * The algorithm can also draw lines with a thickness greater than 1 pixel. In that case, the line hangs below and to
 * the right of the end points.
 */
void LineRenderer::init()
{
	// Figure out whether we're going left or right, and how far we're going horizontally
	if(x2 < x1) {
		xadvance = -1;
		dx = x1 - x2;
	} else {
		xadvance = 1;
		dx = x2 - x1;
	}

	// We'll always draw top to bottom, to reduce the number of cases we have to handle
	if(y2 < y1) {
		std::swap(x1, x2);
		std::swap(y1, y2);
		xadvance = -xadvance;
	}
	dy = y2 - y1;

	// Special-case horizontal, vertical, and diagonal lines, for speed and to avoid
	// nasty boundary conditions and division by 0

	if(dx == 0) {
		/* Vertical line */
		r = Rect(x1, y1, w, dy + 1);
		return rectangles.add(r);
	}

	if(dy == 0) {
		/* Horizontal line */
		r = Rect(std::min(x1, x2), y1, dx + 1, w);
		return rectangles.add(r);
	}

	if(dx == dy) {
		mode = Mode::diagonal;
		r = Rect(x1, y1, w, 1);
		return;
	}

	// Determine whether the line is more horizontal or vertical, and handle accordingly

	if(dx >= dy) {
		initHorizontal();
	} else {
		initVertical();
	}
}

bool LineRenderer::execute(Surface& surface)
{
	for(;;) {
		if(rectangles && !rectangles.render(surface)) {
			return false;
		}

		switch(mode) {
		case Mode::diagonal:
			drawDiagonal();
			break;
		case Mode::horizontal:
			drawHorizontal();
			break;
		case Mode::vertical:
			drawVertical();
			break;
		case Mode::simple:
		case Mode::done:
		default:
			return true;
		}
	}
}

void LineRenderer::drawDiagonal()
{
	if(run_pos++ == dx + 1) {
		mode = Mode::done;
		return;
	}

	rectangles.add(r);
	r.x += xadvance;
	r.y++;
}

void LineRenderer::initHorizontal()
{
	// More horizontal than vertical
	mode = Mode::horizontal;

	if(xadvance < 0) {
		x1++;
		x2++;
	}

	// Minimum # of pixels in a run in this line
	whole_step = dx / dy;

	/* Error term adjust each time Y steps by 1; used to tell when one extra pixel should be drawn as part
		 * of a run, to account for fractional steps along the X axis per 1-pixel steps along Y */
	adj_up = (dx % dy) * 2;

	// Error term adjust when the error term turns over, used to factor out the X step made at that time
	adj_down = dy * 2;

	// Initial error term; reflects an initial step of 0.5 along the Y axis
	error_term = (dx % dy) - (dy * 2);

	/* The initial and last runs are partial, because Y advances only 0.5 for these runs, rather than 1.
		 * Divide one full run, plus the initial pixel, between the initial and last runs */
	initial_run = (whole_step / 2) + 1;
	final_run = initial_run;

	/* If the basic run length is even and there's no fractional advance, we have one pixel that could
		 * go to either the initial or last partial run, which we'll arbitrarily allocate to the last run */
	if(adj_up == 0 && (whole_step % 2) == 0) {
		initial_run--;
	}

	/* If there're an odd number of pixels per run, we have 1 pixel that can't be allocated to either
		 * the initial or last partial run, so we'll add 0.5 to error term so this pixel will be handled by
		 * the normal full-run loop */
	if((whole_step % 2) != 0) {
		error_term += dy;
	}

	// Draw the first, partial run of pixels
	r = Rect(x1, y1, initial_run, w);
	if(xadvance < 0) {
		r.x -= r.w;
		rectangles.add(r);
	} else {
		rectangles.add(r);
		r.x += r.w;
	}
	++r.y;
}

void LineRenderer::drawHorizontal()
{
	if(++run_pos == dy) {
		/* Draw the final run of pixels */
		r.w = final_run;
		if(xadvance < 0) {
			r.x -= r.w;
		}
		rectangles.add(r);
		mode = Mode::done;
		return;
	}

	// Draw all full runs
	run_length = whole_step; // at least

	// Advance the error term and add an extra pixel if the error term so indicates
	if((error_term += adj_up) > 0) {
		++run_length;
		error_term -= adj_down; // reset
	}

	// Draw this scan line's run
	r.w = run_length;
	if(xadvance < 0) {
		r.x -= r.w;
		rectangles.add(r);
	} else {
		rectangles.add(r);
		r.x += r.w;
	}
	++r.y;
}

void LineRenderer::initVertical()
{
	// More vertical than horizontal
	mode = Mode::vertical;

	// Minimum # of pixels in a run in this line
	whole_step = dy / dx;

	/* Error term adjust each time X steps by 1; used to tell when 1 extra pixel should be drawn as part of
	 * a run, to account for fractional steps along the Y axis per 1-pixel steps along X */
	adj_up = (dy % dx) * 2;

	// Error term adjust when the error term turns over, used to factor out the Y step made at that time
	adj_down = dx * 2;

	// Initial error term; reflects initial step of 0.5 along the X axis
	error_term = (dy % dx) - (dx * 2);

	/* The initial and last runs are partial, because X advances only 0.5 for these runs, rather than 1.
	 * Divide one full run, plus the initial pixel, between the initial and last runs */
	initial_run = (whole_step / 2) + 1;
	final_run = initial_run;

	/* If the basic run length is even and there's no fractional advance, we have 1 pixel that could
	 * go to either the initial or last partial run, which we'll arbitrarily allocate to the last run */
	if(adj_up == 0 && (whole_step % 2) == 0) {
		--initial_run;
	}

	/* If there are an odd number of pixels per run, we have one pixel that can't be allocated to either
	 * the initial or last partial run, so we'll add 0.5 to the error term so this pixel will be handled
	 * by the normal full-run loop */
	if((whole_step % 2) != 0) {
		error_term += dx;
	}

	// Draw the first, partial run of pixels
	r = Rect(x1, y1, w, initial_run);
	rectangles.add(r);
	r.x += xadvance;
	r.y += r.h;
}

void LineRenderer::drawVertical()
{
	if(++run_pos == dx) {
		/* Draw the final run of pixels */
		r.h = final_run;
		rectangles.add(r);
		mode = Mode::done;
		return;
	}

	/* Draw all full runs */
	run_length = whole_step; /* at least */

	/* Advance the error term and add an extra
			 * pixel if the error term so indicates */
	if((error_term += adj_up) > 0) {
		++run_length;
		error_term -= adj_down; /* reset */
	}

	/* Draw this scan line's run */
	r.h = run_length;
	rectangles.add(r);
	r.x += xadvance;
	r.y += r.h;
}

/* PolylineRenderer */

bool PolylineRenderer::execute(Surface& surface)
{
	if(!line.pen) {
		line.pen = Pen(object.pen, surface.getPixelFormat());
	}

	// debug_g("POLY(%s, %u)", object.rect.toString().c_str(), object->color);
	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		if(index + 1 >= object.numPoints) {
			return true;
		}

		line.pt1 = object[index];
		line.pt2 = object[index + 1];
		if(!surface.render(line, location.dest, renderer)) {
			return false;
		}
		index += object.connected ? 1 : 2;
	}

	return true;
}

/* FilledRectRenderer */

bool FilledRectRenderer::execute(Surface& surface)
{
	if(blockSize.w == 0) {
		rect.clip(location.dest);
		if(!rect) {
			return true;
		}
		auto pixelFormat = surface.getPixelFormat();
		brush.setPixelFormat(pixelFormat);
		// debug_i("FILL (%s), trans %u, color 0x%08x", rect.toString().c_str(), brush.isTransparent(),
		// 		brush.getPackedColor());
		if(rect.w <= Buffer::bufPixels) {
			// Buffer big enough for a single line, so render in blocks of complete rows
			blockSize = Size(rect.w, std::min(rect.h, uint16_t(Buffer::bufPixels / rect.w)));
		} else {
			// Render in line segments
			blockSize = Size(Buffer::bufPixels, 1);
		}
		buffers[0].format = pixelFormat;
		buffers[1].format = pixelFormat;
	}

	if(!done && busyCount < 2 && queueRead(surface) < 0) {
		return false;
	}

	auto& buffer = buffers[index];
	// debug_i("status %u, %u", buffer.status.readComplete, buffer.status.bytesRead);
	if(!buffer.status.readComplete) {
		return false;
	}
	if(blender) {
		auto color = brush.getPackedColor();
		blender->transform(buffer.format, color, buffer.data.get(), buffer.status.bytesRead);
	} else if(brush.isTransparent()) {
		auto color = brush.getPackedColor();
		BlendAlpha::blend(buffer.format, color, buffer.data.get(), buffer.status.bytesRead);
	} else {
		buffer.status.bytesRead = brush.writePixels({buffer.r, buffer.r}, buffer.data.get(), buffer.r.w);
	}
	// debug_i("[WRITE] (%s), %u", buffer.r.toString().c_str(), buffer.status.bytesRead);
	if(!surface.setAddrWindow(buffer.r)) {
		return false;
	}
	if(!surface.writeDataBuffer(buffer.data, 0, buffer.status.bytesRead)) {
		return false;
	}
	--busyCount;
	return queueRead(surface) == 0 && busyCount == 0;
}

int FilledRectRenderer::queueRead(Surface& surface)
{
	if(pos.y == rect.h) {
		index ^= 1;
		done = true;
		return 0;
	}
	auto& buffer = buffers[index];
	auto w = std::min(blockSize.w, uint16_t(rect.w - pos.x));
	auto h = std::min(blockSize.h, uint16_t(rect.h - pos.y));
	buffer.r = Rect(rect.x + pos.x, rect.y + pos.y, w, h);
	if(blender || brush.isTransparent()) {
		if(!surface.setAddrWindow(buffer.r)) {
			return -1;
		}
		if(surface.readDataBuffer(buffer) < 0) {
			return -1;
		}
	} else {
		buffer.status.bytesRead = w * getBytesPerPixel(surface.getPixelFormat());
		buffer.status.readComplete = true;
	}
	// debug_i("[READ] (%s)", buffer.r.toString().c_str());
	index ^= 1;
	pos.x += w;
	if(pos.x == rect.w) {
		pos.x = 0;
		pos.y += h;
	}
	++busyCount;
	return buffer.r.w;
}

/*
 * RoundedRectRenderer
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */

bool RoundedRectRenderer::execute(Surface& surface)
{
	// debug_g("RECT(%s, %u, %u)", object.rect.toString().c_str(), object->color, object->radius);

	pen.setPixelFormat(surface.getPixelFormat());

	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		auto t = radius * 2;
		switch(state) {
		case 0:
			renderer = std::make_unique<ArcRenderer>(location, pen, Rect(rect.left(), rect.top(), t, t), 90, 180);
			break;
		case 1:
			renderer = std::make_unique<ArcRenderer>(location, pen, Rect(rect.right() - t, rect.top(), t, t), 0, 90);
			break;
		case 2:
			renderer =
				std::make_unique<ArcRenderer>(location, pen, Rect(rect.right() - t, rect.bottom() - t, t, t), 270, 360);
			break;
		case 3:
			renderer =
				std::make_unique<ArcRenderer>(location, pen, Rect(rect.left(), rect.bottom() - t, t, t), 180, 270);
			break;
		case 4:
			return true;
		}

		++state;
	}

	return true;
}

/* FilledRoundedRectRenderer */

bool FilledRoundedRectRenderer::execute(Surface& surface)
{
	// debug_g("RECT(%s, %u, %u)", object.rect.toString().c_str(), object->color, object->radius);

	object.brush.setPixelFormat(surface.getPixelFormat());

	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		switch(state) {
		case 0:
		case 1: {
			auto& rect = this->object.rect;
			auto r = object.radius;
			renderer = std::make_unique<FilledCircleRenderer>(location, object.brush, corners[state], r,
															  rect.w - 2 * (r + 1), 0x01 << state);
			break;
		}
		case 2: {
			// Central rectangle
			object.rect.y += object.radius;
			object.rect.h -= object.radius * 2;
			object.radius = 0;
			if(!surface.render(object, location.dest, renderer)) {
				return false;
			}
			break;
		}
		default:
			return true;
		}

		++state;
	}

	return true;
}

/*
 * CircleRenderer
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */

bool CircleRenderer::execute(Surface& surface)
{
	for(;;) {
		if(!pixels.render(surface)) {
			return false;
		}

		if(x >= y) {
			// Done
			return true;
		}

		if(f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x;

		if(corners & 0x04) {
			pixels.add(x0 + x, y0 + y);
			pixels.add(x0 + y, y0 + x);
		}

		if(corners & 0x02) {
			pixels.add(x0 + x, y0 - y);
			pixels.add(x0 + y, y0 - x);
		}

		if(corners & 0x08) {
			pixels.add(x0 - y, y0 + x);
			pixels.add(x0 - x, y0 + y);
		}

		if(corners & 0x01) {
			pixels.add(x0 - y, y0 - x);
			pixels.add(x0 - x, y0 - y);
		}
	}
}

/*
 * FilledCircleRenderer
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */

bool FilledCircleRenderer::execute(Surface& surface)
{
	for(;;) {
		if(!rectangles.render(surface)) {
			return false;
		}

		if(y >= x) {
			return true;
		}

		if(f >= 0) {
			x--;
			ddF_x += 2;
			f += ddF_x;
		}
		y++;
		ddF_y += 2;
		f += ddF_y;

		// These checks avoid double-drawing certain lines, important
		// for the SSD1306 library which has an INVERT drawing mode.
		if(y <= x) {
			if(quadrants & 0x01) {
				addLine(x0 - x, x0 + x + delta, y0 - y);
			}
			if(quadrants & 0x02) {
				addLine(x0 - x, x0 + x + delta, y0 + y);
			}
		}
		if(x != px) {
			if(quadrants & 0x01) {
				addLine(x0 - py, x0 + py + delta, y0 - px);
			}
			if(quadrants & 0x02) {
				addLine(x0 - py, x0 + py + delta, y0 + px);
			}
			px = x;
		}
		py = y;
	}
}

/* EllipseRenderer */

bool EllipseRenderer::execute(Surface& surface)
{
	if(state == State::init) {
		if(r.w <= 2 || r.h <= 2) {
			rectangles.add(r);
			state = State::done;
		} else {
			// Setup outer and inner ellipses
			Size size(r.size());
			outer = Ellipse(size);

			auto w2 = w * 2;
			size.w = (size.w > w2) ? size.w - w2 : 0;
			size.h = (size.h > w2) ? size.h - w2 : 0;
			inner = Ellipse(size);

			// determine ellipse rectangles
			r1 = Rect(r.x + outer.a, r.y, r.w % 2, 1);
			r2 = Rect(r1.x, r.bottom(), r1.size());

			prev = r1.topLeft();

			state = State::running;
		}
	}

	for(;;) {
		if(rectangles && !rectangles.render(surface)) {
			return false;
		}

		if(state == State::done) {
			break;
		}

		if(state >= State::final1) {
			final();
			continue;
		}

		if(outer.y == 0) {
			// Final steps
			if(outer.x > outer.a || prev.y >= r2.y) {
				break;
			}

			/* draw final line */
			r1.h = r1.y + r1.h - r2.y;
			r1.y = r2.y;

			W = w;
			if(r.x + W != prev.x) {
				W = std::max(W, uint16_t(prev.x - r.x));
			}

			state = (W + W >= r.w) ? State::final1 : State::final2;
			continue;
		}

		while(inner.y == outer.y) {
			innerX = inner.x;
			inner.step();
		}

		W = outer.x - innerX;
		if(r1.x + W < prev.x) {
			W = prev.x - r1.x;
		}
		W = std::max(W, w);

		auto step = outer.step();

		if(step[Ellipse::Move::down]) {
			if(r1.w == 0) {
				r1.x -= 1;
				r1.w += 2;
				r2.x -= 1;
				r2.w += 2;
				step -= Ellipse::Move::out;
			}

			if(r1.y == r2.y - 1) {
				r1.x = r2.x = r.x;
				r1.w = r2.w = r.w;
			} else {
				if(r1.x < r.x) {
					r1.x = r2.x = r.x;
				}
				if(r1.w > r.w) {
					r1.w = r2.w = r.w;
				}
			}

			if((r1.y < r.y + w) || (r1.x + W >= r1.x + r1.w - W)) {
				addRectangles1();
				prev = r1.topLeft();
			} else if(r1.y + r1.h < r2.y) {
				addRectangles2();
				prev = r1.topLeft();
			}

			/* move down */
			r1.y += 1;
			r2.y -= 1;
		}

		if(step[Ellipse::Move::out]) {
			/* move outwards */
			r1.x -= 1;
			r1.w += 2;
			r2.x -= 1;
			r2.w += 2;
		}
	}

	return true;
}

void EllipseRenderer::addRectangles1()
{
	rectangles.add(r1);
	rectangles.add(r2);
}

void EllipseRenderer::addRectangles2()
{
	rectangles.add(Rect(r1.x, r1.y, W, 1));
	rectangles.add(Rect(r1.x + r1.w - W, r1.y, W, 1));
	rectangles.add(Rect(r2.x, r2.y, W, 1));
	rectangles.add(Rect(r2.x + r2.w - W, r2.y, W, 1));
}

void EllipseRenderer::final()
{
	if(state == State::final1) {
		rectangles.add(Rect(r.x, r1.y, r.w, r1.h));
	} else {
		rectangles.add(Rect(r.x, r1.y, W, r1.h));
		rectangles.add(Rect(r.x + r.w - W, r1.y, W, r1.h));
	}
	state = State::done;
}

/* FilledEllipseRenderer */

/*
 *  To fill an axis-aligned ellipse, we use a scan-line algorithm.
 *  We walk downwards from the top Y coordinate, calculating
 *  the width of the ellipse using incremental integer arithmetic.
 *  To save calculation, we observe that the top and bottom halves
 *  of the ellipsoid are mirror-images, therefore we can draw the
 *  top and bottom halves by reflection. As a result, this algorithm
 *  draws rectangles inwards from the top and bottom edges of the
 *  bounding rectangle.
 *
 *  To save rendering time, draw as few rectangles as possible.
 *  Other ellipse-drawing algorithms assume we want to draw each
 *  line, using a draw_pixel operation, or a draw_horizontal_line
 *  operation. This approach is slower than it needs to be in
 *  circumstances where a fill_rect operation is more efficient
 *  (such as in X-Windows, where there is a communication overhead
 *  to the X-Server). For this reason, the algorithm accumulates
 *  rectangles on adjacent lines which have the same width into a
 *  single larger rectangle.
 *
 *  This algorithm forms the basis of the later, more complex,
 *  draw_ellipse algorithm, which renders the rectangular spaces
 *  between an outer and inner ellipse, and also the draw_arc and
 *  fill_arc operations which additionally clip drawing between
 *  a start_angle and an end_angle.
 *  
 */
bool FilledEllipseRenderer::execute(Surface& surface)
{
	if(state == State::init) {
		if(r.w <= 2 || r.h <= 2) {
			rectangles.add(r);
			state = State::done;
		} else {
			e = Ellipse(r.size());
			r1 = Rect(r.x + e.a, r.y, r.w % 2, 1);
			r2 = Rect(r1.x, r.bottom(), r1.size());
			state = State::running;
		}
	}

	for(;;) {
		if(rectangles && !rectangles.render(surface)) {
			return false;
		}

		if(state == State::done) {
			break;
		}

		if(state == State::final) {
			final();
			continue;
		}

		if(e.y == 0) {
			state = State::final;
			if(r1.y < r2.y) {
				/* overlap */
				r1 = Rect(r.x, r1.y, r.w, r2.y + r2.h - r1.y);
				continue;
			}
			if(e.x <= e.a) {
				/* crossover, draw final line */
				r1 = Rect(r.x, r2.y, r.w, r1.y + r1.h - r2.y);
				continue;
			}
			break;
		}

		doStep(e.step());
	}

	return true;
}

void FilledEllipseRenderer::doStep(Ellipse::Step step)
{
	if(step == (Ellipse::Move::down | Ellipse::Move::out)) {
		if((r1.w > 0) && (r1.h > 0)) {
			if(r1.y + r1.h < r2.y) {
				rectangles.add(r1);
				rectangles.add(r2);
			}

			/* move down */
			r1.y += r1.h;
			r1.h = 1;
			r2.y -= 1;
			r2.h = 1;
			step -= Ellipse::Move::down;
		}
	}
	if(step[Ellipse::Move::out]) {
		r1.x -= 1;
		r1.w += 2;
		r2.x -= 1;
		r2.w += 2;
	}
	if(step[Ellipse::Move::down]) {
		r1.h += 1;
		r2.h += 1;
		r2.y -= 1;
	}
}

void FilledEllipseRenderer::final()
{
	rectangles.add(r1);
	state = State::done;
}

/*
 *  Draw an arc of an ellipse from start_angle anti-clockwise to
 *  end_angle. If the angles coincide, draw nothing; if they
 *  differ by 360 degrees or more, draw a full ellipse.
 *  The shape is drawn with the current line thickness,
 *  completely within the bounding rectangle. The shape is also
 *  axis-aligned, so that the ellipse would be horizontally and
 *  vertically symmetric is it was complete.
 *
 *  The draw_arc algorithm is based on draw_ellipse, but unlike
 *  that algorithm is not symmetric in the general case, since
 *  an angular portion is clipped from the shape. 
 *  This clipping is performed by keeping track of two hypothetical
 *  lines joining the centre point to the enclosing rectangle,
 *  at the angles start_angle and end_angle, using a line-intersection
 *  algorithm. Essentially the algorithm just fills the spaces
 *  which are within the arc and also between the angles, going
 *  in an anti-clockwise direction from start_angle to end_angle.
 *  In the top half of the ellipse, this amounts to drawing
 *  to the left of the start_angle line and to the right of
 *  the end_angle line, while in the bottom half of the ellipse,
 *  it involves drawing to the right of the start_angle and to
 *  the left of the end_angle.
 */

/*
 *  Fill a rectangle within an arc, given the centre point p0,
 *  and the two end points of the lines corresponding to the
 *  start_angle and the end_angle. This function takes care of
 *  the logic needed to swap the fill direction below
 *  the central point, and also performs the calculations
 *  needed to intersect the current Y value with each line.
 */
void ArcRectList::fill(const Rect& r, Point p0, Point p1, Point p2, int start_angle, int end_angle)
{
	long rise1 = p1.y - p0.y;
	long run1 = p1.x - p0.x;
	long rise2 = p2.y - p0.y;
	long run2 = p2.x - p0.x;

	int x1, x2;
	bool start_above;
	bool end_above;

	if(r.y <= p0.y) {
		/* in top half of arc ellipse */

		if(p1.y <= r.y) {
			/* start_line is in the top half and is */
			/* intersected by the current Y scan line */
			if(rise1 == 0) {
				x1 = p1.x;
			} else {
				x1 = p0.x + (r.y - p0.y) * run1 / rise1;
			}
			start_above = true;
		} else if((start_angle >= 0) && (start_angle <= 180)) {
			/* start_line is above middle */
			x1 = p1.x;
			start_above = true;
		} else {
			/* start_line is below middle */
			x1 = r.x + r.w;
			start_above = false;
		}
		if(x1 < r.x) {
			x1 = r.x;
		}
		if(x1 > r.x + r.w) {
			x1 = r.x + r.w;
		}

		if(p2.y <= r.y) {
			/* end_line is in the top half and is */
			/* intersected by the current Y scan line */
			if(rise2 == 0) {
				x2 = p2.x;
			} else {
				x2 = p0.x + (r.y - p0.y) * run2 / rise2;
			}
			end_above = true;
		} else if((end_angle >= 0) && (end_angle <= 180)) {
			/* end_line is above middle */
			x2 = p2.x;
			end_above = true;
		} else {
			/* end_line is below middle */
			x2 = r.x;
			end_above = false;
		}
		if(x2 < r.x) {
			x2 = r.x;
		}
		if(x2 > r.x + r.w) {
			x2 = r.x + r.w;
		}

		if(start_above && end_above) {
			if(start_angle > end_angle) {
				/* fill outsides of wedge */
				add(Rect(r.x, r.y, x1 - r.x, r.h));
				return add(Rect(x2, r.y, r.x + r.w - x2, r.h));
			}
			/* fill inside of wedge */
			return add(Rect(x2, r.y, x1 - x2, r.h));
		}
		if(start_above) {
			/* fill to the left of the start_line */
			return add(Rect(r.x, r.y, x1 - r.x, r.h));
		}
		if(end_above) {
			/* fill right of end_line */
			return add(Rect(x2, r.y, r.x + r.w - x2, r.h));
		}
		if(start_angle > end_angle) {
			return add(r);
		}
		return;
	}

	/* in lower half of arc ellipse */
	if(p1.y >= r.y) {
		/* start_line is in the lower half and is */
		/* intersected by the current Y scan line */
		if(rise1 == 0) {
			x1 = p1.x;
		} else {
			x1 = p0.x + (r.y - p0.y) * run1 / rise1;
		}
		start_above = false;
	} else if((start_angle >= 180) && (start_angle <= 360)) {
		/* start_line is below middle */
		x1 = p1.x;
		start_above = false;
	} else {
		/* start_line is above middle */
		x1 = r.x;
		start_above = true;
	}
	if(x1 < r.x) {
		x1 = r.x;
	}
	if(x1 > r.x + r.w) {
		x1 = r.x + r.w;
	}

	if(p2.y >= r.y) {
		/* end_line is in the lower half and is */
		/* intersected by the current Y scan line */
		if(rise2 == 0) {
			x2 = p2.x;
		} else {
			x2 = p0.x + (r.y - p0.y) * run2 / rise2;
		}
		end_above = false;
	} else if((end_angle >= 180) && (end_angle <= 360)) {
		/* end_line is below middle */
		x2 = p2.x;
		end_above = false;
	} else {
		/* end_line is above middle */
		x2 = r.x + r.w;
		end_above = true;
	}
	if(x2 < r.x) {
		x2 = r.x;
	}
	if(x2 > r.x + r.w) {
		x2 = r.x + r.w;
	}

	if(start_above && end_above) {
		if(start_angle > end_angle) {
			add(r);
		}
		return;
	}
	if(start_above) {
		/* fill to the left of end_line */
		return add(Rect(r.x, r.y, x2 - r.x, r.h));
	}
	if(end_above) {
		/* fill right of start_line */
		return add(Rect(x1, r.y, r.x + r.w - x1, r.h));
	}
	if(start_angle > end_angle) {
		/* fill outsides of wedge */
		add(Rect(r.x, r.y, x2 - r.x, r.h));
		return add(Rect(x1, r.y, r.x + r.w - x1, r.h));
	}
	/* fill inside of wedge */
	return add(Rect(x1, r.y, x2 - x1, r.h));
}

static float degreesToRadians(int deg)
{
	return deg * 2 * M_PI / 360;
}

static Point getBoundaryPoint(const Rect& r, int16_t angle)
{
	Point centre = r.centre();

	switch(angle) {
	case 0:
		return Point(r.x + r.w, centre.y);
	case 45:
		return Point(r.x + r.w, r.y);
	case 90:
		return Point(centre.x, r.y);
	case 135:
		return Point(r.x, r.y);
	case 180:
		return Point(r.x, centre.y);
	case 225:
		return Point(r.x, r.y + r.h);
	case 270:
		return Point(centre.x, r.y + r.h);
	case 315:
		return Point(r.x + r.w, r.y + r.h);
	}

	float tangent = tan(degreesToRadians(angle));

	if(angle > 315) {
		return Point(r.x + r.w, centre.y - r.w * tangent / 2);
	}
	if(angle > 225) {
		return Point(centre.x - r.h / tangent / 2, r.y + r.h);
	}
	if(angle > 135) {
		return Point(r.x, centre.y + r.w * tangent / 2);
	}
	return Point(centre.x + r.h / tangent / 2, r.y);
}

Ellipse::Step Ellipse::step()
{
	if(t + a2 * y < xcrit) { /* e(x+1,y-1/2) <= 0 */
		/* move outwards to encounter edge */
		x += 1;
		t += dxt;
		dxt += d2xt;
		return Move::out;
	}
	if(t - b2 * x >= ycrit) { /* e(x+1/2,y-1) > 0 */
		/* drop down one line */
		y -= 1;
		t += dyt;
		dyt += d2yt;
		return Move::down;
	}
	/* drop diagonally down and out */
	x += 1;
	y -= 1;
	t += dxt + dyt;
	dxt += d2xt;
	dyt += d2yt;

	return Move::down | Move::out;
}

/* ArcRenderer */

bool ArcRenderer::execute(Surface& surface)
{
	if(state == State::init) {
		/* draw nothing if the angles are equal */
		if(start_angle == end_angle) {
			return true;
		}

		// find arc wedge line end points
		p0 = r.centre();
		p1 = getBoundaryPoint(r, start_angle);
		p2 = getBoundaryPoint(r, end_angle);
	}

	return EllipseRenderer::execute(surface);
}

void ArcRenderer::final()
{
	if(r1.h == 0) {
		state = State::done;
		return;
	}
	if(state == State::final2) {
		rectangles.fill(Rect(r.x, r1.y, W, 1), p0, p1, p2, start_angle, end_angle);
		rectangles.fill(Rect(r.x + r.w - W, r1.y, W, 1), p0, p1, p2, start_angle, end_angle);
	} else {
		rectangles.fill(Rect(r.x, r1.y, r.w, 1), p0, p1, p2, start_angle, end_angle);
	}
	r1.y += 1;
	r1.h -= 1;
}

void ArcRenderer::addRectangles1()
{
	rectangles.fill(r1, p0, p1, p2, start_angle, end_angle);
	rectangles.fill(r2, p0, p1, p2, start_angle, end_angle);
}

void ArcRenderer::addRectangles2()
{
	rectangles.fill(Rect(r1.x, r1.y, W, 1), p0, p1, p2, start_angle, end_angle);
	rectangles.fill(Rect(r1.x + r1.w - W, r1.y, W, 1), p0, p1, p2, start_angle, end_angle);
	rectangles.fill(Rect(r2.x, r2.y, W, 1), p0, p1, p2, start_angle, end_angle);
	rectangles.fill(Rect(r2.x + r2.w - W, r2.y, W, 1), p0, p1, p2, start_angle, end_angle);
}

/* FilledArcRenderer */

bool FilledArcRenderer::execute(Surface& surface)
{
	if(state == State::init) {
		/* draw nothing if the angles are equal */
		if(start_angle == end_angle) {
			return true;
		}

		/* find arc wedge line end points */
		p0 = r.centre();
		p1 = getBoundaryPoint(r, start_angle);
		p2 = getBoundaryPoint(r, end_angle);
	}

	return FilledEllipseRenderer::execute(surface);
}

void FilledArcRenderer::doStep(Ellipse::Step step)
{
	if(step[Ellipse::Move::down]) {
		if(r1.w == 0) {
			r1.x -= 1;
			r1.w += 2;
			r2.x -= 1;
			r2.w += 2;
			step -= Ellipse::Move::out;
		}

		if(r1.y == r2.y - 1) {
			r1.x = r2.x = r.x;
			r1.w = r2.w = r.w;
		} else {
			if(r1.x < r.x) {
				r1.x = r2.x = r.x;
			}
			if(r1.w > r.w) {
				r1.w = r2.w = r.w;
			}
		}

		if((r1.w > 0) && (r1.y + r1.h < r2.y)) {
			rectangles.fill(r1, p0, p1, p2, start_angle, end_angle);
			rectangles.fill(r2, p0, p1, p2, start_angle, end_angle);
		}

		/* move down */
		r1.y += 1;
		r2.y -= 1;
	}

	if(step[Ellipse::Move::out]) {
		r1.x -= 1;
		r1.w += 2;
		r2.x -= 1;
		r2.w += 2;
	}
}

void FilledArcRenderer::final()
{
	if(r1.h <= 0) {
		state = State::done;
		return;
	}
	rectangles.fill({r1.x, r1.y, r1.w, 1}, p0, p1, p2, start_angle, end_angle);
	r1.y += 1;
	r1.h -= 1;
}

/* ImageRenderer */

bool ImageRenderer::execute(Surface& surface)
{
	if(pixelFormat == PixelFormat::None) {
		// Crop area to be loaded
		auto size = surface.getSize();
		auto& r = location.dest;
		if(r.x + r.w > size.w) {
			r.w = size.w - r.x;
		}
		if(r.y + r.h > size.h) {
			r.h = size.h - r.y;
		}
		auto imgSize = object.getSize();
		r.w = std::min(r.w, imgSize.w);
		r.h = std::min(r.h, imgSize.h);
		r.w = std::min(r.w, location.source.w);
		r.h = std::min(r.h, location.source.h);
		if(!surface.setAddrWindow(r)) {
			return false;
		}
		pixelFormat = surface.getPixelFormat();
		bytesPerPixel = getBytesPerPixel(pixelFormat);
	}

	/*
	 * Normally we'd expect to be able to refill surface buffers faster than the data
	 * is transferred over SPI to the display. However, if reading from a resource in flash
	 * this may no longer be the case so use a timer to guard against hogging the CPU.
	 */
	OneShotFastMs timeout;
	timeout.reset<50>();

	uint16_t available{0};
	uint8_t* buffer{nullptr};
	uint8_t* bufptr{nullptr};
	auto& loc = location;
	while(loc.pos.y < loc.dest.h) {
		if(available < 8) {
			if(buffer != nullptr) {
				surface.commit(bufptr - buffer);
			}
			if(timeout.expired()) {
				return false;
			}
			bufptr = buffer = surface.getBuffer(bytesPerPixel, available);
			if(buffer == nullptr) {
				return false;
			}
			available /= bytesPerPixel;
		}

		auto count = std::min(available, uint16_t(loc.dest.w - loc.pos.x));
		if(count != 0) {
			bufptr += object.readPixels(loc, pixelFormat, bufptr, count);
			loc.pos.x += count;
			available -= count;
		}
		if(loc.pos.x == loc.dest.w) {
			loc.pos.x = 0;
			++loc.pos.y;
		}
	}
	if(bufptr != buffer) {
		surface.commit(bufptr - buffer);
	}
	return true; // All done
}

/* SurfaceRenderer */

bool SurfaceRenderer::execute(Surface& surface)
{
	if(done) {
		return busyCount == 0;
	}

	if(target == surface) {
		debug_e("[GRAPHICS] Cannot render from same surface");
		return true;
	}

	if(pixelFormat == PixelFormat::None) {
		if(!target.setAddrWindow(dest)) {
			return false;
		}

		Rect r(location.source.topLeft() + source, dest.size());
		if(!surface.setAddrWindow(r)) {
			return false;
		}

		pixelFormat = target.getPixelFormat();
		buffers[0] = ReadBuffer{pixelFormat, bufSize};
		buffers[1] = ReadBuffer{pixelFormat, bufSize};
	}

	if(busyCount == 2) {
		return false;
	}

	auto& buffer = buffers[bufIndex];
	int pixelsQueued = surface.readDataBuffer(
		buffer, nullptr,
		[](ReadBuffer& buffer, size_t length, void* param) {
			auto self = static_cast<SurfaceRenderer*>(param);
			self->target.writeDataBuffer(buffer.data, 0, length);
			--self->busyCount;
		},
		this);
	if(pixelsQueued == 0) {
		done = true;
	} else if(pixelsQueued > 0) {
		++busyCount;
		bufIndex ^= 1;
	}
	return false;
}

/* CopyRenderer */

void CopyRenderer::init()
{
	debug_g("Copy %s -> %s", location.source.toString().c_str(), location.dest.toString().c_str());

	unsigned xshift = abs(location.source.x - location.dest.x);
	unsigned yshift = abs(location.source.y - location.dest.y);
	if(xshift > yshift) {
		// Copy vertical lines
		vertical = true;
		lineCount = location.source.w;
		if(location.source.x < location.dest.x) {
			location.source.x = location.source.right();
			location.dest.x = location.dest.right();
			shift.x = -1;
		} else {
			shift.x = 1;
		}
		location.source.w = 1;
		location.dest.w = 1;
		lineSize = location.source.h;
	} else {
		// Copy horizontal lines
		vertical = false;
		lineCount = location.source.h;
		if(location.source.y < location.dest.y) {
			location.source.y = location.source.bottom();
			location.dest.y = location.dest.bottom();
			shift.y = -1;
		} else {
			shift.y = 1;
		}
		location.source.h = 1;
		location.dest.h = 1;
		lineSize = location.source.w;
	}

	// Assume that reading requires space for full 24-bit RGB (e.g. ILI9341)
	auto bufSize = lineSize * Surface::READ_PIXEL_SIZE;
	lineBuffers[0] = LineBuffer{pixelFormat, bufSize};
	lineBuffers[1] = LineBuffer{pixelFormat, bufSize};
}

bool CopyRenderer::execute(Surface& surface)
{
	if(pixelFormat == PixelFormat::None) {
		pixelFormat = surface.getPixelFormat();
		bytesPerPixel = getBytesPerPixel(pixelFormat);

		init();
		startRead(surface);
		return false;
	}

	if(writeIndex >= lineCount) {
		return true;
	}

	// Convert and write line just read
	auto& buffer = lineBuffers[writeIndex % 2];
	if(buffer.status.readComplete) {
		readComplete(buffer.data.get(), buffer.status.bytesRead);
		// debug_d("WRITE sfc %p, buffer %p, #%u, %s", &surface, &buffer, writeIndex % 2, location.dest.toString().c_str());
		surface.setAddrWindow(location.dest);
		if(!surface.writeDataBuffer(buffer.data, 0, buffer.status.bytesRead)) {
			debug_w("[writeDataBuffer] FAIL");
		}
		location.dest.x += shift.x;
		location.dest.y += shift.y;
		++writeIndex;
	} else {
		// debug_d("WRITE buffer %u not ready", writeIndex % 2);
	}

	// Setup next read
	if(readIndex < lineCount) {
		startRead(surface);
	}

	return false;
}

void CopyRenderer::startRead(Surface& surface)
{
	// debug_d("READ sfc %p, buffer %p #%u, %s", &surface, &buffer, readIndex % 2, location.source.toString().c_str());
	surface.setAddrWindow(location.source);
	if(!surface.readDataBuffer(lineBuffers[readIndex % 2])) {
		debug_w("[readPixels] FAIL");
	}
	location.source.x += shift.x;
	location.source.y += shift.y;
	++readIndex;
}

/* ImageCopyRenderer */

void ImageCopyRenderer::readComplete(uint8_t* data, size_t length)
{
	if(blend == nullptr) {
		return;
	}
	auto loc = location;
	loc.source.x = loc.source.y = 0;
	size_t bufSize = loc.dest.w * bytesPerPixel;
	uint8_t buffer[bufSize];

	if(vertical) {
		while(length != 0) {
			image.readPixels(loc, pixelFormat, buffer, loc.dest.w);
			++loc.pos.y;
			blend->transform(pixelFormat, buffer, data, bufSize);
			data += bufSize;
			length -= bufSize;
		}
		++location.pos.x;
	} else {
		image.readPixels(loc, pixelFormat, buffer, loc.dest.w);
		blend->transform(pixelFormat, buffer, data, bufSize);
		++location.pos.y;
	}
}

/* ScrollRenderer */

void ScrollRenderer::init()
{
	fill = pack(object.fill, pixelFormat);

	cx = object.shift.x;
	cy = object.shift.y;
	location.source = object.area + location.dest.topLeft();
	src.w = dst.w = location.source.w;
	src.h = dst.h = location.source.h;
	readArea = src;
	writeArea = dst;
	if(cx < 0) {
		dst.x = dst.w + cx;
	} else {
		dst.x = cx;
	}
	if(cy < 0) {
		dst.y = dst.h + cy;
	} else {
		dst.y = cy;
	}
	if(uint32_t(object.fill) == 0) {
		if(!object.wrapx) {
			dst.w -= abs(cx);
		}
		if(!object.wrapy) {
			dst.h -= abs(cy);
		}
	}

	debug_g("Copy (%s) -> (%s), %d, %d, %u, %u", src.toString().c_str(), dst.toString().c_str(), cx, cy, object.wrapx,
			object.wrapy);

	if(src.h > src.w) {
		// Copy columns
		vertical = true;
		if(cy != 0) {
			if(object.wrapy) {
				if(cy > 0) {
					writeOffset = cy;
				} else {
					writeOffset = dst.h + cy;
				}
				writeOffset *= bytesPerPixel;
			} else if(cy > 0) {
				readOffset = cy * bytesPerPixel;
				src.h -= cy;
			} else {
				src.y = -cy;
				src.h += cy;
			}
			dst.y = 0;
		}
		lineCount = src.w;
		if(!object.wrapx) {
			if(cx > 0) {
				src.w -= cx;
				writeArea.x = cx;
			} else {
				src.w += cx;
				readArea.x = -cx;
			}
			readArea.w = writeArea.w = src.w;
		}
		if(cx < 0) {
			src.x = checkx(src.x + src.w - 1);
			dst.x = checkx(dst.x + src.w - 1);
		}
		src.w = dst.w = 1;
	} else {
		// Copy rows
		vertical = false;
		if(cx != 0) {
			if(object.wrapx) {
				if(cx > 0) {
					writeOffset = cx;
				} else {
					writeOffset = dst.w + cx;
				}
				writeOffset *= bytesPerPixel;
			} else if(cx > 0) {
				readOffset = cx * bytesPerPixel;
				src.w -= cx;
			} else {
				src.x = -cx;
				src.w += cx;
			}
			dst.x = 0;
		}
		lineCount = src.h;
		if(!object.wrapy) {
			if(cy > 0) {
				src.h -= cy;
				writeArea.y = cy;
			} else {
				src.h += cy;
				readArea.y = -cy;
			}
			readArea.h = writeArea.h = src.h;
		}
		if(cy < 0) {
			src.y = checky(src.y + src.h - 1);
			dst.y = checky(dst.y + src.h - 1);
		}
		src.h = dst.h = 1;
	}

	// Enough space for a full line
	auto bufSize = (vertical ? location.source.h : location.source.w) * Surface::READ_PIXEL_SIZE;
	lineBuffers[0] = LineBuffer(pixelFormat, bufSize);
	lineBuffers[1] = LineBuffer(pixelFormat, bufSize);
}

bool ScrollRenderer::execute(Surface& surface)
{
	if(pixelFormat == PixelFormat::None) {
		pixelFormat = surface.getPixelFormat();
		bytesPerPixel = getBytesPerPixel(pixelFormat);
		init();
	}

	if(writeIndex >= lineCount) {
		return true;
	}

	if(state < 2) {
		if(startRead(surface)) {
			++state;
		}
		return false;
	}

	// Convert and write line just read
	auto& buffer = lineBuffers[writeIndex % 2];
	if(buffer.status.readComplete) {
		debug_g("WRITE sfc %p, %s", &surface, dst.toString().c_str());

		auto data = buffer.data.get();
		auto length = (vertical ? dst.h : dst.w) * bytesPerPixel;

		if(writeOffset != 0) {
			uint8_t tmp[writeOffset];
			memcpy(tmp, &data[length - writeOffset], writeOffset);
			memmove(&data[writeOffset], data, length - writeOffset);
			memcpy(data, tmp, writeOffset);
		}

		if(!object.wrapx) {
			if(vertical) {
				if(dst.x < writeArea.left() || dst.x > writeArea.right()) {
					debug_g("FILL sfc %p, buffer #%u, %s", &surface, writeIndex % 2, toString(dst).c_str());
					writeColor(data, fill, pixelFormat, dst.h);
				}
			} else if(cx > 0) {
				writeColor(data, fill, pixelFormat, cx);
			} else if(cx < 0) {
				writeColor(&data[src.w * bytesPerPixel], fill, pixelFormat, -cx);
			}
		}

		if(!object.wrapy) {
			if(!vertical) {
				if(dst.y < writeArea.top() || dst.y > writeArea.bottom()) {
					debug_g("FILL sfc %p, buffer #%u, %s", &surface, writeIndex % 2, dst.toString().c_str());
					writeColor(data, fill, pixelFormat, dst.w);
				}
			} else if(cy > 0) {
				writeColor(data, fill, pixelFormat, cy);
			} else if(cy < 0) {
				writeColor(&data[src.h * bytesPerPixel], fill, pixelFormat, -cy);
			}
		}

		surface.setAddrWindow(dst + location.source.topLeft());
		if(!surface.writeDataBuffer(buffer.data, 0, length)) {
			debug_w("[writeDataBuffer] FAIL");
		}
		if(vertical) {
			stepx(writeIndex, dst.x);
		} else {
			stepy(writeIndex, dst.y);
		}
	}

	// Setup next read
	if(readIndex < lineCount) {
		startRead(surface);
	}

	return false;
}

bool ScrollRenderer::startRead(Surface& surface)
{
	auto& buffer = lineBuffers[readIndex % 2];

	// Check if line is required, skip to next if not
	if(vertical) {
		if(!object.wrapx) {
			if(src.x < readArea.left() || src.x > readArea.right()) {
				debug_g("SKIP sfc %p, buffer #%u, %s", &surface, readIndex % 2, src.toString().c_str());
				buffer.status.readComplete = true;
				stepx(readIndex, src.x);
				return true;
			}
		}
	} else if(!object.wrapy) {
		if(src.y < readArea.top() || src.y > readArea.bottom()) {
			debug_g("SKIP sfc %p, buffer #%u, %s", &surface, readIndex % 2, src.toString().c_str());
			buffer.status.readComplete = true;
			stepy(readIndex, src.y);
			return true;
		}
	}

	debug_g("READ sfc %p, buffer #%u, %s", &surface, readIndex % 2, src.toString().c_str());
	if(!surface.setAddrWindow(src + location.source.topLeft())) {
		return false;
	}
	buffer.format = pixelFormat;
	buffer.offset = readOffset;

	if(surface.readDataBuffer(buffer) <= 0) {
		debug_w("[readPixels] FAIL");
		return false;
	}
	if(vertical) {
		stepx(readIndex, src.x);
	} else {
		stepy(readIndex, src.y);
	}
	return true;
}

int16_t ScrollRenderer::checkx(int16_t x)
{
	if(x < 0) {
		x += location.source.w;
	} else if(x >= location.source.w) {
		x -= location.source.w;
	}
	return x;
}

int16_t ScrollRenderer::checky(int16_t y)
{
	if(y < 0) {
		y += location.source.h;
	} else if(y >= location.source.h) {
		y -= location.source.h;
	}
	return y;
}

// value is never zero
int16_t sign(int16_t value)
{
	return (value < 0) ? -1 : 1;
}

void ScrollRenderer::stepx(uint16_t& index, int16_t& x)
{
	++index;
	if((index * cx) % location.source.w == 0) {
		x = checkx(x + sign(cx));
	} else {
		x = checkx(x + cx);
	}
}

void ScrollRenderer::stepy(uint16_t& index, int16_t& y)
{
	++index;
	if((index * cy) % location.source.h == 0) {
		y = checky(y + sign(cy));
	} else {
		y = checky(y + cy);
	}
}

/* BlendRenderer */

bool BlendRenderer::execute(Surface& surface)
{
	for(;;) {
		if(!surface.execute(renderer)) {
			return false;
		}

		switch(nextState) {
		case State::init:
			pixelFormat = surface.getPixelFormat();
			image = std::make_unique<MemoryImageObject>(pixelFormat, location.dest.size());
			if(!image->isValid()) {
				image.reset();
				// Insufficient RAM, fallback to standard render
				if(!surface.render(object, location.dest, renderer)) {
					return false;
				}
				nextState = State::done;
				break;
			}
			imageSurface.reset(image->createSurface());
			renderer = std::make_unique<SurfaceRenderer>(Location{image->getSize(), location.source}, *imageSurface,
														 image->getSize(), location.dest.topLeft());
			nextState = State::draw;
			break;

		case State::draw: {
			auto blendSurface = image->createSurface(blend);
			blendSurface->render(object, image->getSize());
			delete blendSurface;
			renderer = std::make_unique<ImageRenderer>(location, *image);
			nextState = State::done;
			break;
		}

		case State::done:
			return true;
		}
	}
}

/* TextRenderer */

bool TextRenderer::execute(Surface& surface)
{
	if(pixelFormat == PixelFormat::None) {
		if(element == nullptr) {
			return true;
		}

		pixelFormat = surface.getPixelFormat();
		bytesPerPixel = getBytesPerPixel(pixelFormat);

		// Determine maximum glyph height and initialise alpha buffer
		Size size{};
		for(auto& e : object.elements) {
			if(e.kind != TextObject::Element::Kind::Font) {
				continue;
			}
			auto font = static_cast<const TextObject::FontElement&>(e);
			size.h = std::max(size.h, uint16_t(font.typeface.height()));
		}
		size.w = size.h * 3;
		alphaBuffer.init(size);
		backBuffers[0].format = pixelFormat;
		backBuffers[1].format = pixelFormat;
		getNextRun();
		alphaBuffer.fill();
		startRead(surface);
		startRead(surface);
	}

	for(;;) {
		if(run == nullptr) {
			if(busyCount == 0) {
				return true;
			}
		} else if(busyCount < 2 && !startRead(surface)) {
			return false;
		}

		auto& backBuffer = backBuffers[writeIndex];
		if(!backBuffer.status.readComplete) {
			// debug_i("BUSY");
			return false;
		}
		// debug_i("[READ] (%s), %u bytes, %u pixels", backBuffer.r.toString().c_str(), backBuffer.status.bytesRead,
		// 		backBuffer.status.bytesRead / bytesPerPixel);
		if(!renderBuffer(surface, backBuffer)) {
			return false;
		}
		--busyCount;
		writeIndex ^= 1;
		backBuffer.status.readComplete = false;

		// debug_i("WRITE %u", len);
		if(backBuffer.lastRow) {
			alphaBuffer.shift(backBuffer.glyphPixels);
			alphaBuffer.fill();
		}

		return false;
	}
}

void TextRenderer::getNextRun()
{
	run = nullptr;
	for(; element != nullptr; element = element->getNext()) {
		switch(element->kind) {
		case TextObject::Element::Kind::Font: {
			auto elem = static_cast<const TextObject::FontElement*>(element);
			typeface = &elem->typeface;
			options.scale = elem->scale;
			options.style = elem->style;
			if(options.scale.scaleX() <= 1) {
				options.style -= FontStyle::DotMatrix | FontStyle::VLine;
			}
			if(options.scale.scaleY() <= 1) {
				options.style -= FontStyle::DotMatrix | FontStyle::HLine;
			}
			break;
		}
		case TextObject::Element::Kind::Color: {
			auto elem = static_cast<const TextObject::ColorElement*>(element);
			options.fore = elem->fore;
			options.back = elem->back;
			options.setPixelFormat(pixelFormat);
			break;
		}
		case TextObject::Element::Kind::Run: {
			run = static_cast<const TextObject::RunElement*>(element);
			// Skip any runs which fall outside the destination area
			if(location.pos.y + run->pos.y >= location.dest.h) {
				run = nullptr;
				continue;
			}

			location.pos = Point{};
			element = element->getNext();
			return;
		}
		case TextObject::Element::Kind::Text:
			break;
		}
	}
}

void TextRenderer::AlphaBuffer::fill()
{
	// bool changed{false};

	for(; element != nullptr; element = element->getNext()) {
		if(element->kind == TextObject::Element::Kind::Text) {
			text = &static_cast<const TextObject::TextElement*>(element)->text;
			continue;
		}
		if(element->kind == TextObject::Element::Kind::Font) {
			font = static_cast<const TextObject::FontElement*>(element);
			continue;
		}
		if(element->kind != TextObject::Element::Kind::Run) {
			continue;
		}
		auto run = static_cast<const TextObject::RunElement*>(element);
		while(run->pos.y < ymax && charIndex < run->length) {
			char ch = text->read(run->offset + charIndex);
			auto charMetrics = font->typeface.getMetrics(ch);

			if(x + (charMetrics.advance * 2) > size.w) {
				return;
			}
			x -= advdiff;
			if(x + charMetrics.xOffset < 0) {
				debug_e("[[FONT X2]] %u, %d", x, charMetrics.xOffset);
				x = -charMetrics.xOffset;
			}

			auto glyph = font->typeface.getGlyph(ch, {});
			if(glyph) {
				glyph->readAlpha(data.get(), Point(x, 0), size.w);
				glyph.reset();

				auto line = [&](int8_t line) {
					// Typeface may not  have room for this
					if(line < font->typeface.height()) {
						memset(&data[x + size.w * line], 0xff, charMetrics.advance);
					}
				};

				auto baseline = font->typeface.baseline();
				if(font->style[FontStyle::Underscore]) {
					line(baseline + 1);
				}
				if(font->style[FontStyle::DoubleUnderscore]) {
					line(baseline + 1);
					line(baseline + 3);
				}
				if(font->style[FontStyle::Overscore]) {
					line(1);
				}
				if(font->style[FontStyle::DoubleOverscore]) {
					line(1);
					line(3);
				}
				if(font->style[FontStyle::Strikeout]) {
					line(charMetrics.height / 2);
				}
				if(font->style[FontStyle::DoubleStrikeout]) {
					uint8_t c = charMetrics.height / 2;
					line(c - 1);
					line(c + 2);
				}
			}

			auto x1 = x + charMetrics.advance;
			auto x2 = x + charMetrics.xOffset + charMetrics.width;
			if(x1 >= x2) {
				advdiff = 0;
				x = x1;
			} else {
				advdiff = x2 - x1;
				x = x2;
			}

			++charIndex;

			// changed = true;
		}

		charIndex = 0;
		x -= advdiff;
		advdiff = 0;
	}

	// if(!changed) {
	// 	return;
	// }
	// String s;
	// for(unsigned y = 0; y < alphaBuffer.size.h; ++y) {
	// 	unsigned off = y * alphaBuffer.size.w;
	// 	for(unsigned x = 0; x < alphaBuffer.pos.x; ++x) {
	// 		s += alphaBuffer.data[off + x] ? '@' : ' ';
	// 	}
	// 	s += "\r\n";
	// }
	// m_nputs(s.c_str(), s.length());
}

bool TextRenderer::startRead(Surface& surface)
{
	for(;;) {
		if(run == nullptr) {
			return true;
		}
		if(location.pos.x >= std::min(run->width, location.dest.w)) {
			getNextRun();
			continue;
		}
		uint16_t w = options.scale.scaleX(alphaBuffer.size.w / 3);
		w = std::min(w, uint16_t(run->width - location.pos.x));
		uint16_t glyphHeight = options.scale.scaleY(typeface->height());
		uint16_t h =
			std::min(size_t(glyphHeight - location.pos.y), BackBuffer::bufSize / (w * Surface::READ_PIXEL_SIZE));
		if(h == 0) {
			debug_e("[[TEXT]] Buffer too small");
			assert(false);
		}
		Rect r(location.destPos() + run->pos, w, h);

		auto& backBuffer = backBuffers[readIndex];

		auto rc = intersect(r, location.dest);
		if(!rc) {
			backBuffer.status.readComplete = true;
		} else if(options.back && !options.back.isTransparent()) {
			backBuffer.status.readComplete = true;
		} else if(!surface.setAddrWindow(rc)) {
			return false;
		} else if(surface.readDataBuffer(backBuffer) < 0) {
			return false;
		}

		++busyCount;
		readIndex ^= 1;
		backBuffer.glyphPixels = options.scale.unscaleX(w);
		backBuffer.r = rc;
		backBuffer.pos = location.pos;
		backBuffer.run = run;
		backBuffer.options = options;
		// debug_i("READ STARTED: surface %p, buffer %p, (%s), (%s)", &surface, &backBuffer, r.toString().c_str(),
		// 		location.pos.toString().c_str());
		location.pos.y += r.h;
		backBuffer.lastRow = (location.pos.y == glyphHeight || r.bottom() == object.bounds.bottom());
		if(backBuffer.lastRow) {
			location.pos.x += w;
			location.pos.y = 0;
		}

		return true;
	}
}

bool TextRenderer::renderBuffer(Surface& surface, BackBuffer& backBuffer)
{
	if(!backBuffer.r) {
		return true;
	}
	if(!surface.setAddrWindow(backBuffer.r)) {
		return false;
	}

	// debug_i("READ COMPLETE: surface %p, buffer %p", &surface, &backBuffer);

	auto& options = backBuffer.options;
	auto s = options.scale.scale();
	auto bufptr = backBuffer.data.get();
	Location loc{location.dest, options.scale.scale(alphaBuffer.size), backBuffer.pos};

	if(options.back) {
		auto numPixels = backBuffer.r.w * backBuffer.r.h;
		if(options.back.isTransparent()) {
			auto cl = options.back.getPackedColor();
			BlendAlpha::blend(pixelFormat, cl, backBuffer.data.get(), numPixels * bytesPerPixel);
		} else {
			options.back.writePixels(loc, backBuffer.data.get(), numPixels);
		}
	}

	// Check for negative start x
	uint8_t baseOffset{0};
	int x = location.dest.x + backBuffer.pos.x + backBuffer.run->pos.x;
	if(x < 0) {
		baseOffset = options.scale.unscaleX(-x);
	}

	for(uint16_t y = 0; y < backBuffer.r.h; ++y) {
		uint16_t off = baseOffset + options.scale.unscaleY(backBuffer.pos.y) * alphaBuffer.size.w;
		// debug_i("(%s), (%s), %u", backBuffer.r.toString().c_str(), backBuffer.pos.toString().c_str(), off);
		for(uint16_t x = 0; x < backBuffer.r.w; x += s.w, ++off) {
			auto alpha = alphaBuffer.data[off];
			// m_putc(alpha ? '@' : ' ');
			if(alpha == 0) {
				continue;
			}
			uint8_t len;
			if(options.style[FontStyle::DotMatrix]) {
				if(backBuffer.pos.y % s.h != 0) {
					continue;
				}
				len = 1;
			} else if(options.style[FontStyle::HLine]) {
				if(backBuffer.pos.y % s.h != 0) {
					continue;
				}
				len = s.w;
			} else if(options.style[FontStyle::VLine]) {
				len = 1;
			} else {
				len = s.w;
			}
			loc.pos.x = backBuffer.pos.x + x;
			loc.pos.y = backBuffer.pos.y;
			auto ptr = bufptr + x * bytesPerPixel;
			if(options.fore.isSolid()) {
				auto cl = options.fore.getPackedColor();
				if(cl.alpha < 255) {
					alpha = alpha * cl.alpha / 255;
				}
			}
			if(alpha == 255) {
				options.fore.writePixels(loc, ptr, len);
			} else {
				uint8_t tmp[len * bytesPerPixel];
				options.fore.writePixels(loc, tmp, len);
				BlendAlpha::blend(pixelFormat, tmp, ptr, len * bytesPerPixel, alpha);
			}
		}
		bufptr += backBuffer.r.w * bytesPerPixel;
		++backBuffer.pos.y;
	}
	auto len = bufptr - backBuffer.data.get();
	if(!surface.writeDataBuffer(backBuffer.data, 0, len)) {
		debug_w("[[EEK]] WRITE");
	}

	return true;
}

} // namespace Graphics
