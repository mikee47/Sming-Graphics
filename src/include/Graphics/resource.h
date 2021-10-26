/****
 * resource.h
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

#include <FlashString/String.hpp>
#include "Types.h"

namespace Graphics
{
namespace Resource
{
/**
 * @brief Describes glyph bitmap and position
 */
struct GlyphResource {
	enum class Flag {
		alpha,
	};
	using Flags = BitSet<uint8_t, Flag, 1>;

	uint16_t bmOffset; ///< Offset relative to TypefaceResource::bmpOffset
	uint8_t width;	 ///< Bitmap dimensions in pixels
	uint8_t height;	///< Bitmap dimensions in pixels
	int8_t xOffset;	///< X dist from cursor pos to UL corner
	int8_t yOffset;	///< Y dist from cursor pos to UL corner
	uint8_t xAdvance;  ///< Distance to advance cursor (x axis)
	Flags flags;

	GlyphMetrics getMetrics() const
	{
		return GlyphMetrics{
			.width = width,
			.height = height,
			.xOffset = xOffset,
			.yOffset = yOffset,
			.advance = xAdvance,
		};
	}
};

/**
 * @brief Identifies a run of unicode characters
 */
struct GlyphBlock {
	uint16_t codePoint; ///< First character code
	uint16_t length;	///< Number of consecutive characters

	uint16_t first() const
	{
		return codePoint;
	}

	uint16_t last() const
	{
		return codePoint + length - 1;
	}

	bool contains(uint16_t cp) const
	{
		return cp >= first() && cp <= last();
	}
};

struct TypefaceResource {
	uint32_t bmOffset; ///< Start of bitmap data in resource stream
	uint8_t style;
	uint8_t yAdvance;
	uint8_t descent;
	uint8_t numBlocks;
	const GlyphResource* glyphs;
	const GlyphBlock* blocks;
};

struct FontResource {
	const FSTR::String* name;
	uint8_t yAdvance;
	uint8_t descent;
	uint8_t padding[2];
	const TypefaceResource* faces[4]; // normal, italic, bold, boldItalic

	static const FontResource& empty()
	{
		static FontResource fontEmpty{};
		return fontEmpty;
	}

	explicit operator bool() const
	{
		return name != nullptr;
	}
};

struct ImageResource {
	const FSTR::String* name;
	uint32_t bmOffset;
	uint32_t bmSize;
	uint16_t width;
	uint16_t height;
	PixelFormat format;

	Size getSize() const
	{
		return Size{FSTR::readValue(&width), FSTR::readValue(&height)};
	}

	PixelFormat getFormat() const
	{
		return FSTR::readValue(&format);
	}
};

} // namespace Resource
} // namespace Graphics
