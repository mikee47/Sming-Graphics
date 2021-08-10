/**
 * Asset.cpp
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

#include "include/Graphics/Asset.h"
#include "include/Graphics/Surface.h"
#include "include/Graphics/Stream.h"

String toString(Graphics::Brush::Kind kind)
{
	switch(kind) {
#define XX(name)                                                                                                       \
	case Graphics::Brush::Kind::name:                                                                                  \
		return F(#name);
		GRAPHICS_BRUSH_KIND_MAP(XX)
#undef XX
	default:
		return nullptr;
	}
}

String toString(Graphics::AssetType type)
{
	switch(type) {
#define XX(tag)                                                                                                        \
	case Graphics::AssetType::tag:                                                                                     \
		return F(#tag);
		GRAPHICS_ASSET_TYPE_LIST(XX)
#undef XX
	default:
		return nullptr;
	};
}

namespace Graphics
{
Asset::ID Asset::nextId;

namespace
{
IDataSourceStream* userResourceStream;
ReadStream* resourceStream;
} // namespace

namespace Resource
{
void init(IDataSourceStream* stream)
{
	delete resourceStream;
	delete userResourceStream;
	userResourceStream = stream;
	if(stream == nullptr) {
		resourceStream = nullptr;
	} else {
		resourceStream = new ReadStream(*userResourceStream);
	}
}

IDataSourceStream* createSubStream(uint32_t offset, size_t size)
{
	return new SubStream(*userResourceStream, offset, size);
}
} // namespace Resource

/* Asset */

String Asset::getTypeStr() const
{
	return toString(type());
}

void Asset::write(MetaWriter& meta) const
{
	meta.write("id", mId);
}

/* ObjectAsset */

ObjectAsset::ObjectAsset(const Object* object) : AssetTemplate(), object(object)
{
}

ObjectAsset::ObjectAsset(AssetID id, const Object* object) : AssetTemplate(id), object(object)
{
}

ObjectAsset::~ObjectAsset()
{
}

/* GradientBrush */

size_t GradientBrush::readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t pixelCount) const
{
	auto bufptr = static_cast<uint8_t*>(buffer);
	PixelBuffer c1{color1};
	PixelBuffer c2{color2};

	/*
	 * Linear gradient fills loc.dest area with color1 at top and color2 at bottom
	 * loc.source not currently used.
	 */
	Point pos = loc.pos;
	while(pixelCount != 0) {
		PixelBuffer c;

		for(unsigned i = 0; i < 3; ++i) {
			c.u8[i] = c1.u8[i] + int(pos.y) * (c2.u8[i] - c1.u8[i]) / loc.source.h;
		}

		auto count = std::min(pixelCount, uint16_t(loc.dest.w - pos.x));
		bufptr += writeColor(bufptr, c.color, format, count);
		pixelCount -= count;
		pos.x = 0;
		++pos.y;
	}
	return bufptr - static_cast<uint8_t*>(buffer);
}

/* ImageBrush */

PixelFormat ImageBrush::getPixelFormat() const
{
	return image.getPixelFormat();
}

void ImageBrush::write(MetaWriter& meta) const
{
	TextureBrush::write(meta);
	meta.write("image", image);
}

size_t ImageBrush::readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t pixelCount) const
{
	auto bufptr = static_cast<uint8_t*>(buffer);

	auto imgsize = image.getSize();
	auto pos = (style == BrushStyle::SourceLocal) ? loc.sourcePos() : loc.destPos();
	pos.x %= imgsize.w;
	pos.y %= imgsize.h;
	Location l{{}, loc.source.size(), pos};
	while(pixelCount != 0) {
		auto count = std::min(pixelCount, uint16_t(imgsize.w - l.pos.x));
		bufptr += image.readPixels(l, format, bufptr, count);
		pixelCount -= count;
		l.pos.x = 0;
		++l.pos.y;
		if(l.pos.y == imgsize.h) {
			l.pos.y = 0;
		}
	}
	return bufptr - static_cast<uint8_t*>(buffer);
}

/* AssetList */

Asset* AssetList::find(AssetType type, AssetID id)
{
	return std::find_if(begin(), end(), [&](Asset& asset) {
		if(asset.id() != id) {
			return false;
		}
		if(asset.type() != type) {
			debug_e("[GRAPHICS] Asset #%u wrong type, expected %s got %s", id, toString(type).c_str(),
					toString(asset.type()).c_str());
			return false;
		}
		return true;
	});
}

