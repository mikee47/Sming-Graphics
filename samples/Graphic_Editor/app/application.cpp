#include <SmingCore.h>
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

using ResourceData = String;
HashMap<String, ResourceData> resourceMap;

uint32_t hexValue(const String& s)
{
	return strtoul(s.c_str(), nullptr, 16);
}

class PropertySet;

class CustomLabel : public Label
{
public:
	CustomLabel(const PropertySet& props);

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
	String font;
	// fontstyle
	uint8_t fontscale;
	Align halign;
};

class CustomButton : public Button
{
public:
	CustomButton(const PropertySet& props);

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
	String font;
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
			TextBuilder textBuilder(scene.assets, rect());
			textBuilder.setFont(nullptr);
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
			auto& resdata = resourceMap[image];
			if(!resdata) {
				Serial << "Resource '" << image << "' not found" << endl;
				return;
			}
			auto& imgres = *reinterpret_cast<const Resource::ImageResource*>(resdata.c_str());

			Serial << "bmOffset " << imgres.bmOffset << ", bmSize " << imgres.bmSize << ", width " << imgres.width
				   << ", height " << imgres.height << ", format " << imgres.format << endl;

			ImageObject* img;
			if(imgres.format == PixelFormat::None) {
				img = new BitmapObject(imgres);
				if(!img->init()) {
					debug_e("Bad bitmap");
				}
			} else {
				img = new RawImageObject(imgres);
			}
			Serial << "Image size " << toString(img->getSize()) << endl;
			scene.addAsset(img);
			scene.drawImage(*img, {x, y});
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
	: Label(props.rect(), props.text), back_color(props.back_color), color(props.color), font(props.font),
	  fontscale(props.fontscale), halign(props.halign)
{
}

CustomButton::CustomButton(const PropertySet& props)
	: Button(props.rect(), props.text), border(props.border), back_color(props.back_color), color(props.color),
	  font(props.font), fontscale(props.fontscale)
{
}

bool processClientData(TcpClient& client, char* data, int size)
{
	static SceneObject* scene;
	static String line;
	static std::unique_ptr<ReadWriteStream> resourceStream;

	while(size) {
		line.setLength(0);
		auto p = (const char*)memchr(data, '\n', size);
		auto n = p ? (p - data) : size;
		line.concat(data, n);
		if(!p) {
			break;
		}
		++n;
		data += n;
		size -= n;
		// Serial << line << endl;

		auto lineptr = line.c_str();
		if(lineptr[1] != ':') {
			continue;
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

		auto decodeBinary = [&]() -> unsigned {
			auto offset = lineptr - line.c_str();
			auto charCount = line.length() - offset;
			auto bufptr = line.begin();
			auto bytecount = base64_decode(charCount, lineptr, charCount, reinterpret_cast<uint8_t*>(bufptr));
			lineptr = bufptr;
			return bytecount;
		};

		if(dataKind == 'b') {
			if(!resourceStream) {
				continue;
			}
			auto bytecount = decodeBinary();
			auto len = resourceStream->write(lineptr, bytecount);
			(void)len;
			// Serial << ">> res.write(" << bytecount << "): " << len << endl;
			continue;
		}

		if(dataKind == 'r') {
			// Resource
			String kind = fetch(';');
			String name = fetch(';');
			auto bytecount = decodeBinary();
			resourceMap[name] = String(lineptr, bytecount);
			Serial << kind << " resource '" << name << "', " << bytecount
				   << " bytes: " << makeHexString(lineptr, bytecount) << endl;
			continue;
		}

		PropertySet props;
		String instr = fetch(';');
		Serial << dataKind << " : " << instr << endl;
		String tag;
		String value;
		while(*lineptr) {
			tag = fetch('=');
			value = fetch(';');
			Serial << "  " << tag << " = " << value << endl;
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
			} else if(instr == "resource-begin") {
				auto part = Storage::findPartition(F("resource"));
				resourceStream.reset(new Storage::PartitionStream(part, true));
				Serial << "** Writing resource" << endl;
			} else if(instr == "resource-end") {
				Serial << "** Resource written" << endl;
				resourceStream.reset();
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
