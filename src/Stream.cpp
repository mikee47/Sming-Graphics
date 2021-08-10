/**
 * Stream.cpp
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

#include "include/Graphics/Stream.h"

namespace Graphics
{
void WriteStream::write(const void* buffer, size_t count)
{
	if(length + count > size) {
		flush();
	}
	memcpy(&data[length], buffer, count);
	length += count;
}

void WriteStream::flush()
{
	stream.write(data, length);
	length = 0;
}

size_t ReadStream::read(uint32_t offset, void* buffer, size_t count)
{
	if(offset < start || offset + count > start + length) {
		start = stream.seekFrom(offset, SeekOrigin::Start);
		length = stream.readBytes(data, size);
	}
	size_t len = std::min(count, start + length - offset);
	memcpy(buffer, &data[offset - start], len);
	return len;
}

uint8_t ReadStream::read(uint32_t offset)
{
	if(offset < start || offset >= start + length) {
		start = stream.seekFrom(offset, SeekOrigin::Start);
		length = stream.readBytes(data, size);
	}
	return data[offset - start];
}

} // namespace Graphics
