/**
 * ILI9341.cpp
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
 * This code contains some reworked code from the Adafruit_ILI9341 display driver.
 * 
 * See https://github.com/adafruit/Adafruit_ILI9341
 *
 ****/

#include "include/Graphics/SpiDisplay.h"
#include <SimpleTimer.h>

namespace Graphics
{
bool SpiDisplay::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t resetPin, uint32_t clockSpeed)
{
	if(!HSPI::Device::begin(pinSet, chipSelect, clockSpeed)) {
		return false;
	}
	setBitOrder(MSBFIRST);
	setClockMode(HSPI::ClockMode::mode0);
	setIoMode(HSPI::IoMode::SPIHD);

	this->resetPin = resetPin;
	if(resetPin != PIN_NONE) {
		pinMode(resetPin, OUTPUT);
		reset(false);
		reset(true);
		os_delay_us(10000);
		reset(false);
		os_delay_us(1000);
	}

	return true;
}

class SpiCommandHandler
{
public:
	SpiCommandHandler(SpiDisplay& display, const SpiDisplayList::Commands& commands, const FSTR::ObjectBase& data,
					  SpiDisplay::ExecuteDone callback)
		: display(display), src(commands, addrWindow, data), commands(commands), callback(callback)
	{
		timer.setCallback(staticRun, this);
	}

	void run()
	{
		if(delay != 0) {
			debug_d("[SCH] delay(%u)", delay);
			timer.setInterval(delay);
			timer.startOnce();
			delay = 0;
			return;
		}

		DisplayList::Entry entry;
		while(src.readEntry(entry)) {
			if(entry.code == DisplayList::Code::delay) {
				delay = entry.value;
				if(!sendList()) {
					if(delay != 0) {
						run();
					}
				}
				current = src.readOffset();
				return;
			}
			current = src.readOffset();
		}
		if(sendList()) {
			return;
		}
		debug_d("[SCH] Done");
		if(callback) {
			callback();
		}
		delete this;
	}

private:
	static void staticRun(void* param)
	{
		auto self = static_cast<SpiCommandHandler*>(param);
		self->run();
	}

	bool sendList()
	{
		auto len = current - start;
		debug_d("[SCH] sendList(%u)", len);

		if(len == 0) {
			return false;
		}

		list.reset(new SpiDisplayList(commands, addrWindow, src.getContent() + start, len));
		current = start = src.readOffset();
		display.execute(*list, staticRun, this);
		return true;
	}

	AddressWindow addrWindow{};
	SpiDisplay& display;
	SpiDisplayList src;
	std::unique_ptr<SpiDisplayList> list;
	const SpiDisplayList::Commands& commands;
	SpiDisplay::ExecuteDone callback;
	uint32_t start{0};
	uint32_t current{0};
	unsigned delay{0};
	SimpleTimer timer;
};

void SpiDisplay::execute(const SpiDisplayList::Commands& commands, const FSTR::ObjectBase& data, ExecuteDone callback)
{
	auto handler = new SpiCommandHandler(*this, commands, data, callback);
	handler->run();
}

} // namespace Graphics
