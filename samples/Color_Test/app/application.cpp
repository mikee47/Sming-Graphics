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
	tft.setOrientation(Orientation::deg270);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, F("Color Tests"));
	scene->clear();

	Color color[] = {Color::RED,  Color::GREEN, Color::BLUE, Color::WHITE};
	uint8_t numColors = ARRAY_SIZE(color);

	auto height = size.h;
	auto width = size.w / numColors;

	for(unsigned i=0; i< numColors; i++) {
		scene->fillRect(i * width, 0, width, height, color[i]);
	}

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
