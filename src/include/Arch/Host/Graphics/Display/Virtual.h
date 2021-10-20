/**
 * Virtual.h
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

#include <Graphics/Device.h>
#include <Graphics/AddressWindow.h>
#include <Graphics/AbstractDisplay.h>

class CSocket;

namespace Graphics
{
namespace Display
{
class VirtualSurface;

/**
 * @brief Virtual display device for Host
 * 
 * Talks to python virtual screen application via TCP
 */
class Virtual : public AbstractDisplay
{
public:
	enum class Mode {
		Normal, ///< Aim to produce similar performance to real display
		Debug,  ///< Use standard software renderers, may run slower and less smoothly
	};

	Virtual();
	~Virtual();

	bool begin(uint16_t width = 240, uint16_t height = 320);
	bool begin(const String& ipaddr, uint16_t port, uint16_t width = 240, uint16_t height = 320);

	void setMode(Mode mode)
	{
		this->mode = mode;
	}

	Mode getMode() const
	{
		return mode;
	}

	/* Device */

	String getName() const override
	{
		return F("Virtual Screen");
	}
	Size getNativeSize() const override
	{
		return nativeSize;
	}
	bool setOrientation(Orientation orientation) override;

	/* RenderTarget */

	Size getSize() const override
	{
		return rotate(nativeSize, orientation);
	}

	PixelFormat getPixelFormat() const override
	{
		return PixelFormat::BGR24;
	}

	Surface* createSurface(size_t bufferSize = 0) override;

private:
	class NetworkThread;
	friend NetworkThread;
	friend VirtualSurface;

	bool sizeChanged();

	std::unique_ptr<NetworkThread> thread;
	Size nativeSize{};
	AddressWindow addrWindow{};
	Mode mode;
};

} // namespace Display
} // namespace Graphics
