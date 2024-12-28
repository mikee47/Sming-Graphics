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
		.yOffset = 1 - rawSize.h,
		.advance = rawSize.w + 1,
		.alpha = 0,
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

	size_t readRaw(void* buffer, size_t bufSize) const override;

private:
	std::bitset<rawSize.w> rowBits[rawSize.h];
};

class LcdTypeFace : public TypeFace
{
public:
	GlyphBlock getBlock(unsigned index) const override
	{
		if(index == 0) {
			return GlyphBlock{0, 255};
		}
		return GlyphBlock{};
	}

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

	GlyphObject::Metrics getMetrics(uint16_t) const override
	{
		return LcdGlyph::metrics;
	}

	std::unique_ptr<GlyphObject> getGlyph(uint16_t ch, const GlyphObject::Options& options) const override;
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

	const TypeFace* getFace(FontStyles style) const override
	{
		(void)style;
		return &typeface;
	}

private:
	LcdTypeFace typeface;
};

extern LcdFont lcdFont;

} // namespace Graphics