void AssetList::store(Asset* asset)
{
	if(asset == nullptr) {
		return;
	}
	auto existing = find(asset->id());
	if(existing != nullptr) {
		if(asset->type() != existing->type()) {
			debug_e("[GRAPHICS] Asset #%u exists and type differs, expected %s got %s", toString(asset->type()).c_str(),
					toString(existing->type()).c_str());
		}
		remove(existing);
	}
	add(asset);
}

/* Brush */

String Brush::getTypeStr() const
{
	return toString(kind);
}

void Brush::write(MetaWriter& meta) const
{
	switch(kind) {
	case Kind::Color:
		meta.write("color", color);
		break;
	case Kind::PackedColor:
		meta.write("packedColor", packedColor);
		meta.write("pixelFormat", pixelFormat);
		break;
	case Kind::Texture:
		meta.write("brush", *brush);
		break;
	case Kind::None:;
	}
}

PackedColor Brush::getPackedColor(PixelFormat format) const
{
	switch(kind) {
	case Kind::Color:
		return pack(color, format);
	case Kind::PackedColor:
		return packedColor;
	case Kind::Texture:
		// TODO
		return pack(Color::YELLOW, format);
	default:
		return pack(Color::RED, format);
	}
}

PackedColor Brush::getPackedColor(Point pt) const
{
	return getPackedColor(pixelFormat);
}

bool Brush::setPixel(Surface& surface, const Location& loc) const
{
	if(isSolid()) {
		return surface.setPixel(getPackedColor(), loc.destPos());
	}
	PackedColor color;
	writePixel(loc, &color);
	return surface.setPixel(color, loc.destPos());
}

uint16_t Brush::setPixels(Surface& surface, const Location& loc, uint16_t pixelCount) const
{
	if(!surface.setAddrWindow(loc.dest + loc.pos)) {
		return 0;
	}

	if(isSolid()) {
		return surface.blockFill(getPackedColor(), pixelCount) ? pixelCount : 0;
	}

	// Draw at least this number of pixels from a larger set
	uint16_t minPixels{8};

	auto bytesPerPixel = getBytesPerPixel(pixelFormat);
	auto required = std::min(pixelCount, minPixels);
	uint16_t available;
	auto buffer = surface.getBuffer(required * bytesPerPixel, available);
	if(buffer == nullptr) {
		return false;
	}

	pixelCount = std::min(pixelCount, uint16_t(available / bytesPerPixel));
	auto len = brush->readPixels(loc, pixelFormat, buffer, pixelCount);
	surface.commit(len);
	return pixelCount;
}

uint16_t Brush::writePixel(const Location& loc, void* buffer) const
{
	return writePixels(loc, buffer, 1);
}

uint16_t Brush::writePixels(const Location& loc, void* buffer, uint16_t pixelCount) const
{
	if(isSolid()) {
		return writeColor(buffer, getPackedColor(), pixelFormat, pixelCount);
	}
	if(brush) {
		return brush->readPixels(loc, pixelFormat, buffer, pixelCount);
	}
	return pixelCount * getBytesPerPixel(pixelFormat);
}

/* TypeFace */

uint16_t TypeFace::getTextWidth(const char* text, uint16_t length) const
{
	uint16_t x{0};
	uint8_t width{0};
	uint8_t advance{0};

	while(length-- != 0) {
		auto m = getMetrics(*text++);
		width = m.width + m.xOffset;
		x += advance;
		advance = m.advance;
	}

	return x + std::max(advance, width);
}

/* ResourceGlyph */

class ResourceGlyph : public GlyphObject
{
public:
	ResourceGlyph(const Resource::FontResource& font, const Resource::TypefaceResource& typeface,
				  const Resource::GlyphResource& glyph, const Options& options)
		: GlyphObject(glyph.getMetrics(), options), fontDescent(FSTR::readValue(&font.descent)), typeface(typeface),
		  glyph(glyph)
	{
	}

	bool init() override
	{
		return true;
	}

