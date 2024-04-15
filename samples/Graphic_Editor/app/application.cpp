#include <SmingCore.h>
#include <Data/ObjectMap.h>
#include <esp_spi_flash.h>
#include <FlashString/Map.hpp>
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

// #define RESOURCE_MAP_IN_FLASH

#ifdef RESOURCE_MAP_IN_FLASH
#define RESOURCE_INDEX_SIZE 0x20000
#define RESOURCE_CONST const
const uint8_t resource_index[RESOURCE_INDEX_SIZE] PROGMEM{};
#else
#define RESOURCE_CONST
#endif

struct ResourceInfo {
	ResourceInfo(const LinkedObject* object, const void* data) : object(object), data(data)
	{
	}

	std::unique_ptr<const LinkedObject> object;
	const void* data;
};

class ResourceMap : public ObjectMap<String, const ResourceInfo>
{
public:
#ifdef RESOURCE_MAP_IN_FLASH
	ResourceMap()
	{
		auto resPtr = resource_index;
		auto blockOffset = uint32_t(resource_index) % SPI_FLASH_SEC_SIZE;
		if(blockOffset != 0) {
			resPtr += SPI_FLASH_SEC_SIZE - blockOffset;
		}
		map = reinterpret_cast<decltype(map)>(resPtr);
	}
#endif

	const void* reset(size_t size)
	{
		clear();

#ifdef RESOURCE_MAP_IN_FLASH
		auto blockOffset = uint32_t(resource_index) % SPI_FLASH_SEC_SIZE;

		if(blockOffset + size > RESOURCE_INDEX_SIZE) {
			debug_e("Resource too big");
			mapSize = 0;
			return nullptr;
		}

		mapSize = std::max(size, size_t(RESOURCE_INDEX_SIZE - blockOffset));
#else
		free(map);
		auto buf = malloc(size);
		map = static_cast<decltype(map)>(buf);
		mapSize = buf ? size : 0;
#endif

		return static_cast<const void*>(map);
	}

	std::unique_ptr<ReadWriteStream> createStream()
	{
#if defined(RESOURCE_MAP_IN_FLASH)
		auto addr = flashmem_get_address(map);
		auto part = *Storage::findPartition(Storage::Partition::Type::app);
		Serial << part << endl;
		if(!part.contains(addr)) {
			debug_e("Bad resource index address %p, part %p, map %p", addr, part.address(), map);
			assert(false);
			return nullptr;
		}
		auto offset = addr - part.address();
		debug_i("Resource index @ %p, addr %p, offset %p", map, addr, offset);
		return std::make_unique<Storage::PartitionStream>(part, offset, mapSize, true);
#else
		return std::make_unique<LimitedMemoryStream>(map, mapSize, 0, false);
#endif
	}

	const Font* getFont(const String& name)
	{
		auto obj = findObject(name);
		if(obj) {
			return static_cast<const Font*>(obj);
		}

		auto data = findResource(name);
		if(!data) {
			return nullptr;
		}

		auto fontres = static_cast<const Resource::FontResource*>(data);
		if(fontres->faces[0] == nullptr) {
			Serial << "Font '" << name << "' has no typefaces" << endl;
			return nullptr;
		}
		auto font = new ResourceFont(*fontres);
		set(name, new ResourceInfo{font, data});

		return font;
	}

	const ImageObject* getImage(const String& name)
	{
		auto obj = findObject(name);
		if(obj) {
			return static_cast<const ImageObject*>(obj);
		}

		auto data = findResource(name);
		if(!data) {
			return nullptr;
		}

		auto& imgres = *reinterpret_cast<const Resource::ImageResource*>(data);
		// Serial << "bmOffset " << imgres.bmOffset << ", bmSize " << imgres.bmSize << ", width " << imgres.width
		// 	   << ", height " << imgres.height << ", format " << imgres.format << endl;
		ImageObject* img;
		if(imgres.getFormat() == PixelFormat::None) {
			img = new BitmapObject(imgres);
			if(!img->init()) {
				debug_e("Bad bitmap");
				delete img;
				return nullptr;
			}
		} else {
			img = new RawImageObject(imgres);
		}
		// Serial << "Image size " << toString(img->getSize()) << endl;
		set(name, new ResourceInfo{img, data});

		return img;
	}

private:
	const LinkedObject* findObject(const String& name)
	{
		auto info = get(name).getValue();
		if(!info) {
			// Serial << "Resource '" << name << "' not found" << endl;
			return nullptr;
		}
		return info->object.get();
	}

	const void* findResource(const String& name)
	{
		if(!map) {
			Serial << "Resource map not initialised" << endl;
			return nullptr;
		}

		auto data = (*map)[name];
		if(!data) {
			Serial << "Resource '" << name << "' not found" << endl;
			return nullptr;
		}

		// Serial << "Found " << name << endl;
		return data.content_;
	}

	struct Data {
		uint8_t data;
	};

	RESOURCE_CONST FSTR::Map<FSTR::String, Data>* map{};
	size_t mapSize{0};
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
		} else if(name == "size") {
			size = value.toInt();
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
			auto img = resourceMap.getImage(image);
			if(img) {
				auto r = rect();
				scene.drawObject(*img, r, Point{xoff, yoff});
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
	// resaddr
	uint32_t size{0};
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

void processLine(TcpClient& client, String& line)
{
	static SceneObject* scene;
	static unsigned resourceLockCount;
	static std::unique_ptr<ReadWriteStream> resourceStream;
	static size_t resourceSize;

	if(resourceLockCount) {
		Serial << "RENDER BUSY" << endl;
		return;
	}

	// Serial << line << endl;

	auto lineptr = line.c_str();
	if(lineptr[1] != ':') {
		return;
	}
	char dataKind = lineptr[0];
	lineptr += 2;

	auto split = [&](char sep) -> String {
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
		if(resourceStream) {
			decodeBinary();
			resourceSize += resourceStream->print(line);
		}
		return;
	}

	PropertySet props;
	String instr = split(';');
	// Serial << dataKind << " : " << instr << endl;
	String tag;
	String value;
	while(*lineptr) {
		tag = split('=');
		value = split(';');
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
			break;
		}
		if(instr == "clear") {
			delete scene;
			scene = new SceneObject(tft.getSize());
			scene->clear();
			break;
		}
		if(instr == "render") {
			renderQueue.render(scene, [&](SceneObject* scene) {
				Serial << "Render done" << endl;
				delete scene;
				--resourceLockCount;
			});
			scene = nullptr;
			++resourceLockCount;
			break;
		}
		if(instr == "resaddr") {
			delete scene;
			scene = nullptr;
			auto buffer = resourceMap.reset(props.size);
			String line;
			line += "@:addr=0x";
			line += String(uint32_t(buffer), HEX);
			line += ";\n";
			client.sendString(line);
			break;
		}
		if(instr == "index") {
			resourceStream = resourceMap.createStream();
			resourceSize = 0;
			Serial << "** Writing index" << endl;
			break;
		}
		if(instr == "bitmap") {
			auto part = Storage::findPartition(F("resource"));
			if(!part) {
				Serial << "Resource partition not found" << endl;
				resourceStream.reset();
				break;
			}
			resourceStream = std::make_unique<Storage::PartitionStream>(part, Storage::Mode::BlockErase);
			resourceSize = 0;
			Serial << "** Writing resource bitmap" << endl;
			break;
		}
		if(instr == "end") {
			if(resourceStream) {
				Serial << "** Resource written, " << resourceSize << " bytes" << endl;
				resourceStream.reset();
				resourceSize = 0;
				client.sendString("@:OK\n");
			}
			break;
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

		processLine(client, line);
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
