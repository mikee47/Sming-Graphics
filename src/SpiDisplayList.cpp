/**
 * SpiDisplayList.cpp
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

#include "include/Graphics/SpiDisplayList.h"
#include <HSPI/Controller.h>
#include <Platform/System.h>

#ifdef ARCH_HOSTXX
#define dbg(fmt, ...) host_printf("> " fmt "\r\n", ##__VA_ARGS__)
#define dbgint(fmt, ...) host_printf("# " fmt "\r\n", ##__VA_ARGS__)
#else
#define dbg debug_none
#define dbgint debug_none
#endif

namespace Graphics
{
__forceinline uint16_t swapBytes(uint16_t w)
{
	return (w >> 8) | (w << 8);
}

__forceinline uint32_t makeWord(uint16_t w1, uint16_t w2)
{
	return uint32_t(swapBytes(w1)) | (swapBytes(w2) << 16);
}

bool IRAM_ATTR SpiDisplayList::fillRequest()
{
	request.cmdLen = 0;
	switch(code) {
	case Code::setColumn: {
		auto value = readVar();
		request.out.set32(makeWord(value, value + datalen));
		dbgint("setColumn %u, %u", value, datalen);
		code = Code::none;
		return true;
	}
	case Code::setRow: {
		auto value = readVar();
		request.out.set32(makeWord(value, value + datalen));
		dbgint("setRow %u, %u", value, datalen);
		code = Code::none;
		return true;
	}
	case Code::repeat:
		request.out.length = datalen;
		--repeats;
		if(repeats == 0) {
			code = Code::none;
		}
		return true;
	case Code::none:
		break;
	default:
		if(datalen != 0) {
			dbgint("out %u", datalen);
			request.out.set(&buffer[offset], datalen);
			offset += datalen;
			code = Code::none;
			return true;
		}
	}

	if(offset >= size) {
		// All done
		return false;
	}

	Header hdr{.u8 = buffer[offset++]};
	code = hdr.code;
	datalen = hdr.len;
	if(datalen == Header::lenMax) {
		datalen = readVar();
	}
	request.dummyLen = 0;
	request.maxTransactionSize = HSPI::Controller::hardwareBufferSize;
	request.out.clear();
	request.in.clear();

	void* addr;
	uint8_t cmd;
	switch(hdr.code) {
	case Code::writeData:
		dbgint("data %u", datalen);
		return fillRequest();
	case Code::writeDataBuffer:
		read(&addr, sizeof(addr));
		request.out.set(addr, datalen);
		code = Code::none;
		return true;
	case Code::repeat: {
		repeats = readVar();
		dbgint("repeat %u, %u", datalen, repeats);
		auto dataPtr = &buffer[offset];
		offset += datalen;
		if(repeats > 1 && datalen <= (sizeof(repeatBuffer) / 2)) {
			uint8_t reps = sizeof(repeatBuffer) / datalen;
			if(reps >= repeats) {
				reps = repeats;
				repeats = 0;
			}
			uint16_t len{0};
			for(unsigned i = 0; i < reps; ++i, len += datalen) {
				memcpy(&repeatBuffer[len], dataPtr, datalen);
			}
			if(repeats == 0) {
				code = Code::none;
			} else {
				len = (repeats % reps) * datalen;
				repeats /= reps;
				datalen *= reps;
			}
			request.out.set(repeatBuffer, len);
			dbgint("+ %u, %u, %u", len, datalen, repeats);
		} else {
			request.out.set(dataPtr, datalen);
			--repeats;
		}
		return true;
	}
	case Code::callback: {
		Callback callback;
		read(&callback, sizeof(callback));
		void* params{nullptr};
		if(datalen != 0) {
			offset = ALIGNUP4(offset);
			params = &buffer[offset];
			offset += datalen;
		}
		code = Code::none;
		callback(params);
		return fillRequest();
	}
	case Code::command:
		cmd = buffer[offset++];
		break;
	case Code::setColumn:
		cmd = commands.setColumn;
		break;
	case Code::setRow:
		cmd = commands.setRow;
		break;
	case Code::writeStart:
		cmd = commands.writeStart;
		break;
	case Code::readStart:
	case Code::read:
		cmd = (hdr.code == Code::readStart) ? commands.readStart : commands.read;
		read(&addr, sizeof(addr));
		request.dummyLen = 8;
		request.maxTransactionSize = 63;
		request.in.set(addr, datalen);
		code = Code::none;
		dbgint("read %u", datalen);
		break;
	case Code::delay:
		// Ignore
		++offset;
		return fillRequest();
	default:
		return false;
	}

	dbgint("cmd 0x%02x", cmd);
	request.setCommand8(cmd);
	return true;
}

bool IRAM_ATTR SpiDisplayList::staticRequestCallback(HSPI::Request& request)
{
	auto list = static_cast<SpiDisplayList*>(request.param);
	if(list->fillRequest()) {
		return false;
	}
	// Display list complete
	if(list->callback != nullptr) {
		System.queueCallback(list->callback, list->param);
	}
	return true;
}

} // namespace Graphics
