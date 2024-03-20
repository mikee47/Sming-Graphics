/****
 * Drawing.h
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

#include "../Object.h"
#include "../Stream.h"
#include "Registers.h"
#include <FILO.h>
#include <WHashMap.h>

namespace Graphics
{
class Object;
class DrawingObject;

namespace Drawing
{
class Reader
{
public:
	Reader(const DrawingObject& drawing);
	~Reader();

	Object* readObject();

private:
	struct DrawState {
		Registers reg;

		void reset()
		{
			reg = Registers{};
		}
	};

	struct StackEntry {
		uint32_t returnOffset;
		std::unique_ptr<DrawState> state;
	};

	using SubroutineMap = HashMap<AssetID, uint32_t>;

	const Asset* findAsset(uint16_t id) const;

	template <typename T> const T* findAsset(uint16_t id) const
	{
		auto asset = assets.find<T>(id);
		if(asset == nullptr) {
			asset = drawing.assets.find<T>(id);
		}
		return asset;
	}

	Pen getPen() const
	{
		auto id = state.reg.penId;
		if(id != 0) {
			auto asset = findAsset<PenAsset>(id);
			if(asset != nullptr) {
				return *asset;
			}
		}
		return Pen(state.reg.penColor, state.reg.penWidth);
	}

	Brush getBrush() const
	{
		auto id = state.reg.brushId;
		if(id != 0) {
			auto asset = findAsset(id);
			if(asset != nullptr) {
				switch(asset->type()) {
				case AssetType::Pen:
					return *static_cast<const PenAsset*>(asset);
				case AssetType::SolidBrush:
					return static_cast<const SolidBrush*>(asset)->color;
				case AssetType::TextureBrush:
					return static_cast<const TextureBrush*>(asset);
				default:
					debug_e("[DRAW] Asset #%u is %s, not compatible with Brush", id, ::toString(asset->type()).c_str());
					return Pen(state.reg.penColor, state.reg.penWidth);
				}
			}
		}
		return Brush(state.reg.brushColor);
	}

	bool read(void* buffer, uint8_t count)
	{
		uint8_t len = cache.read(streamPos, buffer, count);
		streamPos += len;
		return len == count;
	}

	void seek(uint32_t offset)
	{
		streamPos = offset;
	}

	const DrawingObject& drawing;
	SubroutineMap subroutines;
	AssetList assets;
	FILO<StackEntry*, 16> stack;
	StackEntry root{};
	StackEntry* sub{nullptr};
	DrawState state;
	uint32_t streamPos{0};
	ReadStream cache;
	bool definingSubroutine{false};
};

} // namespace Drawing
} // namespace Graphics