	Bits getBits(uint16_t row) const override
	{
		Rect bm(abs(glyph.xOffset), typeface.yAdvance + glyph.yOffset - typeface.descent, glyph.width, glyph.height);
		if(row < bm.top() || row > bm.bottom()) {
			return 0;
		}

		Bits bits;

		uint32_t offset = typeface.bmOffset + glyph.bmOffset;
		if(glyph.flags[Resource::GlyphResource::Flag::alpha]) {
			offset += (row - bm.y) * glyph.width;
			for(int x = bm.left(); x <= bm.right(); ++x, ++offset) {
				uint8_t c = resourceStream->read(offset++);
				bits[x] = (c > 0) ? 1 : 0;
			}
		} else {
			unsigned off = (row - bm.y) * glyph.width;
			offset += off / 8;
			uint8_t raw = resourceStream->read(offset++);
			uint8_t mask = 0x80 >> (off % 8);

			for(int x = bm.left(); x <= bm.right(); ++x) {
				if(mask == 0) {
					raw = resourceStream->read(offset++);
					mask = 0x80;
				}
				bits[x] = raw & mask;
				mask >>= 1;
			}
		}

		return bits;
	}

	void readAlpha(void* buffer, Point origin, size_t stride) const override
	{
		unsigned offset = typeface.bmOffset + glyph.bmOffset;
		assert(origin.x + glyph.xOffset >= 0);
		uint16_t off = origin.x + glyph.xOffset;
		auto y = origin.y + typeface.yAdvance + glyph.yOffset - fontDescent - 1;
		assert(y + glyph.height <= typeface.yAdvance);
		off += y * stride;
		auto bufptr = static_cast<uint8_t*>(buffer) + off;

		if(glyph.flags[Resource::GlyphResource::Flag::alpha]) {
			for(unsigned y = 0; y < glyph.height; ++y, offset += glyph.width, bufptr += stride) {
				resourceStream->read(offset, bufptr, glyph.width);
			}
		} else {
			uint8_t raw{0};
			uint8_t mask{0};
			for(unsigned y = 0; y < glyph.height; ++y) {
				auto rowptr = bufptr;
				for(unsigned x = 0; x < glyph.width; ++x) {
					if(mask == 0) {
						raw = resourceStream->read(offset++);
						mask = 0x80;
					}
					if(raw & mask) {
						bufptr[x] = 0xff;
					}
					mask >>= 1;
				}
				rowptr += stride;
				bufptr = rowptr;
			}
		}
	}

private:
	uint8_t fontDescent;
	const Resource::TypefaceResource typeface;
	const Resource::GlyphResource glyph;
};

/* ResourceTypeface */

bool ResourceTypeface::findGlyph(uint16_t codePoint, Resource::GlyphResource& glyph) const
{
	auto glyphPtr = typeface.glyphs;
	auto numBlocks = FSTR::readValue(&typeface.numBlocks);
	for(unsigned i = 0; i < numBlocks; ++i) {
		auto block = FSTR::readValue(&typeface.blocks[i]);
		if(codePoint > block.last()) {
			glyphPtr += block.length;
			continue;
		}
		if(codePoint < block.first()) {
			break;
		}
		glyphPtr += codePoint - block.first();
		glyph = FSTR::readValue(glyphPtr);
		return true;
	}

	return false;
}

GlyphMetrics ResourceTypeface::getMetrics(char ch) const
{
	Resource::GlyphResource glyph;
	if(findGlyph(ch, glyph)) {
		return glyph.getMetrics();
	}

	GlyphMetrics metrics{};
	metrics.height = FSTR::readValue(&font.yAdvance);
	metrics.advance = FSTR::readValue(&typeface.yAdvance) / 2;
	return metrics;
}

GlyphObject* ResourceTypeface::getGlyph(char ch, const GlyphOptions& options) const
{
	Resource::GlyphResource glyph;
	if(findGlyph(ch, glyph)) {
		return new ResourceGlyph(font, typeface, glyph, options);
	}

	return nullptr;
}

/* ResourceFont */

const TypeFace* ResourceFont::getFace(FontStyles style) const
{
	style &= FontStyle::Bold | FontStyle::Italic;
	for(auto& f : typefaces) {
		auto& face = reinterpret_cast<const TypeFace&>(f);
		if(face.getStyle() == style) {
			return &face;
		}
	}
	return reinterpret_cast<const TypeFace*>(typefaces.head());
}

} // namespace Graphics
