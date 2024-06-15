/****
 * Object.h
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
#include "Blend.h"
#include <Data/Stream/LimitedMemoryStream.h>
#include <Data/Stream/MemoryDataStream.h>
#include <FlashString/Stream.hpp>
#include <Data/Stream/IFS/FileStream.h>
#include <bitset>

namespace Graphics
{
#define GRAPHICS_OBJECT_KIND_MAP(XX)                                                                                   \
	XX(Custom)                                                                                                         \
	XX(Point)                                                                                                          \
	XX(Rect)                                                                                                           \
	XX(FilledRect)                                                                                                     \
	XX(Line)                                                                                                           \
	XX(Polyline)                                                                                                       \
	XX(Circle)                                                                                                         \
	XX(FilledCircle)                                                                                                   \
	XX(Ellipse)                                                                                                        \
	XX(FilledEllipse)                                                                                                  \
	XX(Arc)                                                                                                            \
	XX(FilledArc)                                                                                                      \
	XX(Drawing)                                                                                                        \
	XX(Image)                                                                                                          \
	XX(Glyph)                                                                                                          \
	XX(Text)                                                                                                           \
	XX(Scene)                                                                                                          \
	XX(Reference)                                                                                                      \
	XX(Surface)                                                                                                        \
	XX(Copy)                                                                                                           \
	XX(Scroll)

class MetaWriter;
class Brush;
class Surface;
class FileImageSurface;

/**
 * @brief Virtual base class to manage rendering of various types of information to a surface
 */
class Renderer : public LinkedObjectTemplate<Renderer>
{
public:
	using List = LinkedObjectListTemplate<Renderer>;
	using OwnedList = OwnedLinkedObjectListTemplate<Renderer>;

	/**
	 * @brief Constructor
	 * @param location Location information
	 */
	Renderer(const Location& location) : location(location)
	{
	}

	virtual ~Renderer()
	{
	}

	/**
	 * @brief Called to do some writing to the surface
	 * @retval bool true when rendering is complete, false if more work to be done
	 */
	virtual bool execute(Surface& surface) = 0;

protected:
	Location location;
};

/**
 * @brief A drawable object inherits from this virtual base class
 */
class Object : public LinkedObjectTemplate<Object>, public Meta
{
public:
	using List = LinkedObjectListTemplate<Object>;
	using OwnedList = OwnedLinkedObjectListTemplate<Object>;

	enum class Kind {
#define XX(name) name,
		GRAPHICS_OBJECT_KIND_MAP(XX)
#undef XX
	};

	virtual Kind kind() const = 0;

	/**
	 * @brief Create a software renderer for this object
	 * @param location
	 * @retval renderer Returned renderer object
	 * 
	 * Return nullptr if object cannot/should not be rendered
	 */
	virtual Renderer* createRenderer(const Location& location) const = 0;

	bool operator==(const Object& other) const
	{
		return this == &other;
	}

	/* Meta */

	virtual String getTypeStr() const;
	virtual void write(MetaWriter& meta) const = 0;
};

template <Object::Kind object_kind> class ObjectTemplate : public Object
{
public:
	Kind kind() const override
	{
		return object_kind;
	}
};

/**
 * @brief Base class for a custom object
 */
using CustomObject = ObjectTemplate<Object::Kind::Custom>;

/**
 * @brief Reference to another object
 * 
 * Objects are owned by a Scene, so this allows objects to be re-used in multiple scenes,
 * or to other classes which expose an Object interface.
 */
class ReferenceObject : public ObjectTemplate<Object::Kind::Reference>
{
public:
	ReferenceObject(const Object& object, const Rect& pos, const Blend* blend = nullptr)
		: object(object), pos(pos), blend{blend}
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pos", pos);
		meta.write("object", object);
	}

	Renderer* createRenderer(const Location& location) const override;

	const Object& object;
	Rect pos;
	const Blend* blend;
};

/**
 * @brief A single pixel == 1x1 rectangle
 */
