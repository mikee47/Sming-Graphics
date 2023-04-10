/****
 * Renderer.h
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

#include "Scene.h"
#include "Buffer.h"
#include "Surface.h"
#include <Delegate.h>

namespace Graphics
{
/**
 * @brief Fixed list of types
 * 
 * Rendering algorithms create small sets of points, lines or rectangles.
 * Buffering these in a small list simplifies logic considerably.
 */
template <typename T> class ItemList
{
public:
	ItemList(uint8_t capacity) : capacity(capacity)
	{
		items.reset(new T[capacity]);
	}

	void add(T value)
	{
		assert(count < capacity);
		items[count++] = value;
	}

	T* get()
	{
		return (index < count) ? &items[index] : nullptr;
	}

	T* next()
	{
		++index;
		return get();
	}

	void reset()
	{
		index = count = 0;
	}

	explicit operator bool() const
	{
		return count != 0;
	}

private:
	std::unique_ptr<T[]> items;
	uint8_t capacity;
	uint8_t count{0};
	uint8_t index{0};
};

/**
 * @brief Small list of points for drawing
 * 
 * Algorithms generate multiple points within a loop so buffering them in a list simplifies
 * logic considerably.
 */
class PointList : public ItemList<Point>
{
public:
	using ItemList::ItemList;

	PointList(const Rect& bounds, const Brush& brush, uint8_t capacity)
		: ItemList(capacity), bounds(bounds), object(brush, {})
	{
	}

	void add(int16_t x, int16_t y)
	{
		Point pt{x, y};
		if(Rect(bounds.size()).contains(pt)) {
			ItemList::add(pt);
		}
	}

	/**
	 * @brief Render each point
	 * @param surface
	 * @retval bool true if all points have been rendered, false if surface is full
	 */
	bool render(Surface& surface);

private:
	Rect bounds;
	PointObject object;
	std::unique_ptr<Renderer> renderer;
};

/**
 * @brief Small list of rectangles, similar to PointList
 */
class RectList : public ItemList<Rect>
{
public:
	RectList(const Rect& bounds, const Brush& brush, uint8_t capacity)
		: ItemList(capacity), bounds(bounds), object(brush, {})
	{
	}

	void add(const Rect& rect)
	{
		auto r = intersect(rect, bounds.size());
		if(r) {
			ItemList::add(r);
		}
	}

	bool render(Surface& surface);

private:
	Rect bounds;
	FilledRectObject object;
	std::unique_ptr<Renderer> renderer;
};

/**
 * @brief Base class to render multiple objects
 */
class MultiRenderer : public Renderer
{
public:
	using Renderer::Renderer;

	bool execute(Surface& surface) override;

protected:
	virtual void renderDone(const Object* object) = 0;
	virtual const Object* getNextObject() = 0;

private:
	std::unique_ptr<Renderer> renderer;
	const Object* object{nullptr};
};

/**
 * @brief A scene is a list of other objects, so we just iterate through the list and draw each in turn.
 *
 * Rendering is performed by calling `Surface::render()`. Surfaces are provided by devices so may be able
 * to provide optimised renderers for their hardware.
 */
class SceneRenderer : public MultiRenderer
{
public:
	SceneRenderer(const Location& location, const SceneObject& scene) : MultiRenderer(location), scene(scene)
	{
	}

protected:
	void renderDone(const Object* object) override
	{
	}

	const Object* getNextObject() override
	{
		if(nextObject) {
			nextObject = nextObject->getNext();
		} else {
			nextObject = scene.objects.head();
		}
		return nextObject;
	}

private:
	const SceneObject& scene;
	const Object* nextObject{};
};

/**
 * @brief Draws 1-pixel lines
 * 
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class GfxLineRenderer : public Renderer
{
public:
	GfxLineRenderer(const Location& location, const LineObject& object)
		: GfxLineRenderer(location, object.pen, object.pt1, object.pt2)
	{
	}

	GfxLineRenderer(const Location& location, Pen pen, Point pt1, Point pt2)
		: Renderer(location), pen(pen), x0(pt1.x), y0(pt1.y), x1(pt2.x), y1(pt2.y)
	{
		init();
	}

	bool execute(Surface& surface) override;

private:
	void init();

	Point pos{};
	Pen pen;
	uint16_t x0;
	uint16_t y0;
	uint16_t x1;
	uint16_t y1;
	uint16_t xaddr;
	int16_t dx;
	int16_t dy;
	int16_t err;
	int8_t ystep;
	bool steep;
};

/**
 * @brief Draws lines
 * 
 * See http://enchantia.com/graphapp/
 */
