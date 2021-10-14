/**
 * SampleConfig.h
 *
 * Common definitions for sample applications
 *
 ****/

#pragma once

#ifdef ENABLE_VIRTUAL_SCREEN
#include <Graphics/Display/Virtual.h>
#elif defined(ENABLE_TTGO_WATCH)
#ifndef SUBARCH_ESP32
static_assert(false, "TTGO watch is regular ESP32 chip");
#endif
#include <Graphics/Display/ST7789V.h>
#else
#include <Graphics/Display/ILI9341.h>
#endif

namespace
{
#ifdef ENABLE_VIRTUAL_SCREEN
Graphics::Display::Virtual tft;
#else

#ifdef ENABLE_TTGO_WATCH
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
HSPI::SpiPins spiPins = {
	.sck = 18,
	.miso = HSPI::SPI_PIN_NONE,
	.mosi = 19,
};
constexpr uint8_t TFT_CS{5};
constexpr uint8_t TFT_RESET_PIN{Graphics::PIN_NONE};
constexpr uint8_t TFT_DC_PIN{27};
constexpr uint8_t TFT_BL_PIN{12};
constexpr uint8_t TOUCH_CS_PIN{Graphics::PIN_NONE};

HSPI::Controller spi(spiBus, spiPins);
Graphics::Display::ST7789V tft(spi, {240, 240});

#elif defined(ARCH_ESP32)
constexpr HSPI::SpiBus spiBus{HSPI::SpiBus::DEFAULT};
HSPI::SpiPins spiPins{};
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::normal};
#ifdef CONFIG_IDF_TARGET_ESP32
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS_PIN{15};
constexpr uint8_t TFT_BL_PIN{Graphics::PIN_NONE};
#elif defined CONFIG_IDF_TARGET_ESP32S2
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS_PIN{15};
constexpr uint8_t TFT_BL_PIN{Graphics::PIN_NONE};
#elif defined CONFIG_IDF_TARGET_ESP32C3
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
constexpr uint8_t TOUCH_CS_PIN{15};
constexpr uint8_t TFT_BL_PIN{Graphics::PIN_NONE};
#endif

HSPI::Controller spi(spiBus, spiPins);
Graphics::Display::ILI9341 tft(spi);

#else
// Pin setup
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
// Not used in this sample, but needs to be kept high
constexpr uint8_t TOUCH_CS_PIN{15};
constexpr uint8_t TFT_BL_PIN{Graphics::PIN_NONE};
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
	if(TOUCH_CS_PIN != Graphics::PIN_NONE) {
		pinMode(TOUCH_CS_PIN, OUTPUT);
		digitalWrite(TOUCH_CS_PIN, HIGH);
	}

	if(!spi.begin()) {
		return false;
	}

	/*
	 * ILI9341 min. clock cycle is 100ns write, 150ns read.
	 * In practice, writes work at 40MHz, reads at 27MHz.
	 * Attempting to read at 40MHz results in colour corruption.
	 */
	if(!tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 40000000)) {
		return false;
	}

	// Backlight
	if(TFT_BL_PIN != Graphics::PIN_NONE) {
		pinMode(TFT_BL_PIN, OUTPUT);
		digitalWrite(TFT_BL_PIN, HIGH);
	}

	return true;
#endif
}

} // namespace