class PointObject : public ObjectTemplate<Object::Kind::Point>
{
public:
	PointObject(const Brush& brush, const Point& point) : brush(brush), point(point)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("brush", brush);
		meta.write("point", point);
	}

	Renderer* createRenderer(const Location& location) const override;

	Brush brush;
	Point point;
};

/**
 * @brief A rectangular outline
 */
class RectObject : public ObjectTemplate<Object::Kind::Rect>
{
public:
	RectObject(const Pen& pen, const Rect& rect, uint8_t radius = 0) : pen(pen), rect(rect), radius(radius)
	{
	}

	RectObject(int x0, int y0, int w, int h, Color color) : RectObject(color, Rect(x0, y0, w, h))
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.write("rect", rect);
		if(radius != 0) {
			meta.write("radius", radius);
		}
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen;
	Rect rect;
	uint8_t radius{0};
};

/**
 * @brief A filled rectangle
 */
class FilledRectObject : public ObjectTemplate<Object::Kind::FilledRect>
{
public:
	FilledRectObject(Brush brush, const Rect& rect, uint8_t radius = 0) : brush(brush), rect(rect), radius(radius)
	{
	}

	FilledRectObject(int x0, int y0, int w, int h, Color color) : FilledRectObject(color, Rect(x0, y0, w, h))
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("brush", brush);
		meta.write("rect", rect);
		if(radius != 0) {
			meta.write("radius", radius);
		}
	}

	Renderer* createRenderer(const Location& location) const override;

	const Blend* blender{nullptr};
	Brush brush;
	Rect rect;
	uint8_t radius{0};
};

/**
 * @brief A drawn line
 */
class LineObject : public ObjectTemplate<Object::Kind::Line>
{
public:
	LineObject()
	{
	}

	LineObject(Pen pen, Point pt1, Point pt2) : pen(pen), pt1(pt1), pt2(pt2)
	{
	}

	LineObject(Pen pen, int16_t x1, int16_t y1, int16_t x2, int16_t y2) : LineObject(pen, Point(x1, y1), Point(x2, y2))
	{
	}

	LineObject(int x0, int y0, int x1, int y1, Color color) : LineObject(color, Point(x0, y0), Point(x1, y1))
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.write("pt1", pt1);
		meta.write("pt2", pt2);
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen{};
	Point pt1{};
	Point pt2{};
};

/**
 * @brief A sequence of lines
 * 
 * By default, lines are connected. This is used to draw rectangles, triangles or any other type of polygon.
 * A line is drawn between points 0-1, 1-2, 2-3, etc.
 * 
 * Setting the `connected` property to false allows the lines to be discontinuous,
 * so a line is drawn between points 0-1, 2-3, 3-4, etc.
 */
class PolylineObject : public ObjectTemplate<Object::Kind::Polyline>
{
public:
	PolylineObject(Pen pen, size_t count) : pen(pen), points(std::make_unique<Point[]>(count)), numPoints(count)
	{
	}

	template <typename... ParamTypes>
	PolylineObject(Pen pen, ParamTypes... params) : pen(pen), numPoints(sizeof...(ParamTypes))
	{
		this->points.reset(new Point[sizeof...(ParamTypes)]{params...});
	}

	PolylineObject(const RectObject& object) : PolylineObject(object.pen, object.rect, object.radius)
	{
	}

	PolylineObject(Pen pen, const Rect& rect, uint8_t radius) : pen(pen)
	{
		auto pt1 = rect.topLeft();
		auto pt2 = rect.bottomRight();
		if(radius == 0) {
			points.reset(new Point[5]{
				pt1,
				{pt2.x, pt1.y},
				pt2,
				{pt1.x, pt2.y},
				pt1,
			});
			numPoints = 5;
		} else {
			auto t = pen.width - 1;
			points.reset(new Point[8]{
				Point(pt1.x + radius, pt1.y),
				Point(pt2.x - radius, pt1.y),
				Point(pt1.x + radius, pt2.y - t),
				Point(pt2.x - radius, pt2.y - t),
				Point(pt1.x, pt1.y + radius),
				Point(pt1.x, pt2.y - radius),
				Point(pt2.x - t, pt1.y + radius),
				Point(pt2.x - t, pt2.y - radius),
			});
			numPoints = 8;
			connected = false;
		}
	}

