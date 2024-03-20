/**
 * Debug.cpp
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

#include "include/Graphics/Debug.h"

namespace Graphics
{
void highlightText(SceneObject& scene)
{
	for(auto& obj : scene.objects) {
		if(obj.kind() == Object::Kind::Text) {
			highlightText(static_cast<TextObject&>(obj));
		}
	}
}

void highlightText(TextObject& object)
{
	debug_i("Text: (%s)", object.bounds.toString().c_str());
	bool light{false};
	uint16_t textHeight{0};
	for(auto& el : object.elements) {
		if(el.kind == TextObject::Element::Kind::Font) {
			auto& font = static_cast<const TextObject::FontElement&>(el);
			textHeight = font.height();
		}
		if(el.kind != TextObject::Element::Kind::Run) {
			continue;
		}
		auto& seg = static_cast<const TextObject::RunElement&>(el);
		Rect r{object.bounds.topLeft() + seg.pos, seg.width, textHeight};
		object.insertAfter(new FilledRectObject(makeColor(Color::White, 60 + (light ? 0 : 40)), r));
		light = !light;
	}
}

} // namespace Graphics
