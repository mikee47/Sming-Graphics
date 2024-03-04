#include <SmingCore.h>
#include <Data/ObjectMap.h>
#include <Storage/PartitionStream.h>
#include <Data/WebHelpers/escape.h>
#include <Data/WebHelpers/base64.h>
#include <Network/TcpServer.h>
#include <Graphics.h>
#include <Graphics/Controls.h>
#include <Graphics/SampleConfig.h>
#include <Graphics/resource.h>

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID
#define WIFI_SSID "PleaseEnterSSID" // Put your SSID and password here
#define WIFI_PWD "PleaseEnterPass"
#endif

using namespace Graphics;

namespace
{
RenderQueue renderQueue(tft);
TcpServer server;

struct ResourceInfo {
	ResourceInfo()
	{
	}

	ResourceInfo(const LinkedObject* object, void* data)
	{
		this->object.reset(object);
		this->data = data;
	}

	~ResourceInfo()
	{
		free(data);
	}

	ResourceInfo* operator=(ResourceInfo&& other)
	{
		if(this != &other) {
			object = std::move(other.object);
			free(data);
			data = nullptr;
			std::swap(data, other.data);
		}
		return this;
	}

	std::unique_ptr<const LinkedObject> object;
	void* data;
};

class ResourceMap : public ObjectMap<String, const ResourceInfo>
{
public:
	const Font* getFont(const String& name)
	{
		return static_cast<const Font*>(findObject(name));
	}

	const ImageObject* getImage(const String& name)
	{
		return static_cast<const ImageObject*>(findObject(name));
	}

private:
	const LinkedObject* findObject(const String& name)
	{
		auto info = get(name).getValue();
		if(!info) {
			Serial << "Resource '" << name << "' not found" << endl;
			return nullptr;
		}
		// Serial << "Found '" << name << "'" << endl;
		return info->object.get();
	}
};

ResourceMap resourceMap;

uint32_t hexValue(const String& s)
{
	return strtoul(s.c_str(), nullptr, 16);
}

class PropertySet;

class CustomLabel : public Label
{
public:
	CustomLabel(const PropertySet& props);

	const Font* getFont() const override
	{
		return font;
	}

	Color getColor(Element element) const override
	{
		switch(element) {
		case Element::text:
			return color;
		case Element::back:
			return back_color;
		default:
			return Label::getColor(element);
		}
	}

	Align getTextAlign() const override
	{
		return halign;
	}

	Color back_color;
	Color color;
	const Font* font{};
	// fontstyle
	uint8_t fontscale;
	Align halign;
};

class CustomButton : public Button
{
public:
	CustomButton(const PropertySet& props);

	const Font* getFont() const override
	{
		return font;
	}

	Color getColor(Element element) const override
	{
		switch(element) {
		case Element::border:
			return border;
		case Element::text:
			return color;
		case Element::back:
			return back_color;
		default:
			return Button::getColor(element);
		}
	}

	Color border;
	Color back_color;
	Color color;
	const Font* font{};
	// fontstyle
	uint8_t fontscale;
};

struct PropertySet {
	void setProperty(const String& name, const String& value)
	{
		auto getColor = [&]() -> Color { return Color(hexValue(value)); };

		if(name == "x") {
			x = value.toInt();
		} else if(name == "y") {
			y = value.toInt();
		} else if(name == "w") {
			w = value.toInt();
		} else if(name == "h") {
			h = value.toInt();
		} else if(name == "back_color") {
			back_color = getColor();
		} else if(name == "border") {
			border = getColor();
		} else if(name == "color") {
			color = getColor();
		} else if(name == "line_width") {
			line_width = value.toInt();
		} else if(name == "radius") {
			radius = value.toInt();
		} else if(name == "font") {
			font = value;
		} else if(name == "text") {
			text = uri_unescape(value);
		} else if(name == "fontstyle") {
			fontstyles = FontStyles(hexValue(value));
			// Serial << "FontStyle " << fontstyles << endl;
		} else if(name == "fontscale") {
			fontscale = value.toInt();
		} else if(name == "image") {
			image = value;
		} else if(name == "xoff") {
			xoff = value.toInt();
		} else if(name == "yoff") {
			yoff = value.toInt();
		} else if(name == "halign") {
			halign = Align(value.toInt());
		} else if(name == "valign") {
			valign = Align(value.toInt());
		} else if(name == "orient") {
			orientation = Orientation(value.toInt());
		}
	}