class LineRenderer : public Renderer
{
public:
	LineRenderer(const Location& location, const LineObject& object)
		: LineRenderer(location, object.pen, object.pt1, object.pt2)
	{
	}

	LineRenderer(const Location& location, Pen pen, Point pt1, Point pt2)
		: Renderer(location), rectangles(location.dest, pen, 1), w(pen.width), x1(pt1.x), y1(pt1.y), x2(pt2.x),
		  y2(pt2.y)
	{
		init();
	}

	bool execute(Surface& surface) override;

private:
	enum class Mode {
		simple,
		diagonal,
		horizontal,
		vertical,
		done,
	};

	void init();
	void drawDiagonal();
	void initHorizontal();
	void drawHorizontal();
	void initVertical();
	void drawVertical();

	RectList rectangles;
	const uint16_t w;
	uint16_t x1;
	uint16_t y1;
	uint16_t x2;
	uint16_t y2;
	Rect r;
	uint16_t dx;
	uint16_t dy;
	uint16_t adj_up;
	uint16_t adj_down;
	uint16_t whole_step;
	uint16_t initial_run;
	uint16_t final_run;
	uint16_t run_length;
	uint16_t run_pos{0};
	int16_t error_term;
	int8_t xadvance;
	Mode mode{};
};

/**
 * @brief Draws series of lines defined by a `PolylineObject`
 */
class PolylineRenderer : public Renderer
{
public:
	PolylineRenderer(const Location& location, const PolylineObject& object) : Renderer(location), object(object)
	{
	}

	bool execute(Surface& surface) override;

protected:
	const PolylineObject& object;
	LineObject line;
	std::unique_ptr<Renderer> renderer;
	uint16_t index{0};
};

/**
 * @brief Draws a rectangle as a polyline
 */
class RectRenderer : public Renderer
{
public:
	RectRenderer(const Location& location, const Pen& pen, const Rect& rect)
		: Renderer(location), rectangles(location.dest, pen, 4)
	{
		auto w = pen.width;
		auto& r = rect;

		auto w2 = w + w;
		if(w2 >= r.w || w2 >= r.h)
			rectangles.add(r);
		else {
			rectangles.add(Rect(r.x, r.y, r.w - w, w));
			rectangles.add(Rect(r.x, r.y + w, w, r.h - w));
			rectangles.add(Rect(r.x + r.w - w, r.y, w, r.h - w));
			rectangles.add(Rect(r.x + w, r.y + r.h - w, r.w - w, w));
		}
	}

	RectRenderer(const Location& location, const RectObject& object) : RectRenderer(location, object.pen, object.rect)
	{
	}

	bool execute(Surface& surface) override
	{
		return rectangles.render(surface);
	}

private:
	RectList rectangles;
};

/**
 * @brief Draws a filled rectangle
 *
 * To accommodate transparency, etc.
 */
class FilledRectRenderer : public Renderer
{
public:
	FilledRectRenderer(const Location& location, const Brush& brush, const Rect& rect, std::unique_ptr<Blend> blender)
		: Renderer(location), brush(brush), rect{rect}, blender(std::move(blender))
	{
	}

	FilledRectRenderer(const Location& location, const FilledRectObject& object)
		: Renderer(location), brush(object.brush), rect(object.rect)
	{
	}

	FilledRectRenderer(const Location& location, const PointObject& object)
		: Renderer(location), brush(object.brush), rect{object.point, 1, 1}
	{
	}

	bool execute(Surface& surface) override;

private:
	struct Buffer : public ReadStatusBuffer {
		enum class State {
			empty,
			reading,
			writing,
		};
		static constexpr size_t bufSize{256};
		static constexpr size_t bufPixels{bufSize / 3};

		Rect r;

		Buffer() : ReadStatusBuffer{PixelFormat::None, bufSize}
		{
		}
	};

	int queueRead(Surface& surface);

