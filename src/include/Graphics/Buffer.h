/**
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

#include "Colors.h"
#include <Data/LinkedObjectList.h>
#include <debug_progmem.h>

namespace Graphics
{
/**
 * @brief Shared heap-allocated data buffer
 * 
 * Used for write operations with data outside Command List.
 */
class SharedBuffer
{
public:
	class Control
	{
	public:
		Control(size_t bufSize) : data(new uint8_t[bufSize]), size{bufSize}, refCount{1}
		{
			// debug_i("Control(%p, %u)", this, size);
		}

		~Control()
		{
			// debug_i("~Control(%p, %u)", this, size);
			delete[] data;
		}

		void addRef()
		{
			++refCount;
			// debug_i("addRef(%p, %u)", this, refCount);
		}

		size_t release()
		{
			assert(refCount > 0);
			// debug_i("release(%p, %u)", this, refCount - 1);
			if(--refCount != 0) {
				return refCount;
			}
			delete this;
			return 0;
		}

		uint8_t* data;
		size_t size;
		size_t refCount;
	};

	SharedBuffer()
	{
	}

	SharedBuffer(SharedBuffer&& other) = delete;

	SharedBuffer(const SharedBuffer& other)
	{
		*this = other;
	}

	SharedBuffer(size_t bufSize)
	{
		init(bufSize);
	}

	SharedBuffer(SharedBuffer& other) : control(other.control)
	{
		addRef();
	}

	~SharedBuffer()
	{
		release();
	}

	SharedBuffer& operator=(const SharedBuffer& other)
	{
		if(*this != other) {
			// debug_i("operator=(%p, %p)", control, other.control);
			release();
			control = other.control;
			addRef();
		}
		return *this;
	}

	void init(size_t bufSize)
	{
		assert(control == nullptr);
		control = new Control{bufSize};
	}

	explicit operator bool() const
	{
		return control != nullptr;
	}

	uint8_t* get()
	{
		return control ? control->data : nullptr;
	}

	void addRef()
	{
		if(control != nullptr) {
			control->addRef();
		}
	}

	void release()
	{
		if(control == nullptr) {
			return;
		}
		assert(control->refCount != 0);
		if(control->release() == 0) {
			// debug_i("SharedBuffer::release(%p)", control);
			control = nullptr;
		}
	}

	size_t usage_count() const
	{
		return control ? control->refCount : 0;
	}

	size_t size() const
	{
		return control ? control->size : 0;
	}

	uint8_t& operator[](size_t offset)
	{
		if(control == nullptr) {
			abort();
		}
		return control->data[offset];
	}

	bool operator==(const SharedBuffer& other) const
	{
		return control == other.control;
	}

	bool operator!=(const SharedBuffer& other) const
	{
		return !operator==(other);
	}

	Control* getControl()
	{
		return control;
	}

private:
	Control* control{nullptr};
};

/**
 * @brief Buffer used for reading pixel data from device
 */
struct ReadBuffer {
	SharedBuffer data;	///< Buffer to read pixel data
	uint16 offset{0};	 ///< Offset from start of buffer to start writing
	PixelFormat format{}; ///< Input: Requested pixel format, specify 'None' to get native format
	uint8_t reserved{0};

	ReadBuffer()
	{
	}

	ReadBuffer(const ReadBuffer& other) : data(other.data), offset(other.offset), format(other.format)
	{
	}

	ReadBuffer(PixelFormat format, size_t bufSize) : data(bufSize), format(format)
	{
	}

	size_t size() const
	{
		return data.size();
	}
};

/**
 * @brief Stores result of read operation
 */
struct ReadStatus {
	size_t bytesRead{0};  ///< On completion, set to actual length of data read
	PixelFormat format{}; ///< Format of data
	bool readComplete{false};
};

/**
 * @brief Composite ReadBuffer with status
 */
struct ReadStatusBuffer : public ReadBuffer {
	using ReadBuffer::ReadBuffer;
	ReadStatus status;
};

} // namespace Graphics
