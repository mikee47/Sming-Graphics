#include <Graphics/Touch/VirtualTouch.h>
#include <Graphics/Display/Virtual.h>
#include <Platform/System.h>

namespace Graphics
{
enum Button : uint32_t {
	SDL_BUTTON_LEFT,
	SDL_BUTTON_MIDDLE,
	SDL_BUTTON_RIGHT,
	SDL_BUTTON_X1,
	SDL_BUTTON_X2,
};
using Buttons = BitSet<uint32_t, Button, 5>;

struct TouchInfo {
	Buttons state;
	uint16_t x;
	uint16_t y;
};

String toString(Button btn)
{
	switch(btn) {
	case SDL_BUTTON_LEFT:
		return "LEFT";
	case SDL_BUTTON_MIDDLE:
		return "MIDDLE";
	case SDL_BUTTON_RIGHT:
		return "RIGHT";
	case SDL_BUTTON_X1:
		return "X1";
	case SDL_BUTTON_X2:
		return "X2";
	default:
		return String(unsigned(btn));
	}
}

VirtualTouch::VirtualTouch(Display::Virtual& display) : Touch(&display)
{
	display.onTouch([this](const void* buffer, size_t length) {
		if(length != sizeof(TouchInfo)) {
			debug_e("[TOUCH] Size mismatch: expected %u, got %u", sizeof(TouchInfo), length);
			debug_hex(ERR, "TOUCH", buffer, length);
			return;
		}
		auto& t = *static_cast<const TouchInfo*>(buffer);
		debug_d("[TOUCH] buttons [%s], pos (%u, %u)", toString(t.state).c_str(), t.x, t.y);
		state.pressure = t.state[SDL_BUTTON_LEFT] ? 1500 : 0;
		state.pos = Point(t.x, t.y);
		if(callback) {
			System.queueCallback([](void* param) { static_cast<VirtualTouch*>(param)->callback(); }, this);
		}
	});
}

Size VirtualTouch::getNativeSize() const
{
	return device->getNativeSize();
}

} // namespace Graphics

String toString(Graphics::Button btn)
{
	return Graphics::toString(btn);
}
