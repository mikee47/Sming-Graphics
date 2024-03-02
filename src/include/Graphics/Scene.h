/****
 * Scene.h
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

#include "Object.h"

namespace Graphics
{
/**
 * @brief A Scene containing multiple objects
 */
class SceneObject : public Object
{
public:
	using Callback = Delegate<void(SceneObject* scene)>;

	SceneObject()
	{
	}

	SceneObject(Size size, const String& name = nullptr) : Object(), size(size), name(name)
	{
	}

	SceneObject(RenderTarget& target, const String& name = nullptr) : SceneObject(target.getSize(), name)
	{
	}

	Kind kind() const override
	{
		return Kind::Scene;
	}

	void write(MetaWriter& meta) const override
	{
		meta.write("name", name);
		meta.writeArray("objects", "Object", objects);
		meta.writeArray("assets", "Asset", assets);
	}

	Renderer* createRenderer(const Location& location) const override;

	/**
	 * @brief Add a new object to the scene
	 * @param obj This will be owned by the scene
	 * 
	 * Use this method to add custom objects.
	 * To draw an object multiple times use `drawObject` which will add a reference
	 * instead.
	 */
	template <typename T> T* addObject(T* obj)
	{
		objects.add(obj);
		return obj;
	}

	template <typename T> typename std::enable_if<std::is_base_of<Asset, T>::value, T*>::type addAsset(T* asset)
	{
		assets.add(asset);
		return asset;
	}

	ObjectAsset* addAsset(Object* object)
	{
		auto asset = new ObjectAsset(object);
		assets.add(asset);
		return asset;
	}

	Size getSize() const
	{
		return size;
	}

	/**
	 * @brief Reset the scene with a new size
	 */
	void reset(Size size)
	{
		objects.clear();
		this->size = size;
	}

	/**
	 * @brief Clear the scene and fill with a chosen colour
	 */
	void clear(const Brush& brush = Color::Black)
	{
		objects.clear();
		fillRect(brush, size);
	}

	template <typename... ParamTypes> FilledRectObject* fillRect(ParamTypes... params)
	{
		return addObject(new FilledRectObject(params...));
	}

	template <typename... ParamTypes> RectObject* drawRect(ParamTypes... params)
	{
		return addObject(new RectObject(params...));
	}

	RectObject* drawRoundRect(int x0, int y0, int w, int h, int radius, Color color)
	{
		return drawRect(color, Rect(x0, y0, w, h), radius);
	}

	FilledRectObject* fillRoundRect(int x0, int y0, int w, int h, int radius, Color color)
	{
		return fillRect(color, Rect(x0, y0, w, h), radius);
	}

	template <typename... ParamTypes> LineObject* drawLine(ParamTypes... params)
	{
		return addObject(new LineObject(params...));
	}

	template <typename... ParamTypes> PolylineObject* drawTriangle(const Pen& pen, Point pt1, Point pt2, Point pt3)
	{
		return drawPolyline(pen, pt1, pt2, pt3, pt1);
	}

	PolylineObject* drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, Color color)
	{
		return drawTriangle(color, Point(x0, y0), Point(x1, y1), Point(x2, y2));
	}

	template <typename... ParamTypes> PolylineObject* drawPolyline(ParamTypes... params)
	{
		return addObject(new PolylineObject(params...));
	}

	template <typename... ParamTypes> CircleObject* drawCircle(ParamTypes... params)
	{
		return addObject(new CircleObject(params...));
	}

	template <typename... ParamTypes> FilledCircleObject* fillCircle(ParamTypes... params)
	{
		return addObject(new FilledCircleObject(params...));
	}

	template <typename... ParamTypes> EllipseObject* drawEllipse(ParamTypes... params)
	{
		return addObject(new EllipseObject(params...));
	}

	template <typename... ParamTypes> FilledEllipseObject* fillEllipse(ParamTypes... params)
	{
		return addObject(new FilledEllipseObject(params...));
	}

	template <typename... ParamTypes> ArcObject* drawArc(ParamTypes... params)
	{
		return addObject(new ArcObject(params...));
	}

	template <typename... ParamTypes> FilledArcObject* fillArc(ParamTypes... params)
	{
		return addObject(new FilledArcObject(params...));
	}

	template <typename... ParamTypes> ReferenceObject* drawImage(const ImageObject& image, Point pos, ParamTypes... params)
	{
		return drawObject(image, Rect{pos, image.getSize()}, params...);
	}

	template <typename... ParamTypes> ReferenceObject* drawObject(const Object& object, ParamTypes... params)
	{
		return addObject(new ReferenceObject(object, params...));
	}

	SurfaceObject* copySurface(Surface& surface, const Rect& dest, Point source)
	{
		return addObject(new SurfaceObject(surface, dest, source));
	}

	/**
	 * @brief Copy region of display to another
	 * @param source Area to copy
	 * @param dest Top-left corner to copy to
	 */
	CopyObject* copy(const Rect& source, Point dest)
	{
		return addObject(new CopyObject(source, dest));
	}

	/**
	 * @brief Scroll display memory
	 * @param area Region to scroll
	 * @param cx Distance to scroll horizontally
	 * @param cy Distance to scroll vertically
	 * @param wrapx true to scroll, false to clip in X direction
	 * @param wrapx Y scroll/clip
	 * @param fill Optional color to fill in clip mode
	 */
	ScrollObject* scroll(const Rect& area, int16_t cx, int16_t cy, bool wrapx = false, bool wrapy = false,
						 Color fill = Color::None)
	{
		return addObject(new ScrollObject(area, {cx, cy}, wrapx, wrapy, fill));
	}

	ScrollObject* scroll(const Rect& area, int16_t cx, int16_t cy, Color fill)
	{
		return scroll(area, cx, cy, false, false, fill);
	}

	Size size;
	CString name;
	OwnedList objects;
	AssetList assets; // Not drawn directly, but may be referred to
};

} // namespace Graphics
