/**
 * Asset.h
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

#include "Meta.h"
#include <Data/Stream/MemoryDataStream.h>
#include <FlashString/Stream.hpp>
#include "resource.h"

namespace Graphics
{
class Object;
class ImageObject;
class GlyphObject;
class Surface;

#define GRAPHICS_ASSET_TYPE_LIST(XX)                                                                                   \
	XX(Pen)                                                                                                            \
	XX(SolidBrush)                                                                                                     \
	XX(TextureBrush)                                                                                                   \
	XX(Text)                                                                                                           \
	XX(Font)                                                                                                           \
	XX(Typeface)                                                                                                       \
	XX(Blend)                                                                                                          \
	XX(Surface)                                                                                                        \
	XX(Object)

namespace Resource
{
void init(IDataSourceStream* stream);
IDataSourceStream* createSubStream(uint32_t offset, size_t size);
} // namespace Resource

/**
 * @brief An asset is used to render an Object, but is not itself drawable
 */
class Asset : public LinkedObjectTemplate<Asset>, public Meta
{
public:
	using List = LinkedObjectListTemplate<Asset>;
	using OwnedList = OwnedLinkedObjectListTemplate<Asset>;
	using ID = AssetID;

	enum class Type {
#define XX(tag) tag,
		GRAPHICS_ASSET_TYPE_LIST(XX)
#undef XX
	};

	Asset() : mId(nextId++)
	{
	}

	Asset(ID id) : mId(id)
	{
		nextId = std::max(nextId, ID(id + 1));
	}

	using LinkedObjectTemplate::operator==;

	bool operator==(ID id) const
	{
		return this->mId == id;
	}

	ID id() const
	{
		return mId;
	}

	virtual Type type() const = 0;

	/* Meta */

	virtual String getTypeStr() const;
	virtual void write(MetaWriter& meta) const;

private:
	ID mId;
	static ID nextId;
};

static_assert(sizeof(Asset) == 12, "Bad Asset size");

using AssetType = Asset::Type;

template <Asset::Type asset_type> class AssetTemplate : public Asset
{
public:
	static constexpr Asset::Type assetType{asset_type};

	using Asset::Asset;

	virtual Type type() const override
	{
		return asset_type;
	}
};

enum class BrushStyle {
	FullScreen,
	SourceLocal,
};

class SolidBrush : public AssetTemplate<AssetType::SolidBrush>
{
public:
	SolidBrush(AssetID id, Color color) : AssetTemplate(), color(color)
	{
	}

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		return meta.write("color", color);
	}

	Color color;
};

class TextureBrush : public AssetTemplate<AssetType::TextureBrush>
{
public:
	TextureBrush(BrushStyle style) : AssetTemplate(), style(style)
	{
	}

	TextureBrush(AssetID id, BrushStyle style) : AssetTemplate(id), style(style)
	{
	}

	virtual PixelFormat getPixelFormat() const
	{
		return PixelFormat::None;
	}

	virtual size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const = 0;

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		// meta.write("style", style);
	}

protected:
	BrushStyle style;
};

class GradientBrush : public TextureBrush
{
public:
	GradientBrush(BrushStyle style, Color color1, Color color2) : TextureBrush(style), color1(color1), color2(color2)
	{
	}

	GradientBrush(AssetID id, BrushStyle style, Color color1, Color color2)
		: TextureBrush(id, style), color1(color1), color2(color2)
	{
	}

	void write(MetaWriter& meta) const override
	{
		TextureBrush::write(meta);
		meta.write("color1", color1);
		meta.write("color2", color2);
	}

	size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t pixelCount) const;

	String getTypeStr() const override
	{
		return F("GradientBrush");
	}

private:
	Color color1;
	Color color2;
};

/**
 * @brief Brush using pixels from image
 */
class ImageBrush : public TextureBrush
{
public:
	ImageBrush(BrushStyle style, ImageObject& image) : TextureBrush(style), image(image)
	{
	}

	ImageBrush(AssetID id, BrushStyle style, ImageObject& image) : TextureBrush(id, style), image(image)
	{
	}

	PixelFormat getPixelFormat() const override;

	size_t readPixels(const Location& loc, PixelFormat format, void* buffer, uint16_t width) const override;

	/* Meta */

