/**
 * Object.cpp
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

#include "include/Graphics/Object.h"
#include "include/Graphics/ImageSurface.h"
#include "include/Graphics/Drawing/Renderer.h"
#include <HSPI/Common.h>

String toString(Graphics::Object::Kind kind)
{
	switch(kind) {
#define XX(tag)                                                                                                        \
	case Graphics::Object::Kind::tag:                                                                                  \
		return F(#tag);
		GRAPHICS_OBJECT_KIND_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

namespace Graphics
{
/* Object */

String Object::getTypeStr() const
{
	return toString(kind());
}

/* ReferenceObject */

void ReferenceObject::adjustLocation(Location& loc) const
{
	loc.source += sourceOffset;
	auto& r = loc.dest;
	r += pos.topLeft();
	r.w -= pos.x;
	r.h -= pos.y;
	r.w = std::min(r.w, pos.w);
	r.h = std::min(r.h, pos.h);
}

/* PointObject */

Renderer* PointObject::createRenderer(const Location& location) const
{
	return new FilledRectRenderer(location, *this);
}

/* RectObject */

Renderer* RectObject::createRenderer(const Location& location) const
{
	if(radius == 0) {
		return new RectRenderer(location, *this);
	} else {
		return new RoundedRectRenderer(location, *this);
	}
}

/* FilledRectObject */

Renderer* FilledRectObject::createRenderer(const Location& location) const
{
	if(radius == 0) {
		return new FilledRectRenderer(location, *this);
	} else {
		return new FilledRoundedRectRenderer(location, *this);
	}
}

/* LineObject */

Renderer* LineObject::createRenderer(const Location& location) const
{
	return new LineRenderer(location, pen, pt1, pt2);
}

/* PolylineObject */

Renderer* PolylineObject::createRenderer(const Location& location) const
{
	return new PolylineRenderer(location, *this);
}

/* CircleObject */

Renderer* CircleObject::createRenderer(const Location& location) const
{
	if(pen.width <= 1 and !pen.isTransparent()) {
		return new CircleRenderer(location, *this);
	}
	return new EllipseRenderer(location, *this);
}

/* FilledCircleObject */

Renderer* FilledCircleObject::createRenderer(const Location& location) const
{
	if(brush.isTransparent()) {
		return new FilledEllipseRenderer(location, *this);
	} else {
		return new FilledCircleRenderer(location, *this);
	}
}

/* EllipseObject */

Renderer* EllipseObject::createRenderer(const Location& location) const
{
	return new EllipseRenderer(location, pen, rect);
}

/* FilledEllipseObject */

Renderer* FilledEllipseObject::createRenderer(const Location& location) const
{
	return new FilledEllipseRenderer(location, brush, rect);
}

/* ArcObject */

Renderer* ArcObject::createRenderer(const Location& location) const
{
	/* if angles differ by 360 degrees or more, close the shape */
	if((startAngle + 360 <= endAngle) || (startAngle - 360 >= endAngle)) {
		return new EllipseRenderer(location, pen, rect);
	}
	return new ArcRenderer(location, pen, rect, startAngle, endAngle);
}

/* FilledArcObject */

Renderer* FilledArcObject::createRenderer(const Location& location) const
{
	/* if angles differ by 360 degrees or more, close the shape */
	if((startAngle + 360 <= endAngle) || (startAngle - 360 >= endAngle)) {
		return new FilledEllipseRenderer(location, brush, rect);
	}
	return new FilledArcRenderer(location, brush, rect, startAngle, endAngle);
}

/* ImageObject */

Renderer* ImageObject::createRenderer(const Location& location) const
{
	return new ImageRenderer(location, *this);
}

/* TextObject */

Renderer* TextObject::createRenderer(const Location& location) const
{
	return new TextRenderer(location, *this);
}

/* SurfaceObject */

void SurfaceObject::write(MetaWriter& meta) const
{
	meta.write("surface", surface);
	meta.write("dest", dest);
	meta.write("source", source);
}

Renderer* SurfaceObject::createRenderer(const Location& location) const
{
	return new SurfaceRenderer(location, *this);
}

/* CopyObject */

Renderer* CopyObject::createRenderer(const Location& location) const
{
	return new CopyRenderer(location, *this);
}

/* ScrollObject */

