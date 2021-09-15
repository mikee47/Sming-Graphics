#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/Debug.h>
#include <Graphics/SampleConfig.h>

using Color = Graphics::Color;
using FontStyle = Graphics::FontStyle;

namespace
{
Graphics::RenderQueue renderQueue(tft);

// Demonstrate other stuff can be done whilst rendering proceeds
SimpleTimer backgroundTimer;
CpuCycleTimer interval;
OneShotFastMs sceneRenderTime;

using namespace Graphics;

class MandelbrotRenderer : public Renderer
{
public:
	MandelbrotRenderer(const Location& location, float zoom) : Renderer(location), zoom(zoom)
	{
	}

	bool execute(Surface& surface) override
	{
		constexpr PointF c{-0.086f, 0.85f};

		if(!started) {
			if(!surface.setAddrWindow(location.dest)) {
				return false;
			}
			PointF s(2.0f * zoom, 1.5f * zoom);
			pt1 = c - s;
			pt2 = c + s;
			pos = Point{};
			started = true;
		}

		return calculate(surface);
	}

	bool calculate(Surface& surface)
	{
		const unsigned iterations{256};

		PointF s = pt2 - pt1;

		for(; pos.y < location.dest.h; ++pos.y) {
			for(; pos.x < location.dest.w; ++pos.x) {
				if(getAlpha(color) != 0) {
					if(!surface.writePixel(color)) {
						return false;
					}
				}

				auto c = PointF(pos) * s / location.dest.size() + pt1;
				float x{0};
				float y{0};
				float xx{0};
				float yy{0};
				unsigned i;
				for(i = 0; i <= iterations && xx + yy < 4.0f; ++i) {
					xx = x * x;
					yy = y * y;
					y = (x + x) * y + c.y;
					x = xx - yy + c.x;
				}
				color = makeColor(i << 7, i << 4, i);
			}
			pos.x = 0;
		}

		return true;
	}

private:
	Point pos{};
	Color color{};
	bool started{false};
	PointF pt1;
	PointF pt2;
	float zoom;
};

class MandelbrotObject : public CustomObject
{
public:
	MandelbrotObject(const Rect& dest, float zoom = 1.0) : dest(dest), zoom(zoom)
	{
	}

	void write(MetaWriter& meta) const override
	{
		meta.write(_F("dest"), dest);
		meta.write(_F("zoom"), String(zoom, 6));
	}

	Renderer* createRenderer(const Location& location) const override
	{
		Location loc{location};
		loc.dest = dest;
		loc.dest += location.dest.topLeft();
		return new MandelbrotRenderer(loc, zoom);
	}

	Rect dest;
	float zoom;
};

bool started;
float zoom{1.0};
bool zoomOut;
SimpleTimer timer;

void render()
{
	tft.setOrientation(Orientation::deg270);
	auto scene = new SceneObject(tft, F("Mandelbrot"));

	scene->objects.clear();
	if(!started) {
		scene->clear();
		started = true;
	}

	Rect r{tft.getSize()};
	r.w /= 2;
	r.h /= 2;

	auto fixedText = new TextAsset(FS("zoom: BUSY DONE "));
	scene->assets.add(fixedText);
	TextBuilder text(*scene);
	auto add = [&](uint16_t x, uint16_t y) {
		r.x = x;
		r.y = y;
		// Info
		text.setClip(r);
		text.setTextAlign(Align::Left);
		text.setLineAlign(Align::Bottom);
		text.setColor(Color::Aqua, Color::Black);
		text.parse(*fixedText, 0, 5);
		text.print(zoom, 6);
		// Status = busy
		text.setTextAlign(Align::Right);
		text.setColor(Color::BLACK, Color::WHITE);
		text.parse(*fixedText, 5, 6);
		text.commit(*scene);
		// Mandelbrot
		auto r2 = r;
		r2.h -= text.getTextHeight() + 2;
		scene->addObject(new MandelbrotObject{r2, zoom});
		// Status = done
		text.setClip(r);
		text.setColor(Color::LightGreen, Color::Black);
		text.parse(*fixedText, 10, 6);
		text.commit(*scene);
		// Update zoom level
		if(zoomOut) {
			zoom /= 0.7;
			zoomOut = (zoom < 5.0);
		} else {
			zoom *= 0.7;
			zoomOut = (zoom < 0.000001);
		}
	};
	add(0, 0);
	add(r.w, 0);
	add(0, r.h);
	add(r.w, r.h);

	MetaWriter(Serial).write(*scene);
	// highlightText(*scene);

	sceneRenderTime.start();
	renderQueue.render(scene, [](SceneObject* scene) {
		auto elapsed = sceneRenderTime.elapsedTime();
		debug_i("Scene '%s' render complete in %s", scene->name.c_str(), elapsed.toString().c_str());
		delete scene;
		timer.initializeMs<5000>(render).startOnce();
	});
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

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	Serial.println("Display start");
	initDisplay();

	backgroundTimer.initializeMs<500>([]() {
		debug_i("Background timer %u, free heap %u", interval.elapsedTicks(), system_get_free_heap_size());
		interval.start();
	});
	backgroundTimer.start();

	render();
}
