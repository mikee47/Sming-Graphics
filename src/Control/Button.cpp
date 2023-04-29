#include <Graphics/Control/Button.h>
#include <Graphics/TextBuilder.h>

namespace Graphics
{
void Button::draw(SceneObject& scene) const
{
	auto backColor = getColor(Element::back);
	Rect r = bounds.size();
	r.inflate(-1, -1);
	scene.drawRect(Pen(getColor(Element::border), 4), r, 6);
	r.inflate(-2, -2);
	scene.fillRect(backColor, r, 6);
	TextBuilder text(scene);
	text.setColor(getColor(Element::text));
	text.setBackColor(backColor);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(caption);
	text.commit(scene);
}

} // namespace Graphics
