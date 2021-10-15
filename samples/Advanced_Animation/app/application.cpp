#include <SmingCore.h>
#include <Graphics/LcdFont.h>
#include <Graphics/Renderer.h>
#include <Services/Profiling/MinMax.h>

#define ENABLE_TTGO_WATCH
#include <Graphics/SampleConfig.h>
#include "axp20x.h"

namespace
{
// Demonstrate other stuff can be done whilst rendering proceeds
SimpleTimer backgroundTimer;
CpuCycleTimer interval;
AXP20X_Class power;

using namespace Graphics;

class Frame
{
public:
	// Display buffers need to be large enough to hold an entire frame, determine by experimentation
	static constexpr size_t bufferSize{2048};

	enum class State {
		empty,
		ready,
		rendering,
	};

	Frame()
	{
		surface.reset(tft.createSurface(bufferSize));
	}

	std::unique_ptr<Surface> surface;
	OneShotFastMs drawTimer;
	State state{};

	void reset()
	{
		state = State::empty;
	}
};

constexpr Scale textScale{1, 2};
constexpr unsigned numStatusLines{7};
constexpr uint8_t numStatusChars{5};
char statusText[numStatusLines][numStatusChars]{};
constexpr Rect statusArea{0, 0, numStatusChars* textScale.scaleX(LcdGlyph::metrics.width),
						  numStatusLines* textScale.scaleY(LcdGlyph::metrics.height)}; ///< Keepout area
constexpr Range rectSize{5, 20};													   //< Range of sizes for rectangles
constexpr Range vector{1, 10};														   ///< Range of movement
constexpr unsigned numRectangles{40}; ///< How many rectangles to display
constexpr unsigned frameInterval{20}; ///< Time between frames in ms (0 to bypass timer for max. frame rate)
constexpr unsigned updateFrameCount{1000 / frameInterval}; ///< Update status once per second

Frame frames[2];		 ///< Use two frames: one is being renderered whilst the next is constructed
uint8_t frameIndex;		 ///< The frame currently being renderered
SimpleTimer updateTimer; ///< Start a new render when this timer fires
struct Stat {
	unsigned frameCount;	   ///< Number of frames drawn
	unsigned missedFrameCount; ///< Number of frames missed, still busy when timer fired
	unsigned overflowCount;	///< Number of frames incomplete due to buffer overflow
	size_t maxUsedSurfaceBytes;
};
Stat stat;
Profiling::MinMax32 frameTime(nullptr); ///< Time to draw frame
Size tftSize;							///< Cache display size
PixelFormat pixelFormat;				///< Cache display pixelformat

bool check(bool res)
{
	if(res) {
		return true;
	}
	debug_i("Surface full");
	++stat.overflowCount;
	return false;
}

bool fillRect(Surface& surface, Color colour, const Rect& r)
{
	return check(surface.fillRect(pack(colour, pixelFormat), r));
}

bool fillRect(Surface& surface, Color colour, const Region& rgn)
{
	for(auto& r : rgn.rects) {
		if(r && !fillRect(surface, colour, r)) {
			return false;
		}
	}
	return true;
}

class Rectangle
{
public:
	Color colour{};
	Rect r{};
	int8_t vx{0};
	int8_t vy{0};
	bool visible{false};