Renderer* ScrollObject::createRenderer(const Location& location) const
{
	return new ScrollRenderer(location, *this);
}

/* SceneObject */

Renderer* SceneObject::createRenderer(const Location& location) const
{
	return new SceneRenderer(location, *this);
}

struct __attribute__((packed)) BmpFileHeader {
	uint16_t signature;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t imageOffset;
};

static_assert(sizeof(BmpFileHeader) == 14, "BmpFileHeader wrong size");

struct __attribute__((packed)) DibHeader {
	uint32_t size;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bitcount;
	uint32_t compress;
};

static_assert(sizeof(DibHeader) == 20, "DibHeader wrong size");

/*
 * BitmapObject
 *
 * Code based on https://github.com/adafruit/Adafruit-GFX-Library
 */

bool BitmapObject::init()
{
	debug_d("Loading image");

	seek(0);

	BmpFileHeader fileHeader;
	read(&fileHeader, sizeof(fileHeader));

	if(fileHeader.signature != 0x4D42) {
		debug_e("[BMP] Invalid signature");
		return false;
	}

	debug_d("[BMP] File size: %u", fileHeader.size);
	debug_d("[BMP] Image Offset: %u", fileHeader.imageOffset);

	imageOffset = fileHeader.imageOffset;

	DibHeader dib;
	read(&dib, sizeof(dib));
	debug_d("[BMP] Header size: %u", dib.size);

	// We have size so can render something even if following checks  fail
	debug_d("[BMP] Image size %d x %d", dib.width, dib.height);

	// If bmpHeight is negative, image is in top-down order.
	// This is not canon but has been observed in the wild.
	flip = dib.height >= 0; // BMP is stored bottom-to-top
	if(!flip) {
		dib.height = -dib.height;
	}
	imageSize = Size(dib.width, dib.height);

	// BMP rows are padded (if needed) to 4-byte boundary
	stride = ALIGNUP4(imageSize.w * 3);

	if(dib.planes != 1) { // # planes -- must be '1'
		debug_e("[BMP] Un-supported planes");
	}

	if(dib.bitcount != 24 || dib.compress != 0) {
		debug_e("[BMP] Un-supported depth %u", dib.bitcount);
	}

	return true;
}

size_t BitmapObject::readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const
{
	auto pos = loc.sourcePos();
	uint32_t offset = imageOffset;
	if(flip) {
		// Bitmap is stored bottom-to-top order (normal BMP)
		offset += (imageSize.h - 1 - pos.y) * stride;
	} else {
		// Bitmap is stored top-to-bottom
		offset += pos.y * stride;
	}
	offset += pos.x * 3;

	seek(offset);

	if(format == PixelFormat::BGR24) {
		auto len = width * 3;
		read(buffer, len);
		return len;
	}

	auto bytesPerPixel = getBytesPerPixel(format);
	auto bufptr = static_cast<uint8_t*>(buffer);
	constexpr size_t pixBufSize{32};
	PixelBuffer::RGB24 pixelBuffer[pixBufSize];
	for(unsigned x = 0; x < width; ++x) {
		// Time to read more pixel data?
		if(x % pixBufSize == 0) {
			read(pixelBuffer, sizeof(pixelBuffer));
		}

		// Convert pixel from BMP to TFT format
		PixelBuffer src;
		src.rgb24 = pixelBuffer[x % pixBufSize];
		writeColor(bufptr, src.color, format);
		bufptr += bytesPerPixel;
	}

	return width * bytesPerPixel;
}

/* PngImageObject */

const uint64_t pngSignature{0x0a1a0a0d474e5089ULL};
const uint32_t pngChunkIHDR{0x52444849};
const uint32_t pngChunkPLTE{0x45544c50};
const uint32_t pngChunktRNS{0x534e5274};
const uint32_t pngChunkIDAT{0x54414449};

struct PngChunk {
	uint32_t length;
	uint32_t type;
	uint8_t data[];
	// Checksum

	void bswap()
	{
		length = HSPI::bswap32(length);
	}
};

// type == IHDR
struct __attribute__((packed)) PngHeader {
	uint32_t width;
	uint32_t height;
	uint8_t bitDepth;
	uint8_t colorType;
	uint8_t compressionMethod;
	uint8_t filterMethod;
	uint8_t interlaceMethod;

