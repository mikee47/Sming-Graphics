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

bool Base::begin(HSPI::PinSet pinSet, uint8_t chipSelect, uint8_t dcPin, uint8_t resetPin, uint32_t clockSpeed)
{
	if(!SpiDisplay::begin(pinSet, chipSelect, resetPin, clockSpeed)) {
		return false;
	}

	this->dcPin = dcPin;
	pinMode(dcPin, OUTPUT);
	digitalWrite(dcPin, HIGH);
	dcState = true;
	onTransfer(transferBeginEnd);

	return initialise() && setOrientation(orientation);
}

bool Base::setOrientation(Orientation orientation)
{
	uint8_t mode = defaultAddressMode;
	switch(orientation) {
	case Orientation::deg0:
		addrOffset = Point{};
		break;
	case Orientation::deg90:
		mode ^= DCS_ADDRESS_MODE_MIRROR_X | DCS_ADDRESS_MODE_SWAP_XY;
		addrOffset = Point{};
		break;
	case Orientation::deg180:
		mode ^= DCS_ADDRESS_MODE_MIRROR_X | DCS_ADDRESS_MODE_MIRROR_Y;
		addrOffset = Point(0, resolution.h - nativeSize.h);
		break;
	case Orientation::deg270:
		mode ^= DCS_ADDRESS_MODE_SWAP_XY | DCS_ADDRESS_MODE_MIRROR_Y;
		addrOffset = Point(resolution.h - nativeSize.h, 0);
		break;
	default:
		return false;
	}

	SpiDisplayList list(Mipi::commands, addrWindow, 16);
	list.writeCommand(Mipi::DCS_SET_ADDRESS_MODE, mode, 1);
	execute(list);
	this->orientation = orientation;
	return true;
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
