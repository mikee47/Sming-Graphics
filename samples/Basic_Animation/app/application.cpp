#include <SmingCore.h>
#include <Graphics.h>
#include <Services/Profiling/MinMax.h>
#include <Graphics/SampleConfig.h>

namespace
{
using namespace Graphics;

struct Frame {
	enum class State {
		empty,
		ready,
		rendering,
	};

	SceneObject scene;
	OneShotFastMs drawTimer;
	State state{};

	void reset()
	{
		scene.objects.clear();
		scene.assets.clear();
		state = State::empty;
	}
};

constexpr Scale textScale{1, 2};
constexpr unsigned numStatusLines{5};
constexpr uint8_t numStatusChars{5};
char statusText[numStatusLines][numStatusChars]{};
constexpr Rect statusArea{0, 0, numStatusChars* textScale.scaleX(LcdGlyph::metrics.width),
						  numStatusLines* textScale.scaleY(LcdGlyph::metrics.height)}; ///< Keepout area
constexpr Range rectSize{5, 20};													   //< Range of sizes for rectangles
constexpr Range vector{1, 10};														   ///< Range of movement
constexpr unsigned numRectangles{40};	///< How many rectangles to display
constexpr unsigned frameInterval{20};	///< Time between frames in ms (0 to bypass timer for max. frame rate)
constexpr unsigned updateFrameCount{50}; ///< Update status once per second

Graphics::RenderQueue renderQueue(tft);
Frame frames[2];						///< Use two frames: one is being renderered whilst the next is constructed
uint8_t frameIndex;						///< The frame currently being renderered
SimpleTimer updateTimer;				///< Start a new render when this timer fires
unsigned frameCount;					///< Number of frames drawn
unsigned missedFrameCount;				///< Number of frames missed, still busy when timer fired
Profiling::MinMax32 frameTime(nullptr); ///< Time to draw frame
Size tftSize;							///< Cache display size

class Rectangle
{
public:
	Color colour{};
	Rect r{};
	int8_t vx{0};
	int8_t vy{0};
	bool visible{false};

	void update(SceneObject& scene)
	{
		// Erase old rectangle (or create new one)
		if(colour == Color{}) {
			Range rng{0, 255};
			colour = makeColor(rng.random(), rng.random(), rng.random());
			vx = vector.random();
			vy = vector.random();
			r.w = rectSize.random();
			r.h = rectSize.random();
			r.x = Range(0, tftSize.w - r.w).random();
			r.y = Range(0, tftSize.h - r.h).random();
		} else if(visible) {
			scene.fillRect(Color::Black, r);
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

		visible = !statusArea.contains(r.topLeft());
		if(visible) {
			scene.fillRect(colour, r);
		}
	}
};

Rectangle rectangles[numRectangles];

void prepareFrame()
{
	auto i = 1 - frameIndex;
	auto& frame = frames[i];
	assert(frame.state == Frame::State::empty);

	frame.scene.reset(tftSize);

	for(auto& r : rectangles) {
		r.update(frame.scene);
	}

	// Character drawing is expensive, so update one character every frame
	auto n = frameCount % updateFrameCount;
	if(n == 0) {
		// Update status block
		auto setNum = [&](unsigned i, unsigned value) { ultoa_w(value, statusText[i], 10, numStatusChars); };
		static_assert(numStatusLines == 5, "Wrong numStatusLines");
		setNum(0, frameTime.getMin());
		setNum(1, frameTime.getAverage());
		setNum(2, frameTime.getMax());
		setNum(3, frameCount);
		setNum(4, missedFrameCount);
		frameTime.clear();
	};
	auto line = n / numStatusChars;
	if(line < numStatusLines) {
		auto col = n % numStatusChars;

		TextBuilder text(frame.scene);
		text.setScale(textScale);
		text.setColor(Color::White, Color::Black);
		text.setStyle(FontStyle::HLine);
		text.setCursor(Point(col, line) * textScale.scale(LcdGlyph::metrics.size()));
		text.write(statusText[line][col]);
		text.commit(frame.scene);
	}

	frame.state = Frame::State::ready;
	++frameCount;
}

void renderFrame()
{
	auto& frame = frames[frameIndex];
	if(frame.state != Frame::State::ready) {
		++missedFrameCount;
		return;
	}

	frame.state = Frame::State::rendering;
	frame.drawTimer.start();
	renderQueue.render(&frame.scene, [](SceneObject*) {
		// Frame has finished rendering so we can prepare the next one
		auto& frame = frames[frameIndex];
		frameIndex = 1 - frameIndex;
		frameTime.update(frame.drawTimer.elapsedTime());
		frame.reset();
		prepareFrame();
		// Timer will render the frame when it fires, unless we're going full tilt
		if(frameInterval == 0) {
			renderFrame();
		}
	});
}

void setup()
{
	Serial.println("Display start");
	bool initOk = initDisplay();

	if(!initOk) {
		Serial.println(F("TFT initialisation failed"));
		return;
	}

	tft.setOrientation(Orientation::deg270);
	tftSize = tft.getSize();

	// Perform a background fill using surface directly
	auto surface = tft.createSurface(64);
	surface->fillRect(pack(makeColor(30, 30, 30), surface->getPixelFormat()), tftSize);
	surface->present(); // This will block as no callback has been provided
	delete surface;

	// Now build a couple of frames
	frameIndex = 1;
	prepareFrame();
	frameIndex = 0;
	prepareFrame();

	// For zero frame interval we'll render the frames using task callbacks
	if(frameInterval == 0) {
		renderFrame();
	} else {
		// Otherwise render using a timer
		updateTimer.initializeMs(frameInterval, renderFrame).start();
	}
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

	System.onReady(setup);
}