	String getTypeStr() const override
	{
		return F("ImageBrush");
	}

	void write(MetaWriter& meta) const override;

private:
	ImageObject& image;
};

/**
 * @brief The source of colour for drawing
 */
class Brush : public Meta
{
public:
#define GRAPHICS_BRUSH_KIND_MAP(XX)                                                                                    \
	XX(None)                                                                                                           \
	XX(Color)                                                                                                          \
	XX(PackedColor)                                                                                                    \
	XX(Texture)

	enum class Kind : uint8_t {
#define XX(name) name,
		GRAPHICS_BRUSH_KIND_MAP(XX)
#undef XX
	};

	Brush() : brush(nullptr), kind(Kind::None)
	{
	}

	Brush(Color color) : color(color), kind(Kind::Color)
	{
	}

	Brush(const Brush& other) : brush(other.brush), kind(other.kind)
	{
	}

	Brush(const Brush& other, PixelFormat format) : Brush(other)
	{
		setPixelFormat(format);
	}

	Brush(PackedColor color) : packedColor(color), kind(Kind::PackedColor)
	{
	}

	Brush(const TextureBrush* brush) : brush(brush), kind(Kind::Texture)
	{
	}

	void setColor(Color color)
	{
		this->color = color;
		this->kind = Kind::Color;
	}

	void setPixelFormat(PixelFormat format)
	{
		if(kind == Kind::Color) {
			packedColor = pack(color, format);
			kind = Kind::PackedColor;
		}
		pixelFormat = format;
	}

	Kind getKind() const
	{
		return kind;
	}

	explicit operator bool() const
	{
		return kind != Kind::None;
	}

	bool isSolid() const
	{
		return kind == Kind::Color || kind == Kind::PackedColor;
	}

	bool isTransparent() const
	{
		if(kind == Kind::Color) {
			return getAlpha(color) < 255;
		}
		if(kind == Kind::PackedColor) {
			return packedColor.alpha < 255;
		}
		return false;
	}

	bool operator==(Color color) const
	{
		return kind == Kind::Color && this->color == color;
	}

	bool operator!=(Color color) const
	{
		return !operator==(color);
	}

	bool operator==(const Brush& other) const
	{
		return kind == other.kind && brush == other.brush;
	}

	bool operator!=(const Brush& other) const
	{
		return !operator==(other);
	}

	Color getColor() const
	{
		if(kind == Kind::Color) {
			return color;
		}
		if(kind == Kind::PackedColor) {
			return unpack(packedColor, pixelFormat);
		}
		assert(false);
		return Color::Black;
	}

	PackedColor getPackedColor() const
	{
		assert(kind == Kind::PackedColor);
		return packedColor;
	}

	PackedColor getPackedColor(PixelFormat format) const;
	PackedColor getPackedColor(Point pt) const;

	const TextureBrush& getObject() const
	{
		assert(kind == Kind::Texture);
		return *brush;
	}

	bool setPixel(Surface& surface, const Location& loc) const;
	uint16_t setPixels(Surface& surface, const Location& loc, uint16_t pixelCount) const;
	uint16_t writePixel(const Location& loc, void* buffer) const;
	uint16_t writePixels(const Location& loc, void* buffer, uint16_t pixelCount) const;

	/* Meta */

	String getTypeStr() const;
	void write(MetaWriter& meta) const;

private:
	union {
		Color color;
		PackedColor packedColor;
		const TextureBrush* brush;
	};
	Kind kind;
	PixelFormat pixelFormat{};
};

static_assert(sizeof(Brush) == 8, "Brush Size");

class Pen : public Brush
{
public:
	Pen() = default;

	Pen(const Pen& other) : Brush(other), width(other.width)
	{
	}

	Pen(const Brush& brush, uint16_t width = 1) : Brush(brush), width(width)
	{
	}

	Pen(Color color, uint16_t width = 1) : Brush(color), width(width)
	{
	}

	Pen(const Pen& other, PixelFormat format) : Brush(other, format), width(other.width)
	{
	}

	Pen(const TextureBrush& brush, uint16_t width = 1) : Brush(&brush), width(width)
	{
	}

	void write(MetaWriter& meta) const
	{
		Brush::write(meta);
		meta.write(F("width"), width);
	}

