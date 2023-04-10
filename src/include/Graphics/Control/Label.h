#pragma once

#include "Control.h"

namespace Graphics
{
/**
 * @brief Non-interactive text label
 */
class Label : public Control
{
public:
	using Control::Control;

	void draw(SceneObject& scene) const override;
};

} // namespace Graphics
