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
	XX(Mask, "dst = dst AND src")                                                                                      \
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

template <class Class, BlendMode blendMode> class BlendTemplate : public Blend
{
public:
	Mode mode() const override
	{
		return blendMode;
	}

	void transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const override
	{
		auto self = static_cast<const Class*>(this);
		return self->blend(format, src, dstptr, length);
	}

	void transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const override
	{
		auto self = static_cast<const Class*>(this);
		return self->blend(format, srcptr, dstptr, length);
	}
};

class BlendWrite : public BlendTemplate<BlendWrite, BlendMode::Write>
{
public:
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length)
	{
		writeColor(dstptr, src, format, length / getBytesPerPixel(format));
	}

	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length)
	{
		(void)format;
		memcpy(dstptr, srcptr, length);
	}
};

class BlendXor : public BlendTemplate<BlendXor, BlendMode::Xor>
{
public:
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length);
	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length);
};

class BlendXNor : public BlendTemplate<BlendXNor, BlendMode::XNor>
{
public:
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length);
	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length);
};

class BlendMask : public BlendTemplate<BlendMask, BlendMode::Mask>
{
public:
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length);
	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length);
};

class BlendTransparent : public BlendTemplate<BlendTransparent, BlendMode::Transparent>
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

	static void blend(PixelFormat, PackedColor, uint8_t*, size_t)
	{
		// Makes no sense for this blender
	}

	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length, Color key);

	void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const
	{
		blend(format, srcptr, dstptr, length, key);
	}

private:
	Color key;
};

class BlendAlpha : public BlendTemplate<BlendAlpha, BlendMode::Alpha>
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

	static PackedColor blend(PixelFormat format, PackedColor src, PackedColor dst);
	static void blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length);
	static void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha);
	static uint16_t IRAM_ATTR blendRGB565(uint16_t src, uint16_t dst, uint8_t alpha);
	static void IRAM_ATTR blendRGB565(uint16_t src, uint8_t* dstptr, size_t length, uint8_t alpha);
	static void IRAM_ATTR blendRGB565(const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha);
	static uint8_t blendChannel(uint8_t fg, uint8_t bg, uint8_t alpha);
	static void blendRGB24(PackedColor src, uint8_t* dstptr, size_t length);
	static PixelBuffer blendColor(PixelBuffer fg, PixelBuffer bg, uint8_t alpha);

	void blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const
	{
		blend(format, srcptr, dstptr, length, alpha);
	}

	Color blendColor(Color fg, Color bg, uint8_t alpha) const
	{
		return blendColor(PixelBuffer{fg}, PixelBuffer{bg}, alpha).color;
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
