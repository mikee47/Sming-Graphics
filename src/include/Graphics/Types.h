/****
 * Types.h
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
#include <Data/CStringArray.h>
#include <Data/BitSet.h>
#include <Data/Range.h>
#include <memory>
#include <cassert>
#include "Colors.h"

namespace Graphics
{
/**
 * @brief Undefined I/O pin value
 */
static constexpr uint8_t PIN_NONE{255};

/**
 * @brief Numeric identifiers for re-useable objects
 */
using AssetID = uint16_t;

/**
 * @brief Defines orientation of display
 */
enum class Orientation {
	normal,
	deg0 = normal,
	deg90,
	deg180,
	deg270,
};

enum class Align {
	Near,			 // 0
	Centre,			 // 1
	Far,			 // 2
	Left = Near,	 // 0
	Top = Near,		 // 0
	Center = Centre, // 1
	Right = Far,	 // 2
	Bottom = Far,	// 2
};

/**
 * @brief Points on a compass
 * 
 * These are ordered by angles in increments of 45 degrees.
 */
enum class Origin {
	E,  // 0 degrees
	NE, // 45
	N,  // 90
	NW, // 135
	W,  // 180
	SW, // 225
	S,  // 270
	SE, // 315
	Centre,
	TopLeft = NW,
	Top = N,
	TopRight = NE,
	Left = W,
	Center = Centre,
	Right = E,
	BottomLeft = SW,
	Bottom = S,
	BottomRight = SE,
};

/**
 * @brief Get the origin for the opposite side of the rectangle
 * 
 * e.g. E -> W, NE -> SW
 */
inline Origin opposite(Origin o)
{
	return (o == Origin::Centre) ? o : Origin((unsigned(o) + 4) % 8);
}

/**
 * @brief Size of rectangular area (width x height)
 */
struct Size {
	uint16_t w{0};
	uint16_t h{0};

	constexpr Size()
	{
	}

	constexpr Size(uint16_t w, uint16_t h) : w(w), h(h)
	{
	}

	String toString() const;
};

static_assert(sizeof(Size) == 4, "Size too big");

constexpr inline Size rotate(Size size, Orientation orientation)
{
	if(orientation == Orientation::deg90 || orientation == Orientation::deg270) {
		std::swap(size.w, size.h);
	}
	return size;
}

/**
 * @brief An (x, y) display co-ordinate
 */
