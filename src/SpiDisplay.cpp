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

namespace Graphics
{
bool SpiDisplay::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t resetPin, uint32_t clockSpeed)
{
	if(!HSPI::Device::begin(pinSet, chipSelect, clockSpeed)) {
		return false;
	}
	setBitOrder(MSBFIRST);
	setClockMode(HSPI::ClockMode::mode0);
	setIoMode(HSPI::IoMode::SPI);

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

void SpiDisplay::execute(const SpiDisplayList::Commands& commands, const FSTR::ObjectBase& data)
{
	SpiDisplayList src(commands, addrWindow, data);

	uint32_t start{0};
	uint32_t current{0};

	auto sendList = [&]() {
		auto len = current - start;
		if(len != 0) {
			SpiDisplayList list(commands, addrWindow, src.getContent() + start, len);
			execute(list);
		}
		current = start = src.readOffset();
	};

	DisplayList::Entry entry;
	while(src.readEntry(entry)) {
		if(entry.code == DisplayList::Code::delay) {
			sendList();
			os_delay_us(entry.value * 1000);
			continue;
		}
		current = src.readOffset();
	}
	sendList();
}

} // namespace Graphics
