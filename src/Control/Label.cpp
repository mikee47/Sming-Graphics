#include <Graphics/Control/Label.h>
#include <Graphics/TextBuilder.h>

namespace Graphics
{
void Label::draw(SceneObject& scene) const
{
	auto backColor = getColor(Element::back);

	Rect r = bounds.size();
	// scene.drawRect(getColor(Element::border), r);
	// r.inflate(-2, -2);
	scene.fillRect(backColor, r);
	TextBuilder text(scene);
	text.setClip(r);
	text.setFont(getFont());
	text.setColor(getColor(Element::text));
	text.setBackColor(backColor);
	text.setTextAlign(getTextAlign());
	text.setLineAlign(Align::Centre);
	text.print(caption.c_str());
	text.commit(scene);
}

Color Label::getColor(Element element) const
{
	if(element == Element::back) {
		return Color::Black;
	}

	return Control::getColor(element);
}

} // namespace Graphics