	Point operator[](unsigned index) const
	{
		assert(index < numPoints);
		return points[index];
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.writeArray("points", "Point", points.get(), numPoints);
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen;
	std::unique_ptr<Point[]> points;
	uint16_t numPoints;
	bool connected{true};
};

/**
 * @brief A circle outline
 */
class CircleObject : public ObjectTemplate<Object::Kind::Circle>
{
public:
	CircleObject(const Pen& pen, Point centre, uint16_t radius) : pen(pen), centre(centre), radius(radius)
	{
	}

	CircleObject(const Pen& pen, const Rect& rect) : CircleObject(pen, rect.centre(), std::min(rect.w, rect.h) / 2)
	{
	}

	CircleObject(int16_t x, int16_t y, uint16_t radius, Color color) : CircleObject(color, Point{x, y}, radius)
	{
	}

	/**
	 * @brief Get bounding retangle for this circle
	 */
	Rect getRect() const
	{
		uint16_t dia = radius * 2;
		return Rect(centre.x - radius, centre.y - radius, dia, dia);
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.write("centre", centre);
		meta.write("radius", radius);
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen;
	Point centre;
	uint16_t radius;
};

/**
 * @brief A filled circle
 */
class FilledCircleObject : public ObjectTemplate<Object::Kind::FilledCircle>
{
public:
	FilledCircleObject(Brush brush, Point centre, uint16_t radius) : brush(brush), centre(centre), radius(radius)
	{
	}

	FilledCircleObject(Brush brush, const Rect& rect)
		: FilledCircleObject(brush, rect.centre(), std::min(rect.w, rect.h) / 2)
	{
	}

	FilledCircleObject(int16_t x, int16_t y, uint16_t radius, Color color)
		: FilledCircleObject(color, Point{x, y}, radius)
	{
	}

	/**
	 * @brief Get bounding retangle for this circle
	 */
	Rect getRect() const
	{
		uint16_t dia = radius * 2;
		return Rect(centre.x - radius, centre.y - radius, dia, dia);
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("brush", brush);
		meta.write("centre", centre);
		meta.write("radius", radius);
	}

	Renderer* createRenderer(const Location& location) const override;

	Brush brush;
	Point centre;
	uint16_t radius;
};

/**
 * @brief An ellipse outline
 */
class EllipseObject : public ObjectTemplate<Object::Kind::Ellipse>
{
public:
	EllipseObject(Pen pen, const Rect& rect) : pen(pen), rect(rect)
	{
	}

	EllipseObject(Pen pen, Point centre, uint16_t a, uint16_t b)
		: pen(pen), rect(centre, Size(a * 2, b * 2), Origin::Centre)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.write("rect", rect);
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen;
	Rect rect;
};

/**
 * @brief A filled ellipse
 */
class FilledEllipseObject : public ObjectTemplate<Object::Kind::FilledEllipse>
{
public:
	FilledEllipseObject(const Brush& brush, const Rect& rect) : brush(brush), rect(rect)
	{
	}

	FilledEllipseObject(const Brush& brush, Point centre, uint16_t a, uint16_t b)
		: brush(brush), rect(centre, Size(a * 2, b * 2), Origin::Centre)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("brush", brush);
		meta.write("rect", rect);
	}

	Renderer* createRenderer(const Location& location) const override;

	Brush brush;
	Rect rect;
};

/**
 * @brief An arc outline
 */
class ArcObject : public ObjectTemplate<Object::Kind::Arc>
{
public:
	ArcObject(Pen pen, const Rect& rect, int16_t startAngle, int16_t endAngle)
		: pen(pen), rect(rect), startAngle(startAngle), endAngle(endAngle)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("pen", pen);
		meta.write("rect", rect);
		meta.write("startAngle", startAngle);
		meta.write("endAngle", endAngle);
	}

