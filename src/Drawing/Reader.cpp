/**
 * Drawing.cpp
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

#include "../include/Graphics/Drawing/Reader.h"
#include "../include/Graphics/Drawing/Header.h"
#include "../include/Graphics/LcdFont.h"

namespace Graphics
{
namespace Drawing
{
Reader::Reader(const DrawingObject& drawing) : drawing(drawing), cache(*drawing.stream)
{
	seek(0);
	state.reset();
	sub = &root;
}

Reader::~Reader()
{
	while(!stack.empty()) {
		sub = stack.pop();
		if(sub == &root) {
			break;
		}
		delete sub;
	}
}

const Asset* Reader::findAsset(uint16_t id) const
{
	auto asset = assets.find(id);
	if(asset == nullptr) {
		asset = drawing.assets.find(id);
	}
	if(asset == nullptr) {
		debug_e("[DRAW] Asset #%u not found", id);
	}
	return asset;
}

Object* Reader::readObject()
{
	Header hdr;
	while(read(&hdr, 1)) {
		if(definingSubroutine) {
			if(hdr.opcode == OpCode::execute) {
				if(hdr.cmd == Command::beginSub) {
					debug_e("[DRAW] Illegal nested beginSub");
				} else if(hdr.cmd == Command::endSub) {
					definingSubroutine = false;
				}
			} else if(hdr.type == Header::Type::resource) {
				if(hdr.resource.dataType == Header::DataType::charArray) {
					uint16_t length{0};
					read(&length, 1 << uint8_t(hdr.resource.lengthSize));
					streamPos += length;
				} else {
					streamPos += sizeof(void*);
				}
			} else {
				streamPos += 1 << uint8_t(hdr.type);
			}
			continue;
		}

		if(hdr.opcode != OpCode::execute) {
			switch(hdr.type) {
			case Header::Type::uint8: {
				uint8_t value;
				read(&value, sizeof(value));
				debug_d("@0x%08x %s %s, %u", streamPos, toString(hdr.opcode).c_str(),
						Registers::nameAt(hdr.index).c_str(), value);
				if(hdr.opcode == OpCode::store) {
					state.reg.update(hdr.index, hdr.opcode, value);
				} else {
					state.reg.update(hdr.index / 2, hdr.opcode, uint16_t(value));
				}
				break;
			}
			case Header::Type::uint16: {
				uint16_t value;
				read(&value, sizeof(value));
				debug_d("@0x%08x %s %s, %u", streamPos, toString(hdr.opcode).c_str(),
						Registers::nameAt(hdr.index * 2).c_str(), value);
				state.reg.update(hdr.index, hdr.opcode, value);
				break;
			}
			case Header::Type::uint32: {
				uint32_t value;
				read(&value, sizeof(value));
				debug_d("@0x%08x %s %s, 0x%08x", streamPos, toString(hdr.opcode).c_str(),
						Registers::nameAt(hdr.index * 4).c_str(), value);
				state.reg.update(hdr.index, hdr.opcode, value);
				break;
			}
			case Header::Type::resource: {
				auto& res = hdr.resource;
				switch(res.dataType) {
				case Header::DataType::charArray: {
					uint16_t len{0};
					read(&len, 1 << uint8_t(res.lengthSize));
					char buf[len];
					read(buf, len);
					debug_d("@0x%08x %s, resource #%u, '%s'", streamPos, toString(hdr.opcode).c_str(), state.reg.id,
							String(buf, len).c_str());
					assets.store(new TextAsset(state.reg.id, buf, len));
					break;
				}
				}
				break;
			}
			}
			continue;
		}

		debug_d("%s %s", toString(hdr.opcode).c_str(), ::toString(hdr.cmd).c_str());

		switch(hdr.cmd) {
		case Command::reset:
			state.reset();
			break;
		case Command::push:
			sub->state.reset(new DrawState{state});
			break;
		case Command::pop:
			if(sub->state) {
				state = *sub->state;
				sub->state.reset();
			} else {
				debug_e("[DRAW] No saved state");
			}
			break;
		case Command::storePen:
			assets.store(state.reg.id, getPen());
			break;
		case Command::storeBrush:
			assets.store(state.reg.id, getBrush());
			break;
		case Command::move:
			state.reg.x1 = state.reg.x2;
			state.reg.y1 = state.reg.y2;
			break;
		case Command::incx:
			++state.reg.x2;
			break;
		case Command::decx:
			--state.reg.x2;
			break;
		case Command::incy:
			++state.reg.y2;
			break;
		case Command::decy:
			--state.reg.y2;
			break;
		case Command::setPixel:
			return new PointObject(getBrush(), state.reg.pt2());
		case Command::line:
			return new LineObject(getPen(), state.reg.pt1(), state.reg.pt2());
		case Command::lineto: {
			auto obj = new LineObject(getPen(), state.reg.pt1(), state.reg.pt2());
			state.reg.x1 = state.reg.x2;
			state.reg.y1 = state.reg.y2;
			return obj;
		}
		case Command::drawArc:
			return new ArcObject(getPen(), state.reg.rect(), state.reg.startAngle, state.reg.endAngle());
		case Command::fillArc:
			return new FilledArcObject(getBrush(), state.reg.rect(), state.reg.startAngle, state.reg.endAngle());
		case Command::drawRect:
			return new RectObject(getPen(), state.reg.rect(), state.reg.radius);
		case Command::fillRect:
			return new FilledRectObject(getBrush(), state.reg.rect(), state.reg.radius);
		case Command::drawCircle:
			return new CircleObject(getPen(), state.reg.pt2(), state.reg.radius);
		case Command::fillCircle:
			return new FilledCircleObject(getBrush(), state.reg.pt2(), state.reg.radius);
		case Command::drawEllipse:
			return new EllipseObject(getPen(), state.reg.rect());
		case Command::fillEllipse:
			return new FilledEllipseObject(getBrush(), state.reg.rect());
		case Command::drawText: {
			auto text = findAsset<TextAsset>(state.reg.textId);
			if(text == nullptr) {
				break;
			}
			auto len = text->getLength();
			if(state.reg.offset >= len) {
				return nullptr;
			}
			auto font = findAsset<Font>(state.reg.fontId) ?: &lcdFont;
			auto obj = new TextObject(state.reg.rect());
			obj->addText(*text);
			auto& typeface = *font->getFace(state.reg.style);
			obj->addFont(typeface, {/* TODO: SCALE*/}, state.reg.style);
			obj->addColor(getPen(), getBrush());

			len = std::min(uint16_t(len - state.reg.offset), state.reg.length);
			char buf[len];
			text->read(0, buf, len);
			auto width = typeface.getTextWidth(buf, len);
			// width = scale.scaleX(width);
			width = std::min(width, obj->bounds.w);
			obj->addRun({}, width, state.reg.offset, len);

			state.reg.x1 += width;
			return obj;
		}
		case Command::beginSub: {
			assert(!definingSubroutine);
			assert(stack.empty());
			subroutines[state.reg.id] = streamPos;
			definingSubroutine = true;
			break;
		}
		case Command::endSub:
			// This has already been handled above for subroutine definition
			assert(!definingSubroutine);
			if(stack.empty()) {
				debug_e("[DRAW] Not in subroutine");
				break;
			}
			if(sub->state) {
				state = *sub->state;
			}
			seek(sub->returnOffset);
			delete sub;
			sub = stack.pop();
			break;
		case Command::call: {
			auto s = static_cast<const SubroutineMap&>(subroutines)[state.reg.id];
			if(!s) {
				debug_e("[DRAW] Subroutine %u not found", state.reg.id);
				break;
			}

			stack.push(sub);
			sub = new StackEntry{streamPos};
			seek(s);
			break;
		}
		default:
			debug_w("Drawing command %u not handled", unsigned(hdr.cmd));
		}
	}

	return nullptr;
}

} // namespace Drawing
} // namespace Graphics
