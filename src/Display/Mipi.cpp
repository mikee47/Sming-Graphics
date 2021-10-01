#include <Graphics/Display/Mipi.h>
#include <Digital.h>

namespace Graphics
{
namespace Display
{
namespace Mipi
{
uint32_t Base::readRegister(uint8_t cmd, uint8_t byteCount)
{
	HSPI::Request req;
	req.setCommand8(cmd);
	req.dummyLen = (byteCount > 2);
	req.in.set32(0, byteCount);
	execute(req);
	return req.in.data32;
}

uint32_t Base::readDisplayId()
{
	return readRegister(DCS_GET_DISPLAY_ID, 4) >> 8;
}

uint32_t Base::readDisplayStatus()
{
	return readRegister(DCS_GET_DISPLAY_STATUS, 4);
}

uint8_t Base::readPowerMode()
{
	return readRegister(DCS_GET_POWER_MODE, 1);
}

uint8_t Base::readMADCTL()
{
	return readRegister(DCS_GET_ADDRESS_MODE, 1);
}

uint8_t Base::readPixelFormat()
{
	return readRegister(DCS_GET_PIXEL_FORMAT, 1);
}

uint8_t Base::readImageFormat()
{
	return readRegister(DCS_GET_DISPLAY_MODE, 1);
}

uint8_t Base::readSignalMode()
{
	return readRegister(DCS_GET_DISPLAY_MODE, 1);
}

uint8_t Base::readSelfDiag()
{
	return readRegister(DCS_GET_DIAGNOSTIC_RESULT, 1);
}

// Protected methods

bool IRAM_ATTR Base::transferBeginEnd(HSPI::Request& request)
{
	if(request.busy) {
		auto device = reinterpret_cast<Base*>(request.device);
		auto newState = (request.cmdLen == 0);
		if(device->dcState != newState) {
			digitalWrite(device->dcPin, newState);
			device->dcState = newState;
		}
	}
	return true;
}

} // namespace Mipi
} // namespace Display
} // namespace Graphics