	Renderer* createRenderer(const Location& location) const override;

	Pen pen;
	Rect rect;
	int16_t startAngle;
	int16_t endAngle;
};

/**
 * @brief A filled arc
 */
class FilledArcObject : public ObjectTemplate<Object::Kind::FilledArc>
{
public:
	FilledArcObject(Brush brush, const Rect& rect, int16_t startAngle, int16_t endAngle)
		: brush(brush), rect(rect), startAngle(startAngle), endAngle(endAngle)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("brush", brush);
		meta.write("rect", rect);
		meta.write("startAngle", startAngle);
		meta.write("endAngle", endAngle);
	}

	Renderer* createRenderer(const Location& location) const override;

	Brush brush;
	Rect rect;
	int16_t startAngle;
	int16_t endAngle;
};

/**
 * @brief Virtual base class for an image
 */
class ImageObject : public ObjectTemplate<Object::Kind::Image>
{
public:
	ImageObject(Size size) : imageSize(size)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("size", imageSize);
	}

	Renderer* createRenderer(const Location& location) const override;

	Size getSize() const
	{
		return imageSize;
	}

	uint16_t width() const
	{
		return imageSize.w;
	}

	uint16_t height() const
	{
		return imageSize.h;
	}

	/**
	 * @brief Initialise the object, e.g. parse header content and obtain dimensions
	 */
	virtual bool init() = 0;

	/**
	 * @brief Get native pixel format
	 * @retval PixelFormat Return None if ambivalent about format (e.g. calculated pixel data)
	 */
	virtual PixelFormat getPixelFormat() const = 0;

	/**
	 * @brief Read pixels in requested format
	 * @param loc Start position
	 * @param format Required pixel format
	 * @param buffer Buffer for pixels
	 * @param width Number of pixels to read
	 * @retval size_t Number of bytes written
	 */
	virtual size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const = 0;

protected:
	Size imageSize{};
};

/**
 * @brief Image whose contents are stored in a stream, typically in a file or flash memory.
 */
class StreamImageObject : public ImageObject
{
public:
	StreamImageObject(IDataSourceStream* source, Size size) : ImageObject(size), stream(source)
	{
	}

	StreamImageObject(const FSTR::String& image) : StreamImageObject(new FSTR::Stream(image), {})
	{
	}

	void write(MetaWriter& meta) const override
	{
		ImageObject::write(meta);
		if(stream) {
			meta.write("stream", stream->getName());
		}
	}

protected:
	void seek(uint32_t offset) const
	{
		if(streamPos != offset) {
			streamPos = stream->seekFrom(offset, SeekOrigin::Start);
		}
	}

	void read(void* buffer, size_t length) const
	{
		streamPos += stream->readBytes(static_cast<uint8_t*>(buffer), length);
	}

	std::unique_ptr<IDataSourceStream> stream;
	mutable uint32_t streamPos{};
};

/**
 * @brief A BMP format image
 * 
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class BitmapObject : public StreamImageObject
{
public:
	using StreamImageObject::StreamImageObject;

	BitmapObject(const Resource::ImageResource& image)
		: StreamImageObject(Resource::createSubStream(image.bmOffset, image.bmSize), image.getSize())
	{
	}

	void write(MetaWriter& meta) const override
	{
		StreamImageObject::write(meta);
		meta.write("size", imageSize);
	}

	bool init() override;

	PixelFormat getPixelFormat() const override
	{
		return PixelFormat::RGB24;
	}

	size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const override;

private:
	uint32_t imageOffset;
	uint16_t stride;
	bool flip;
};

/**
 * @brief Image stored as raw pixels in a specific format
 * 
 * Use images stored in native display format for best performance as no conversion is required.
 */
class RawImageObject : public StreamImageObject
{
public:
	RawImageObject(IDataSourceStream* image, PixelFormat format, Size size)
		: StreamImageObject(image, size), pixelFormat(format)
	{
	}