	void bswap()
	{
		width = HSPI::bswap32(width);
		height = HSPI::bswap32(height);
	}
};

static_assert(sizeof(PngHeader) == 13);

struct __attribute__((packed)) PngFileHeader {
	uint64_t signature;
	uint32_t length;
	uint32_t type;
	PngHeader data;
	uint32_t crc;

	void bswap()
	{
		length = HSPI::bswap32(length);
		data.bswap();
	}
};

static_assert(sizeof(PngFileHeader) == 8 + 8 + 13 + 4);

bool PngImageObject::init()
{
	seek(0);

	PngFileHeader hdr;
	auto len = read(&hdr, sizeof(hdr));
	if(len != sizeof(hdr)) {
		debug_e("[PNG] Bad PNG, read %u bytes", len);
		return false;
	}
	hdr.bswap();
	if(hdr.signature != pngSignature) {
		debug_e("[PNG] Bad signature");
		return false;
	}
	if(hdr.type != pngChunkIHDR) {
		debug_e("[PNG] Expected IHDR");
		m_printHex("HDR", &hdr, len);
		return false;
	}
	imageSize.w = hdr.data.width;
	imageSize.h = hdr.data.height;
	mColorType = hdr.data.colorType;
	debug_i("[PNG] Color type %u", mColorType);

	// Look for palette, stop if IDAT found
	while(true) {
		PngChunk chunk{};
		if(read(&chunk, sizeof(chunk)) != sizeof(chunk)) {
			break;
		}
		chunk.bswap();
		if(chunk.type == pngChunkIDAT) {
			break;
		}
		if(chunk.type == pngChunkPLTE) {
			mPaletteSize = chunk.length;
			debug_i("[PNG] Found palette size %u", mPaletteSize);
		} else if(chunk.type == pngChunktRNS) {
			mHasTransparency = true;
		}
		stream->seekFrom(chunk.length + 4, SeekOrigin::Current);
	}

	seek(0);
	return true;
}

/* JpegImageObject */

bool JpegImageObject::init()
{
	seek(0);

	const uint16_t SOI = 0xffd8;
	const uint16_t SOF0 = 0xffc0;

	auto read16 = [&]() -> uint16_t {
		uint16_t mark{};
		read(&mark, sizeof(mark));
		return HSPI::bswap16(mark);
	};

	if(read16() != SOI) {
		debug_e("[JPEG] Bad SOI");
		return false;
	}

	struct Frame {
		uint16_t marker;
		uint16_t length;

		void bswap()
		{
			marker = HSPI::bswap16(marker);
			length = HSPI::bswap16(length);
		}
	};

	/*
		ff c0		SOF0
		00 11		Length (17 - 2) = 15 bytes
		08			sample precision
		03 84		lines (HEIGHT)
		04 fe		samples per line (WIDTH)
		03 			image components
		01 22 00	C1=1, H1=2, V1=2, Tq1=0
		02 11 01	C2=2, H2=1, V2=1, Tq2=1
		03 11 01	C3=3, H3=1, V3=1, Tq3=1
	*/
	struct __attribute__((packed)) BaselineDct {
		uint8_t samplePrecision;
		uint16_t numberOfLines;
		uint16_t samplesPerLine;
		uint8_t numberOfImageComponents;
		uint8_t imageComponents[];

		void bswap()
		{
			numberOfLines = HSPI::bswap16(numberOfLines);
			samplesPerLine = HSPI::bswap16(samplesPerLine);
		}
	};

	auto readFrame = [&]() -> Frame {
		Frame frame;
		read(&frame, sizeof(frame));
		frame.bswap();
		return frame;
	};

	// Look for SOF0 marker
	while(true) {
		auto frame = readFrame();
		if(frame.marker == 0) {
			debug_w("[JPEG] SOF0 not found");
			return false;
		}
		if(frame.marker != SOF0) {
			stream->seekFrom(frame.length - 2, SeekOrigin::Current);
			continue;
		}

		BaselineDct dct;
		read(&dct, sizeof(dct));
		dct.bswap();
		imageSize.w = dct.samplesPerLine;
		imageSize.h = dct.numberOfLines;
		debug_w("[JPEG] SOF0 found, %u x %u", dct.samplesPerLine, dct.numberOfLines);
		break;
	}

	seek(0);
	return true;
}

/* RawImageObject */