	void draw(SceneObject& scene, const String& type)
	{
		if(type == "Rect") {
			scene.drawRect(Pen(color, line_width), rect(), radius);
		} else if(type == "FilledRect") {
			scene.fillRect(color, rect(), radius);
		} else if(type == "Ellipse") {
			scene.drawEllipse(Pen(color, line_width), rect());
		} else if(type == "FilledEllipse") {
			scene.fillEllipse(color, rect());
		} else if(type == "Text") {
			// font
			auto font = resourceMap.getFont(this->font);
			TextBuilder textBuilder(scene.assets, rect());
			textBuilder.setFont(font);
			textBuilder.setStyle(fontstyles);
			textBuilder.setColor(color, back_color);
			textBuilder.setScale(fontscale);
			textBuilder.setTextAlign(halign);
			textBuilder.setLineAlign(valign);
			textBuilder.print(text);
			textBuilder.commit(scene);
			textBuilder.commit(scene);
		} else if(type == "Image") {
			// xoff
			// yoff
			auto img = resourceMap.getImage(image);
			if(img) {
				scene.drawImage(*img, {x, y});
			}
		} else if(type == "Button") {
			scene.addObject(new CustomButton(*this));
		} else if(type == "Label") {
			scene.addObject(new CustomLabel(*this));
		}
	}

	Rect rect() const
	{
		return Rect(x, y, w, h);
	}

	int16_t x = 0;
	int16_t y = 0;
	uint16_t w = 0;
	uint16_t h = 0;
	Color back_color = Color::Gray;
	Color border = Color::White;
	Color color = Color::Black;
	uint16_t line_width = 1;
	uint16_t radius = 0;
	String font;
	String text;
	FontStyles fontstyles{};
	uint8_t fontscale = 1;
	String image;
	int16_t xoff = 0;
	int16_t yoff = 0;
	Align halign{};
	Align valign{};
	// Size command
	Orientation orientation{};
};

CustomLabel::CustomLabel(const PropertySet& props)
	: Label(props.rect(), props.text), back_color(props.back_color), color(props.color), fontscale(props.fontscale),
	  halign(props.halign)
{
	font = resourceMap.getFont(props.font);
}

CustomButton::CustomButton(const PropertySet& props)
	: Button(props.rect(), props.text), border(props.border), back_color(props.back_color), color(props.color),
	  fontscale(props.fontscale)
{
	font = resourceMap.getFont(props.font);
}

struct ResourceWriter {
	ResourceWriter()
	{
		stream.reset(new MemoryDataStream);
	}

	virtual ~ResourceWriter()
	{
	}

	virtual void complete() = 0;

	size_t write(const void* data, size_t size)
	{
		if(!stream) {
			return 0;
		}
		auto written = stream->write(static_cast<const uint8_t*>(data), size);
		len += written;
		// Serial << ">> res.write(" << size << "): " << written << endl;
		return written;
	}

	std::unique_ptr<ReadWriteStream> stream;
	uint32_t len{0};
	String name;
};

struct BitmapResourceWriter : public ResourceWriter {
	using ResourceWriter::ResourceWriter;

	BitmapResourceWriter()
	{
		auto part = Storage::findPartition(F("resource"));
		stream.reset(new Storage::PartitionStream(part, true));
		Serial << "BITMAP" << endl;
	}

	void complete() override
	{
		// Nothing to do
	}
};

struct ImageResourceWriter : public ResourceWriter {
	using ResourceWriter::ResourceWriter;

	void complete() override
	{
		String data;
		stream->moveString(data);
		auto buf = data.getBuffer();
		buf.data = (char*)realloc(buf.data, buf.length);

		auto& imgres = *reinterpret_cast<const Resource::ImageResource*>(buf.data);
		// Serial << "bmOffset " << imgres.bmOffset << ", bmSize " << imgres.bmSize << ", width " << imgres.width
		// 	   << ", height " << imgres.height << ", format " << imgres.format << endl;
		ImageObject* img;
		if(imgres.format == PixelFormat::None) {
			img = new BitmapObject(imgres);
			if(!img->init()) {
				debug_e("Bad bitmap");
				delete img;
				free(buf.data);
				return;
			}
		} else {
			img = new RawImageObject(imgres);
		}
		// Serial << "Image size " << toString(img->getSize()) << endl;
		resourceMap.set(name, new ResourceInfo{img, buf.data});
	}
};

struct FontResourceWriter : public ResourceWriter {
	using ResourceWriter::ResourceWriter;

	void complete() override
	{
		String data;
		stream->moveString(data);
		auto buf = data.getBuffer();
		buf.data = (char*)realloc(buf.data, buf.length);

		auto fontres = reinterpret_cast<Resource::FontResource*>(buf.data);
		fixupFont(fontres);
		auto font = new ResourceFont(*fontres);
		resourceMap.set(name, new ResourceInfo{font, buf.data});
	}

	template <typename T> static void fixup(const T*& ptr, const Resource::FontResource* font)
	{
		auto baseptr = reinterpret_cast<const uint8_t*>(font);
		auto newptr = const_cast<uint8_t*>(baseptr + uint32_t(ptr));
		debug_i("ptr %p -> %p", ptr, newptr);
		ptr = reinterpret_cast<T*>(newptr);
	}

	static void fixupFace(const Resource::TypefaceResource* face, const Resource::FontResource* font)
	{
		// debug_i("FACE @%p: %p, %u, %u, %u, %u, %p, %p", face, face->bmOffset, face->style, face->yAdvance,
		// 		face->descent, face->numBlocks, face->glyphs, face->blocks);

		auto f = const_cast<Resource::TypefaceResource*>(face);
		fixup(f->glyphs, font);
		fixup(f->blocks, font);

		// debug_i(">> FACE @%p: %p, %u, %u, %u, %u, %p, %p", face, face->bmOffset, face->style, face->yAdvance,
		// 		face->descent, face->numBlocks, face->glyphs, face->blocks);
	}

