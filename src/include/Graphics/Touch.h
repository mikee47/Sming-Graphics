#pragma once

#include "Types.h"

namespace Graphics
{
class Device;

class Touch
{
public:
	struct State {
		Point pos;
		uint16_t pressure;
	};

	/**
     * @brief Callback function
     */
	using Callback = Delegate<void()>;

	Touch(Device* device = nullptr) : device(device)
	{
	}

	/**
	 * @brief Set display orientation
	 */
	virtual bool setOrientation(Orientation orientation);

	/**
	 * @brief Get physical size of display
	 * @retval Size Dimensions for NORMAL orientation
	 */
	virtual Size getNativeSize() const = 0;

	/**
     * @brief Get current state
     */
	virtual State getState() const = 0;

	/**
     * @brief Register callback to be invoked when state changes
     */
	virtual void setCallback(Callback callback)
	{
		this->callback = callback;
	}

	/**
	 * @brief Get native dimensions for current orientation
	 */
	Size getSize() const
	{
		auto size = getNativeSize();
		if(orientation == Orientation::deg90 || orientation == Orientation::deg270) {
			std::swap(size.w, size.h);
		}
		return size;
	}

	/**
	 * @brief Get current display orientation
	 */
	Orientation getOrientation()
	{
		return orientation;
	}

protected:
	Device* device;
	Orientation orientation{};
	Callback callback;
};

} // namespace Graphics

String toString(Graphics::Touch::State state);
