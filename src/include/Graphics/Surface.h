/****
 * Surface.h
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

#include "Object.h"
#include "Buffer.h"

namespace Graphics
{
class Device;

/**
 * @brief Interface for a drawing surface
 * 
 * Represents a rectangular area of pixels which can be read or written.
 * 
 * A display device has at least one of these, representing the primary display area.
 * More complex devices with large amounts of display memory may allow additional surfaces
 * to be used to perform screen updates by 'flipping' (switching active surface) or fast
 * copies using display hardware.
 */
class Surface : public AssetTemplate<AssetType::Surface>
{
public:
	using List = LinkedObjectListTemplate<Surface>;
	using OwnedList = OwnedLinkedObjectListTemplate<Surface>;

	// Assume that reading requires space for full 24-bit RGB (e.g. ILI9341)
	static constexpr size_t READ_PIXEL_SIZE{3};

	enum class Type {
		Memory,
		File,
		Device,
		Drawing,
		Blend,
	};

	struct Stat {
		size_t used;
		size_t available;
	};

	using PresentCallback = void (*)(void* param);

	/**
	 * @brief Callback for readPixel() operations
	 * @param buffer Buffer passed to readPixel() call
	 */
	using ReadCallback = void (*)(ReadBuffer& data, size_t length, void* param);

	/* Meta */

	void write(MetaWriter& meta) const override
	{
	}

	/* Surface */

	virtual Type getType() const = 0;
	virtual Stat stat() const = 0;
	virtual Size getSize() const = 0;
	virtual PixelFormat getPixelFormat() const = 0;
	virtual bool setAddrWindow(const Rect& rect) = 0;
	virtual uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) = 0;
	virtual void commit(uint16_t length) = 0;
	virtual bool blockFill(const void* data, uint16_t length, uint32_t repeat) = 0;
	virtual bool writeDataBuffer(SharedBuffer& buffer, size_t offset, uint16_t length) = 0;
	virtual bool setPixel(PackedColor color, Point pt) = 0;
	virtual bool writePixels(const void* data, uint16_t length);

	bool writePixel(PackedColor color)
	{
		return writePixels(&color, getBytesPerPixel(getPixelFormat()));
	}

	bool writePixel(Color color)
	{
		return writePixel(pack(color, getPixelFormat()));
	}

	/**
	 * @brief Set margins for hardware scrolling
	 * @param top Number of fixed pixels at top of screen
	 * @param bottom Number of fixed pixels at bottom of screen
	 */
	virtual bool setScrollMargins(uint16_t top, uint16_t bottom) = 0;

	/**
	 * @brief Set hardware scrolling offset
	 * @param line Offset of first line to display within scrolling region
	 *
	 * Caller must manage rendering when using hardware scrolling to avoid wrapping
	 * into unintended regions. See :cpp:class:`Graphics::Console`.
	 */
	virtual bool setScrollOffset(uint16_t line) = 0;

	/**
	 * @brief Read some pixels
	 * @param buffer Details requested format and buffer to read
	 * @param status Optional. Stores result of read operation.
	 * @param callback Optional. Invoked when read has completed
	 * @param param Parameters passed to callback
	 * @retval int Number of pixels queued for reading (or read); 0 if no further pixels to read, < 0 to try again later
	 *
	 * Call `setAddrWindow` to set up region to be read.
	 * Returns true when all pixels have been queued for reading.
	 */
	virtual int readDataBuffer(ReadBuffer& buffer, ReadStatus* status = nullptr, ReadCallback callback = nullptr,
							   void* param = nullptr) = 0;

	virtual int readDataBuffer(ReadStatusBuffer& buffer, ReadCallback callback = nullptr, void* param = nullptr)
	{
		return readDataBuffer(buffer, &buffer.status, callback, param);
	}

	/**
	 * @brief Start rendering an object
	 * @param object What to render
	 * @param location Placement information
	 * @param renderer If operation cannot be completed in hardware, create a renderer instance to manage the process
	 * @retval Return true on success, false to retry later
	 *
	 * Surfaces may override this method to implement alternative rendering using specific
	 * hardware features of the display device.
	 *
	 * Software renderers should be run by calling `surface::execute`.
	 */
	virtual bool render(const Object& object, const Rect& location, std::unique_ptr<Renderer>& renderer);

	/**
	 * @brief Render an object in one cycle
	 * @param surface Where to render the object to
	 * @param location
	 * @retval bool true on success
	 *
	 * Use this method for simple renders which should complete in one cycle.
	 * Typically used for memory surface renders or where an object is expected
	 * to render within a single surface.
	 */
	bool render(const Object& object, const Rect& location);

	/**
	 * @brief Execute a renderer
	 * @param renderer Will be released when render has completed
	 * @retval bool true if render is complete
	 */
	bool execute(std::unique_ptr<Renderer>& renderer)
	{
		if(renderer) {
			if(!renderer->execute(*this)) {
				return false;
			}
			renderer.reset();
		}
		return true;
	}

	/**
	 * @brief Reset surface ready for more commands
	 */
	virtual void reset() = 0;

	/**
	 * @brief Present surface to display device
	 * @param callback Invoked when surface is available for more commands
	 * @param param Passed to callback
	 * @retval bool true If callback has been queued, false if surface is empty
	 *
	 * Hardware devices will queue buffered commands to the display device then return.
	 * The surface will be marked as BUSY.
	 * Attempting to call present() on a BUSY surface must return true.
	 * If surface is EMPTY (no buffered commands), must return false.
	 */
	virtual bool present(PresentCallback callback = nullptr, void* param = nullptr) = 0;

	uint16_t width() const
	{
		return getSize().w;
	}

	uint16_t height() const
	{
		return getSize().h;
	}

	bool blockFill(PackedColor color, uint32_t repeat)
	{
		return blockFill(&color, getBytesPerPixel(getPixelFormat()), repeat);
	}

	bool clear()
	{
		return fillRect(pack(Color::Black, getPixelFormat()), getSize());
	}

	virtual bool fillRect(PackedColor color, const Rect& rect);

	/**
	 * @brief Fill a small rectangle using a non-transparent brush
	 */
	bool fillSmallRect(const Brush& brush, const Rect& location, const Rect& rect);

	/**
	 * @brief Draw a simple horizontal line using a filled rectangle
	 */
	bool drawHLine(PackedColor color, uint16_t x0, uint16_t x1, uint16_t y, uint16_t w);

	/**
	 * @brief Draw a simple vertical line using a filled rectangle
	 */
	bool drawVLine(PackedColor color, uint16_t x, uint16_t y0, uint16_t y1, uint16_t w);
};

} // namespace Graphics
