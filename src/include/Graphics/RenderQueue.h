/****
 * RenderQueue.h
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

#include "Surface.h"
#include "Renderer.h"

namespace Graphics
{
/**
 * @brief Top-level manager to queue objects for rendering to a specific target
 * 
 * Use this to render single objects, typically Scenes or Drawings
 */
class RenderQueue : private MultiRenderer
{
public:
	using Completed = Delegate<void(Object* object)>;

	/**
	 * @brief Constructor
	 * @param target Where to render scenes
	 * @param bufferSize Size of each allocated surface buffer. Specify 0 to use default.
	 * @param surfaceCount Number of surfaces to allocate
	 *
	 * Surfaces are created by the target display device.
	 *
	 * For minimum RAM usage use a single surface.
	 *
	 * For best performance use two, so one can be prepared whilst the other is
	 * being written to the screen.
	 *
	 * The RenderQueue owns these surfaces.
	 */
	RenderQueue(RenderTarget& target, uint8_t surfaceCount = 2, size_t bufferSize = 0)
		: MultiRenderer(Location{}), target(target)
	{
		while(surfaceCount-- != 0) {
			surfaces.add(target.createSurface(bufferSize));
		}
	}

	/**
	 * @brief Add object to the render queue and start rendering if it isn't already
	 * @param object Scene, Drawing, etc. to render
	 * @param location Where to draw the object
	 * @param callback Optional callback to invoke when render is complete
	 * @param delayMs Delay between render completion and callback
	 */
	template <typename T>
	void render(T* object, const Location& location, typename T::Callback callback = nullptr, uint16_t delayMs = 0)
	{
		renderObject(object, location, *reinterpret_cast<Completed*>(&callback), delayMs);
	}

	template <typename T> void render(T* object, typename T::Callback callback = nullptr, uint16_t delayMs = 0)
	{
		renderObject(object, {target.getSize()}, *reinterpret_cast<Completed*>(&callback), delayMs);
	}

	bool isActive() const
	{
		return !queue.isEmpty();
	}

private:
	void renderObject(Object* object, const Location& location, Completed callback, uint16_t delayMs);
	void renderDone(const Object* object) override;
	const Object* getNextObject() override;

	// A queued object plus callback information
	class Item : public LinkedObjectTemplate<Item>
	{
	public:
		using OwnedList = OwnedLinkedObjectListTemplate<Item>;

		Item(const Object& object, const Location& location, Completed callback, uint16_t delayMs)
			: object(object), location(location), callback(callback), delayMs(delayMs)
		{
		}

		const Object& object;
		Location location;
		Completed callback;
		uint16_t delayMs;
	};

	void run();

	RenderTarget& target;
	Item::OwnedList queue;
	std::unique_ptr<Item> item;  ///< Item being rendered
	Surface::OwnedList surfaces; ///< Available for writing
	Surface::OwnedList active;   ///< Locked - in transit
	bool done{false};
};

} // namespace Graphics