	Brush brush;
	Rect rect;
	Point pos{};
	Size blockSize;
	std::unique_ptr<Blend> blender;
	Buffer buffers[2];
	uint8_t index{0};
	uint8_t busyCount{0};
	bool done{false};
};

/**
 * @brief Draws a rectangle outline with rounded corners
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class RoundedRectRenderer : public Renderer
{
public:
	RoundedRectRenderer(const Location& location, const RectObject& object)
		: Renderer(location), polyline(object), pen(object.pen), rect(object.rect),
		  radius(object.radius), corners{Point(rect.left() + radius, rect.top() + radius),
										 Point(rect.right() - radius, rect.top() + radius),
										 Point(rect.right() - radius, rect.bottom() - radius),
										 Point(rect.left() + radius, rect.bottom() - radius)}
	{
		renderer.reset(new PolylineRenderer(location, polyline));
	}

	bool execute(Surface& surface) override;

private:
	std::unique_ptr<Renderer> renderer;
	PolylineObject polyline;
	Pen pen;
	Rect rect;
	uint8_t radius;
	uint8_t state{0};
	Point corners[4];
};

/**
 * @brief Draws a filled rectangle with rounded corners
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class FilledRoundedRectRenderer : public Renderer
{
public:
	FilledRoundedRectRenderer(const Location& location, const FilledRectObject& object)
		: Renderer(location), object(object)
	{
		auto& rect = this->object.rect;
		auto r = object.radius;
		corners[0] = Point(rect.x + r, rect.y + r);
		corners[1] = Point(rect.x + r, rect.bottom() - r);
	}

	bool execute(Surface& surface) override;

private:
	std::unique_ptr<Renderer> renderer;
	FilledRectObject object;
	uint8_t state{0};
	Point corners[2];
};

/**
 * @brief Draws a circle outline
 * 
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class CircleRenderer : public Renderer
{
public:
	CircleRenderer(const Location& location, const CircleObject& object)
		: CircleRenderer(location, object.pen, object.centre, object.radius, 0, 0x0f)
	{
		pixels.add(x0, y0 + y);
		pixels.add(x0, y0 - y);
		pixels.add(x0 + y, y0);
		pixels.add(x0 - y, y0);
	}

	/**
	 * @brief Used to draw corners only
	 */
	CircleRenderer(const Location& location, const Pen& pen, Point centre, uint16_t radius, uint16_t delta,
				   uint8_t corners)
		: Renderer(location), pixels(location.dest, pen, 8), x0(centre.x), y0(centre.y), f(1 - radius), ddF_x(1),
		  ddF_y(-2 * radius), x(0), y(radius), delta(delta), corners(corners)
	{
	}

	bool execute(Surface& surface) override;

private:
	PointList pixels;
	int16_t x0;
	int16_t y0;
	int16_t f;
	int16_t ddF_x;
	int16_t ddF_y;
	int16_t x;
	int16_t y;
	uint16_t delta;
	uint8_t corners;
};

/**
 * @brief Draws a filled circle
 * 
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */
class FilledCircleRenderer : public Renderer
{
public:
	FilledCircleRenderer(const Location& location, const FilledCircleObject& object)
		: FilledCircleRenderer(location, object.brush, object.centre, object.radius, 0, 0x03)
	{
		addLine(x0 - x, x0 + x, y0);
	}

	/**
	 * @brief Used to draw rounded parts of a rounded rectangle
	 * These are handled by drawing lines between the left/right corners
	 */
	FilledCircleRenderer(const Location& location, const Brush& brush, Point centre, uint16_t radius, uint16_t delta,
						 uint8_t quadrants)
		: Renderer(location), rectangles(location.dest, brush, 4), x0(centre.x), y0(centre.y), f(1 - radius),
		  ddF_x(-2 * radius), ddF_y(1), x(radius), y(0), px(x), py(y), delta(delta), quadrants(quadrants)
	{
	}

	bool execute(Surface& surface) override;

private:
	void addLine(uint16_t x0, uint16_t x1, uint16_t y)
	{
		rectangles.add(Rect(x0, y, 1 + x1 - x0, 1));
	}

