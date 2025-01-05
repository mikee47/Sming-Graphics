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
/**
 * @brief Image container format
 */
enum class ImageFormat : uint8_t {
	RAW, ///< Contains raw image data described by PixelFormat
	BMP,
	JPEG,
	PNG,
};

namespace Resource
{
/**
 * @brief Describes glyph bitmap and position
 */
struct GlyphResource {
	// log2(bits per pixel)
	enum Alpha {
		L1 = 0,
		L2 = 1,
		L4 = 2,
		L8 = 3,
	};

	uint16_t bmOffset; ///< Offset relative to TypefaceResource::bmpOffset
	uint8_t width;	 ///< Bitmap dimensions in pixels
	uint8_t height;	///< Bitmap dimensions in pixels
	int8_t xOffset;	///< X dist from cursor pos to UL corner
	int8_t yOffset;	///< Y dist from cursor pos to UL corner
	uint8_t xAdvance;  ///< Distance to advance cursor (x axis)
	uint8_t alpha : 2;

	GlyphMetrics getMetrics() const
	{
		return GlyphMetrics{
			.width = width,
			.height = height,
			.xOffset = xOffset,
			.yOffset = yOffset,
			.advance = xAdvance,
			.alpha = alpha,
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
	union Format {
		uint8_t value;
		struct {
			uint8_t style : 4;
			uint8_t alpha : 2;
			uint8_t reserved : 2;
		};
	};

	uint32_t bmOffset; ///< Start of bitmap data in resource stream
	uint32_t bmSize;
	uint8_t format;
	uint8_t yAdvance;
	uint8_t descent;
	uint8_t numBlocks;
	const GlyphResource* glyphs;
	const GlyphBlock* blocks;
};

struct __attribute__((packed)) FontResource {
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
	ImageFormat format;
	PixelFormat pixelFormat;

	Size getSize() const
	{
		return Size{FSTR::readValue(&width), FSTR::readValue(&height)};
	}

	ImageFormat getFormat() const
	{
		return FSTR::readValue(&format);
	}

	PixelFormat getPixelFormat() const
	{
		return FSTR::readValue(&pixelFormat);
	}
};

} // namespace Resource
} // namespace Graphics
