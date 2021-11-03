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

#include "Surface.h"
#include "AddressWindow.h"

namespace Graphics
{
/**
 * @brief Virtual class to access an image as a Surface
 * 
 * Use to create off-screen bitmaps by drawing or copying areas from display memory.
 */
class ImageSurface : public Surface
{
public:
	ImageSurface(ImageObject& image, PixelFormat format, size_t bufferSize)
		: image(image), imageSize(image.getSize()), buffer(bufferSize), pixelFormat(format),
		  bytesPerPixel(getBytesPerPixel(pixelFormat))
	{
		auto size = image.getSize();
		imageBytes = size.w * size.h * bytesPerPixel;
	}

	Stat stat() const override
	{
		return Stat{
			.used = 0,
			.available = buffer.size(),
		};
	}

	Size getSize() const override
	{
		return image.getSize();
	}

	PixelFormat getPixelFormat() const override
	{
		return pixelFormat;
	}

	bool setAddrWindow(const Rect& rect) override
	{
		addressWindow = rect;
		return true;
	}

	uint8_t* getBuffer(uint16_t minBytes, uint16_t& available) override;
	void commit(uint16_t length) override;
	bool blockFill(const void* data, uint16_t length, uint32_t repeat) override;
	bool writeDataBuffer(SharedBuffer& data, size_t offset, uint16_t length) override
	{
		return writePixels(&data[offset], length);
	}
	bool setPixel(PackedColor color, Point pt) override;
	bool writePixels(const void* data, uint16_t length) override;

	bool setScrollMargins(uint16_t top, uint16_t bottom) override
	{
		(void)top;
		(void)bottom;
		return false;
	}

	bool setScrollOffset(uint16_t line) override
	{
		(void)line;
		return false;
	}

	int readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param) override;
	void reset() override
	{
	}
	bool present(PresentCallback callback, void* param) override;

	bool fillRect(PackedColor color, const Rect& rect) override;

	using Surface::write;

protected:
	virtual void read(uint32_t offset, void* buffer, size_t length) = 0;
	virtual void write(uint32_t offset, const void* data, size_t length) = 0;

	ImageObject& image;
	AddressWindow addressWindow{};
	Size imageSize;
	size_t imageBytes;
	SharedBuffer buffer;
	PixelFormat pixelFormat;
	uint8_t bytesPerPixel;
};

/**
 * @brief Image surface using RAM as backing store
 * 
 * Useful for sprites, etc.
 */
class MemoryImageSurface : public ImageSurface
{
public:
	MemoryImageSurface(MemoryImageObject& image, PixelFormat format, const Blend* blend, size_t bufferSize,
					   uint8_t* imageData)
		: ImageSurface(image, format, bufferSize), imageData(imageData), blend(blend)
	{
	}

	Type getType() const override
	{
		return Type::Memory;
	}

protected:
	void read(uint32_t offset, void* buffer, size_t length) override;
	void write(uint32_t offset, const void* data, size_t length) override;

	uint8_t* imageData;
	const Blend* blend;
};

/**
 * @brief Image surface using filing system as backing store
 * 
 * Slower than RAM but size is unrestricted. Use for constructing complex scenes for faster drawing.
 */
class FileImageSurface : public ImageSurface
{
public:
	FileImageSurface(FileImageObject& image, PixelFormat format, size_t bufferSize, IFS::FileStream& file)
		: ImageSurface(image, format, bufferSize), file(file)
	{
	}

	Type getType() const
	{
		return Type::File;
	}

protected:
	void read(uint32_t offset, void* buffer, size_t length) override;
	void write(uint32_t offset, const void* data, size_t length) override;

private:
	IFS::FileStream& file;
};

} // namespace Graphics
