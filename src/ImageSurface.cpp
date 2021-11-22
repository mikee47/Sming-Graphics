/**
 * Surface.cpp
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

#include "include/Graphics/ImageSurface.h"
#include <Platform/System.h>

namespace Graphics
{
/* ImageSurface */

uint8_t* ImageSurface::getBuffer(uint16_t minBytes, uint16_t& available)
{
	available = buffer.size();
	return (available >= minBytes) ? buffer.get() : nullptr;
}

void ImageSurface::commit(uint16_t length)
{
	writePixels(buffer.get(), length);
}

bool ImageSurface::writePixels(const void* data, uint16_t length)
{
	addressWindow.setMode(AddressWindow::Mode::write);
	auto rowoffset = addressWindow.bounds.y * imageSize.w;
	auto src = static_cast<const uint8_t*>(data);
	uint16_t pixelCount = length / bytesPerPixel;
	while(pixelCount > 0) {
		auto count = std::min(pixelCount, uint16_t(addressWindow.bounds.w - addressWindow.column));
		write((rowoffset + addressWindow.left()) * bytesPerPixel, src, count * bytesPerPixel);
		rowoffset += imageSize.w;
		src += count * bytesPerPixel;
		pixelCount -= count;
		uint16_t seekres = addressWindow.seek(count);
		if(seekres != count) {
			break;
		}
	}
	return true;
}

bool ImageSurface::present(PresentCallback callback, void* param)
{
	if(callback) {
		System.queueCallback(callback, param);
	}
	return true;
}

bool ImageSurface::blockFill(const void* data, uint16_t length, uint32_t repeat)
{
	while(repeat-- != 0) {
		writePixels(data, length);
	}
	return true;
}

bool ImageSurface::setPixel(PackedColor color, Point pt)
{
	uint32_t offset = (pt.x + (pt.y * imageSize.w)) * bytesPerPixel;
	if(offset + bytesPerPixel >= imageBytes) {
		return true;
	}
	if(color.alpha < 255) {
		PackedColor cur;
		read(offset, &cur, bytesPerPixel);
		color = BlendAlpha::transform(pixelFormat, color, cur);
	}
	write(offset, &color, bytesPerPixel);
	return true;
}

int ImageSurface::readDataBuffer(ReadBuffer& buffer, ReadStatus* status, ReadCallback callback, void* param)
{
	addressWindow.setMode(AddressWindow::Mode::read);
	auto bpp = getBytesPerPixel(buffer.format);
	size_t bufPixels = std::min(buffer.size() / bpp, addressWindow.getPixelCount());
	auto bufptr = buffer.data.get();
	Location loc{imageSize};
	while(bufPixels != 0) {
		auto width = std::min(size_t(addressWindow.bounds.w - addressWindow.column), bufPixels);
		loc.source = addressWindow.bounds;
		loc.source.x += addressWindow.column;
		auto len = image.readPixels(loc, buffer.format, bufptr, width);
		bufptr += len;
		bufPixels -= width;
		addressWindow.seek(width);
	}

	size_t bytesRead = bufptr - buffer.data.get();
	if(status != nullptr) {
		status->bytesRead = bytesRead;
		status->readComplete = true;
	}
	if(callback) {
		System.queueCallback([&]() { callback(buffer, bytesRead, param); });
	}

	return bytesRead / bpp;
}

bool ImageSurface::fillRect(PackedColor color, const Rect& rect)
{
	Rect r = intersect(rect, imageSize);
	setAddrWindow(r);

	auto offset = (r.x + r.y * imageSize.w) * bytesPerPixel;
	size_t rowBytes = r.w * bytesPerPixel;
	size_t stride = imageSize.w * bytesPerPixel;
	for(unsigned y = 0; y < r.h; ++y, offset += stride) {
		uint8_t rowBuffer[rowBytes];
		if(color.alpha == 255) {
			writeColor(rowBuffer, color, pixelFormat, r.w);
		} else {
			read(offset, rowBuffer, rowBytes);
			BlendAlpha::blend(pixelFormat, color, rowBuffer, rowBytes);
		}
		write(offset, rowBuffer, rowBytes);
	}

	return true;
}

/* MemoryImageSurface */

void MemoryImageSurface::read(uint32_t offset, void* buffer, size_t length)
{
	assert(offset + length <= imageBytes);
	memcpy(buffer, &imageData[offset], length);
}

void MemoryImageSurface::write(uint32_t offset, const void* data, size_t length)
{
	if(offset > imageBytes) {
		return;
	}
	length = std::min(length, size_t(imageBytes - offset));

	if(blend == nullptr) {
		memcpy(&imageData[offset], data, length);
	} else {
		blend->transform(pixelFormat, static_cast<const uint8_t*>(data), &imageData[offset], length);
	}
}

/* FileImageSurface */

void FileImageSurface::read(uint32_t offset, void* buffer, size_t length)
{
	auto& img = reinterpret_cast<FileImageObject&>(image);
	img.seek(offset);
	img.streamPos += file.readBytes(static_cast<uint8_t*>(buffer), length);
}

void FileImageSurface::write(uint32_t offset, const void* data, size_t length)
{
	auto& img = reinterpret_cast<FileImageObject&>(image);
	img.seek(offset);
	img.streamPos += file.write(static_cast<const uint8_t*>(data), length);
}

} // namespace Graphics
