#include <Graphics/Control/Label.h>
#include <Graphics/TextBuilder.h>

namespace Graphics
{
void Label::draw(SceneObject& scene) const
{
	auto colors = getColors();

	colors.back = Color::Black;

	Rect r = bounds.size();
	// scene.drawRect(colors.border, r);
	// r.inflate(-2, -2);
	scene.fillRect(colors.back, r);
	TextBuilder text(scene);
	// text.setFont(&statusFont);
	text.setColor(colors.text);
	text.setBackColor(colors.back);
	// text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(caption);
	text.commit(scene);
}

} // namespace Graphics
