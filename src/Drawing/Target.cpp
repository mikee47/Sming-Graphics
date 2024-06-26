/**
 * Target.cpp
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

#include "../include/Graphics/Drawing/Target.h"
#include "../include/Graphics/Scene.h"

namespace Graphics
{
namespace Drawing
{
bool Target::render(const Object& object, const Rect& location)
{
	switch(object.kind()) {
	case Object::Kind::Reference: {
		auto obj = static_cast<const ReferenceObject&>(object);
		Rect loc = location;
		loc += obj.pos;
		loc.w -= obj.pos.x;
		loc.h -= obj.pos.y;
		return render(obj.object, loc);
	}
	case Object::Kind::Rect: {
		auto& rect = static_cast<const RectObject&>(object);
		writer.setPen(rect.pen);
		writer.moveto(rect.rect.topLeft());
		writer.drawRect(rect.rect.bottomRight(), rect.radius);
		break;
	}
	case Object::Kind::FilledRect: {
		auto& rect = static_cast<const FilledRectObject&>(object);
		writer.setBrush(rect.brush);
		writer.moveto(rect.rect.topLeft());
		writer.fillRect(rect.rect.bottomRight(), rect.radius);
		break;
	}
	case Object::Kind::Line: {
		auto& line = static_cast<const LineObject&>(object);
		writer.setPen(line.pen);
		writer.moveto(line.pt1);
		writer.lineto(line.pt2);
		break;
	}
	case Object::Kind::Polyline: {
		auto& line = static_cast<const PolylineObject&>(object);
		writer.setPen(line.pen);
		if(line.connected) {
			writer.moveto(line[0]);
			for(unsigned i = 1; i < line.numPoints; ++i) {
				writer.lineto(line[i]);
			}
		} else {
			for(unsigned i = 0; i < line.numPoints; i += 2) {
				writer.moveto(line[i]);
				writer.lineto(line[i + 1]);
			}
		}
		break;
	}
	case Object::Kind::Circle: {
		auto& circle = static_cast<const CircleObject&>(object);
		writer.setPen(circle.pen);
		writer.drawCircle(circle.centre, circle.radius);
		break;
	}
	case Object::Kind::FilledCircle: {
		auto& circle = static_cast<const FilledCircleObject&>(object);
		writer.setBrush(circle.brush);
		writer.fillCircle(circle.centre, circle.radius);
		break;
	}
	case Object::Kind::Ellipse: {
		auto& ellipse = static_cast<const EllipseObject&>(object);
		writer.setPen(ellipse.pen);
		writer.moveto(ellipse.rect.topLeft());
		writer.drawEllipse(ellipse.rect.bottomRight());
		break;
	}
	case Object::Kind::FilledEllipse: {
		auto& ellipse = static_cast<const FilledEllipseObject&>(object);
		writer.setBrush(ellipse.brush);
		writer.moveto(ellipse.rect.topLeft());
		writer.fillEllipse(ellipse.rect.bottomRight());
		break;
	}
	case Object::Kind::Arc: {
		auto& arc = static_cast<const ArcObject&>(object);
		writer.setPen(arc.pen);
		writer.moveto(arc.rect.topLeft());
		writer.drawArc(arc.rect.bottomRight(), arc.startAngle, arc.endAngle);
		break;
	}
	case Object::Kind::FilledArc: {
		auto& arc = static_cast<const FilledArcObject&>(object);
		writer.setBrush(arc.brush);
		writer.moveto(arc.rect.topLeft());
		writer.fillArc(arc.rect.bottomRight(), arc.startAngle, arc.endAngle);
		break;
	}
	case Object::Kind::Scene: {
		auto& scene = static_cast<const SceneObject&>(object);
		for(auto& obj : scene.objects) {
			if(!render(obj, location)) {
				return false;
			}
		}
		writer.flush();
		break;
	}
	default:; // Ignore
	}

	return true;
}

} // namespace Drawing
} // namespace Graphics