	static void fixupFont(Resource::FontResource* font)
	{
		// debug_i("FONT @%p: %u, %u, %u, %u, %p, %p, %p, %p", font, font->yAdvance, font->descent, font->padding[0],
		// 		font->padding[1], font->faces[0], font->faces[1], font->faces[2], font->faces[3]);

		for(auto& face : font->faces) {
			if(!face) {
				continue;
			}
			fixup(face, font);
			fixupFace(face, font);
		}

		// debug_i(">> FONT @%p: %u, %u, %u, %u, %p, %p, %p, %p", font, font->yAdvance, font->descent, font->padding[0],
		// 		font->padding[1], font->faces[0], font->faces[1], font->faces[2], font->faces[3]);
	}
};

void processLine(String& line)
{
	static SceneObject* scene;
	static std::unique_ptr<ResourceWriter> resource;

	// Serial << line << endl;

	auto lineptr = line.c_str();
	if(lineptr[1] != ':') {
		return;
	}
	char dataKind = lineptr[0];
	lineptr += 2;

	auto fetch = [&](char sep) -> String {
		auto p = lineptr;
		auto psep = strchr(lineptr, sep);
		if(!psep) {
			lineptr += strlen(lineptr);
			return String(p);
		}
		lineptr = psep + 1;
		return String(p, psep - p);
	};

	// Decode base64 data at lineptr in-situ, replacing line contents (will always be smaller)
	auto decodeBinary = [&]() {
		auto offset = lineptr - line.c_str();
		auto charCount = line.length() - offset;
		auto bufptr = line.begin();
		auto bytecount = base64_decode(charCount, lineptr, charCount, reinterpret_cast<uint8_t*>(bufptr));
		line.setLength(bytecount);
	};

	if(dataKind == 'b') {
		if(!resource) {
			return;
		}
		decodeBinary();
		resource->write(line.c_str(), line.length());
		return;
	}

	if(dataKind == 'r') {
		// Resource
		String kind = fetch(';');
		String name = fetch(';');
		Serial << "Resource " << dataKind << ": " << name << endl;

		if(kind == "image") {
			resource.reset(new ImageResourceWriter);
			resource->name = name;
			Serial << "** Writing image" << endl;
			return;
		}

		if(kind == "font") {
			resource.reset(new FontResourceWriter);
			resource->name = name;
			Serial << "** Writing font" << endl;
			return;
		}

		if(kind == "bitmap") {
			resource.reset(new BitmapResourceWriter);
			Serial << "** Writing resource bitmap" << endl;
			return;
		}
	}

	PropertySet props;
	String instr = fetch(';');
	// Serial << dataKind << " : " << instr << endl;
	String tag;
	String value;
	while(*lineptr) {
		tag = fetch('=');
		value = fetch(';');
		// Serial << "  " << tag << " = " << value << endl;
		props.setProperty(tag, value);
	}
	switch(dataKind) {
	case '@':
		if(instr == "size") {
#ifdef ENABLE_VIRTUAL_SCREEN
			tft.setDisplaySize(props.w, props.h, props.orientation);
#else
			tft.setOrientation(props.orientation);
#endif
		} else if(instr == "clear") {
			delete scene;
			scene = new SceneObject(tft.getSize());
			scene->clear();
		} else if(instr == "render") {
			renderQueue.render(scene, [](SceneObject* scene) {
				Serial << "Render done" << endl;
				delete scene;
			});
			scene = nullptr;
		} else if(instr == "end") {
			if(resource) {
				resource->complete();
				Serial << "** Resource '" << resource->name + "' written, " << resource->len << " bytes" << endl;
				resource.reset();
			}
		}
		break;

	case 'i': {
		if(!scene) {
			Serial << "NO SCENE!";
			break;
		}
		props.draw(*scene, instr);
		break;
	}
	}
}

bool processClientData(TcpClient& client, char* data, int size)
{
	static String line;

	while(size) {
		auto p = (const char*)memchr(data, '\n', size);
		auto n = p ? (p - data) : size;
		line.concat(data, n);
		if(!p) {
			break;
		}
		++n;
		data += n;
		size -= n;

		processLine(line);
		line.setLength(0);
	}

	return true;
}

void gotIP(IpAddress ip, IpAddress netmask, IpAddress gateway)
{
	server.setClientReceiveHandler(processClientData);
	server.listen(23);
	Serial << _F("\r\n=== TCP server started ===") << endl;
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

	Serial << _F("Display start") << endl;
	initDisplay();

	auto part = Storage::findPartition(F("resource"));
	Serial << part << endl;
	Graphics::Resource::init(new Storage::PartitionStream(part));

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);

	WifiEvents.onStationGotIP(gotIP);
}
