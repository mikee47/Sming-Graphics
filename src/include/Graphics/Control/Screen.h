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

	void addControl(Control& ctrl)
	{
		controls.add(&ctrl);
	}

	void removeControl(Control& ctrl)
	{
		controls.remove(&ctrl);
	}

	Control* findControl(Point pos);

protected:
	virtual void draw(SceneObject& scene);
	virtual void handleControlEvent(ControlEvent event, Control& ctrl);

private:
	enum class Flag {
		redraw,
		redrawFull,
		inputDown,
	};

	RenderTarget& target;
	RenderQueue renderQueue;
	DrawMethod drawMethod;
	ControlMethod controlMethod;
	Control::List controls;
	BitSet<uint8_t, Flag> flags;
	Control* activeControl{};
	OneShotFastMs ctrlTimer;
};

} // namespace Graphics
