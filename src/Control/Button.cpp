#include <Graphics/Control/Button.h>
#include <Graphics/TextBuilder.h>

namespace Graphics
{
void Button::draw(SceneObject& scene) const
{
	auto colors = getColors();
	Rect r = bounds.size();
	r.inflate(-1, -1);
	scene.drawRect(Pen(colors.border, 4), r, 6);
	r.inflate(-2, -2);
	scene.fillRect(colors.back, r, 6);
	TextBuilder text(scene);
	text.setColor(colors.text);
	text.setBackColor(colors.back);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(caption);
	text.commit(scene);
}

} // namespace Graphics