	RawImageObject(const FSTR::String& image, PixelFormat format, Size size)
		: RawImageObject(new FSTR::Stream(image), format, size)
	{
	}

	RawImageObject(const Resource::ImageResource& image)
		: RawImageObject(Resource::createSubStream(image.bmOffset, image.bmSize), image.getFormat(), image.getSize())
	{
	}

	void write(MetaWriter& meta) const override
	{
		StreamImageObject::write(meta);
		meta.write("pixelFormat", pixelFormat);
	}

	bool init() override
	{
		return true;
	}

	PixelFormat getPixelFormat() const override
	{
		return pixelFormat;
	}

	size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const override;

protected:
	PixelFormat pixelFormat;
};

/**
 * @brief Interface for objects which support writing via surfaces
 */
class RenderTarget
{
public:
	virtual ~RenderTarget()
	{
	}

	/**
	 * @brief Get target dimensions
	 */
	virtual Size getSize() const = 0;

	/**
	 * @brief All surfaces support the same pixel format
	 */
	virtual PixelFormat getPixelFormat() const = 0;

	/**
	 * @brief Create a surface for use with this render target
	 * @param bufferSize Size of internal command/data buffer
	 * @retval Surface* The surface to use
	 * 
	 * Caller is responsible for destroying the surface when no longer required.
	 */
	virtual Surface* createSurface(size_t bufferSize = 0) = 0;

	PackedColor getColor(Color color) const
	{
		return pack(color, getPixelFormat());
	}
};

class MemoryImageObject : public RawImageObject, public RenderTarget
{
public:
	MemoryImageObject(PixelFormat format, Size size);

	~MemoryImageObject()
	{
		debug_i("[IMG] %p, destroyed", imageData);
	}

	Surface* createSurface(const Blend* blend, size_t bufferSize = 0);

	bool isValid() const
	{
		return imageData != nullptr;
	}

	/* RenderTarget */

	Size getSize() const override
	{
		return imageSize;
	}

	PixelFormat getPixelFormat() const override
	{
		return RawImageObject::getPixelFormat();
	}

	Surface* createSurface(size_t bufferSize = 0) override
	{
		return createSurface(nullptr, bufferSize);
	}

private:
	size_t imageBytes;
	uint8_t* imageData{nullptr};
};

class FileImageObject : public RawImageObject, public RenderTarget
{
public:
	FileImageObject(IFS::FileStream* file, PixelFormat format, Size size)
		: RawImageObject(file, format, size), imageBytes(size.w * size.h * getBytesPerPixel(format))
	{
	}

	/* RenderTarget */

	Size getSize() const override
	{
		return imageSize;
	}

	PixelFormat getPixelFormat() const override
	{
		return RawImageObject::getPixelFormat();
	}

	Surface* createSurface(size_t bufferSize = 0) override;

private:
	friend FileImageSurface;

	size_t imageBytes;
};

/**
 * @brief A character glyph image
 *
 * Characters are accessed like regular images but there are some specialisations which may
 * be necessary which this class exposes:
 *
 * - To support devices with custom renderers we require access to the raw monochrome bits
 * - Images currently don't support transparency, although this could be enabled using a flag
 * 	 bits in the PackedColor format. The display driver or renderer would then need to interpret
 *   this flag and render accordingly. An alternative method is to nominate a transparent colour.
 *
 */
class GlyphObject : public ImageObject
{
public:
	using Bits = std::bitset<64>;
	using Options = TextOptions;
	using Metrics = GlyphMetrics;

	GlyphObject(const Metrics& metrics, const Options& options)
		: ImageObject(options.scale.scale(metrics.size())), metrics(metrics), options(options)
	{
	}

	Kind kind() const override
	{
		return Kind::Glyph;
	}

	PixelFormat getPixelFormat() const override
	{
		return PixelFormat::None;
	}

	size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const override;

	virtual Bits getBits(uint16_t row) const = 0;

