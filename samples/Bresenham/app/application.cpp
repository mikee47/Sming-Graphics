#include <SmingCore.h>
#include <Graphics.h>
#include "bresenham.h"

#ifdef ENABLE_VIRTUAL_SCREEN
#include <Graphics/Display/Virtual.h>
#else
#include <Graphics/Display/ILI9341.h>
#endif

using namespace Graphics;

namespace
{
#ifdef ENABLE_VIRTUAL_SCREEN
Display::Virtual tft;
#else
HSPI::Controller spi;
Display::ILI9341 tft(spi);

// Pin setup
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
// Not used in this sample, but needs to be kept high
constexpr uint8_t TOUCH_CS_PIN{15};
#endif

constexpr Orientation portrait{Orientation::deg180};
constexpr Orientation landscape{Orientation::deg270};

struct DrawingContext {
	Drawing::Writer writer;
	Point offset{};
	Color color{Color::White};
	size_t pixelCount{0};

	DrawingContext(Stream& stream) : writer(stream)
	{
	}

	void setPixel(int x0, int y0)
	{
		Point pt(x0, y0);
		pt += offset;
		writer.setBrushColor(color);
		writer.setPixel(pt);
		++pixelCount;
	}

	void setPixelAA(int x0, int y0, uint8_t alpha)
	{
		Point pt(x0, y0);
		pt += offset;
		writer.setBrushColor(makeColor(color, alpha));
		writer.setPixel(pt);
		++pixelCount;
	}
};

SceneObject* activeScene;
DrawingContext* context;
RenderQueue renderQueue(tft);
PixelFormat tftPixelFormat;
SimpleTimer guiTimer;

// Demonstrate other stuff can be done whilst rendering proceeds
OneShotFastUs sceneRenderTime;

void nextScene()
{
#ifdef ENABLE_HSPI_STATS
	debug_e("[SPI] requests %u, trans %u, wait cycles %u", spi.stats.requestCount, spi.stats.transCount,
			spi.stats.waitCycles);
#endif
	guiTimer.startOnce();
}

void lineTests()
{
	activeScene->name = F("Line Drawing Tests");

	plotLine(0, 0, 100, 100);
	plotEllipseRect(20, 20, 200, 150);
	plotQuadBezier(0, 50, 100, 50, 50, 150);

	context->offset.x += 50;
	context->color = Color::Green;
	plotLine(0, 0, 100, 100);
	plotEllipseRectAA(20, 20, 200, 150);
	plotQuadBezierSegAA(0, 150, 30, 100, 200, 0);

	context->offset.y += 50;
	context->color = Color::Orange;
	plotQuadRationalBezierSegAA(0, 150, 30, 100, 200, 0, 100);

	// plotQuadRationalBezierSegAA
}

const InterruptCallback functionList[] PROGMEM{
	lineTests,
};

void run()
{
	static uint8_t state;

	if(state >= ARRAY_SIZE(functionList)) {
		state = 0;
	}

	tft.setOrientation(landscape);
	Rect r = tft.getSize();
	activeScene = new SceneObject(r.size());
	activeScene->objects.add(new FilledRectObject(Color::Black, r));

	// auto stream = new FileStream("tmp.bin", File::CreateNewAlways | File::ReadWrite);
	auto stream = new MemoryDataStream;
	stream->ensureCapacity(12000); // Speeds construction considerably

	context = new DrawingContext(*stream);
	OneShotFastUs timer;
	functionList[state++]();
	auto elapsed = timer.elapsedTime();
	stream->seekFrom(0, SeekOrigin::Start);
	debug_i("Drawing took %s to construct, contains %u points in %u bytes", elapsed.toString().c_str(),
			context->pixelCount, stream->available());
	delete context;
	context = nullptr;

	activeScene->objects.add(new DrawingObject(stream));

	TextBuilder text(*activeScene);
	text.setStyle(FontStyle::HLine);
	text.setScale(2);
	text.setLineAlign(Align::Bottom);
	text.print(activeScene->name.c_str());
	text.commit(*activeScene);

	sceneRenderTime.start();
	auto callback = [](SceneObject* scene) {
		auto elapsed = sceneRenderTime.elapsedTime();
		String name = scene->name.c_str();
		delete scene;
		debug_i("Scene '%s' render complete in %s, free heap = %u", name.c_str(), elapsed.toString().c_str(),
				system_get_free_heap_size());
		nextScene();
	};
	renderQueue.render(activeScene, callback);
}

} // namespace

void setPixel(int x0, int y0)
{
	context->setPixel(x0, y0);
}

void setPixel3d(int x0, int y0, int z0)
{
}

void setPixelAA(int x0, int y0, uint8_t blend)
{
	context->setPixelAA(x0, y0, 255 - blend);
}

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

#ifndef DISABLE_WIFI
	//WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(false);
	WifiAccessPoint.enable(false);
#endif

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	spiffs_mount();

	Serial.println("Display start");
#ifdef ENABLE_VIRTUAL_SCREEN
	tft.begin();
	// tft.setMode(Display::Virtual::Mode::Debug);
#else

	// Touch CS
	pinMode(TOUCH_CS_PIN, OUTPUT);
	digitalWrite(TOUCH_CS_PIN, HIGH);

	spi.begin();

	/*
	 * ILI9341 min. clock cycle is 100ns write, 150ns read.
	 * In practice, writes work at 40MHz, reads at 27MHz.
	 * Attempting to read at 40MHz results in colour corruption.
	 */
	tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 27000000);
#endif

	tftPixelFormat = tft.getPixelFormat();

	guiTimer.initializeMs<5000>(run);
	run();
}
