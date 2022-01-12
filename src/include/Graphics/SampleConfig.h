/****
 * SampleConfig.h
 *
 * Common definitions for sample applications
 *
 ****/

#pragma once

#ifdef ENABLE_VIRTUAL_SCREEN
#include <Graphics/Display/Virtual.h>
#else
#include <Graphics/Display/ILI9341.h>
#endif

namespace
{
#ifdef ENABLE_VIRTUAL_SCREEN
Graphics::Display::Virtual tft;
#else
HSPI::Controller spi;
Graphics::Display::ILI9341 tft(spi);

#if defined(ARCH_ESP32)
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS_PIN{15};
#elif defined(ARCH_RP2040)
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
constexpr uint8_t TFT_CS{20};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS_PIN{15};
#else
// Pin setup
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
// Not used in this sample, but needs to be kept high
constexpr uint8_t TOUCH_CS_PIN{15};
#endif
#endif

bool initDisplay()
{
#ifdef ENABLE_VIRTUAL_SCREEN
	if(!tft.begin()) {
		return false;
	}
	// tft.setMode(Display::Virtual::Mode::Debug);
	return true;
#else
	// Touch CS
	pinMode(TOUCH_CS_PIN, OUTPUT);
	digitalWrite(TOUCH_CS_PIN, HIGH);

	if(!spi.begin()) {
		return false;
	}

	/*
	 * ILI9341 min. clock cycle is 100ns write, 150ns read.
	 * In practice, writes work at 40MHz, reads at 27MHz.
	 * Attempting to read at 40MHz results in colour corruption.
	 */
	return tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 27000000);
#endif
}

} // namespace