	uint16_t width{1};
};

static_assert(sizeof(Pen) == 8, "Pen Size");

class PenAsset : public AssetTemplate<AssetType::Pen>, public Pen
{
public:
	template <typename... ParamTypes> PenAsset(AssetID id, ParamTypes... params) : AssetTemplate(id), Pen(params...)
	{
	}

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		return Pen::write(meta);
	}
};

class TextOptions : public Meta
{
public:
	Brush fore{Color::WHITE};
	Brush back{Color::BLACK};
	Scale scale;
	FontStyles style;

	TextOptions()
	{
	}

	TextOptions(Brush fore, Brush back, Scale scale, FontStyles style)
		: fore(fore), back(back), scale(scale), style(style)
	{
	}

	void setPixelFormat(PixelFormat format)
	{
		fore.setPixelFormat(format);
		back.setPixelFormat(format);
	}

	bool isTransparent() const
	{
		return !back || fore == back;
	}

	/* Meta */

	virtual String getTypeStr() const
	{
		return F("TextOptions");
	}

	void write(MetaWriter& meta) const
	{
		meta.write("fore", fore);
		meta.write("back", back);
		if(scale) {
			meta.write("scale", scale);
		}
		if(style) {
			meta.write("style", style);
		}
	}
};

using GlyphOptions = TextOptions;

/**
 * @brief Base class for a loaded typeface, e.g. Sans 16pt bold
 */
class TypeFace : public AssetTemplate<AssetType::Typeface>
{
public:
	/**
	 * @brief Style of this typeface (bold, italic, etc.)
	 */
	virtual FontStyles getStyle() const = 0;

	/**
	 * @brief Get height of typeface, same for all characters
	 */
	virtual uint8_t height() const = 0;

	/**
	 * How many pixels from bottom of em-square to baseline
	 */
	virtual uint8_t descent() const = 0;

	/**
	 * @brief Get metrics for a character
	 */
	virtual GlyphMetrics getMetrics(char ch) const = 0;

	/**
	 * @brief Get the glyph for a character
	 * @param ch
	 * @param options Options to control how the glyph is drawn (colour, shading, etc)
	 * @retval GlyphObject* The glyph, nullptr if no glyph exists in the typeface for this character
	 * 
	 * Caller is responsible for destroying the glyph when no longer required.
	 */
	virtual GlyphObject* getGlyph(char ch, const GlyphOptions& options) const = 0;

	/**
	 * @brief Get baseline relative to top of mbox
	 */
	uint8_t baseline() const
	{
		return height() - descent();
	}

	/**
	 * @brief Compute displayed width for a text string
	 */
	uint16_t getTextWidth(const char* text, uint16_t length) const;

	/* Meta */

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		meta.write("style", getStyle());
		meta.write("height", height());
		meta.write("descent", descent());
	}
};

/**
 * @brief Base class for a loaded font
 *
 * As there are various formats for defining fonts we need a consistent interface for accessing
 * and rendering them, which this class defines.
 *
 * Fonts are specified by family and size, and can be registered with various typefaces.
 *
 */
class Font : public AssetTemplate<AssetType::Font>
{
public:
	using AssetTemplate::AssetTemplate;

	virtual String name() const = 0;

	virtual uint16_t height() const = 0;

	virtual const TypeFace* getFace(FontStyles style) const = 0;

	/* Meta */

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		meta.write("font", name());
	}
};

class ResourceTypeface : public Graphics::TypeFace
{
public:
	ResourceTypeface(const Resource::FontResource& font, const Resource::TypefaceResource& typeface)
		: font(font), typeface(typeface)
	{
	}

	FontStyles getStyle() const override
	{
		return FSTR::readValue(&typeface.style);
	}

	uint8_t height() const override
	{
		return FSTR::readValue(&font.yAdvance);
	}

	uint8_t descent() const override
	{
		return FSTR::readValue(&font.descent);
	}

	GlyphMetrics getMetrics(char ch) const override;

	GlyphObject* getGlyph(char ch, const GlyphOptions& options) const override;

private:
	bool findGlyph(uint16_t codePoint, Resource::GlyphResource& res) const;

	const Resource::FontResource& font;
	const Resource::TypefaceResource& typeface;
};

