#pragma once

#include "Control.h"
#include "../RenderQueue.h"
#include <Data/BitSet.h>
#include <Platform/Timers.h>

namespace Graphics
{
enum class InputEvent {
	move,
	down,
	up,
};

enum class ControlEvent {
	activate,
	deactivate,
};

class Screen
{
public:
	/**
	 * @brief Invoked when screen is drawn
	 * @param scene Where to compose screen
	 * @retval bool Return true to continue default processing
	 */
	using DrawMethod = Delegate<bool(SceneObject& scene)>;

	/**
	 * @brief Invoked in response to user input
	 * @param event
	 * @param control
	 * @retval bool Return true to continue default processing
	 */
	using ControlMethod = Delegate<bool(ControlEvent event, Control& control)>;

	Screen(RenderTarget& target) : target(target), renderQueue(target), flags(Flag::redrawFull)
	{
	}

	void input(InputEvent event, Point pos);

	void update(bool fullRedraw = false);

	void onDraw(DrawMethod method)
	{
		drawMethod = method;
	}

	void onControl(ControlMethod method)
	{
		controlMethod = method;
	}

	Control::List controls;

protected:
	virtual void draw(SceneObject& scene);
	virtual void handleControlEvent(ControlEvent event, Control& ctrl);

private:
	void doUpdate();

	enum class Flag {
		redraw,
		redrawFull,
		inputDown,
	};

	RenderTarget& target;
	RenderQueue renderQueue;
	DrawMethod drawMethod;
	ControlMethod controlMethod;
	BitSet<uint8_t, Flag> flags;
	Control* activeControl{};
	OneShotFastMs ctrlTimer;
};

} // namespace Graphics
