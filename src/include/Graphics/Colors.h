/**
 * Colors.h
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

#include <WString.h>

/*
 * Basic colour definitions (from Adafruit ILI9341)
 */
#define COLOR_NAME_MAP(XX)                                                                                             \
	XX(BLACK, 0, 0, 0)                                                                                                 \
	XX(NAVY, 0, 0, 128)                                                                                                \
	XX(DARKGREEN, 0, 128, 0)                                                                                           \
	XX(DARKCYAN, 0, 128, 128)                                                                                          \
	XX(MAROON, 128, 0, 0)                                                                                              \
	XX(PURPLE, 128, 0, 128)                                                                                            \
	XX(OLIVE, 128, 128, 0)                                                                                             \
	XX(LIGHTGREY, 192, 192, 192)                                                                                       \
	XX(DARKGREY, 128, 128, 128)                                                                                        \
	XX(BLUE, 0, 0, 255)                                                                                                \
	XX(GREEN, 0, 255, 0)                                                                                               \
	XX(CYAN, 0, 255, 255)                                                                                              \
	XX(RED, 255, 0, 0)                                                                                                 \
	XX(MAGENTA, 255, 0, 255)                                                                                           \
	XX(YELLOW, 255, 255, 0)                                                                                            \
	XX(WHITE, 255, 255, 255)                                                                                           \
	XX(ORANGE, 255, 165, 0)                                                                                            \
	XX(GREENYELLOW, 173, 255, 47)                                                                                      \
	XX(PINK, 255, 192, 203)