class ResourceFont : public Graphics::Font
{
public:
	// TODO: Call Font(id)
	ResourceFont(const Resource::FontResource& font) : Font(), font(font)
	{
		init();
	}

	ResourceFont(AssetID id, const Resource::FontResource& font) : Font(id), font(font)
	{
		init();
	}

	String name() const override
	{
		return *font.name;
	}

	uint16_t height() const override
	{
		return FSTR::readValue(&font.yAdvance);
	}

	const TypeFace* getFace(FontStyles style) const override;

private:
	void init()
	{
		for(auto& face : font.faces) {
			if(face != nullptr) {
				typefaces.add(new ResourceTypeface(font, *face));
			}
		}
	}

	const Resource::FontResource& font;
	TypeFace::OwnedList typefaces;
};

class TextAsset : public AssetTemplate<AssetType::Text>
{
public:
	TextAsset(String&& content) : AssetTemplate(), stream(new MemoryDataStream(std::move(content)))
	{
	}

	TextAsset(AssetID id) : AssetTemplate(id), stream(new MemoryDataStream)
	{
	}

	TextAsset(IDataSourceStream* stream) : AssetTemplate(), stream(stream)
	{
	}

	TextAsset(AssetID id, IDataSourceStream* stream) : AssetTemplate(id), stream(stream)
	{
	}

	TextAsset(const FSTR::String& fstr) : TextAsset(new FSTR::Stream(fstr))
	{
	}

	TextAsset(AssetID id, const char* text, size_t length) : TextAsset(id)
	{
		reinterpret_cast<MemoryDataStream*>(stream.get())->write(text, length);
	}

	TextAsset(AssetID id, const String& s) : TextAsset(id, s.c_str(), s.length())
	{
	}

	TextAsset(AssetID id, String&& s) : AssetTemplate(id), stream(new MemoryDataStream(std::move(s)))
	{
	}

	size_t getLength() const
	{
		return stream ? stream->seekFrom(0, SeekOrigin::End) : 0;
	}

	size_t read(uint32_t offset, char* buffer, size_t length) const
	{
		if(!stream) {
			return 0;
		}
		stream->seekFrom(offset, SeekOrigin::Start);
		return stream->readBytes(buffer, length);
	}

	char read(uint32_t offset) const
	{
		char ch{'\0'};
		read(offset, &ch, 1);
		return ch;
	}

	String readString(size_t maxlen) const
	{
		return stream ? stream->readString(maxlen) : nullptr;
	}

	/* Meta */

	void write(MetaWriter& meta) const override
	{
		AssetTemplate::write(meta);
		meta.write("length", getLength());
		if(stream) {
			meta.write("content", *stream);
		}
	}

private:
	std::unique_ptr<IDataSourceStream> stream;
};

class ObjectAsset : public AssetTemplate<AssetType::Object>
{
public:
	ObjectAsset(const Object* object);
	ObjectAsset(AssetID id, const Object* object);
	~ObjectAsset();

	std::unique_ptr<const Object> object;
};

class AssetList : public OwnedLinkedObjectListTemplate<Asset>
{
public:
	Asset* find(AssetID id)
	{
		return std::find(begin(), end(), id);
	}

	const Asset* find(AssetID id) const
	{
		return std::find(begin(), end(), id);
	}

	Asset* find(AssetType type, AssetID id);

	const Asset* find(AssetType type, AssetID id) const
	{
		return const_cast<AssetList*>(this)->find(type, id);
	}

	template <typename T> T* find(AssetID id)
	{
		return reinterpret_cast<T*>(find(T::assetType, id));
	}

	template <typename T> const T* find(AssetID id) const
	{
		return reinterpret_cast<const T*>(find(T::assetType, id));
	}

	void store(Asset* asset);

	void store(AssetID id, Pen pen)
	{
		store(new PenAsset(id, pen));
	}

	// void store(AssetID id, Brush brush)
	// {
	// 	if(brush.isSolid()) {
	// 		store(new SolidBrush(id, brush.getColor()));
	// 	} else {
	// 		auto b = new TextureBrush(brush.getObject());
	// 		b->id = id;
	// 		store(b);
	// 	}
	// }
};

} // namespace Graphics

String toString(Graphics::AssetType type);
String toString(Graphics::Brush::Kind kind);
