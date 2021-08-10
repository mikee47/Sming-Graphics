/**
 * Colors.cpp
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

#include "include/Graphics/Colors.h"
#include <FlashString/Map.hpp>
#include <stringconversion.h>
#include <cassert>

String toString(Graphics::Color color)
{
	char buf[16]{"#"};
	ultoa_wp(uint32_t(color), &buf[1], 16, 8, '0');
	return String(buf, 9);
}

String toString(Graphics::PackedColor color)
{
	char buf[16]{"#"};
	uint32_t value;
	memcpy(&value, &color, sizeof(value));
	ultoa_wp(value, &buf[1], 16, 8, '0');
	return String(buf, 9);
}

String toString(Graphics::PixelFormat format)
{
	switch(format) {
#define XX(name, bytes, bpp, colorOrder, desc)                                                                         \
	case Graphics::PixelFormat::name:                                                                                  \
		return F(#name);
		PIXEL_FORMAT_MAP(XX)
#undef XX
	default:
		return String(uint8_t(format), HEX);
	}
}

namespace Graphics
{
#define XX(name, argb) DEFINE_FSTR_LOCAL(colorString_##name, #name)
GDIPLUS_COLOR_TABLE(XX)
#undef XX

#define XX(name, argb) {Color::name, &colorString_##name},
DEFINE_FSTR_MAP(colorNames, Color, FSTR::String, GDIPLUS_COLOR_TABLE(XX))
#undef XX

bool fromString(const char* s, Color& color)
{
	if(s == nullptr || *s == '\0') {
		return false;
	}
	if(*s == '#') {
		++s;
		char* p;
		uint32_t value = strtoul(s, &p, 16);
		if(p - s > 6) {
			color = Color(value);
		} else {
			color = makeColor(value);
		}
		return true;
	}
	for(auto& e : colorNames) {
		if(e.content().equalsIgnoreCase(s)) {
			color = e.key();
			return true;
		}
	}
	return false;
}

PixelBuffer pack(PixelBuffer src, PixelFormat format)
{
	switch(format) {
	case PixelFormat::RGB565: {
		PixelBuffer dst;
		dst.rgb565.r = src.bgra32.r >> 3;
		dst.rgb565.g = src.bgra32.g >> 2;
		dst.rgb565.b = src.bgra32.b >> 3;
		dst.packed.alpha = src.bgra32.a;
		std::swap(dst.u8[0], dst.u8[1]);
		return dst;
	}
	case PixelFormat::RGB24:
		std::swap(src.bgra32.r, src.bgra32.b);
		return src;
	case PixelFormat::BGR24:
	case PixelFormat::BGRA32:
	default:
		return src;
	}
}

PixelBuffer unpack(PixelBuffer src, PixelFormat format)
{
	switch(format) {
	case PixelFormat::RGB565: {
		std::swap(src.u8[0], src.u8[1]);
		PixelBuffer dst;
		dst.bgra32.b = src.rgb565.b << 3;
		dst.bgra32.g = src.rgb565.g << 2;
		dst.bgra32.r = src.rgb565.r << 3;
		dst.bgra32.a = 255;
		return dst;
	}
	case PixelFormat::RGB24:
		std::swap(src.bgra32.r, src.bgra32.b);
	case PixelFormat::BGR24:
		src.bgra32.a = 255;
	default:
		return src;
	}
}

size_t writeColor(void* buffer, PackedColor color, PixelFormat format)
{
	auto ptr = static_cast<uint8_t*>(buffer);
	auto len = getBytesPerPixel(format);
	switch(len) {
	case 1:
		*ptr = color.value;
		break;
	case 2:
		*ptr++ = color.value;
		*ptr++ = color.value >> 8;
		break;
	case 3:
		*ptr++ = color.value;
		*ptr++ = color.value >> 8;
		*ptr++ = color.value >> 16;
		break;
	default:
		assert(false);
	}
	return len;
}

size_t writeColor(void* buffer, PackedColor color, PixelFormat format, size_t count)
{
	auto ptr = static_cast<uint8_t*>(buffer);
	switch(getBytesPerPixel(format)) {
	case 1:
		memset(buffer, color.value, count);
		ptr += count;
		break;
	case 2:
		while(count-- > 0) {
			*ptr++ = color.value;
			*ptr++ = color.value >> 8;
		}
		break;
	case 3:
		while(count-- > 0) {
			*ptr++ = color.value;
			*ptr++ = color.value >> 8;
			*ptr++ = color.value >> 16;
		}
		break;
	default:
		assert(false);
	}
	return ptr - static_cast<uint8_t*>(buffer);
}

size_t convert(const void* srcData, PixelFormat srcFormat, void* dstBuffer, PixelFormat dstFormat, size_t numPixels)
{
	auto srcptr = static_cast<const uint8_t*>(srcData);
	auto dstptr = static_cast<uint8_t*>(dstBuffer);
	switch(getBytesPerPixel(srcFormat)) {
	case 1:
		while(numPixels-- > 0) {
			PixelBuffer src{.u8{*srcptr++}};
			dstptr += writeColor(dstptr, unpack(src, srcFormat).color, dstFormat);
		}
		break;
	case 2:
		while(numPixels-- > 0) {
			PixelBuffer src{.u8{*srcptr++, *srcptr++}};
			dstptr += writeColor(dstptr, unpack(src, srcFormat).color, dstFormat);
		}
		break;
	case 3:
		while(numPixels-- > 0) {
			PixelBuffer src{.u8{*srcptr++, *srcptr++, *srcptr++}};
			dstptr += writeColor(dstptr, unpack(src, srcFormat).color, dstFormat);
		}
		break;
	default:
		assert(false);
	}
	return dstptr - static_cast<uint8_t*>(dstBuffer);
}

} // namespace Graphics
