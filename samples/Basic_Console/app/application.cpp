#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/SampleConfig.h>
#include <Graphics/Console.h>

using Color = Graphics::Color;
using FontStyle = Graphics::FontStyle;

namespace
{
Graphics::RenderQueue renderQueue(tft);
Graphics::Console console(tft, renderQueue);
SimpleTimer timer;

void test()
{
	using Section = Graphics::Console::Section;

	console.setScrollMargins(50, 50);
	console.clear();
	console.moveTo(Section::top);
	console.println("This is the top section");
	console.moveTo(Section::bottom);
	console.println("This is the bottom section");
	console.moveTo(Section::middle);
	for(unsigned i = 0; i < 10; ++i) {
		console.printf("%u This is the middle section\r\n", system_get_time());
	}
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

	return test();

	console.setScrollMargins(50, 50);
	console.setCursor({0, 50});
	console.println("Hello and welcome.");
	timer.initializeMs<1000>([]() { console.printf("%u Message goes <here>.\r\n", system_get_time()); });
	timer.start();
}