/*
 * gdipluscolor.h
 *
 * GDI+ color
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Markus Koenig <markus@stber-koenig.de>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#define GDIPLUS_COLOR_TABLE(XX)                                                                                        \
	XX(AliceBlue, 0xFFF0F8FFUL)                                                                                        \
	XX(AntiqueWhite, 0xFFFAEBD7UL)                                                                                     \
	XX(Aqua, 0xFF00FFFFUL)                                                                                             \
	XX(Aquamarine, 0xFF7FFFD4UL)                                                                                       \
	XX(Azure, 0xFFF0FFFFUL)                                                                                            \
	XX(Beige, 0xFFF5F5DCUL)                                                                                            \
	XX(Bisque, 0xFFFFE4C4UL)                                                                                           \
	XX(Black, 0xFF000000UL)                                                                                            \
	XX(BlanchedAlmond, 0xFFFFEBCDUL)                                                                                   \
	XX(Blue, 0xFF0000FFUL)                                                                                             \
	XX(BlueViolet, 0xFF8A2BE2UL)                                                                                       \
	XX(Brown, 0xFFA52A2AUL)                                                                                            \
	XX(BurlyWood, 0xFFDEB887UL)                                                                                        \
	XX(CadetBlue, 0xFF5F9EA0UL)                                                                                        \
	XX(Chartreuse, 0xFF7FFF00UL)                                                                                       \
	XX(Chocolate, 0xFFD2691EUL)                                                                                        \
	XX(Coral, 0xFFFF7F50UL)                                                                                            \
	XX(CornflowerBlue, 0xFF6495EDUL)                                                                                   \
	XX(Cornsilk, 0xFFFFF8DCUL)                                                                                         \
	XX(Crimson, 0xFFDC143CUL)                                                                                          \
	XX(Cyan, 0xFF00FFFFUL)                                                                                             \
	XX(DarkBlue, 0xFF00008BUL)                                                                                         \
	XX(DarkCyan, 0xFF008B8BUL)                                                                                         \
	XX(DarkGoldenrod, 0xFFB8860BUL)                                                                                    \
	XX(DarkGray, 0xFFA9A9A9UL)                                                                                         \
	XX(DarkGreen, 0xFF006400UL)                                                                                        \
	XX(DarkKhaki, 0xFFBDB76BUL)                                                                                        \
	XX(DarkMagenta, 0xFF8B008BUL)                                                                                      \
	XX(DarkOliveGreen, 0xFF556B2FUL)                                                                                   \
	XX(DarkOrange, 0xFFFF8C00UL)                                                                                       \
	XX(DarkOrchid, 0xFF9932CCUL)                                                                                       \
	XX(DarkRed, 0xFF8B0000UL)                                                                                          \
	XX(DarkSalmon, 0xFFE9967AUL)                                                                                       \
	XX(DarkSeaGreen, 0xFF8FBC8FUL)                                                                                     \
	XX(DarkSlateBlue, 0xFF483D8BUL)                                                                                    \
	XX(DarkSlateGray, 0xFF2F4F4FUL)                                                                                    \
	XX(DarkTurquoise, 0xFF00CED1UL)                                                                                    \
	XX(DarkViolet, 0xFF9400D3UL)                                                                                       \
	XX(DeepPink, 0xFFFF1493UL)                                                                                         \
	XX(DeepSkyBlue, 0xFF00BFFFUL)                                                                                      \
	XX(DimGray, 0xFF696969UL)                                                                                          \
	XX(DodgerBlue, 0xFF1E90FFUL)                                                                                       \
	XX(Firebrick, 0xFFB22222UL)                                                                                        \
	XX(FloralWhite, 0xFFFFFAF0UL)                                                                                      \
	XX(ForestGreen, 0xFF228B22UL)                                                                                      \
	XX(Fuchsia, 0xFFFF00FFUL)                                                                                          \
	XX(Gainsboro, 0xFFDCDCDCUL)                                                                                        \
	XX(GhostWhite, 0xFFF8F8FFUL)                                                                                       \
	XX(Gold, 0xFFFFD700UL)                                                                                             \
	XX(Goldenrod, 0xFFDAA520UL)                                                                                        \
	XX(Gray, 0xFF808080UL)                                                                                             \
	XX(Green, 0xFF008000UL)                                                                                            \
	XX(GreenYellow, 0xFFADFF2FUL)                                                                                      \
	XX(Honeydew, 0xFFF0FFF0UL)                                                                                         \
	XX(HotPink, 0xFFFF69B4UL)                                                                                          \
	XX(IndianRed, 0xFFCD5C5CUL)                                                                                        \
	XX(Indigo, 0xFF4B0082UL)                                                                                           \
	XX(Ivory, 0xFFFFFFF0UL)                                                                                            \
	XX(Khaki, 0xFFF0E68CUL)                                                                                            \
	XX(Lavender, 0xFFE6E6FAUL)                                                                                         \
	XX(LavenderBlush, 0xFFFFF0F5UL)                                                                                    \
	XX(LawnGreen, 0xFF7CFC00UL)                                                                                        \
	XX(LemonChiffon, 0xFFFFFACDUL)                                                                                     \
	XX(LightBlue, 0xFFADD8E6UL)                                                                                        \
	XX(LightCoral, 0xFFF08080UL)                                                                                       \
	XX(LightCyan, 0xFFE0FFFFUL)                                                                                        \
	XX(LightGoldenrodYellow, 0xFFFAFAD2UL)                                                                             \
	XX(LightGray, 0xFFD3D3D3UL)                                                                                        \
	XX(LightGreen, 0xFF90EE90UL)                                                                                       \
	XX(LightPink, 0xFFFFB6C1UL)                                                                                        \
	XX(LightSalmon, 0xFFFFA07AUL)                                                                                      \
	XX(LightSeaGreen, 0xFF20B2AAUL)                                                                                    \
	XX(LightSkyBlue, 0xFF87CEFAUL)                                                                                     \
	XX(LightSlateGray, 0xFF778899UL)                                                                                   \
	XX(LightSteelBlue, 0xFFB0C4DEUL)                                                                                   \
	XX(LightYellow, 0xFFFFFFE0UL)                                                                                      \
	XX(Lime, 0xFF00FF00UL)                                                                                             \
	XX(LimeGreen, 0xFF32CD32UL)                                                                                        \
	XX(Linen, 0xFFFAF0E6UL)                                                                                            \
	XX(Magenta, 0xFFFF00FFUL)                                                                                          \
	XX(Maroon, 0xFF800000UL)                                                                                           \
	XX(MediumAquamarine, 0xFF66CDAAUL)                                                                                 \
	XX(MediumBlue, 0xFF0000CDUL)                                                                                       \
	XX(MediumOrchid, 0xFFBA55D3UL)                                                                                     \
	XX(MediumPurple, 0xFF9370DBUL)                                                                                     \
	XX(MediumSeaGreen, 0xFF3CB371UL)                                                                                   \
	XX(MediumSlateBlue, 0xFF7B68EEUL)                                                                                  \
	XX(MediumSpringGreen, 0xFF00FA9AUL)                                                                                \
	XX(MediumTurquoise, 0xFF48D1CCUL)                                                                                  \
	XX(MediumVioletRed, 0xFFC71585UL)                                                                                  \
	XX(MidnightBlue, 0xFF191970UL)                                                                                     \
	XX(MintCream, 0xFFF5FFFAUL)                                                                                        \
	XX(MistyRose, 0xFFFFE4E1UL)                                                                                        \
	XX(Moccasin, 0xFFFFE4B5UL)                                                                                         \
	XX(NavajoWhite, 0xFFFFDEADUL)                                                                                      \
	XX(Navy, 0xFF000080UL)                                                                                             \
	XX(OldLace, 0xFFFDF5E6UL)                                                                                          \
	XX(Olive, 0xFF808000UL)                                                                                            \
	XX(OliveDrab, 0xFF6B8E23UL)                                                                                        \
	XX(Orange, 0xFFFFA500UL)                                                                                           \
	XX(OrangeRed, 0xFFFF4500UL)                                                                                        \
	XX(Orchid, 0xFFDA70D6UL)                                                                                           \
	XX(PaleGoldenrod, 0xFFEEE8AAUL)                                                                                    \
	XX(PaleGreen, 0xFF98FB98UL)                                                                                        \
	XX(PaleTurquoise, 0xFFAFEEEEUL)                                                                                    \
	XX(PaleVioletRed, 0xFFDB7093UL)                                                                                    \
	XX(PapayaWhip, 0xFFFFEFD5UL)                                                                                       \
	XX(PeachPuff, 0xFFFFDAB9UL)                                                                                        \
	XX(Peru, 0xFFCD853FUL)                                                                                             \
	XX(Pink, 0xFFFFC0CBUL)                                                                                             \
	XX(Plum, 0xFFDDA0DDUL)                                                                                             \
	XX(PowderBlue, 0xFFB0E0E6UL)                                                                                       \
	XX(Purple, 0xFF800080UL)                                                                                           \
	XX(Red, 0xFFFF0000UL)                                                                                              \
	XX(RosyBrown, 0xFFBC8F8FUL)                                                                                        \
	XX(RoyalBlue, 0xFF4169E1UL)                                                                                        \
	XX(SaddleBrown, 0xFF8B4513UL)                                                                                      \
	XX(Salmon, 0xFFFA8072UL)                                                                                           \
	XX(SandyBrown, 0xFFF4A460UL)                                                                                       \
	XX(SeaGreen, 0xFF2E8B57UL)                                                                                         \
	XX(SeaShell, 0xFFFFF5EEUL)                                                                                         \
	XX(Sienna, 0xFFA0522DUL)                                                                                           \
	XX(Silver, 0xFFC0C0C0UL)                                                                                           \
	XX(SkyBlue, 0xFF87CEEBUL)                                                                                          \
	XX(SlateBlue, 0xFF6A5ACDUL)                                                                                        \
	XX(SlateGray, 0xFF708090UL)                                                                                        \
	XX(Snow, 0xFFFFFAFAUL)                                                                                             \
	XX(SpringGreen, 0xFF00FF7FUL)                                                                                      \
	XX(SteelBlue, 0xFF4682B4UL)                                                                                        \
	XX(Tan, 0xFFD2B48CUL)                                                                                              \
	XX(Teal, 0xFF008080UL)                                                                                             \
	XX(Thistle, 0xFFD8BFD8UL)                                                                                          \
	XX(Tomato, 0xFFFF6347UL)                                                                                           \
	XX(Transparent, 0x00FFFFFFUL)                                                                                      \
	XX(Turquoise, 0xFF40E0D0UL)                                                                                        \
	XX(Violet, 0xFFEE82EEUL)                                                                                           \
	XX(Wheat, 0xFFF5DEB3UL)                                                                                            \
	XX(White, 0xFFFFFFFFUL)                                                                                            \
	XX(WhiteSmoke, 0xFFF5F5F5UL)                                                                                       \
	XX(Yellow, 0xFFFFFF00UL)                                                                                           \
	XX(YellowGreen, 0xFF9ACD32UL)

namespace Graphics
{
/**
 * @brief Obtain 24-bit colour value from red, green and blue components
 */
