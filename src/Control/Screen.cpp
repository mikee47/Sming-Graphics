#include <Graphics/Control/Screen.h>

#define CLICK_TIME 100

namespace Graphics
{
void Screen::update(bool fullRedraw)
{
	flags += Flag::redraw;
	if(fullRedraw) {
		flags += Flag::redrawFull;
	}

	if(renderQueue.isActive()) {
		return;
	}

	auto scene = new SceneObject(target);
	if(flags[Flag::redrawFull]) {
		scene->clear();
	}

	flags -= Flag::redraw | Flag::redrawFull;

	draw(*scene);

	renderQueue.render(scene, [this](SceneObject* scene) {
		delete scene;
		if(flags[Flag::redraw]) {
			update(flags[Flag::redrawFull]);
		}
	});
}

void Screen::draw(SceneObject& scene)
{
	if(drawMethod) {
		drawMethod(scene);
	}

	for(auto& c : controls) {
		if(c.isDirty()) {
			scene.drawObject(c, c.getBounds());
		}
	}
}

Control* Screen::findControl(Point pos)
{
	for(auto& c : controls) {
		if(c.getBounds().contains(pos)) {
			return &c;
		}
	}

	return nullptr;
}

void Screen::input(InputEvent event, Point pos)
{
	switch(event) {
	case InputEvent::down: {
		if(!flags[Flag::inputDown]) {
			assert(!activeControl);
			auto ctrl = findControl(pos);
			if(ctrl && ctrl->isEnabled()) {
				ctrl->setFlag(Control::Flag::active, true);
				activeControl = ctrl;
				debug_i("ACTIVATE %s", ctrl->getCaption().c_str());
				ctrlTimer.reset<CLICK_TIME>();
				handleControlEvent(ControlEvent::activate, *ctrl);
			}
		}
		flags += Flag::inputDown;
		update();
		break;
	}

	case InputEvent::up: {
		if(activeControl) {
			auto ctrl = findControl(pos);
			bool isActiveControl = (ctrl == activeControl);
			activeControl->setFlag(Control::Flag::active, false);
			activeControl = nullptr;
			if(isActiveControl && ctrlTimer.expired()) {
				debug_i("DECACTIVATE %s", ctrl->getCaption().c_str());
				handleControlEvent(ControlEvent::deactivate, *ctrl);
			}
		}
		flags -= Flag::inputDown;
		update();
		break;
	}

	case InputEvent::move: {
		break;
	}
	}
}

void Screen::handleControlEvent(ControlEvent event, Control& ctrl)
{
	if(controlMethod && !controlMethod(event, ctrl)) {
		return;
	}

	switch(event) {
	case ControlEvent::activate:
		break;

	case ControlEvent::deactivate:
		ctrl.select(!ctrl.isSelected());
		break;
	}
}

} // namespace Graphics
