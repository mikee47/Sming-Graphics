/**
 * TextBuilder.h
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

#include "Scene.h"
#include "LcdFont.h"

namespace Graphics
{
/**
 * @brief Simplifies construction of TextObject instances.
 */
class TextParser
{
public:
	TextParser(const Rect& bounds) : bounds(bounds)
	{
		resetClip();
	}

	const Rect& getBounds() const
	{
		return bounds;
	}

	TextObject* release()
	{
		endRun();
		curAsset = nullptr;
		curFont = nullptr;
		curColor = nullptr;
		return object.release();
	}

	TextObject* commit(SceneObject& scene)
	{
		return scene.addObject(release());
	}

	static void setDefaultFont(Font* font)
	{
		defaultFont = font;
	}

	void setFont(const Font* font)
	{
		if(font == nullptr) {
			font = defaultFont;
		}
		if(this->font != font) {
			curSeg = nullptr;
			curFont = nullptr;
			typeface = nullptr;
			this->font = font;
		}
	}

	void setFont(const Font& font)
	{
		setFont(&font);
	}

	const Font& getFont() const;

	const TypeFace& getTypeFace() const;

	void setScale(Scale scale)
	{
		if(scale != options.scale) {
			curSeg = nullptr;
			curFont = nullptr;
			options.scale = scale;
		}
	}

	void setScale(uint8_t sx, uint8_t sy)
	{
		setScale(Scale(sx, sy));
	}

	void setScale(uint8_t size)
	{
		setScale(size, size);
	}

	uint16_t getTextHeight() const
	{
		return options.scale.scaleY(getTypeFace().height());
	}

	const TextOptions& getOptions() const
	{
		return options;
	}

	void setStyle(FontStyles style)
	{
		if(options.style != style) {
			curSeg = nullptr;
			curFont = nullptr;
			typeface = nullptr;
			options.style = style;
		}
	}

	void addStyle(FontStyles style)
	{
		setStyle(options.style + style);
	}

	void removeStyle(FontStyles style)
	{
		setStyle(options.style - style);
	}

	void setTextAlign(Align align)
	{
		if(align != textAlign) {
			endRun();
			textAlign = align;
		}
	}

	Align getTextAlign() const
	{
		return textAlign;
	}

	void setLineAlign(Align align)
	{
		if(align != lineAlign) {
			endRun();
			lineAlign = align;
		}
	}

	Align getLineAlign() const
	{
		return lineAlign;
	}

	Point getCursor() const
	{
		return cursor;
	}

	/**
	 * @brief Set location to start new text segment
	 */
	void setCursor(Point pt)
	{
		if(pt != cursor) {
			endRun();
			cursor = pt;
		}
	}

	void setCursor(int16_t x, int16_t y)
	{
		setCursor({x, y});
	}

	void moveCursor(Point offset)
	{
		setCursor(cursor + offset);
	}

	void moveCursor(int16_t x, int16_t y)
	{
		moveCursor({x, y});
	}

	void setColor(const Brush& fore, const Brush& back = {})
	{
		if(options.fore == fore && options.back == back) {
			return;
		}
		curSeg = nullptr;
		curColor = nullptr;
		options.fore = fore;
		options.back = back;
	}

	void setForeColor(const Brush& color)
	{
		setColor(color, options.back);
	}

	void setBackColor(const Brush& color)
	{
		setColor(options.fore, color);
	}

	void setClip(const Rect& r)
	{
		endRun();
		clip = intersect(r, bounds.size());
		cursor = Point{};
	}

	const Rect& getClip() const
	{
		return clip;
	}

	void resetClip()
	{
		endRun();
		clip = bounds.size();
		cursor = Point{};
	}

	void setWrap(bool wrap)
	{
		this->wrap = wrap;
	}

	void parse(const TextAsset& asset, uint32_t start, size_t size);

private:
	void newLine()
	{
		curSeg = breakSeg = lineSeg = nullptr;
		textHeight += lineHeight;
		cursor.y += lineHeight;
		breakIndex = lineHeight = 0;
	}

	void endRun()
	{
		curSeg = breakSeg = lineSeg = startSeg = nullptr;
		breakIndex = lineHeight = textHeight = ystart = 0;
		overflow = false;
	}

	void addTextSegment(Point textpos, uint16_t endx, const TextAsset& asset, uint16_t start, uint8_t length);

	Rect bounds;
	Rect clip;
	uint32_t breakIndex{0};
	const TextAsset* curAsset{nullptr};
	TextObject::FontElement* curFont{nullptr};
	TextObject::ColorElement* curColor{nullptr};
	TextObject::RunElement* breakSeg{nullptr};
	uint16_t breakx{0};
	uint8_t breakw{0};
	char breakChar{'\0'};
	std::unique_ptr<TextObject> object;
	bool wrap{true};
	bool overflow{false};
	mutable const Font* font{nullptr};
	mutable const TypeFace* typeface{nullptr};
	TextOptions options{};
	uint16_t lineHeight{0};
	uint16_t textHeight{0};
	uint16_t ystart{0};						   ///< Account for empty lines at start of block
	TextObject::RunElement* startSeg{nullptr}; // First segment in block
	TextObject::RunElement* lineSeg{nullptr};  // First segment on this line
	TextObject::RunElement* curSeg{nullptr};   // Current segment on this line
	Align textAlign{};
	Align lineAlign{};
	Point cursor{}; ///<Current position relative to clipping origin, ignoring alignment
	static Font* defaultFont;
};

/**
 * @brief Simplifies construction of TextObject instances.
 */
class TextBuilder : public TextParser, public Print
{
public:
	TextBuilder(AssetList& assets, const Rect& bounds)
		: TextParser(bounds), stream(new MemoryDataStream), text(*new TextAsset(stream))
	{
		assets.add(&text);
		resetClip();
	}

	TextBuilder(SceneObject& scene) : TextBuilder(scene.assets, scene.getSize())
	{
	}

	using Print::write;

	size_t write(uint8_t c) override
	{
		return write(&c, 1);
	}

	size_t write(const uint8_t* buffer, size_t size) override
	{
		auto pos = text.getLength();
		stream->write(buffer, size);
		parse(text, pos, size);
		return size;
	}

private:
	MemoryDataStream* stream;
	TextAsset& text;
};

} // namespace Graphics
