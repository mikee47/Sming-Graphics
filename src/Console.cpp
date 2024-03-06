/****
 * Console.cpp
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
 * @author: October 2021 - mikee47 <mike@sillyhouse.net>
 *
 ****/

#include "include/Graphics/Console.h"
#include "include/Graphics/LcdFont.h"

namespace Graphics
{
size_t Console::write(const uint8_t* data, size_t size)
{
	auto str = reinterpret_cast<const char*>(data);
	if(paused) {
		pauseBuffer.concat(str, size);
	} else {
		buffer.concat(str, size);
	}
	update();
	return size;
}

void Console::pause(bool state)
{
	paused = state;
	if(paused) {
		return;
	}
	if(pauseBuffer) {
		if(buffer.length() == 0) {
			buffer = std::move(pauseBuffer);
		} else {
			buffer.concat(pauseBuffer);
		}
		pauseBuffer = nullptr;
	}
	update();
}

void Console::update()
{
	if(scene) {
		// Busy: Will be called again when current scene has completed
		return;
	}

	if(buffer.length() == 0) {
		return;
	}

	scene = std::make_unique<SceneObject>(display);
	Scale scale{1, 2};
	constexpr FontStyles style{};
	auto face = lcdFont.getFace(style);
	auto lineHeight = scale.scaleY(face->height());
	auto text = new TextObject(display.getSize());
	constexpr Color fore(Color::White);
	constexpr Color back(Color::Black);

	Point pt = cursor;
	auto len = buffer.length();
	auto buf = buffer.begin();
	unsigned start = 0;
	unsigned offset = start;

	// Clear unused space at end of line
	auto clreol = [&](Point pos) {
		Rect r(pos, text->bounds.w - pos.x, lineHeight);
		if(r.w != 0) {
			scene->fillRect(back, r);
		}
	};

	// Add a run of text
	auto addLine = [&]() {
		if(offset > start) {
			text->addRun(cursor, pt.x - cursor.x, start, offset - start);
			start = offset;
		}
		cursor = pt;
	};

	// Parse text buffer and emit RUN elements
	for(unsigned i = 0; i < len; ++i) {
		auto c = buf[i];
		if(c == '\n') {
			addLine();
			cursor.y += lineHeight;
			pt = cursor;
			continue;
		}
		if(c == '\r') {
			addLine();
			cursor.x = pt.x = 0;
			continue;
		}
		if(c == '\t') {
			c = ' ';
		} else if(c < ' ') {
			continue;
		}
		buf[offset++] = c;
		auto metrics = face->getMetrics(c);
		auto adv = scale.scaleX(metrics.advance);
		if(pt.x + adv > text->bounds.w) {
			addLine();
			cursor.x = 0;
			cursor.y += lineHeight;
			pt = cursor;
		}
		pt.x += adv;
	}
	if(pt.x > cursor.x) {
		++offset;
		addLine();
	}

	// If cursor has extended below bottom of screen then scroll display
	int overflow = cursor.y + lineHeight - text->bounds.h;
	if(overflow > 0) {
		cursor.y -= overflow;
		// scroll display up
		display.scroll(overflow);

		// Adjust element positions
		for(auto& el : text->elements) {
			auto seg = reinterpret_cast<TextObject::RunElement*>(&el);
			seg->pos.y -= overflow;
		}

		// Remove any elements which have scrolled off top of screen
		TextObject::RunElement* seg;
		while((seg = reinterpret_cast<TextObject::RunElement*>(text->elements.head()))) {
			if(seg->pos.y >= 0) {
				break;
			}
			text->elements.remove(seg);
		}

		// Ensure final line is properly erased
		clreol(cursor);
	}

	// Move buffer into an asset and add to scene
	auto textAsset = new TextAsset(std::move(buffer));
	scene->addAsset(textAsset);
	// Insert config. elements ahead of runs
	text->elements.insert(new TextObject::FontElement(*face, scale, style));
	text->elements.insert(new TextObject::ColorElement(fore, back));
	text->elements.insert(new TextObject::TextElement(*textAsset));

	scene->addObject(text);

	renderQueue.render(scene.get(), [this](SceneObject*) {
		scene.reset();
		update();
	});
}

void Console::systemDebugOutput(bool enable)
{
	if(enable) {
		m_setPuts([this](const char* str, size_t len) -> size_t {
			return write(reinterpret_cast<const uint8_t*>(str), len);
		});
	} else {
		m_setPuts(nullptr);
	}
}

} // namespace Graphics