	RectList rectangles;
	int16_t x0;
	int16_t y0;
	int16_t f;
	int16_t ddF_x;
	int16_t ddF_y;
	int16_t x;
	int16_t y;
	int16_t px;
	int16_t py;
	uint16_t delta;
	uint8_t quadrants;
	Location loc;
	uint16_t pixelsToWrite{0};
};

/**
 * @brief State information for tracing an ellipse outline
 */
struct Ellipse {
	Ellipse()
	{
	}

	/* ellipse: e(x,y) = b*b*x*x + a*a*y*y - a*a*b*b */
	Ellipse(Size size)
		: a(size.w / 2), b(size.h / 2), x(0), y(b), a2(a * a), b2(b * b), xcrit((3 * a2 / 4) + 1),
		  ycrit((3 * b2 / 4) + 1), t(b2 + a2 - 2 * a2 * b), dxt(b2 * (3 + x + x)), dyt(a2 * (3 - y - y)), d2xt(b2 + b2),
		  d2yt(a2 + a2)
	{
	}

	enum class Move {
		down,
		out,
	};
	using Step = BitSet<uint8_t, Move, 2>;

	Step step();

	uint16_t a; // semi-major axis length
	uint16_t b; // semi-minor axis length
	uint16_t x;
	uint16_t y;
	int32_t a2;
	int32_t b2;
	int32_t xcrit;
	int32_t ycrit;
	int32_t t;
	int32_t dxt;
	int32_t dyt;
	int32_t d2xt;
	int32_t d2yt;
};

class ArcRectList : public RectList
{
public:
	using RectList::RectList;

	void fill(const Rect& r, Point p0, Point p1, Point p2, int start_angle, int end_angle);
};

/**
 * @brief Draws an ellipse outline
 * 
 * Uses McIlroy's Ellipse Algorithm
 *
 * See http://enchantia.com/graphapp/doc/tech/ellipses.html
 */
class EllipseRenderer : public Renderer
{
public:
	EllipseRenderer(const Location& location, const Pen& pen, const Rect& rect)
		: Renderer(location), r(rect), rectangles(location.dest, pen, 8), w(pen.width)
	{
	}

	EllipseRenderer(const Location& location, const EllipseObject& object)
		: EllipseRenderer(location, object.pen, object.rect)
	{
	}

	EllipseRenderer(const Location& location, const CircleObject& object)
		: EllipseRenderer(location, object.pen, object.getRect())
	{
	}

	bool execute(Surface& surface) override;

protected:
	enum class State {
		init,
		running,
		final1,
		final2,
		done,
	};

	virtual void addRectangles1();
	virtual void addRectangles2();
	virtual void final();

	const Rect r;
	Rect r1;
	Rect r2;
	ArcRectList rectangles;
	const uint16_t w;
	uint16_t W;
	State state{};

private:
	Ellipse outer;
	Ellipse inner;
	Point prev;
	uint16_t innerX{0};
};

/**
 * @brief Draws a filled ellipse
 * 
 * See http://enchantia.com/graphapp/doc/tech/ellipses.html
 */
class FilledEllipseRenderer : public Renderer
{
public:
	FilledEllipseRenderer(const Location& location, const Brush& brush, const Rect& rect)
		: Renderer(location), r(rect), rectangles(location.dest, brush, 4)
	{
	}

	FilledEllipseRenderer(const Location& location, const FilledEllipseObject& object)
		: FilledEllipseRenderer(location, object.brush, object.rect)
	{
	}

	FilledEllipseRenderer(const Location& location, const FilledCircleObject& object)
		: FilledEllipseRenderer(location, object.brush, object.getRect())
	{
	}

	bool execute(Surface& surface) override;

protected:
	virtual void doStep(Ellipse::Step step);
	virtual void final();

	enum class State {
		init,
		running,
		final,
		done,
	};

	const Rect r;
	ArcRectList rectangles;
	Ellipse e;
	Rect r1;
	Rect r2;
	State state{};
};

/**
 * @brief Render arc outline with adjustable line width
 */
class ArcRenderer : public EllipseRenderer
{
public:
	ArcRenderer(const Location& location, const Pen& pen, const Rect& rect, int start_angle, int end_angle)
		: EllipseRenderer(location, pen, rect), start_angle(normaliseAngle(start_angle)),
		  end_angle(normaliseAngle(end_angle))
	{
	}

