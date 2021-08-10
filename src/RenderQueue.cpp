/**
 * RenderQueue.cpp
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

#include "include/Graphics/RenderQueue.h"
#include <Platform/System.h>
#include <Timer.h>

namespace Graphics
{
void RenderQueue::renderObject(Object* object, const Location& location, Completed callback, uint16_t delayMs)
{
	assert(object != nullptr);
	queue.add(new Item{*object, location, callback, delayMs});
	done = false;
	run();
}

void RenderQueue::run()
{
	auto callback = [](void* param) {
		auto self = static_cast<RenderQueue*>(param);
		// Release surface back to available queue and continue rendering
		auto surface = self->active.pop();
		surface->reset();
		self->surfaces.add(surface);
		self->run();
	};

	Surface* surface{nullptr};
	while(!done) {
		if(surface == nullptr) {
			surface = surfaces.pop();
			if(surface == nullptr) {
				break;
			}
		}

		done = execute(*surface);
		if(done && !queue.isEmpty()) {
			// Surface not full, keep rendering
			continue;
		}

		if(!surface->present(callback, this)) {
			/*
			 * Surface was empty, no callback will be made from that surface so queue our own.
			 * This gives other callbacks (e.g. from read operations) a chance to run.
			 */
			System.queueCallback([](void* param) { static_cast<RenderQueue*>(param)->run(); }, this);
			break;
		}

		active.add(surface);
		surface = nullptr;
	}

	// Release unused surface
	if(surface != nullptr) {
		surface->reset();
		surfaces.add(surface);
	}
}

void RenderQueue::renderDone(const Object* object)
{
	if(!item->callback) {
		item.reset();
		return;
	}
	auto callback = [](void* param) {
		auto item = static_cast<Item*>(param);
		item->callback(const_cast<Object*>(&item->object));
		delete item;
	};
	if(item->delayMs == 0) {
		System.queueCallback(callback, item.release());
	} else {
		auto timer = new AutoDeleteTimer;
		auto delayMs = item->delayMs;
		timer->initializeMs(delayMs, callback, item.release());
		timer->startOnce();
	}
}

const Object* RenderQueue::getNextObject()
{
	if(queue.isEmpty()) {
		return nullptr;
	}

	item.reset(queue.pop());
	location = item->location;
	return &item->object;
}

} // namespace Graphics