inline constexpr uint32_t getColorValue(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

/**
 * @brief Standard colour definitions
 */
enum class Color : uint32_t {
	None = 0,
#define XX(name, r, g, b) name = getColorValue(r, g, b),
	COLOR_NAME_MAP(XX)
#undef XX

#define XX(name, argb) name = argb,
		GDIPLUS_COLOR_TABLE(XX)
#undef XX
};

/**
 * @brief Function to create a custom colour
 */
inline constexpr Color makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
	return Color(getColorValue(r, g, b, a));
}

inline constexpr Color makeColor(uint32_t color, uint8_t alpha = 255)
{
	color = (color & 0xffffff) | (alpha << 24);
	return Color(color);
}

inline constexpr Color makeColor(Color color, uint8_t alpha)
{
	return makeColor(uint32_t(color), alpha);
}

inline constexpr uint8_t getAlpha(Color color)
{
	return uint32_t(color) >> 24;
}

inline constexpr uint8_t getRed(Color color)
{
	return uint32_t(color) >> 16;
}

inline constexpr uint8_t getGreen(Color color)
{
	return uint32_t(color) >> 8;
}

inline constexpr uint8_t getBlue(Color color)
{
	return uint32_t(color);
}

bool fromString(const char* s, Color& color);

inline bool fromString(const String& s, Color& color)
{
	return fromString(s.c_str(), color);
}