size_t RawImageObject::readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const
{
	auto pos = loc.sourcePos();
	auto bpp = getBytesPerPixel(pixelFormat);
	uint32_t offset = ((pos.y * imageSize.w) + pos.x) * bpp;
	seek(offset);
	if(format == pixelFormat) {
		size_t count = width * bpp;
		read(buffer, count);
		return count;
	}

	// Fall back to format conversion
	auto dstptr = static_cast<uint8_t*>(buffer);
	while(width != 0) {
		constexpr uint16_t bufPixels{32};
		uint8_t buf[bufPixels * bpp];
		auto numPixels = std::min(width, bufPixels);
		read(&buf, numPixels * bpp);
		dstptr += convert(buf, pixelFormat, dstptr, format, numPixels);
		width -= numPixels;
	}
	return dstptr - static_cast<uint8_t*>(buffer);
}

/* MemoryImageObject */

MemoryImageObject::MemoryImageObject(PixelFormat format, Size size)
	: RawImageObject(nullptr, format, size), imageBytes(size.w * size.h * getBytesPerPixel(format))
{
	constexpr size_t minFreeHeap{8192};
	auto heapFree = system_get_free_heap_size();
	if(heapFree < minFreeHeap + imageBytes) {
		debug_w("[IMG] Not enough memory for %s image", size.toString().c_str());
		return;
	}

	imageData = new uint8_t[imageBytes];
	stream = std::make_unique<LimitedMemoryStream>(imageData, imageBytes, imageBytes, true);
	memset(imageData, 0, imageBytes);
	debug_i("[IMG] %p, %s created, heap %u -> %u", imageData, size.toString().c_str(), heapFree,
			system_get_free_heap_size());
}

Surface* MemoryImageObject::createSurface(const Blend* blend, size_t bufferSize)
{
	return new MemoryImageSurface(*this, pixelFormat, blend, bufferSize ?: 512U, imageData);
}

/* FileImageObject */

Surface* FileImageObject::createSurface(size_t bufferSize)
{
	return new FileImageSurface(*this, pixelFormat, bufferSize ?: 512U, *static_cast<IFS::FileStream*>(stream.get()));
}

/* GlyphObject */

size_t GlyphObject::readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const
{
	options.fore.setPixelFormat(format);
	options.back.setPixelFormat(format);

	auto empty = [&]() { return options.back.writePixels(loc, buffer, width); };

	auto s = options.scale.scale();
	bool isDotted = options.style[FontStyle::DotMatrix] && (s.w > 1);
	bool isHLine = options.style[FontStyle::HLine] && (s.h > 1);
	bool isVLine = options.style[FontStyle::VLine] && (s.w > 1);
	if((isDotted || isHLine) && (loc.pos.y % s.h) != 0) {
		return empty();
	}

	auto bits = getBits(options.scale.unscaleY(loc.pos.y));
	if(bits == 0) {
		return empty();
	}

	auto bufptr = static_cast<uint8_t*>(buffer);
	unsigned end = options.scale.unscaleX(loc.pos.x + width);
	Location l = loc;
	for(unsigned col = options.scale.unscaleX(loc.pos.x); col < end; ++col, l.pos.x += s.w) {
		auto& brush = bits[col] ? options.fore : options.back;
		if(!isDotted && !isVLine) {
			bufptr += brush.writePixels(l, bufptr, s.w);
			continue;
		}
		if(bits[col]) {
			bufptr += brush.writePixel(l, bufptr);
			++l.pos.x;
			bufptr += options.back.writePixels(l, bufptr, s.w - 1);
			--l.pos.x;
			continue;
		}
		bufptr += options.back.writePixels(l, bufptr, s.w);
	}

	return bufptr - static_cast<uint8_t*>(buffer);
}

/* DrawingObject */

Renderer* DrawingObject::createRenderer(const Location& location) const
{
	return new Drawing::Renderer(location, *this);
}

void DrawingObject::write(MetaWriter& meta) const
{
	stream->seekFrom(0, SeekOrigin::Start);
	meta.write("size", stream->available());

	meta.beginArray("content", "Object");
	Drawing::Reader reader(*this);
	Object* obj;
	while((obj = reader.readObject()) != nullptr) {
		meta.write(*obj);
		delete obj;
	}
	meta.endArray();
}

} // namespace Graphics
