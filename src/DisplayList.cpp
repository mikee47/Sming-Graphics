/**
 * DisplayList.cpp
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

#include "include/Graphics/DisplayList.h"
#include <HSPI/Controller.h>
#include <Platform/System.h>
#include <esp_attr.h>
#include <cassert>

namespace Graphics
{
String DisplayList::toString(Code code)
{
	switch(code) {
#define XX(code_, arglen, desc)                                                                                        \
	case Code::code_:                                                                                                  \
		return F(#code_);
		GRAPHICS_DL_COMMAND_LIST(XX)
#undef XX
	default:
		return nullptr;
	}
}

void DisplayList::reset()
{
	// debug_i("%p DisplayList::reset()", this);
	offset = size = 0;
	for(unsigned i = 0; i < lockCount; ++i) {
		lockedBuffers[i] = SharedBuffer{};
	}
	lockCount = 0;
}

bool DisplayList::lockBuffer(SharedBuffer& buffer)
{
	if(lockCount >= maxLockedBuffers) {
		debug_w("[DL] Lock list full");
		return false;
	}
	lockedBuffers[lockCount++] = buffer;
	return true;
}

uint8_t* DisplayList::getBuffer(uint16_t& available)
{
	constexpr size_t hdrsize{3};
	available = capacity - size;
	if(!require(hdrsize)) {
		return nullptr;
	}
	available -= hdrsize;
	return &buffer[size + hdrsize];
}

void DisplayList::commit(uint16_t length)
{
	constexpr size_t hdrsize{3};
	assert(require(hdrsize + length));
	writeHeader(getWriteCode(), 0x8000 | length);
	size += length;
}

bool DisplayList::writeCommand(uint8_t command, const void* data, uint16_t length)
{
	constexpr size_t hdrsize = codelen_command;
	if(!require(hdrsize + length)) {
		return false;
	}
	addrWindow.mode = AddressWindow::Mode::none;
	writeHeader(Code::command, length);
	write(command);
	write(data, length);
	return true;
}

bool DisplayList::writeData(const void* data, uint16_t length)
{
	constexpr size_t hdrsize = codelen_writeStart;
	if(!require(hdrsize + length)) {
		return false;
	}
	writeHeader(getWriteCode(), length);
	write(data, length);
	return true;
}

bool DisplayList::writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length)
{
	if(!canLockBuffer()) {
		return false;
	}
	constexpr size_t hdrsize = codelen_writeDataBuffer;
	if(!require(hdrsize)) {
		return false;
	}
	if(addrWindow.setMode(AddressWindow::Mode::write)) {
		writeHeader(Code::writeStart, 0);
	}
	writeHeader(Code::writeDataBuffer, length);
	auto dataptr = &data[offset];
	write(&dataptr, sizeof(dataptr));
	lockBuffer(data);
	return true;
}

void DisplayList::internalSetAddrWindow(const Rect& rect)
{
	writeHeader(Code::setColumn, rect.w - 1);
	writeVar(rect.x);
	writeHeader(Code::setRow, rect.h - 1);
	writeVar(rect.y);
	addrWindow = rect;
}

bool DisplayList::setAddrWindow(const Rect& rect)
{
	constexpr size_t hdrsize = codelen_setColumn + codelen_setRow;
	if(!require(hdrsize)) {
		return false;
	}
	internalSetAddrWindow(rect);
	return true;
}

bool DisplayList::setPixel(PackedColor color, uint8_t bytesPerPixel, Point pt)
{
	constexpr size_t hdrsize = codelen_setColumn + codelen_setRow;
	if(!require(hdrsize + bytesPerPixel)) {
		return false;
	}
	internalSetAddrWindow({pt, 1, 1});
	writeData(&color, bytesPerPixel);
	return true;
}

bool DisplayList::readMem(void* dataptr, uint16_t length)
{
	constexpr size_t hdrsize = codelen_read;
	if(!require(hdrsize)) {
		return false;
	}
	writeHeader(getReadCode(), length);
	write(&dataptr, sizeof(dataptr));
	return true;
}

bool DisplayList::writeCallback(Callback callback, void* params, uint16_t paramLength)
{
	constexpr size_t hdrsize = codelen_callback;
	if(!require(hdrsize + paramLength)) {
		return false;
	}
	writeHeader(Code::callback, paramLength);
	write(&callback, sizeof(callback));
	if(paramLength != 0) {
		size = ALIGNUP4(size);
		write(params, paramLength);
	}
	return true;
}

bool DisplayList::blockFill(const void* data, uint16_t length, uint32_t repeat)
{
	if(repeat < 2) {
		return writeData(data, length);
	}
	assert(data != nullptr && length != 0);
	// Ensure repeat count fits into 15 bits (packed)
	uint16_t blockLength;
	uint16_t blockCopies;
	if(repeat < 0x8000) {
		blockLength = length;
		blockCopies = 1;
	} else {
		blockCopies = (repeat + 0x7ffe) / 0x7fff;
		blockLength = blockCopies * length;
		repeat = ((length * repeat) + blockLength - 1) / blockLength;
	}
	constexpr size_t hdrsize = codelen_repeat;
	if(!require(hdrsize + blockLength)) {
		return false;
	}
	if(addrWindow.setMode(AddressWindow::Mode::write)) {
		writeHeader(Code::writeStart, 0);
	}
	writeHeader(Code::repeat, blockLength);
	writeVar(repeat);
	for(unsigned i = 0; i < blockCopies; ++i) {
		write(data, length);
	}
	return true;
}

bool DisplayList::fill(const Rect& rect, PackedColor color, uint8_t bytesPerPixel, FillInfo::Callback callback)
{
	FillInfo info;
	info.color = color;
	info.length = rect.w * rect.h * bytesPerPixel;

	constexpr size_t hdrsize =
		codelen_setColumn + codelen_setRow + codelen_readStart + codelen_callback + sizeof(info) + codelen_writeStart;
	if(!require(hdrsize + info.length)) {
		return false;
	}

	// 1. Setup address window
	internalSetAddrWindow(rect);
	// 2. Read screen into buffer located in (4)
	writeHeader(Code::readStart, info.length);
	auto readAddrOffset = size;
	size += sizeof(void*);
	// 3. Callback to blend colour into buffer (4)
	writeHeader(Code::callback, sizeof(info));
	write(&callback, sizeof(callback));
	size = ALIGNUP4(size);
	auto callbackParamOffset = size;
	size += sizeof(info);
	// 4. Write resulting data back to display
	writeHeader(Code::writeStart, info.length);
	info.dstptr = &buffer[size];
	size += info.length;
	// 5. Fixup address
	memcpy(&buffer[readAddrOffset], &info.dstptr, sizeof(info.dstptr));
	memcpy(&buffer[callbackParamOffset], &info, sizeof(info));

	return true;
}

bool DisplayList::readEntry(Entry& entry)
{
	assert(offset <= size);
	if(offset >= size) {
		// All done
		return false;
	}

	Header hdr{.u8 = buffer[offset++]};
	entry = Entry{hdr.code, hdr.len};
	if(hdr.len == Header::lenMax) {
		entry.length = readVar();
	}

	switch(hdr.code) {
	case Code::none:
		break;
	case Code::writeData:
		break;
	case Code::writeDataBuffer:
		read(&entry.data, sizeof(entry.data));
		return true;
	case Code::repeat:
		entry.repeats = readVar();
		break;
	case Code::callback:
		read(&entry.callback, sizeof(entry.callback));
		if(entry.length != 0) {
			offset = ALIGNUP4(offset);
			entry.data = &buffer[offset];
			offset += entry.length;
		}
		return true;
	case Code::command:
		entry.cmd = buffer[offset++];
		break;
	case Code::setColumn:
		entry.value = readVar();
		return true;
	case Code::setRow:
		entry.value = readVar();
		return true;
	case Code::writeStart:
		break;
	case Code::readStart:
	case Code::read:
		read(&entry.data, sizeof(entry.data));
		return true;
	case Code::delay:
		entry.value = buffer[offset++];
		break;
	default:
		return false;
	}

	if(entry.length != 0) {
		entry.data = &buffer[offset];
		offset += entry.length;
	}
	return true;
}

} // namespace Graphics
