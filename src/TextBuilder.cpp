/**
 * TextBuilder.cpp
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

#include "include/Graphics/TextBuilder.h"

namespace Graphics
{
Font* TextParser::defaultFont;

void TextParser::parse(const TextAsset& asset, uint32_t start, size_t size)
{
	if(!object) {
		object = std::make_unique<TextObject>(getBounds());
	}

	getTypeFace();

	uint32_t index = start;

	auto textpos = cursor;

	auto addSeg = [&]() {
		if(start == index) {
			return;
		}
		addTextSegment(textpos, cursor.x, asset, start, index - start);
		start = index;
		textpos = cursor;
		breakSeg = curSeg;
	};

	for(unsigned i = 0; i < size; ++i, ++index) {
		char ch = asset.read(index);
		if(ch == '\n') {
			addSeg();
			newLine();
			textpos = cursor;
			++start;
			// Account for \n at start of text block
			if(startSeg == nullptr) {
				ystart = cursor.y;
			}
			continue;
		}
		if(ch == '\r') {
			addSeg();
			curSeg = lineSeg = nullptr;
			cursor.x = textpos.x = 0;
			overflow = false;
			++start;
			continue;
		}

		auto metrics = typeface->getMetrics(ch);
		auto adv = options.scale.scaleX(metrics.advance);
		int xw = metrics.xOffset + metrics.width;
		auto w = (xw > 0) ? std::max(options.scale.scaleX(xw), adv) : adv;

		if(overflow) {
			++start;
		} else if(wrap) {
			if(cursor.x + w > clip.w) {
				if(breakIndex == 0) {
					// Line does not contain break char, break here
					addTextSegment(textpos, cursor.x, asset, start, index - start);
					start = index;
					cursor.x = 0;
					newLine();
					textpos = Point{0, cursor.y};
				} else if(breakSeg == nullptr) {
					// Break character located in current run
					auto len = breakIndex - start;
					if(len != 0) {
						if(breakChar != ' ') {
							++len;
							breakx += breakw;
							breakw = 0;
						}
						addTextSegment(textpos, breakx, asset, start, len);
						start = breakIndex + 1;
					} else if(breakChar == ' ') {
						textpos.x += breakx;
						++start;
					}
					cursor.x -= breakx + breakw;
					newLine();
					textpos = Point{0, cursor.y};
				} else {
					// Break character in previous run on this line
					auto bs = breakSeg;
					auto brkidx = breakIndex;
					cursor.x -= breakx + breakw;
					newLine();
					textpos = Point{0, cursor.y};
					if(brkidx != bs->offset) {
						++brkidx;
						auto splitSeg = new TextObject::RunElement(textpos, bs->pos.x + bs->width - breakx - breakw,
																   brkidx, bs->offset + bs->length - brkidx);
						bs->width -= splitSeg->width;
						bs->length -= splitSeg->length;
						splitSeg->insertAfter(bs);
						bs = splitSeg;
					}
					for(TextObject::Element* elem = bs; elem != nullptr; elem = elem->getNext()) {
						if(elem->kind != TextObject::Element::Kind::Run) {
							continue;
						}
						auto seg = static_cast<TextObject::RunElement*>(elem);
						seg->pos = textpos;
						textpos.x += seg->width;
					}
				}
			}
			if(strchr(" -/,.:;", ch) != nullptr) {
				breakChar = ch;
				breakx = cursor.x;
				breakw = w;
				breakIndex = index;
				breakSeg = curSeg;
			}
		} else if(cursor.x > clip.right()) {
			addSeg();
			++start;
			overflow = true;
		}

		cursor.x += adv;
	}

	addSeg();
}

void TextParser::addTextSegment(Point textpos, uint16_t endx, const TextAsset& asset, uint16_t start, uint8_t length)
{
	lineHeight = std::max(lineHeight, options.scale.scaleY(typeface->height()));
	if(&asset != curAsset) {
		object->addText(asset);
		curAsset = &asset;
		curSeg = nullptr;
	}
	if(curFont == nullptr) {
		curFont = object->addFont(*typeface, options.scale, options.style);
	}
	if(curColor == nullptr) {
		curColor = object->addColor(options.fore, options.back);
	}

	Point offset;
	if(textAlign != Align::Near) {
		offset.x = clip.w - endx;
		if(textAlign == Align::Centre) {
			offset.x /= 2;
		}
	}
	if(lineAlign != Align::Near) {
		offset.y = clip.h - (textHeight + lineHeight);
		if(startSeg == nullptr) {
			offset.y -= textpos.y;
		}
		if(lineAlign == Align::Centre) {
			offset.y /= 2;
		}
	}

	size_t segWidth = endx - textpos.x;
	if(curSeg == nullptr || curSeg->offset + curSeg->length != start) {
		curSeg = object->addRun(clip.topLeft() + textpos, segWidth, start, length);
		if(lineSeg == nullptr) {
			lineSeg = curSeg;
			curSeg->pos.x += offset.x;
		} else if(textAlign != Align::Near) {
			curSeg->pos.x += lineSeg->pos.x;
		}
		if(startSeg == nullptr) {
			startSeg = curSeg;
			curSeg->pos.y += offset.y;
		} else if(lineAlign != Align::Near) {
			curSeg->pos.y += startSeg->pos.y - ystart - clip.y;
		}
	} else {
		curSeg->width += segWidth;
		curSeg->length += length;
	}

	if(textAlign != Align::Near && curSeg != lineSeg) {
		offset.x -= lineSeg->pos.x;
		for(TextObject::Element* elem = lineSeg; elem != nullptr; elem = elem->getNext()) {
			if(elem->kind != TextObject::Element::Kind::Run) {
				continue;
			}
			auto seg = static_cast<TextObject::RunElement*>(elem);
			seg->pos.x += offset.x;
		}
	}

	if(lineAlign != Align::Near && curSeg != startSeg) {
		offset.y -= startSeg->pos.y - ystart - clip.y;
		for(TextObject::Element* elem = startSeg; elem != nullptr; elem = elem->getNext()) {
			if(elem->kind != TextObject::Element::Kind::Run) {
				continue;
			}
			auto seg = static_cast<TextObject::RunElement*>(elem);
			seg->pos.y += offset.y;
		}
	}
}

const Font& TextParser::getFont() const
{
	if(font == nullptr) {
		if(defaultFont == nullptr) {
			defaultFont = &lcdFont;
		}
		font = defaultFont;
	}
	return *font;
}

const TypeFace& TextParser::getTypeFace() const
{
	getFont();
	if(typeface == nullptr) {
		typeface = font->getFace(options.style);
	}
	return *typeface;
}

} // namespace Graphics