template <typename T> struct TPoint {
	T x{0};
	T y{0};

	constexpr TPoint()
	{
	}

	constexpr TPoint(T x, T y) : x(x), y(y)
	{
	}

	/**
	 * @brief Conversion constructor
	 */
	template <typename Q> explicit constexpr TPoint(TPoint<Q> pt) : x(pt.x), y(pt.y)
	{
	}

	explicit constexpr TPoint(Size sz) : x(sz.w), y(sz.h)
	{
	}

	explicit operator bool() const
	{
		return x != 0 || y != 0;
	}

	template <typename Q> bool operator==(TPoint<Q> other) const
	{
		return x == other.x && y == other.y;
	}

	template <typename Q> bool operator!=(TPoint<Q> other) const
	{
		return !operator==(other);
	}

	template <typename Q> constexpr TPoint& operator+=(TPoint<Q> other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator-=(TPoint<Q> other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator*=(TPoint<Q> other)
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator*=(Q scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator/=(TPoint<Q> other)
	{
		x /= other.x;
		y /= other.y;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator/=(Q scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator%=(TPoint<Q> other)
	{
		x %= other.x;
		y %= other.y;
		return *this;
	}

	template <typename Q> constexpr TPoint& operator%=(Q scalar)
	{
		x %= scalar;
		y %= scalar;
		return *this;
	}

	explicit operator uint32_t() const
	{
		return (y << 16) | x;
	}

	String toString() const
	{
		char buf[16];
		m_snprintf(buf, sizeof(buf), "%d, %d", x, y);
		return buf;
	}
};

template <typename T, typename Q> constexpr TPoint<T> operator+(TPoint<T> pt, const Q& other)
{
	pt += other;
	return pt;
}

template <typename T, typename Q> constexpr TPoint<T> operator-(TPoint<T> pt, const Q& other)
{
	pt -= other;
	return pt;
}

template <typename T, typename Q> constexpr TPoint<T> operator*(TPoint<T> pt, const Q& other)
{
	pt *= other;
	return pt;
}

template <typename T> constexpr TPoint<T> operator*(TPoint<T> pt, const Size& other)
{
	return pt * TPoint<T>(other);
}

template <typename T, typename Q> constexpr TPoint<T> operator/(TPoint<T> pt, const Q& other)
{
	pt /= other;
	return pt;
}

template <typename T> constexpr TPoint<T> operator/(TPoint<T> pt, const Size& other)
{
	return pt / TPoint<T>(other);
}

template <typename T, typename Q> constexpr TPoint<T> operator%(TPoint<T> pt, const Q& other)
{
	pt %= other;
	return pt;
}

using Point = TPoint<int16_t>;
using IntPoint = TPoint<int>;
using PointF = TPoint<float>;

/**
 * @brief Location and size of rectangular area (x, y, w, h)
 */
struct Rect {
	int16_t x{0};
	int16_t y{0};
	uint16_t w{0};
	uint16_t h{0};

	constexpr Rect()
	{
	}

	constexpr Rect(int16_t x, int16_t y, uint16_t w, uint16_t h) : x(x), y(y), w(w), h(h)
	{
	}

	constexpr Rect(int16_t x, int16_t y, Size size) : x(x), y(y), w(size.w), h(size.h)
	{
	}

	constexpr Rect(Point pt, Size size) : x(pt.x), y(pt.y), w(size.w), h(size.h)
	{
	}

	constexpr Rect(Point pt, uint16_t w, uint16_t h) : x(pt.x), y(pt.y), w(w), h(h)
	{
	}

	constexpr Rect(Size size) : Rect({}, size)
	{
	}

	constexpr Rect(Point pt1, Point pt2)
	{
		if(pt1.x > pt2.x) {
			std::swap(pt1.x, pt2.x);
		}
		if(pt1.y > pt2.y) {
			std::swap(pt1.y, pt2.y);
		}
		x = pt1.x;
		y = pt1.y;
		w = 1 + pt2.x - pt1.x;
		h = 1 + pt2.y - pt1.y;
	}

	constexpr Rect(Point pt, Size size, Origin origin)
	{
		switch(origin) {
		case Origin::TopLeft:
			*this = Rect(pt, size);
			break;
		case Origin::Top:
			*this = Rect(pt.x - size.w / 2, pt.y, size);
			break;
		case Origin::TopRight:
			*this = Rect(pt.x - size.w, pt.y, size);
			break;
		case Origin::Left:
			*this = Rect(pt.x, pt.y - size.h / 2, size);
			break;
		case Origin::Centre:
			*this = Rect(pt.x - size.w / 2, pt.y - size.h / 2, size);
			break;
		case Origin::Right:
			*this = Rect(pt.x - size.w, pt.y - size.h / 2, size);
			break;
		case Origin::BottomLeft:
			*this = Rect(pt.x, pt.y - size.h, size);
			break;
		case Origin::Bottom:
			*this = Rect(pt.x - size.w / 2, pt.y - size.h, size);
			break;
		case Origin::BottomRight:
			*this = Rect(pt.x - size.w, pt.y - size.h, size);
			break;
		default:
			*this = Rect(pt, size);
		}
	}

	Point getPoint(Origin origin) const
	{
		switch(origin) {
		case Origin::E:
			return Point(right(), centre().y);
		case Origin::NE:
			return topRight();
		case Origin::N:
			return Point(centre().x, top());
		case Origin::NW:
			return topLeft();
		case Origin::W:
			return Point(left(), centre().y);
		case Origin::SW:
			return bottomLeft();
		case Origin::S:
			return Point(centre().x, bottom());
		case Origin::SE:
			return bottomRight();
		case Origin::Centre:
			return centre();
		default:
			return topLeft();
		}
	}

	Point operator[](Origin origin) const
	{
		return getPoint(origin);
	}

	int16_t left() const
	{
		return x;
	}

	int16_t right() const
	{
		return x + w - 1;
	}

	int16_t top() const
	{
		return y;
	}

	int16_t bottom() const
	{
		return y + h - 1;
	}

	Point topLeft() const
	{
		return Point{left(), top()};
	}

	Point topRight() const
	{
		return Point{right(), top()};
	}

	Point bottomLeft() const
	{
		return Point{left(), bottom()};
	}

	Point bottomRight() const
	{
		return Point{right(), bottom()};
	}

	Point centre() const
	{
		return Point(x + w / 2, y + h / 2);
	}

	Point center() const
	{
		return centre();
	}

	Size size() const
	{
		return Size{w, h};
	}

	explicit operator bool() const
	{
		return w != 0 && h != 0;
	}

	bool operator==(const Rect& other) const
	{
		return x == other.x && y == other.y && w == other.w && h == other.h;
	}

	bool operator!=(const Rect& other) const
	{
		return !operator==(other);
	}

	Rect& operator+=(const Point& off)
	{
		x += off.x;
		y += off.y;
		return *this;
	}

	Rect& operator-=(const Point& off)
	{
		x -= off.x;
		y -= off.y;
		return *this;
	}

	bool contains(Point pt) const
	{
		return pt.x >= left() && pt.x <= right() && pt.y >= top() && pt.y <= bottom();
	}

	int16_t clipX(int16_t x) const
	{
		if(x < left()) {
			return left();
		}
		if(x > right()) {
			return right();
		}
		return x;
	}

	int16_t clipY(int16_t y) const
	{
		if(y < top()) {
			return top();
		}
		if(y > bottom()) {
			return bottom();
		}
		return y;
	}

	Point clip(Point pt) const
	{
		return Point{clipX(pt.x), clipY(pt.y)};
	}

	bool intersects(const Rect& r) const
	{
		return right() >= r.left() && left() <= r.right() && bottom() >= r.top() && top() <= r.bottom();
	}

	/**
	 * @brief Obtain intersection with another rectangle
	 */
	Rect& clip(const Rect& r)
	{
		if(intersects(r)) {
			Point pt1{std::max(x, r.x), std::max(y, r.y)};
			Point pt2{std::min(right(), r.right()), std::min(bottom(), r.bottom())};
			if(pt1.x <= pt2.x && pt1.y <= pt2.y) {
				*this = Rect{pt1, pt2};
				return *this;
			}
		}
		*this = Rect{};
		return *this;
	}

	/**
	 * @brief Obtain smallest rectangle enclosing this rectangle and another
	 */
	Rect& operator+=(const Rect& r)
	{
		if(!*this) {
			*this = r;
		} else if(r) {
			Point pt1{std::min(left(), r.left()), std::min(top(), r.top())};
			Point pt2{std::max(right(), r.right()), std::max(bottom(), r.bottom())};
			*this = Rect{pt1, pt2};
		}
		return *this;
	}

	void inflate(int16_t cw, int16_t ch)
	{
		x -= cw;
		w += cw + cw;
		y -= ch;
		h += ch + ch;
	}

	void inflate(int16_t cwh)
	{
		inflate(cwh, cwh);
	}

	String toString() const;
};

static_assert(sizeof(Rect) == 8, "Rect too big");

template <typename T> Rect operator+(const Rect& rect, const T& other)
{
	Rect r(rect);
	r += other;
	return r;
}

inline Rect operator-(const Rect& rect, const Point& offset)
{
	Rect r(rect);
	r -= offset;
	return r;
}

inline Rect intersect(Rect r1, const Rect& r2)
{
	return r1.clip(r2);
}

/**
 * @brief Represents the intersection of two rectangles.
 *
 * This produces up to 4 separate, non-overlapping rectangles.
 */
class Region
{
public:
	constexpr Region() = default;
	constexpr Region(const Region& other) = default;

	constexpr Region(const Rect& r) : rects{r}
	{
	}

	/**
	 * @brief Add rectangle to this region
	 *
	 * Produces a single enclosing rectangle.
	 */
	Region& operator+=(const Rect& r)
	{
		rects[0] += rects[1] + rects[2] + rects[3];
		rects[1] = rects[2] = rects[3] = Rect{};
		return *this;
	}

	/**
	 * @brief Remove rectangle from this region
	 *
	 * Operation is currently performed on bounding box ONLY.
	 * TODO: Implement region updates using existing information.
	 */
	Region& operator-=(const Rect& r)
	{
		Rect u = bounds();
		auto i = intersect(u, r);
		if(!i) {
			return *this;
		}

		clear();

		auto u2 = u.bottomRight();
		auto i2 = i.bottomRight();

		// #1
		if(u.y < i.y) {
			rects[0] = Rect(u.x, u.y, u.w, i.y - u.y);
		}
		// #2
		if(u2.y > i2.y) {
			rects[1] = Rect(u.x, 1 + i2.y, u.w, u2.y - i2.y);
		}
		// #3
		if(u.x < i.x) {
			rects[2] = Rect(u.x, i.y, i.x - u.x, i.h);
		}
		// #4
		if(u2.x > i2.x) {
			rects[3] = Rect(1 + i2.x, i.y, u2.x - i2.x, i.h);
		}

		return *this;
	}

	Rect bounds() const
	{
		return rects[0] + rects[1] + rects[2] + rects[3];
	}

	void clear()
	{
		memset(&rects, 0, sizeof(rects));
	}

	explicit operator bool() const
	{
		return bool(rects[0]) || bool(rects[1]) || bool(rects[2]) || bool(rects[3]);
	}

	String toString() const;

	Rect rects[4]{};
};

inline Region operator-(const Region& rgn, const Rect& r)
{
	Region rgn2{rgn};
	rgn2 -= r;
	return rgn2;
}

/**
 * @brief Identifies position within bounding rectangle
 */
struct Location {
	/**
	 * @brief Where to write pixels on surface
	 */
	Rect dest;

	/**
	 * @brief Reference source area
	 * 
	 * This is generally used by brushes to locate colour information.
	 * For example, with an ImageBrush objects can be filled to either re-use
	 * the same portion of the reference image, or to reveal a particular part of
	 * it.
	 */
	Rect source;

	/**
	 * @brief Position relative to dest/source top-left corner
	 */
	Point pos;

	Point destPos() const
	{
		return dest.topLeft() + pos;
	}

	Point sourcePos() const
	{
		return source.topLeft() + pos;
	}

	String toString() const;
};

using Range = TRange<uint16_t>;

class ColorRange
{
public:
	ColorRange()
	{
	}

	static Color random(uint8_t alpha = 0xff)
	{
		return makeColor(TRange<uint32_t>(0, 0xffffff).random(), alpha);
	}
};

class Scale
{
public:
	constexpr Scale() : xscale(0), yscale(0)
	{
	}

	constexpr Scale(uint8_t sxy) : Scale(sxy, sxy)
	{
	}

	constexpr Scale(uint8_t sx, uint8_t sy) : xscale(sx > 0 ? sx - 1 : 0), yscale(sy > 0 ? sy - 1 : 0)
	{
	}

	constexpr bool operator==(const Scale& other)
	{
		return xscale == other.xscale && yscale == other.yscale;
	}

	constexpr bool operator!=(const Scale& other)
	{
		return !operator==(other);
	}

	constexpr uint8_t scaleX() const
	{
		return 1 + xscale;
	}

	constexpr uint16_t scaleX(uint16_t x) const
	{
		return x * scaleX();
	}

	constexpr uint16_t unscaleX(uint16_t x) const
	{
		return x / scaleX();
	}

	constexpr uint8_t scaleY() const
	{
		return 1 + yscale;
	}

	constexpr uint16_t scaleY(uint16_t y) const
	{
		return y * scaleY();
	}

	constexpr uint16_t unscaleY(uint16_t y) const
	{
		return y / scaleY();
	}

	constexpr Size scale() const
	{
		return {scaleX(), scaleY()};
	}

	constexpr Size scale(Size size) const
	{
		return {scaleX(size.w), scaleY(size.h)};
	}

	constexpr Size unscale(Size size) const
	{
		return {unscaleX(size.w), unscaleY(size.h)};
	}

	explicit operator bool() const
	{
		return xscale != 0 || yscale != 0;
	}

	String toString() const;

private:
	uint8_t xscale : 4;
	uint8_t yscale : 4;
};

#define GRAPHICS_FONT_STYLE(XX)                                                                                        \
	XX(Bold, "")                                                                                                       \
	XX(Italic, "")                                                                                                     \
	XX(Underscore, "")                                                                                                 \
	XX(Overscore, "")                                                                                                  \
	XX(Strikeout, "")                                                                                                  \
	XX(DoubleUnderscore, "")                                                                                           \
	XX(DoubleOverscore, "")                                                                                            \
	XX(DoubleStrikeout, "")                                                                                            \
	XX(DotMatrix, "Draw only top-left dot in scaled glyphs")                                                           \
	XX(HLine, "Draw only top line in scaled glyphs")                                                                   \
	XX(VLine, "Draw only left line in scaled glyphs")

enum class FontStyle {
#define XX(name, desc) name,
	GRAPHICS_FONT_STYLE(XX)
#undef XX
};

using FontStyles = BitSet<uint16_t, FontStyle, 10>;

/**
 * @brief Glyph metrics
 */
struct GlyphMetrics {
	uint8_t width;   ///< Width of glyph
	uint8_t height;  ///< Height of glyph
	int8_t xOffset;  ///< Glyph position relative to cursor
	int8_t yOffset;  ///< Distance from upper-left corner to baseline
	uint8_t advance; ///< Distance to next character

	Size size() const
	{
		return Size{width, height};
	}
};

/**
 * @brief Get corresponding angle for given origin
 */
inline uint16_t originToDegrees(Origin origin)
{
	return 45 * (unsigned(origin) % 8);
}

/**
 * @brief Get origin closest to given angle (expressed in degrees)
 */
Origin degreesToOrigin(uint16_t angle);

/**
 * @brief Make 0 <= angle < 360
 */
uint16_t normaliseAngle(int angle);

} // namespace Graphics

String toString(Graphics::Orientation orientation);
String toString(Graphics::Align align);
String toString(Graphics::Origin origin);
String toString(Graphics::FontStyle style);

template <typename T> inline String toString(Graphics::TPoint<T> pt)
{
	return pt.toString();
}

inline String toString(const Graphics::Rect& r)
{
	return r.toString();
}

inline String toString(const Graphics::Location& loc)
{
	return loc.toString();
}

inline String toString(Graphics::Size sz)
{
	return Graphics::Point(sz.w, sz.h).toString();
}

inline String toString(Graphics::Scale scale)
{
	return toString(Graphics::Size(scale.scaleX(), scale.scaleY()));
}
