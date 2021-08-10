#include <SmingCore.h>

#include <Graphics/Display/ILI9341.h>
#include <Graphics/Touch/XPT2046.h>
#include <Graphics/RenderQueue.h>
#include <Graphics/TextBuilder.h>

namespace
{
HSPI::Controller spi;
Graphics::Display::ILI9341 tft(spi);
Graphics::XPT2046 touch(spi, tft);
Graphics::RenderQueue renderQueue(tft);

using namespace Graphics;

// Pin setup
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};

constexpr HSPI::PinSet TOUCH_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TOUCH_CS{0};
constexpr uint8_t TOUCH_IRQ_PIN{2};
// constexpr uint8_t TOUCH_IRQ_PIN{PIN_NONE};

SimpleTimer backgroundTimer;
OneShotFastUs frameTimer;
NanoTime::Time<uint32_t> lastFrameTime;

IMPORT_FSTR(SMING_RAW, PROJECT_DIR "/resource/sming.raw")

// Re-usable assets
RawImageObject rawImage(SMING_RAW, PixelFormat::RGB565, {128, 128});

class TouchCalibration
{
public:
	TouchCalibration()
	{
	}

	TouchCalibration(Point ref1, Point ref2, Point target1, Point target2)
	{
		touchOrigin = ref1;
		targetOrigin = target1;
		num = target2 - target1;
		den = ref2 - ref1;
	}

	Point operator[](Point pt)
	{
		if(den.x == 0 || den.y == 0) {
			return Point{};
		}
		IntPoint p(pt);
		p -= touchOrigin;
		p *= num;
		p /= den;
		p += targetOrigin;
		return Point(p);
	}

private:
	Point touchOrigin;
	Point targetOrigin;
	Point num;
	Point den;
};

class TouchCalibrator
{
public:
	enum class State {
		reset,
		pt1,
		pt2,
		ready,
	};

	TouchCalibrator(RenderTarget& target, TouchCalibration& calib) : target(target), calib(calib)
	{
	}

	void begin()
	{
		Rect r(target.getSize());
		pt1 = r.topLeft() + cross;
		pt2 = r.bottomRight() - cross;
		surface.reset(target.createSurface(64));
		drawCross(pt1);
		ref1 = IntPoint{};
		ref2 = IntPoint{};
		sampleCount = 0;
		state = State::pt1;
	}

	bool isReady() const
	{
		return state == State::ready;
	}

	bool update(Point pos)
	{
		auto updateRef = [&](IntPoint& ref) -> bool {
			constexpr unsigned jitter{50};
			auto diff = pos - lastPos;
			lastPos = pos;
			if(abs(diff.x) > jitter || abs(diff.y) > jitter) {
				ref = IntPoint(pos);
				sampleCount = 1;
				return false;
			}
			ref += pos;
			if(++sampleCount < refSamples) {
				return false;
			}
			ref /= refSamples;
			return true;
		};

		// debug_i("update %s", pos.toString().c_str());
		switch(state) {
		case State::pt1: {
			if(!updateRef(ref1)) {
				break;
			}
			debug_i("ref1 = %s", ref1.toString().c_str());
			drawCross(pt2);
			sampleCount = 0;
			state = State::pt2;
			break;
		}
		case State::pt2: {
			if(!updateRef(ref2)) {
				break;
			}
			debug_i("ref2 = %s", ref2.toString().c_str());
			surface->reset();
			surface->clear();
			surface->present();
			calib = TouchCalibration(Point(ref1), Point(ref2), pt1, pt2);
			state = State::ready;
			break;
		}
		case State::ready:
			surface.reset();
			return true;
		case State::reset:
		default:
			debug_e("Calib::begin() not called!");
		}

		return false;
	}

private:
	///< Average a number of samples
	static constexpr unsigned refSamples{16};

	void drawCross(Point pt)
	{
		Rect r(target.getSize());
		surface->reset();
		surface->clear();
		auto color = target.getColor(Color::White);
		surface->drawHLine(color, pt.x - cross.x, pt.x + cross.x, pt.y, 1);
		surface->drawVLine(color, pt.x, pt.y - cross.y, pt.y + cross.y, 1);
		surface->present();
	}

	RenderTarget& target;
	TouchCalibration& calib;
	std::unique_ptr<Surface> surface;
	State state{};
	static constexpr Point cross{20, 20};
	Point lastPos{};
	Point pt1;
	Point pt2;
	IntPoint ref1;
	IntPoint ref2;
	uint8_t sampleCount;
};

TouchCalibration calibration;
TouchCalibrator* calibrator;

Point imgpos;
std::unique_ptr<SceneObject> scene;

void updateScreen(Point newpos)
{
	if(scene) {
		return;
	}

	frameTimer.start();

	auto screenSize = tft.getSize();
	scene.reset(new SceneObject(screenSize));

	// auto sz = rawImage.getSize();
	Size sz{20, 20};

	newpos -= Point(sz.w / 2, sz.h / 2);
	if(newpos != imgpos) {
		// canvas.fillRect(Color::Black, Rect{imgpos, sz});
		// imgpos = newpos;
		// canvas.drawImage(rawImage, imgpos);

		scene->fillRect(Color::Black, Rect{imgpos, sz});
		imgpos = newpos;
		scene->fillRect(Color::RED, Rect{imgpos, sz});
	}

	// for(unsigned i = 0; i < 100; ++i) {
	// 	auto x = os_random();
	// 	Rect r((x % screenSize.w) - sz.w, ((x >> 16) % screenSize.h) - sz.h, sz);
	// 	canvas.fillRect(makeColor(os_random()), r);
	// }

	TextBuilder text(*scene);
	// text.setScale(2);
	text.setCursor(0, 180);
	text.println(toString(touch.getState()) + "  ");
	text.println(newpos.toString() + "  ");
	text.println(lastFrameTime.toString());
	// text.print(calib.pt1.toString().c_str());
	// text.print(" -> ");
	// text.println(calib.pt2.toString().c_str());
	// text.print(lastFrameTime);
	// text.write(' ');
	text.commit(*scene);

	renderQueue.render(scene.get(), [](SceneObject*) {
		lastFrameTime = frameTimer.elapsedTime();
		scene.reset();
		// debug_i("Frame time: %s", lastFrameTime.toString().c_str());
	});
}

void touchChanged()
{
	// debug_i("touch changed: %s", toString(touch.getState()).c_str());
	auto state = touch.getState();
	if(calibrator != nullptr && calibrator->update(state.pos)) {
		delete calibrator;
		calibrator = nullptr;
	}
	updateScreen(calibration[state.pos]);
}

Timer statusTimer;

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

	Serial.println("Display start");
	spi.begin();
	tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 40000000);
	touch.begin(TOUCH_PINSET, TOUCH_CS, TOUCH_IRQ_PIN);
	touch.setOrientation(Orientation::deg270);
	touch.setCallback(touchChanged);

	calibrator = new TouchCalibrator(tft, calibration);
	calibrator->begin();

	// statusTimer.initializeMs<1000>([&]() { debug_i("Max tasks: %u", System.getMaxTaskCount()); }).start();
}
