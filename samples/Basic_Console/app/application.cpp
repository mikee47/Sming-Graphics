#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/SampleConfig.h>
#include <Graphics/Console.h>

using Color = Graphics::Color;
using FontStyle = Graphics::FontStyle;

Graphics::RenderQueue renderQueue(tft);
Graphics::Console console(tft, renderQueue);
SimpleTimer timer;

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	Serial.println("Display start");
	initDisplay();

	console.println("Hello and welcome.");
	timer.initializeMs<250>([]() { console.printf("%u Message goes <here>.\r\n", system_get_time()); });
	timer.start();
}