	void update(Surface& surface)
	{
		// Erase old rectangle (or create new one)
		if(colour == Color{}) {
			colour = ColorRange::random();
			vx = vector.random();
			vy = vector.random();
			r.w = rectSize.random();
			r.h = rectSize.random();
			r.x = Range(0, tftSize.w - r.w).random();
			r.y = Range(0, tftSize.h - r.h).random();
		} else if(visible && !fillRect(surface, Color::Black, r - statusArea)) {
			return;
		}

		// Move the rectangle
		int x = int(r.x) + vx;
		int y = int(r.y) + vy;
		if((x < 0) || (x + r.w > tftSize.w)) {
			vx = -vx;
			x += vx * 2;
		}
		if((y < 0) || (y + r.h > tftSize.h)) {
			vy = -vy;
			y += vy * 2;
		}
		r.x = x;
		r.y = y;

		auto rgn = r - statusArea;
		visible = bool(rgn);
		if(visible) {
			fillRect(surface, colour, rgn);
		}
	}
};

Rectangle rectangles[numRectangles];

void prepareFrame()
{
	auto i = 1 - frameIndex;
	auto& frame = frames[i];
	assert(frame.state == Frame::State::empty);

	auto& surface = *frame.surface.get();
	surface.reset();

	for(auto& r : rectangles) {
		r.update(surface);
	}

	// Character drawing is expensive, so update one character every frame
	auto n = stat.frameCount % updateFrameCount;
	if(n == 0) {
		// Update status block
		auto setNum = [&](unsigned i, unsigned value) { ultoa_w(value, statusText[i], 10, numStatusChars); };
		static_assert(numStatusLines == 7, "Wrong numStatusLines");
		setNum(0, frameTime.getMin());
		setNum(1, frameTime.getAverage());
		setNum(2, frameTime.getMax());
		setNum(3, stat.frameCount);
		setNum(4, stat.missedFrameCount);
		setNum(5, stat.overflowCount);
		setNum(6, stat.maxUsedSurfaceBytes);
		frameTime.clear();
	};
	auto line = n / numStatusChars;
	if(line < numStatusLines) {
		auto col = n % numStatusChars;
		Point pos(col, line);
		Size charSize = textScale.scale(LcdGlyph::metrics.size());
		pos *= Point(charSize);
		GlyphOptions options{Color::White, Color::Black, textScale, FontStyle::HLine};
		auto glyph = lcdFont.getFace(options.style)->getGlyph(statusText[line][col], options);
		surface.render(*glyph, {pos, charSize});
		delete glyph;
	}

	stat.maxUsedSurfaceBytes = std::max(stat.maxUsedSurfaceBytes, surface.stat().used);

	frame.state = Frame::State::ready;
	++stat.frameCount;
}

void renderFrame()
{
	auto& frame = frames[frameIndex];
	if(frame.state != Frame::State::ready) {
		++stat.missedFrameCount;
		return;
	}

	frame.state = Frame::State::rendering;
	frame.drawTimer.start();
	frame.surface->present([](void* param) {
		auto& frame = frames[frameIndex];
		frameIndex = 1 - frameIndex;
		frameTime.update(frame.drawTimer.elapsedTime());
		frame.reset();
		prepareFrame();
		if(frameInterval == 0) {
			renderFrame();
		}
	});
}

void initPower()
{
	Wire.begin(21, 22);

	int ret = power.begin();
	if(ret == AXP_FAIL) {
		debug_e("AXP Power begin failed");
		return;
	}

	//Change the shutdown time to 4 seconds
	power.setShutdownTime(AXP_POWER_OFF_TIME_4S);
	// Turn off the charging instructions, there should be no
	power.setChgLEDMode(AXP20X_LED_OFF);
	// Turn off external enable
	power.setPowerOutPut(AXP202_EXTEN, false);
	//axp202 allows maximum charging current of 1800mA, minimum 300mA
	power.setChargeControlCur(300);

	// #ifdef LILYGO_WATCH_HAS_SIM868
	// 		//Setting gps power
	// 		power.setPowerOutPut(AXP202_LDO3, false);
	// 		power.setLDO3Mode(0);
	// 		power.setLDO3Voltage(3300);
	// #endif /*LILYGO_WATCH_HAS_SIM868*/

	power.setLDO2Voltage(3300);

	// #ifdef LILYGO_WATCH_2020_V1
	//In the 2020V1 version, the ST7789 chip power supply
	//is shared with the backlight, so LDO2 cannot be turned off
	power.setPowerOutPut(AXP202_LDO2, AXP202_ON);
	// #endif /*LILYGO_WATCH_2020_V1*/

	// #ifdef LILYGO_WATCH_2020_V2
	// 		// New features of Twatch V2
	// 		power.limitingOff();

	// 		//GPS power domain is AXP202 LDO4
	// 		power.setPowerOutPut(AXP202_LDO4, false);
	// 		power.setLDO4Voltage(AXP202_LDO4_3300MV);

	// 		//Adding a hardware reset can reduce the current consumption of the capacitive touch
	// 		power.setPowerOutPut(AXP202_EXTEN, true);
	// 		delay(10);
	// 		power.setPowerOutPut(AXP202_EXTEN, false);
	// 		delay(8); //Trst Min = 5ms
	// 		power.setPowerOutPut(AXP202_EXTEN, true);

	// 		/*
	//             In 2020V2, LDO3 is allocated to the display and touch screen power.
	//             When ESP32 is set to sleep or other situations where power consumption needs to be saved,
	//             LDO3 can be turned off
	//             */
	// 		power.setPowerOutPut(AXP202_LDO3, false);
	// 		power.setLDO3Voltage(3300);
	// 		power.setPowerOutPut(AXP202_LDO3, true);

	// #endif /*LILYGO_WATCH_2020_V2*/

	// #ifdef LILYGO_WATCH_2020_V3
	// 		// New features of Twatch V3
	// 		power.limitingOff();

	// 		//Audio power domain is AXP202 LDO4
	// 		power.setPowerOutPut(AXP202_LDO4, false);
	// 		power.setLDO4Voltage(AXP202_LDO4_3300MV);
	// 		power.setPowerOutPut(AXP202_LDO4, true);
	// 		// No use
	// 		power.setPowerOutPut(AXP202_LDO3, false);
	// #endif /*LILYGO_WATCH_2020_V3*/
}

void setup()
{
	Serial.println("Display start");
	initDisplay();

	tft.setOrientation(Orientation::deg180);
	tftSize = tft.getSize();
	pixelFormat = tft.getPixelFormat();

	// Perform a background fill using surface directly
	auto surface = frames[0].surface.get();
	surface->fillRect(pack(makeColor(30, 30, 30), pixelFormat), tftSize);
	surface->present(); // This will block as no callback has been provided

	frameIndex = 1;
	prepareFrame();
	frameIndex = 0;
	prepareFrame();
	if(frameInterval == 0) {
		renderFrame();
	} else {
		updateTimer.initializeMs(frameInterval, renderFrame).start();
	}

	backgroundTimer.initializeMs<500>([]() {
		debug_i("Background timer %u, free heap %u", interval.elapsedTicks(), system_get_free_heap_size());
		interval.start();
	});
	backgroundTimer.start();
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

#ifndef DISABLE_WIFI
	//WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(false);
	WifiAccessPoint.enable(false);
#endif

	initPower();

	System.onReady(setup);
}
