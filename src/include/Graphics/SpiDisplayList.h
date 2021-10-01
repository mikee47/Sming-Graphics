/**
 * SpiDisplayList.h
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

#include "DisplayList.h"
#include <HSPI/Request.h>

namespace Graphics
{
/**
 * @brief Display list for hardware SPI devices
 *
 * A single HSPI request packet is used for all requests and is re-filled in interrupt context from this list.
 */
class SpiDisplayList : public DisplayList
{
public:
	/**
	 * @brief Commonly-used display-specific command codes
	 *
	 * Short codes are used to represent these commands.
	 * Other commands are stored directly in the list.
	 */
	struct Commands {
		uint8_t setColumn;
		uint8_t setRow;
		uint8_t readStart;
		uint8_t read;
		uint8_t writeStart;
	};

	template <typename... Params>
	SpiDisplayList(const Commands& commands, Params&&... params) : DisplayList(params...), commands(commands)
	{
	}

	bool isBusy() const
	{
		return request.busy;
	}

	/**
	 * @brief Called from interrupt context to re-fill the SPI request packet
	 * @retval bool true on success, false if there are no further requests
	 */
	bool fillRequest();

	void prepare(Callback callback, void* param)
	{
		DisplayList::prepare(callback, param);
		request.setAsync(staticRequestCallback, this);
		if(callback == nullptr) {
			request.async = false;
		}
	}

	/**
	 * @brief The HSPI request packet used by this list
	 */
	HSPI::Request request;

protected:
	static bool staticRequestCallback(HSPI::Request& request);

	const Commands& commands;
	uint16_t datalen{0};	  ///< Size of data at current position
	uint16_t repeats{0};	  ///< How many remaining repeats for this data block
	Code code{};			  ///< Command being executed
	uint8_t repeatBuffer[64]; ///< Buffer to fill out small repeated data chunks
};

} // namespace Graphics
