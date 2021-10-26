/****
 * Stream.h
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

#include <Data/Stream/DataSourceStream.h>

namespace Graphics
{
class WriteStream
{
public:
	WriteStream(Print& stream) : stream(stream)
	{
	}

	void write(const void* buffer, size_t count);

	void flush();

private:
	static constexpr size_t size{256};
	Print& stream;
	uint8_t data[size];
	uint16_t length{0};
};

class ReadStream
{
public:
	ReadStream(IDataSourceStream& stream) : stream(stream)
	{
	}

	size_t read(uint32_t offset, void* buffer, size_t count);
	uint8_t read(uint32_t offset);

private:
	static constexpr size_t size{64};
	IDataSourceStream& stream;
	uint8_t data[size];
	uint32_t start{0};
	uint16_t length{0};
};

class SubStream : public IDataSourceStream
{
public:
	SubStream(IDataSourceStream& source, uint32_t offset, size_t size) : source(source), startOffset(offset), size(size)
	{
		int sourceSize = source.seekFrom(0, SeekOrigin::End);
		if(sourceSize < int(offset)) {
			size = 0;
			return;
		}

		size = std::min(size, sourceSize - offset);

		source.seekFrom(offset, SeekOrigin::Start);
	}

	int available() override
	{
		return size - readPos;
	}

	uint16_t readMemoryBlock(char* data, int bufSize) override
	{
		return source.readMemoryBlock(data, bufSize);
	}

	int seekFrom(int offset, SeekOrigin origin) override
	{
		size_t newPos;
		switch(origin) {
		case SeekOrigin::Start:
			newPos = offset;
			break;
		case SeekOrigin::Current:
			newPos = readPos + offset;
			break;
		case SeekOrigin::End:
			newPos = size + offset;
			break;
		default:
			return -1;
		}

		if(newPos > size) {
			return -1;
		}

		source.seekFrom(startOffset + newPos, SeekOrigin::Start);
		readPos = newPos;
		return readPos;
	}

	bool isFinished() override
	{
		return available() <= 0;
	}

private:
	IDataSourceStream& source;
	uint32_t startOffset;
	uint32_t readPos{0};
	size_t size;
};

} // namespace Graphics
