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
