#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/Debug.h>
#include <Graphics/Drawing/Macros.h>
#include <Graphics/SampleConfig.h>

using Color = Graphics::Color;
using FontStyle = Graphics::FontStyle;

using namespace Graphics;

namespace
{
RenderQueue renderQueue(tft);

void done(SceneObject* scene)
{
	Serial.println("Scene ready");
}

void render()
{
	auto size = tft.getSize();
	auto scene = new SceneObject(size, F("Color Tests"));
	scene->clear();

	Color color[] = {Color::RED, Color::GREEN, Color::BLUE, Color::WHITE};
	uint8_t numColors = ARRAY_SIZE(color);

	auto height = size.h / 4;
	auto width = size.w / numColors;

	TextBuilder text(*scene);
	text.setColor(Color::White, Color::Black);
	text.setScale(2);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	for(unsigned i = 0; i < numColors; i++) {
		for(unsigned j = 0; j < 4; j++) {
			PixelBuffer pix{color[i]};
			auto scale = [&](uint8_t& c) { c = c * (j + 1) / 5; };
			scale(pix.bgr24.r);
			scale(pix.bgr24.g);
			scale(pix.bgr24.b);
			Rect r(i * width, j * height, width, height);
			scene->fillRect(pix.color, r);

			text.setClip(r);
			text.printf("%c%c", 'A' + i, '0' + j);
		}
	}
	text.commit(*scene);

	renderQueue.render(scene, done);
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	Serial.println("Display start");
	initDisplay();

	render();
}
