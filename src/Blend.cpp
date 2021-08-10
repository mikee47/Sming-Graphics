/**
 * Blend.cpp
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

#include "include/Graphics/Blend.h"

String toString(Graphics::BlendMode mode)
{
	switch(mode) {
#define XX(name, desc)                                                                                                 \
	case Graphics::BlendMode::name:                                                                                    \
		return F(#name);
		GRAPHICS_BLENDMODE_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

namespace Graphics
{
void Blend::write(MetaWriter& meta) const
{
	meta.write("mode", ::toString(mode()));
}

/* BlendXor */

void BlendXor::transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const
{
	auto srcptr = reinterpret_cast<uint8_t*>(&src);
	switch(getBytesPerPixel(format)) {
	case 1:
		for(; length != 0; length -= 1) {
			*dstptr++ ^= srcptr[0];
		}
		break;
	case 2:
		for(; length != 0; length -= 2) {
			*dstptr++ ^= srcptr[0];
			*dstptr++ ^= srcptr[1];
		}
		break;
	case 3:
		for(; length != 0; length -= 3) {
			*dstptr++ ^= srcptr[0];
			*dstptr++ ^= srcptr[1];
			*dstptr++ ^= srcptr[2];
		}
		break;
	}
}

void BlendXor::transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const
{
	(void)format;
	while(length-- != 0) {
		*dstptr++ ^= *srcptr++;
	}
}

/* BlendXNor */

void BlendXNor::transform(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length) const
{
	src.value = ~src.value;
	BlendXor().transform(format, src, dstptr, length);
}

void BlendXNor::transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const
{
	(void)format;
	while(length-- != 0) {
		*dstptr++ ^= ~*srcptr++;
	}
}

/* BlendTransparent */

void BlendTransparent::transform(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length) const
{
	PixelBuffer ref{key};
	ref = pack(ref, format);
	switch(format) {
	case PixelFormat::RGB565:
		for(; length != 0; length -= 2) {
			PixelBuffer src{.u8{srcptr[0], srcptr[1]}};
			src = unpack(src, format);
			auto lum = src.rgb24.r + src.rgb24.g + src.rgb24.b;
			auto lumRef = ref.rgb24.r + ref.rgb24.g + ref.rgb24.b;
			if(lum <= lumRef) {
				memcpy(dstptr, srcptr, 2);
			}
			dstptr += 2;
			srcptr += 2;
		}
		break;
	case PixelFormat::RGB24:
		for(; length != 0; length -= 3) {
			PixelBuffer src{.rgb24{srcptr[0], srcptr[1], srcptr[2]}};
			if(src.rgb24.r <= ref.rgb24.r && src.rgb24.g <= ref.rgb24.g && src.rgb24.b <= ref.rgb24.b) {
				// auto lum = buf.rgb24.r + buf.rgb24.g + buf.rgb24.b;
				// auto lumRef = ref.rgb24.r + ref.rgb24.g + ref.rgb24.b;
				// if(lum <= lumRef) {
				memcpy(dstptr, srcptr, 3);
			}
			dstptr += 3;
			srcptr += 3;
		}
		break;
	case PixelFormat::BGR24:
	case PixelFormat::BGRA32:
		for(; length != 0; length -= 3) {
			PixelBuffer src{.bgr24{srcptr[0], srcptr[1], srcptr[2]}};
			if(src.rgb24.r <= ref.rgb24.r && src.rgb24.g <= ref.rgb24.g && src.rgb24.b <= ref.rgb24.b) {
				// auto lum = buf.rgb24.r + buf.rgb24.g + buf.rgb24.b;
				// auto lumRef = ref.rgb24.r + ref.rgb24.g + ref.rgb24.b;
				// if(lum <= lumRef) {
				memcpy(dstptr, srcptr, 3);
			}
			dstptr += 3;
			srcptr += 3;
		}
		break;
	case PixelFormat::None:
		break;
	}
}

/* BlendAlpha */

// Fast RGB565 pixel blending
// Found in a pull request for the Adafruit framebuffer library. Clever!
// https://github.com/tricorderproject/arducordermini/pull/1/files#diff-d22a481ade4dbb4e41acc4d7c77f683d
uint16_t BlendAlpha::blendRGB565(uint16_t src, uint16_t dst, uint8_t alpha)
{
	// Alpha converted from [0..255] to [0..31]
	alpha = (alpha + 4) >> 3;

	constexpr uint32_t mask{0b00000111111000001111100000011111};

	// Converts  0000000000000000rrrrrggggggbbbbb
	//     into  00000gggggg00000rrrrr000000bbbbb
	// with mask 00000111111000001111100000011111
	// This is useful because it makes space for a parallel fixed-point multiply
	uint32_t bg = (dst | (dst << 16)) & mask;
	uint32_t fg = (src | (src << 16)) & mask;

	// This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
	// This can be factorized into: result = bg + (fg - bg) * alpha
	// alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
	uint32_t result = (fg - bg) * alpha; // parallel fixed-point multiply of all components
	result >>= 5;
	result += bg;
	result &= mask;					// mask out fractional parts
	return (result >> 16) | result; // contract result
}