/**
 * @brief Order refers to colour order within bitstream
 */
enum ColorOrder { orderRGB, orderBGR };
// name, bytes per pixel, bits per pixel, color order, byte order
#define PIXEL_FORMAT_MAP(XX)                                                                                           \
	XX(RGB24, 3, 24, orderRGB, "24-bit RGB")                                                                           \
	XX(BGRA32, 4, 32, orderRGB, "32-bit ARGB")                                                                         \
	XX(BGR24, 3, 24, orderBGR, "24-bit BGR")                                                                           \
	XX(RGB565, 2, 16, orderRGB, "16-bit RGB 5/6/5")

enum class PixelFormat : uint8_t {
	None,
#define XX(name, bytes, bpp, colorOrder, desc) name = (bytes - 1) | ((bpp / 2) << 2) | (colorOrder << 7),
	PIXEL_FORMAT_MAP(XX)
#undef XX
};

union PixelFormatStruct {
	PixelFormat format;
	struct {
		uint8_t mByteCount : 2;
		uint8_t mBitsPerPixel : 5;
		uint8_t mColorOrder : 1;
	};

	uint8_t byteCount() const
	{
		return mByteCount + 1;
	}

	uint8_t bitsPerPixel() const
	{
		return mBitsPerPixel * 2;
	}

	ColorOrder colorOrder() const
	{
		return ColorOrder(mColorOrder);
	}
};

static_assert(sizeof(PixelFormatStruct) == 1);

/**
 * @brief Get number of bytes required to store a pixel in the given format
 */
inline uint8_t getBytesPerPixel(PixelFormat format)
{
	return PixelFormatStruct{format}.byteCount();
}

/**
 * @brief Colour in device pixel format
 */
struct PackedColor {
	uint32_t value : 24;
	uint32_t alpha : 8;
};

/**
 * @brief Structure used to perform pixel format conversions
 */
union __attribute__((packed)) PixelBuffer {
	Color color;
	PackedColor packed;
	uint8_t u8[4];

	// Layout as for Color
	struct BGRA32 {
		uint8_t b;
		uint8_t g;
		uint8_t r;
		uint8_t a;
	};
	BGRA32 bgra32;

	struct RGB24 {
		uint8_t r;
		uint8_t g;
		uint8_t b;
	};
	RGB24 rgb24;

	struct BGR24 {
		uint8_t b;
		uint8_t g;
		uint8_t r;
	};
	BGR24 bgr24;

	struct RGB565 {
		uint16_t b : 5;
		uint16_t g : 6;
		uint16_t r : 5;
	};
	RGB565 rgb565;
};

/**
 * @brief Convert RGB colour into packed format
 * @param color The RGB colour to convert
 * @param format The desired pixel format
 * @retval PackedColor
 */
PixelBuffer pack(PixelBuffer src, PixelFormat format);

inline PackedColor pack(Color color, PixelFormat format)
{
	PixelBuffer src{color};
	return pack(src, format).packed;
}

/**
 * @brief Convert packed colour into RGB
 * @param src PixelBuffer loaded with packed colour
 * @param format The exact format of the source colour
 * @retval Color The corresponding RGB colour
 */
PixelBuffer unpack(PixelBuffer src, PixelFormat format);

inline Color unpack(PackedColor packed, PixelFormat format)
{
	PixelBuffer src{.packed = packed};
	return unpack(src, format).color;
}

inline Color unpack(uint32_t packed, PixelFormat format)
{
	return unpack(PixelBuffer{.packed = PackedColor{packed}}, format).color;
}

/**
 * @brief Store a packed colour value into memory
 * @param buffer Where to write the colour value
 * @param color The value to write
 * @retval size_t The number of bytes written
 */
size_t writeColor(void* buffer, PackedColor color, PixelFormat format);

inline size_t writeColor(void* buffer, Color color, PixelFormat format)
{
	return writeColor(buffer, pack(color, format), format);
}

/**
 * @brief Store a block of packed colours into memory
 * @param buffer Where to write the colour value
 * @param color The value to write
 * @param count The number of times to repeat the colour
 * @retval size_t The number of bytes written
 */
size_t writeColor(void* buffer, PackedColor color, PixelFormat format, size_t count);

inline size_t writeColor(void* buffer, Color color, PixelFormat format, size_t count)
{
	return writeColor(buffer, pack(color, format), format, count);
}

/**
 * @brief Convert block of data from one pixel format to another
 */
size_t convert(const void* srcData, PixelFormat srcFormat, void* dstBuffer, PixelFormat dstFormat, size_t numPixels);

} // namespace Graphics

String toString(Graphics::Color color);
String toString(Graphics::PackedColor color);
String toString(Graphics::PixelFormat format);