	/**
	 * @brief Obtain glyph information as block of 8-bit alpha values
	 * @param buffer
	 * @param origin Location of cursor within buffer
	 * @param stride Number of bytes per row in buffer
	 * 
	 * This method is called with a positive origin to accommodate negative x/y glyph offsets.
	 * Italic and script typefaces do this a lot!
	 */
	virtual void readAlpha(void* buffer, Point origin, size_t stride) const = 0;

	const Metrics& getMetrics() const
	{
		return metrics;
	}

protected:
	Metrics metrics;
	mutable Options options;
};

/**
 * @brief A block of text consisting of zero or more segments.
 *
 * A segment is a straight run of text using a specific typeface and style.
 */
class TextObject : public ObjectTemplate<Object::Kind::Text>
{
public:
	TextObject(const Rect& bounds) : bounds(bounds)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("bounds", bounds);
		meta.beginArray("elements", "Element");
		const TextElement* text{nullptr};
		for(auto& obj : elements) {
			meta.write(obj);
			if(obj.kind == Element::Kind::Text) {
				text = static_cast<const TextElement*>(&obj);
			} else if(obj.kind == Element::Kind::Run) {
				auto& run = static_cast<const RunElement&>(obj);
				String s;
				s.setLength(run.length);
				text->text.read(run.offset, s.begin(), s.length());
				meta.write("text", s);
			}
		}
		// meta.writeArray("elements", "Element", elements);
		meta.endArray();
	}

	Renderer* createRenderer(const Location& location) const override;

	class Element : public LinkedObjectTemplate<Element>, public Meta
	{
	public:
		using OwnedList = OwnedLinkedObjectListTemplate<Element>;

#define GRAPHICS_TEXT_ELEMENT_MAP(XX)                                                                                  \
	XX(Text)                                                                                                           \
	XX(Font)                                                                                                           \
	XX(Color)                                                                                                          \
	XX(Run)

		enum class Kind : uint8_t {
#define XX(tag) tag,
			GRAPHICS_TEXT_ELEMENT_MAP(XX)
#undef XX
		};

		Element(Kind kind) : kind(kind)
		{
		}

		/* Meta */

		String getTypeStr() const
		{
			switch(kind) {
#define XX(tag)                                                                                                        \
	case Kind::tag:                                                                                                    \
		return F(#tag);
				GRAPHICS_TEXT_ELEMENT_MAP(XX)
#undef XX
			default:
				return nullptr;
			}
		}

		virtual void write(MetaWriter& meta) const = 0;

		Kind kind;
	};
	GRAPHICS_VERIFY_SIZE(Element, 12);

	class TextElement : public Element
	{
	public:
		TextElement(const TextAsset& text) : Element(Kind::Text), text(text)
		{
		}

		void write(MetaWriter& meta) const override
		{
			meta.write("text", text);
		}

		const TextAsset& text;
	};

	class FontElement : public Element
	{
	public:
		FontElement(const TypeFace& typeface, Scale scale, FontStyles style)
			: Element(Kind::Font), scale(scale), style(style), typeface(typeface)
		{
		}

		void write(MetaWriter& meta) const override
		{
			meta.write("typeface", typeface);
			meta.write("scale", scale);
			meta.write("style", style);
		}

		uint16_t height() const
		{
			return scale.scaleY(typeface.height());
		}

		Scale scale;
		FontStyles style;
		const TypeFace& typeface;
	};
	GRAPHICS_VERIFY_SIZE(FontElement, 16);

	class ColorElement : public Element
	{
	public:
		ColorElement(const Brush& fore, const Brush& back) : Element(Kind::Color), fore(fore), back(back)
		{
		}

		void write(MetaWriter& meta) const override
		{
			meta.write("fore", fore);
			meta.write("back", back);
		}

		Brush fore{Color::WHITE};
		Brush back{Color::BLACK};
	};

	class RunElement : public Element
	{
	public:
		RunElement(Point pos, uint16_t width, uint16_t offset, uint8_t length)
			: Element(Kind::Run), pos(pos), width(width), offset(offset), length(length)
		{
		}

		void write(MetaWriter& meta) const override
		{
			meta.write("pos", pos);
			meta.write("width", width);
			meta.write("offset", offset);
			meta.write("length", length);
		}

		Point pos;
		uint16_t width;
		uint16_t offset;
		uint8_t length;
	};
	GRAPHICS_VERIFY_SIZE(RunElement, 20);

	template <typename T> T* addElement(T* elem)
	{
		elements.add(elem);
		return elem;
	}

	TextElement* addText(const TextAsset& text)
	{
		return addElement(new TextElement(text));
	}

	FontElement* addFont(const Font& font, Scale scale, FontStyles style)
	{
		return addElement(new FontElement(*font.getFace(style), scale, style));
	}

	FontElement* addFont(const TypeFace& typeface, Scale scale, FontStyles style)
	{
		return addElement(new FontElement(typeface, scale, style));
	}

	ColorElement* addColor(const Brush& fore, const Brush& back)
	{
		return addElement(new ColorElement(fore, back));
	}

	RunElement* addRun(Point pos, uint16_t width, uint16_t offset, uint8_t length)
	{
		return addElement(new RunElement(pos, width, offset, length));
	}

	Rect bounds{};
	Element::OwnedList elements;
};

