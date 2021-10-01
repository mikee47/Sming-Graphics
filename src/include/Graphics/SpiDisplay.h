/**
 * SpiDisplay.h
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

#include "Device.h"
#include <HSPI/Device.h>
#include "SpiDisplayList.h"
#include <Digital.h>

namespace Graphics
{
class SpiDisplay : protected HSPI::Device, public Device, public RenderTarget
{
public:
	SpiDisplay(HSPI::Controller& spi) : HSPI::Device(spi)
	{
	}

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t resetPin = PIN_NONE, uint32_t clockSpeed = 4000000);

	using HSPI::Device::getSpeed;

protected:
	using HSPI::Device::execute;

	void execute(SpiDisplayList& list, DisplayList::Callback callback = nullptr, void* param = nullptr)
	{
		list.prepare(callback, param);
		list.fillRequest();
		execute(list.request);
	}

	void execute(const SpiDisplayList::Commands& commands, const FSTR::ObjectBase& data);

	void reset(bool state)
	{
		if(resetPin != PIN_NONE) {
			digitalWrite(resetPin, !state);
		}
	}

	HSPI::IoModes getSupportedIoModes() const override
	{
		return HSPI::IoMode::SPI;
	}

	uint8_t resetPin{PIN_NONE};
	AddressWindow addrWindow{};
};

} // namespace Graphics
