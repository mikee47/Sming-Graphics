#include <Graphics/Touch/XPT2046.h>
#include <Digital.h>
#include <Interrupts.h>
#include <FlashString/Array.hpp>

namespace Graphics
{
namespace
{
constexpr int16_t Z_THRESHOLD{400};
constexpr int16_t Z_THRESHOLD_INT{75};
constexpr uint32_t clockSpeed{2000000};

XPT2046* isrTouch;

// Bit masks for creating control byte values
enum Control {
	// Start bit
	Start = 1 << 7,
	Stop = 0 << 7,
	// Address
	Z1 = 3 << 4,
	Z2 = 4 << 4,
	X = 5 << 4,
	Y = 1 << 4,
	// Mode
	mode12 = 0 << 3,
	mode8 = 1 << 3,
	// RefSelect
	DER = 0 << 2,
	SER = 1 << 2,
	// PowerDownMode
	PD0 = 0,
	PD1 = 1,
	PD2 = 2,
	PD3 = 3,
};

constexpr uint8_t ctrl{DER | mode12 | Start};

#define COMMAND(powermode, addr) ((DER | mode12 | Start | powermode | addr) << 8),

DEFINE_FSTR_ARRAY_LOCAL(commands, uint16_t, //
						COMMAND(PD1, Z1)	//
						COMMAND(PD1, Z2)	//
						COMMAND(PD1, X)		// To be discarded, always noisy
						COMMAND(PD1, X)		//
						COMMAND(PD1, Y)		//
						COMMAND(PD1, X)		//
						COMMAND(PD1, Y)		//
						COMMAND(PD1, X)		//
						COMMAND(PD0, Y)		//
						Stop)

uint16_t swap(uint16_t value)
{
	return (value << 8) | (value >> 8);
}

int16_t bestTwoAverage(int16_t a, int16_t b, int16_t c)
{
	int16_t ab = abs(a - b);
	int16_t ac = abs(a - c);
	int16_t bc = abs(c - b);

	if(ab <= ac && ab <= bc) {
		return (a + b) / 2;
	}

	if(ac <= ab && ac <= bc) {
		return (a + c) / 2;
	}

	return (b + c) / 2;
}

} // namespace

bool XPT2046::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t irqPin)
{
	if(!HSPI::Device::begin(pinSet, chipSelect, clockSpeed)) {
		return false;
	}

	setBitOrder(MSBFIRST);
	setClockMode(HSPI::ClockMode::mode0);
	setIoMode(HSPI::IoMode::SPI);

	timer.initializeMs<20>([](void* param) { static_cast<XPT2046*>(param)->requestUpdate(); }, this);
	timer.start();

	this->irqPin = irqPin;
	if(irqPin != PIN_NONE) {
		isrTouch = this;
		pinMode(irqPin, INPUT);
		attachInterrupt(irqPin, isr, FALLING);
	}

	return true;
}

void IRAM_ATTR XPT2046::isr()
{
	if(!isrTouch->updateRequested) {
		System.queueCallback(isrTouch->staticOnChange, isrTouch);
		isrTouch->updateRequested = true;
	}
}

void XPT2046::printBuffer(const char* tag)
{
	m_puts(tag);
	m_puts(": ");
	for(auto& c : buffer) {
		m_printf(" %04x", c);
	}
	m_puts("\r\n");
}

void XPT2046::beginUpdate()
{
	updateRequested = true;

	assert(!req.busy);

	assert(commands.length() == ARRAY_SIZE(buffer));
	commands.read(0, buffer, commands.length());

	req.out.set(buffer, sizeof(buffer));
	req.in.set(buffer, sizeof(buffer));

	req.setAsync(staticRequestComplete, this);
	execute(req);
}

void XPT2046::update()
{
	updateRequested = false;

	for(auto& c : buffer) {
		c = swap(c) >> 3;
	}

	// printBuffer(" IN");

	uint16_t z1 = buffer[1];
	uint16_t z2 = buffer[2];
	uint16_t x1 = buffer[4];
	uint16_t y1 = buffer[5];
	uint16_t x2 = buffer[6];
	uint16_t y2 = buffer[7];
	uint16_t x3 = buffer[8];
	uint16_t y3 = buffer[9];

	// debug_d("z1 %5d, z2 %5d, xy (%5d, %5d), xy (%5d, %5d), xy (%5d, %5d)", z1, z2, x1, y1, x2, y2, x3, y3);

	uint16_t z = sampleMax + z1 - z2;

	// debug_d("z %5d, z1 %5d, z2 %5d", z, z1, z2);

	if(z < Z_THRESHOLD) {
		zraw = 0;
		if(offcount == 4) {
			if(irqPin != PIN_NONE) {
				timer.stop();
			}
			if(callback) {
				callback();
			}
		} else {
			++offcount;
		}
		return;
	}

	offcount = 0;
	if(!timer.isStarted()) {
		timer.start();
	}

	zraw = z;

	int16_t x = bestTwoAverage(x1, x2, x3);
	int16_t y = bestTwoAverage(y1, y2, y3);

	// debug_d("z %5d, z1 %5d, z2 %5d, x %5d, y %5d", z, z1, z2, x, y);

	switch(orientation) {
	case Orientation::deg90:
		xraw = sampleMax - y;
		yraw = sampleMax - x;
		break;
	case Orientation::deg180:
		xraw = sampleMax - x;
		yraw = y;
		break;
	case Orientation::deg270:
		xraw = y;
		yraw = x;
		break;
	case Orientation::normal:
	default:
		xraw = x;
		yraw = sampleMax - y;
	}

	if(callback) {
		callback();
	}
}

} // namespace Graphics