	bool execute(Surface& surface) override;

protected:
	void addRectangles1() override;
	void addRectangles2() override;
	void final() override;

private:
	/* Input parameters */
	uint16_t start_angle;
	uint16_t end_angle;

	/* arc wedge line end points */
	Point p0;
	Point p1;
	Point p2;
};

/**
 * @brief Render arc outline with adjustable line width
 */
class FilledArcRenderer : public FilledEllipseRenderer
{
public:
	FilledArcRenderer(const Location& location, const Brush& brush, const Rect& rect, int start_angle, int end_angle)
		: FilledEllipseRenderer(location, brush, rect), start_angle(normaliseAngle(start_angle)),
		  end_angle(normaliseAngle(end_angle))
	{
	}

	bool execute(Surface& surface) override;

protected:
	void doStep(Ellipse::Step step) override;
	void final() override;

private:
	int16_t start_angle;
	int16_t end_angle;
	Point p0;
	Point p1;
	Point p2;
};

/**
 * @brief Render an image object
 */
class ImageRenderer : public Renderer
{
public:
	ImageRenderer(const Location& location, const ImageObject& object) : Renderer(location), object(object)
	{
	}

	bool execute(Surface& surface) override;

private:
	const ImageObject& object;
	uint8_t bytesPerPixel{0};
	PixelFormat pixelFormat{};
};

/**
 * @brief Copy an area to another surface
 * 
 * Typically used to copy display memory into RAM
 */
class SurfaceRenderer : public Renderer
{
public:
	SurfaceRenderer(const Location& location, const SurfaceObject& object)
		: SurfaceRenderer(location, object.surface, object.dest, object.source)
	{
	}

	SurfaceRenderer(const Location& location, Surface& target, const Rect& dest, Point source)
		: Renderer(location), target(target), dest(dest), source(source)
	{
	}

	bool execute(Surface& surface) override;

private:
	static constexpr uint16_t bufSize{512};
	ReadBuffer buffers[2];
	Surface& target;
	Rect dest;
	Point source;
	PixelFormat pixelFormat{};
	uint8_t bufIndex{0};
	bool done{false};
	uint8_t busyCount{0};
};

/**
 * @brief Copy an area within the same surface
 */
class CopyRenderer : public Renderer
{
public:
	using Renderer::Renderer;

	CopyRenderer(const Location& location, const CopyObject& object) : Renderer(location)
	{
		Rect src = object.source;
		Rect dst(object.dest, object.source.size());
		src += this->location.dest.topLeft();
		dst += this->location.dest.topLeft();
		// TODO: Do X/Y separately
		src.clip(location.dest);
		dst.clip(location.dest);
		src.w = dst.w = std::min(src.w, dst.w);
		src.h = dst.h = std::min(src.h, dst.h);
		this->location.dest = dst;
		this->location.source = src;
	}

	bool execute(Surface& surface) override;

protected:
	void init();
	void startRead(Surface& surface);

	/* Position is given in `location` */
	virtual void readComplete(uint8_t* data, size_t length)
	{
	}

	PixelFormat pixelFormat{};
	uint8_t bytesPerPixel;
	bool vertical;

private:
	using LineBuffer = ReadStatusBuffer;
	LineBuffer lineBuffers[2];
	uint16_t lineSize; // Width or height
	uint16_t lineCount;
	uint16_t readIndex{0};
	uint16_t writeIndex{0};
	TPoint<int8_t> shift{};
};

class ImageCopyRenderer : public CopyRenderer
{
public:
	ImageCopyRenderer(const Location& location, const ImageObject& image, const Blend* blend)
		: CopyRenderer({location.dest, location.dest}), image(image), blend(blend)
	{
		auto& dst = location.dest;
		this->location.source.w = this->location.dest.w = std::min(dst.w, image.width());
		this->location.source.h = this->location.dest.h = std::min(dst.h, image.height());
	}

protected:
	void readComplete(uint8_t* data, size_t length) override;

private:
	const ImageObject& image;
	const Blend* blend;
};

/**
 * @brief Scroll an area
 */
class ScrollRenderer : public Renderer
{
public:
	ScrollRenderer(const Location& location, const ScrollObject& object) : Renderer(location), object(object)
	{
	}

