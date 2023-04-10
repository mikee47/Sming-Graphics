/****
 * SampleConfig.h
 *
 * Common definitions for sample applications
 *
 ****/

#pragma once

#ifdef ENABLE_VIRTUAL_SCREEN
#include <Graphics/Display/Virtual.h>
#include <Graphics/Touch/VirtualTouch.h>
#else
#include <Graphics/Display/ILI9341.h>
#include <Graphics/Touch/XPT2046.h>
#endif

namespace
{
#ifdef ENABLE_VIRTUAL_SCREEN
Graphics::Display::Virtual tft;
Graphics::VirtualTouch touch(tft);
#else
#if defined(ARCH_ESP32)
HSPI::SpiPins spiPins{};
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS{15};
constexpr uint8_t TOUCH_IRQ_PIN{10};
#elif defined(ARCH_RP2040)
HSPI::SpiPins spiPins{.sck = 18, .miso = 16, .mosi = 19};
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
constexpr uint8_t TFT_CS{9};
constexpr uint8_t TFT_RESET_PIN{6};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS{13};
constexpr uint8_t TOUCH_IRQ_PIN{10};
constexpr uint8_t TFT_LED_PIN{14};
#elif defined(ARCH_ESP8266) || defined(ARCH_HOST)
HSPI::SpiPins spiPins{};
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS{0};
constexpr uint8_t TOUCH_IRQ_PIN{2};
#else
#error "Unsupported SOC"
#endif

HSPI::Controller spi(spiBus, spiPins);
Graphics::Display::ILI9341 tft(spi);
Graphics::XPT2046 touch(spi, tft);
#endif

bool initDisplay()
{
#ifdef ENABLE_VIRTUAL_SCREEN
	if(!tft.begin()) {
		return false;
	}
	// tft.setMode(Display::Virtual::Mode::Debug);
	touch.begin();
	return true;
#else
	if(!spi.begin()) {
		return false;
	}

	/*
	 * ILI9341 min. clock cycle is 100ns write, 150ns read.
	 * In practice, writes work at 40MHz, reads at 27MHz.
	 * Attempting to read at 40MHz results in colour corruption.
	 */
	if(!tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 27000000)) {
		return false;
	}

	touch.begin(TFT_PINSET, TOUCH_CS, TOUCH_IRQ_PIN);

	return true;
#endif
}

} // namespace
