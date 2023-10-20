#pragma once

#include "Control.h"

namespace Graphics
{
/**
 * @brief Basic interactive button on screen
 *
 * Responsible for drawing area within designated rectangle
 */
class Button : public Control
{
public:
	using Control::Control;

	void draw(SceneObject& scene) const override;
};

} // namespace Graphics