	bool execute(Surface& surface) override;

private:
	void init();
	bool startRead(Surface& surface);
	int16_t checkx(int16_t x);
	int16_t checky(int16_t y);
	void stepx(uint16_t& index, int16_t& x);
	void stepy(uint16_t& index, int16_t& y);

	using LineBuffer = ReadStatusBuffer;

	const ScrollObject& object;
	Rect src{};
	Rect dst{};
	int16_t cx;
	int16_t cy;
	uint16_t readOffset{0};
	uint16_t writeOffset{0};
	LineBuffer lineBuffers[2];
	uint16_t lineCount;
	uint16_t readIndex{0};
	uint16_t writeIndex{0};
	Rect readArea;
	Rect writeArea;
	PackedColor fill;
	PixelFormat pixelFormat{};
	uint8_t bytesPerPixel;
	bool vertical; // If true, copy vertical lines
	uint8_t state{0};
};

/**
 * @brief Perform blending with draw
 */
class BlendRenderer : public Renderer
{
public:
	BlendRenderer(const Location& location, const Object& object, const Blend* blend)
		: Renderer(location), object(object), blend(blend)
	{
	}

	bool execute(Surface& surface) override;

private:
	enum class State {
		init,
		draw,
		done,
	};

	const Object& object;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<MemoryImageObject> image;
	std::unique_ptr<Surface> imageSurface;
	PixelFormat pixelFormat;
	const Blend* blend;
	State nextState{};
};

/**
 * @brief Draw a line of text
 * 
 * If foreground and background colours are the same then the text is renderered transparently.
 */
class TextRenderer : public Renderer
{
public:
	TextRenderer(const Location& location, const TextObject& object)
		: Renderer(location), object(object), alphaBuffer(object, location.dest.h - location.pos.y),
		  element(alphaBuffer.element)
	{
		this->location.dest += object.bounds.topLeft();
		this->location.pos = Point{};
	}

	bool execute(Surface& surface) override;

private:
	struct AlphaBuffer {
		const TextObject::Element* element;
		const TextAsset* text{nullptr};
		const TextObject::FontElement* font{nullptr};
		std::unique_ptr<uint8_t[]> data;
		Size size{};
		uint16_t charIndex{0};
		uint16_t x{0};
		uint16_t xo{0};
		uint16_t ymax;
		uint8_t advdiff{0};

		AlphaBuffer(const TextObject& object, uint16_t ymax) : element{object.elements.head()}, ymax(ymax)
		{
		}

		void init(Size size)
		{
			data.reset(new uint8_t[size.w * size.h]{});
			this->size = size;
		}

		void clear()
		{
			memset(data.get(), 0, size.w * size.h);
		}

		void fill();

		bool finished()
		{
			return element == nullptr;
		}

		// Shift content from given position to start of buffer, clear the vacated space
		void shift(uint16_t count)
		{
			count = std::min(count, x);
			auto row = data.get();
			for(unsigned i = 0; i < size.h; ++i) {
				memmove(row, &row[count], size.w - count);
				memset(&row[size.w - count], 0, count);
				row += size.w;
			}
			xo += count;
			x -= count;
		}
	};

	struct BackBuffer : public ReadStatusBuffer {
		static constexpr size_t bufSize{512};

		BackBuffer() : ReadStatusBuffer(PixelFormat::None, bufSize)
		{
		}

		Rect r{};
		Point pos{};
		const TextObject::RunElement* run{nullptr};
		TextOptions options;
		uint16_t glyphPixels;
		bool lastRow{false};
	};

	void getNextRun();
	bool startRead(Surface& surface);
	bool renderBuffer(Surface& surface, BackBuffer& backBuffer);

	const TextObject& object;
	AlphaBuffer alphaBuffer;
	const TextObject::RunElement* run{nullptr};
	const TextObject::Element* element;
	GlyphObject::Options options;
	BackBuffer backBuffers[2];
	uint8_t readIndex{0};
	uint8_t writeIndex{0};
	const TypeFace* typeface{nullptr};
	PixelFormat pixelFormat{};
	uint8_t bytesPerPixel;
	uint8_t busyCount{0};
};

} // namespace Graphics
