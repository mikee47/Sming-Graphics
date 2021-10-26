/****
 * XPT2046.h
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

#include "../Touch.h"
#include <HSPI/Device.h>
#include <Platform/System.h>
#include <SimpleTimer.h>

namespace Graphics
{
class XPT2046 : private HSPI::Device, public Touch
{
public:
	static constexpr uint16_t sampleMax{0x0fff};

	XPT2046(HSPI::Controller& controller) : HSPI::Device(controller)
	{
	}

	XPT2046(HSPI::Controller& controller, Graphics::Device& device) : HSPI::Device(controller), Touch(&device)
	{
	}

	~XPT2046()
	{
		timer.stop();
	}

	bool begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t irqPin = PIN_NONE);

	void end()
	{
		timer.stop();
	}

	/* Touch */

	Size getNativeSize() const override
	{
		return Size{sampleMax, sampleMax};
	}

	State getState() const override
	{
		return State{Point(xraw, yraw), zraw};
	}

	void requestUpdate()
	{
		if(!updateRequested) {
			beginUpdate();
		}
	}

private:
	HSPI::IoModes getSupportedIoModes() const override
	{
		return HSPI::IoMode::SPI;
	}

	static void isr();
	static void staticOnChange(void* param)
	{
		static_cast<XPT2046*>(param)->beginUpdate();
	}
	void beginUpdate();

	static bool IRAM_ATTR staticRequestComplete(HSPI::Request& req)
	{
		System.queueCallback(staticUpdate, req.param);
		return true;
	}
	static void staticUpdate(void* param)
	{
		static_cast<XPT2046*>(param)->update();
	}
	void update();

	void printBuffer(const char* tag);

	SimpleTimer timer;
	HSPI::Request req;
	uint16_t buffer[10];
	uint8_t irqPin{PIN_NONE};
	bool updateRequested{false};
	uint16_t xraw{0};
	uint16_t yraw{0};
	uint16_t zraw{0};
	uint8_t offcount{0}; ///< Disable polling after N reads indicate no touch
};

} // namespace Graphics