void BlendAlpha::blendRGB565(uint16_t src, uint8_t* dstptr, size_t length, uint8_t alpha)
{
	for(; length != 0; length -= 2) {
		uint16_t dst = (dstptr[0] << 8) | dstptr[1];
		dst = blendRGB565(src, dst, alpha);
		*dstptr++ = dst >> 8;
		*dstptr++ = dst;
	}
}

void BlendAlpha::blendRGB565(const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha)
{
	for(; length != 0; length -= 2) {
		uint16_t src = (srcptr[0] << 8) | srcptr[1];
		srcptr += 2;
		uint16_t dst = (dstptr[0] << 8) | dstptr[1];
		dst = blendRGB565(src, dst, alpha);
		*dstptr++ = dst >> 8;
		*dstptr++ = dst;
	}
}

uint8_t BlendAlpha::blendChannel(uint8_t fg, uint8_t bg, uint8_t alpha)
{
	uint32_t adst = (255 - alpha) * bg;
	uint32_t asrc = alpha * fg;
	uint32_t total = asrc + adst;
	return total / 255;
}

void BlendAlpha::blendRGB24(PackedColor src, uint8_t* dstptr, size_t length)
{
	auto srcptr = reinterpret_cast<const uint8_t*>(&src);
	for(unsigned i = 0; i < length; i += 3, dstptr += 3) {
		dstptr[0] = blendChannel(srcptr[0], dstptr[0], src.alpha);
		dstptr[1] = blendChannel(srcptr[1], dstptr[1], src.alpha);
		dstptr[2] = blendChannel(srcptr[2], dstptr[2], src.alpha);
	}
}

PixelBuffer BlendAlpha::blendColor(PixelBuffer fg, PixelBuffer bg, uint8_t alpha)
{
	PixelBuffer dst;
	dst.rgb24.r = blendChannel(fg.rgb24.r, bg.rgb24.r, alpha);
	dst.rgb24.g = blendChannel(fg.rgb24.g, bg.rgb24.g, alpha);
	dst.rgb24.b = blendChannel(fg.rgb24.b, bg.rgb24.b, alpha);
	return dst;
}

PackedColor BlendAlpha::transform(PixelFormat format, PackedColor src, PackedColor dst)
{
	if(src.alpha == 0) {
		// Transparent - do nothing
		return dst;
	}
	if(src.alpha == 255) {
		// Fully opaque
		return src;
	}

	switch(format) {
	case PixelFormat::RGB565:
		dst.value = blendRGB565(src.value, dst.value, src.alpha);
		return dst;

	case PixelFormat::RGB24:
		dst = blendColor(PixelBuffer{.packed = src}, PixelBuffer{.packed = dst}, src.alpha).packed;
		return dst;

	default:
		auto fg = unpack(PixelBuffer{.packed = src}, format);
		auto bg = unpack(PixelBuffer{.packed = dst}, format);
		auto res = blendColor(fg, bg, src.alpha);
		return pack(PixelBuffer{.packed = res}, format).packed;
	}
}

void BlendAlpha::blend(PixelFormat format, PackedColor src, uint8_t* dstptr, size_t length)
{
	if(src.alpha == 0) {
		// Transparent - do nothing
		return;
	}
	if(src.alpha == 255) {
		// Fully opaque
		writeColor(dstptr, src, format, length / getBytesPerPixel(format));
		return;
	}

	switch(format) {
	case PixelFormat::RGB565: {
		blendRGB565(__builtin_bswap16(src.value), dstptr, length, src.alpha);
		break;
	}

	case PixelFormat::RGB24:
		blendRGB24(src, dstptr, length);
		break;

	default:
		auto bytesPerPixel = getBytesPerPixel(format);
		auto fg = unpack(PixelBuffer{.packed{src}}, format);
		for(unsigned i = 0; i < length; i += bytesPerPixel) {
			PixelBuffer dst{};
			memcpy(&dst, dstptr, bytesPerPixel);
			auto bg = unpack(dst, format);
			auto cl = blendColor(fg, bg, src.alpha);
			dstptr += writeColor(dstptr, cl.color, format);
		}
	}
}

void BlendAlpha::blend(PixelFormat format, const uint8_t* srcptr, uint8_t* dstptr, size_t length, uint8_t alpha)
{
	if(alpha == 0) {
		// Transparent - do nothing
		return;
	}
	if(alpha == 255) {
		// Fully opaque
		memcpy(dstptr, srcptr, length);
		return;
	}

	switch(format) {
	case PixelFormat::RGB565:
		blendRGB565(srcptr, dstptr, length, alpha);
		break;

	case PixelFormat::RGB24:
		while(length-- != 0) {
			auto value = blendChannel(*srcptr++, *dstptr, alpha);
			*dstptr++ = value;
		}
		break;

	default:
		auto bytesPerPixel = getBytesPerPixel(format);
		for(unsigned i = 0; i < length; i += bytesPerPixel) {
			PixelBuffer src{};
			memcpy(&src, srcptr, bytesPerPixel);
			PixelBuffer dst{};
			memcpy(&dst, dstptr, bytesPerPixel);
			auto fg = unpack(src, format);
			auto bg = unpack(dst, format);
			auto cl = blendColor(fg, bg, alpha);
			dst = pack(PixelBuffer{cl}, format);
			memcpy(dstptr, &dst, bytesPerPixel);
			srcptr += bytesPerPixel;
			dstptr += bytesPerPixel;
		}
	}
}

} // namespace Graphics
