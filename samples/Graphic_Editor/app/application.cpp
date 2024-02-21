#include <SmingCore.h>
#include <Data/WebHelpers/escape.h>
#include <Network/TcpServer.h>
#include <Graphics.h>
#include <Graphics/Controls.h>
#include <Graphics/SampleConfig.h>

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

class PropertySet
{
public:
	void setProperty(const String& name, const String& value)
	{
		auto getColor = [&]() -> Color {
			auto n = strtol(value.c_str(), nullptr, 16);
			return makeColor(n);
		};

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
		} else if(name == "align") {
			//
		} else if(name == "fontstyle") {
			//
		} else if(name == "fontscale") {
			fontscale = value.toInt();
		} else if(name == "image") {
			image = value;
		} else if(name == "xoff") {
			xoff = value.toInt();
		} else if(name == "yoff") {
			yoff = value.toInt();
		} else if(name == "halign") {
			//
		}
	}

	Object* createObject(SceneObject& scene, const String& type)
	{
		if(type == "Rect") {
			return new RectObject(Pen(color, line_width), rect(), radius);
		}
		if(type == "FilledRect") {
			// Serial << "FilledRectObject(" << color << ", " << rect().toString() << ", " << radius << ")" << endl;
			return new FilledRectObject(color, rect(), radius);
		}
		if(type == "Ellipse") {
			return new EllipseObject(Pen(color, line_width), rect());
		}
		if(type == "FilledEllipse") {
			return new FilledEllipseObject(color, rect());
		}
		if(type == "Text") {
			// font
			// fontstyle
			// back_color
			TextBuilder textBuilder(scene.assets, rect());
			textBuilder.setFont(nullptr);
			textBuilder.setColor(color, Color::Black);
			textBuilder.setScale(fontscale);
			textBuilder.setTextAlign(Align::Centre);
			textBuilder.setLineAlign(Align::Centre);
			textBuilder.print(text);
			textBuilder.commit(scene);
			return textBuilder.release();
		}
		if(type == "Image") {
			// image
			// xoff
			// yoff
			// return new ImageObject(...);
			return nullptr;
		}
		if(type == "Button") {
			// border
			// back_color
			// color
			// font
			// fontstyle
			// fontscale
			Serial << "Button(" << rect().toString() << ", \"" << text << "\"" << endl;
			auto btn = new Button(rect(), text);
			btn->setBounds(rect());
			return btn;
		}
		if(type == "Label") {
			// back_color
			// color
			// font
			// fontstyle
			// fontscale
			// halign
			return new Label(rect(), text);
		}

		return nullptr;
	}

	Rect rect() const
	{
		return Rect(x, y, w, h);
	}

private:
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
	// align: list[str] = dataclasses.field(default_factory=list)
	// fontstyle: list[str] = dataclasses.field(default_factory=list)
	uint16_t fontscale = 1;
	String image;
	int16_t xoff = 0;
	int16_t yoff = 0;
	// halign: str = ''
};

bool processClientData(TcpClient& client, char* data, int size)
{
	static SceneObject* scene;
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
		// Serial << line << endl;

		auto lineptr = line.c_str();
		auto fetch = [&](char sep) -> String {
			auto psep = strchr(lineptr, sep);
			if(!psep) {
				return nullptr;
			}
			auto p = lineptr;
			lineptr = psep + 1;
			return String(p, psep - p);
		};

		String type = fetch(':');
		Serial << type << endl;
		if(type == "CMD") {
			auto cmd = fetch(';');
			Serial << cmd << endl;
			if(cmd == "clear") {
				delete scene;
				scene = new SceneObject(tft.getSize());
				scene->clear();
			} else if(cmd == "render") {
				renderQueue.render(scene, [](SceneObject* scene) {
					Serial << "Render done" << endl;
					delete scene;
				});
				scene = nullptr;
			}
		} else if(!scene) {
			Serial << "NO SCENE!";
		} else {
			PropertySet props;
			while(*lineptr) {
				String tag = fetch('=');
				String value = fetch(';');
				props.setProperty(tag, value);
			}
			auto obj = props.createObject(*scene, type);
			if(obj) {
				scene->addObject(obj);
			}
		}

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
	tft.setOrientation(Orientation::deg270);

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiAccessPoint.enable(false);

	WifiEvents.onStationGotIP(gotIP);
}
