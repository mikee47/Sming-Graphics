/****
 * glcdfont.cpp
 *
 * This is the 'classic' fixed-space bitmap font for Adafruit_GFX since 1.0.
 *
 * See https://github.com/adafruit/Adafruit-GFX-Library
 *
 ****/

#pragma once

#include "Object.h"

namespace Graphics
{
class LcdGlyph : public GlyphObject
{
public:
	static constexpr Size rawSize{5, 8};
	static constexpr Metrics metrics{
		.width = rawSize.w + 1,
		.height = rawSize.h,
		.xOffset = 0,
		.yOffset = rawSize.h,
		.advance = rawSize.w + 1,
	};

	LcdGlyph(size_t bmOffset, const Options& options);

	bool init() override
	{
		return true;
	}

	Bits getBits(uint16_t row) const override
	{
		return rowBits[row].to_ulong();
	}

	void readAlpha(void* buffer, Point origin, size_t stride) const override;

private:
	PackedColor clFore;
	PackedColor clBack;
	uint8_t scale;
	std::bitset<rawSize.w> rowBits[rawSize.h];
};

class LcdTypeFace : public TypeFace
{
public:
	FontStyles getStyle() const override
	{
		return 0;
	}

	uint8_t height() const override
	{
		return LcdGlyph::rawSize.h;
	}

	uint8_t descent() const override
	{
		return 1;
	}

	GlyphObject::Metrics getMetrics(char ch) const override
	{
		(void)ch;
		return LcdGlyph::metrics;
	}

	GlyphObject* getGlyph(char ch, const GlyphObject::Options& options) const override;
};

class LcdFont : public Font
{
public:
	String name() const override
	{
		return F("glcdfont");
	}

	uint16_t height() const override
	{
		return LcdGlyph::rawSize.h;
	}

	const TypeFace* getFace(FontStyles style) const
	{
		(void)style;
		return &typeface;
	}

private:
	LcdTypeFace typeface;
};

extern LcdFont lcdFont;

} // namespace Graphics