/**
 * @brief Describes a target surface and corresponding source location
 * 
 * Used in surface copy operations to read display memory contents.
 */
class SurfaceObject : public ObjectTemplate<Object::Kind::Surface>
{
public:
	/**
	 * @brief Constructor
	 * @param surface
	 * @param dest Where on the surface to write
	 * @param source Start position of source
	 */
	SurfaceObject(Surface& surface, const Rect& dest, Point source) : surface(surface), dest(dest), source(source)
	{
	}

	Renderer* createRenderer(const Location& location) const override;

	/* Meta */

	void write(MetaWriter& meta) const override;

	Surface& surface;
	Rect dest;
	Point source;
};

/**
 * @brief Describes a copy operation within the same surface
 */
class CopyObject : public ObjectTemplate<Object::Kind::Copy>
{
public:
	CopyObject(const Rect& source, Point dest) : source(source), dest(dest)
	{
	}

	Renderer* createRenderer(const Location& location) const override;

	/* Meta */

	void write(MetaWriter& meta) const override
	{
		meta.write("source", source);
		meta.write("dest", dest);
	}

	Rect source;
	Point dest;
};

/**
 * @brief Describes a scrolling operation
 */
class ScrollObject : public ObjectTemplate<Object::Kind::Scroll>
{
public:
	ScrollObject(const Rect& area, Point shift, bool wrapx, bool wrapy, Color fill)
		: area(area), shift(shift), wrapx(wrapx), wrapy(wrapy), fill(fill)
	{
	}

	Renderer* createRenderer(const Location& location) const override;

	/* Meta */

	void write(MetaWriter& meta) const override
	{
		meta.write("area", area);
		meta.write("shift", shift);
		meta.write("wrapx", wrapx);
		meta.write("wrapy", wrapy);
		meta.write("fill", fill);
	}

	Rect area;
	Point shift;
	bool wrapx;
	bool wrapy;
	Color fill;
};

/**
 * @brief A collection of line and curve drawing operations
 * 
 * Stored in a compact format which can be streamed.
 */
class DrawingObject : public ObjectTemplate<Object::Kind::Drawing>
{
public:
	using Callback = Delegate<void(DrawingObject* drawing)>;

	DrawingObject(IDataSourceStream* content) : stream(content)
	{
	}

	DrawingObject(const FSTR::ObjectBase& source) : DrawingObject(new FSTR::Stream(source))
	{
	}

	DrawingObject(String&& content) : stream(new MemoryDataStream(std::move(content)))
	{
	}

	void write(MetaWriter& meta) const override;

	Renderer* createRenderer(const Location& location) const override;

	IDataSourceStream& getStream() const
	{
		return *stream.get();
	}

	std::unique_ptr<IDataSourceStream> stream;
	AssetList assets;
};

} // namespace Graphics

String toString(Graphics::Object::Kind kind);
