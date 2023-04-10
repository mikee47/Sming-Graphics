#include <Graphics/Control/Button.h>
#include <Graphics/TextBuilder.h>

namespace Graphics
{
void Button::draw(SceneObject& scene) const
{
	Color borderColor;
	Color backColor;
	Color textColor;
	if(flags[Flag::active]) {
		borderColor = Color::Red;
		backColor = Color::LightGray;
		textColor = Color::Black;
	} else if(flags[Flag::enabled]) {
		if(flags[Flag::selected]) {
			borderColor = Color::DarkRed;
			backColor = Color::Yellow;
			textColor = Color::Black;
		} else {
			borderColor = Color::DarkRed;
			backColor = Color::Gray;
			textColor = Color::White;
		}
	} else {
		borderColor = Color::Gray;
		backColor = Color::DarkGray;
		textColor = Color::Gray;
	}

	scene.clear(Color::Black);
	Rect r = bounds.size();
	r.inflate(-1, -1);
	scene.drawRect(Pen(borderColor, 3), r, 3);
	r.inflate(-3, -3);
	scene.fillRect(backColor, r);
	TextBuilder text(scene);
	text.setColor(textColor);
	text.setBackColor(backColor);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(caption);
	text.commit(scene);
}

} // namespace Graphics
