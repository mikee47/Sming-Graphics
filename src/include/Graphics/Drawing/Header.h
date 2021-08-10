/**
 * Header.h
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

#include "Command.h"
#include "Registers.h"

namespace Graphics
{
namespace Drawing
{
/**
 * @brief Command header structure
 */
#pragma pack(1)
union Header {
	enum class Type : uint8_t {
		uint8,
		uint16,
		uint32,
		resource,
	};
	enum class DataType : uint8_t {
		charArray,
	};
	enum class ResourceKind : uint8_t {
		text,
		image,
	};
	// type = uint8, uint16, uint32
	struct {
		uint8_t index : 4; ///< Register index
		Type type : 2;
		OpCode opcode : 2; ///< Operation to perform
		uint32_t param;
	};
	///< Size of resource length field
	enum class LengthSize : uint8_t {
		uint8,
		uint16,
	};
	// type = resource
	struct {
		ResourceKind kind : 1;
		LengthSize lengthSize : 1;
		DataType dataType : 2;
		Type type : 2;
		OpCode opcode : 2; ///< = OpCode::store
	} resource;
	Command cmd : 6;
};
#pragma pack()

static_assert(sizeof(Header) == 5, "Header wrong size");

} // namespace Drawing
} // namespace Graphics
