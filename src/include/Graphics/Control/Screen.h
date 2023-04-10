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

class Screen
{
public:
	using DrawMethod = Delegate<void(SceneObject& scene)>;

	Screen(RenderTarget& target) : target(target), renderQueue(target), flags(Flag::redrawFull)
	{
	}

	void input(InputEvent event, Point pos);

	void update(bool fullRedraw = false);

	void onDraw(DrawMethod method)
	{
		drawMethod = method;
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

private:
	enum class Flag {
		redraw,
		redrawFull,
		inputDown,
	};

	RenderTarget& target;
	RenderQueue renderQueue;
	DrawMethod drawMethod;
	Control::List controls;
	BitSet<uint8_t, Flag> flags;
	Control* activeControl{};
	OneShotFastMs ctrlTimer;
};

} // namespace Graphics
