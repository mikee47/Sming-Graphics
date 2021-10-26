/****
 * Blend.h
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

#include "Asset.h"

namespace Graphics
{
#define GRAPHICS_BLENDMODE_MAP(XX)                                                                                     \
	XX(Write, "Write normally")                                                                                        \
	XX(Xor, "dst = dst XOR src0")                                                                                      \
	XX(XNor, "dst = dst XOR (NOT src)")                                                                                \
	XX(Transparent, "Make nominated colour transparent")                                                               \
	XX(Alpha, "Blend using alpha value")

/**
 * @brief Blend operations
 * 
 * See MemoryImageSurface::write
 */
class Blend : public AssetTemplate<AssetType::Blend>
{
public:
	enum class Mode {
#define XX(name, desc) name,
		GRAPHICS_BLENDMODE_MAP(XX)
#undef XX
	};

	virtual Mode mode() const = 0;
	virtual void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const = 0;
	virtual void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const = 0;

	/* Meta */

	void write(MetaWriter& meta) const override;
};

using BlendMode = Blend::Mode;

template <BlendMode blendMode> class BlendTemplate : public Blend
{
public:
	Mode mode() const override
	{
		return blendMode;
	}
};

class BlendWrite : public BlendTemplate<BlendMode::Write>
{
public:
	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override
	{
		writeColor(dstptr, src, format, length / getBytesPerPixel(format));
	}

	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override
	{
		(void)format;
		memcpy(dstptr, srcptr, length);
	}
};

class BlendXor : public BlendTemplate<BlendMode::Xor>
{
public:
	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override;
	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override;
};

class BlendXNor : public BlendTemplate<BlendMode::XNor>
{
public:
	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override;
	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override;
};

class BlendTransparent : public BlendTemplate<BlendMode::Transparent>
{
public:
	BlendTransparent(Color key) : key(key)
	{
	}

	void write(MetaWriter& meta) const override
	{
		Blend::write(meta);
		meta.write("key", key);
	}

	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override
	{
		// Makes no sense for this blender
	}

	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override;

private:
	Color key;
};

class BlendAlpha : public BlendTemplate<BlendMode::Alpha>
{
public:
	BlendAlpha(uint8_t alpha) : alpha(alpha)
	{
	}

	BlendAlpha(Color color) : alpha(getAlpha(color))
	{
	}

	BlendAlpha(PackedColor color) : alpha(color.alpha)
	{
	}

	void write(MetaWriter& meta) const override
	{
		Blend::write(meta);
		meta.write("alpha", alpha);
	}

	static PackedColor transform(PixelFormat format, PackedColor src, PackedColor dst);
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length);
	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha);
	static uint16_t IRAM_ATTR blendRGB565(uint16_t src, uint16_t dst, uint8_t alpha);
	static void IRAM_ATTR blendRGB565(uint16_t src, uint8_t* dstptr, size_t length, uint8_t alpha);
	static void IRAM_ATTR blendRGB565(const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha);
	static uint8_t blendChannel(uint8_t fg, uint8_t bg, uint8_t alpha);
	static void blendRGB24(PackedColor src, uint8_t* dstptr, size_t length);
	static PixelBuffer blendColor(PixelBuffer fg, PixelBuffer bg, uint8_t alpha);

	Color blendColor(Color fg, Color bg, uint8_t alpha) const
	{
		return blendColor(PixelBuffer{fg}, PixelBuffer{bg}, alpha).color;
	}

	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override
	{
		return blend(format, src, dstptr, length);
	}

	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override
	{
		return blend(format, srcptr, dstptr, length, alpha);
	}

private:
	uint16_t blendRGB565(uint16_t src, uint16_t dst) const
	{
		return blendRGB565(src, dst, alpha);
	}

	uint8_t blendChannel(uint8_t fg, uint8_t bg) const
	{
		return blendChannel(fg, bg, alpha);
	}

	PixelBuffer blendColor(PixelBuffer fg, PixelBuffer bg) const
	{
		return blendColor(fg, bg, alpha);
	}

	uint8_t alpha; ///< 255 = source opaque, 0 = source invisible
};

} // namespace Graphics

String toString(Graphics::BlendMode mode);
