#include <Graphics/Control/Control.h>
#include <Graphics/Renderer.h>

namespace Graphics
{
namespace
{
class ControlRenderer : public SceneRenderer
{
public:
	ControlRenderer(const Location& location) : SceneRenderer(location, scene)
	{
	}

	SceneObject scene;
};

}; // namespace

Renderer* Control::createRenderer(const Location& location) const
{
	auto renderer = new ControlRenderer(location);
	renderer->scene.size = bounds.size();
	draw(renderer->scene);
	flags -= Flag::dirty;
	return renderer;
}

Color Control::getColor(Element element) const
{
	unsigned state = !flags[Flag::enabled] ? 0 : flags[Flag::active] ? 1 : flags[Flag::selected] ? 2 : 3;

	// Disabled, Active, Selected, Default
	const Color colors[3][4]{
		// border
		{Color::Gray, Color::Red, Color::DarkRed, Color::DarkRed},
		// back
		{Color::DarkGray, Color::LightGray, Color::Yellow, Color::Gray},
		// text
		{Color::Gray, Color::Black, Color::Black, Color::White},
	};

	return colors[unsigned(element)][state];
}

Control* Control::List::find(Point pos)
{
	for(auto& c : *this) {
		if(c.getBounds().contains(pos)) {
			return &c;
		}
	}

	return nullptr;
}

} // namespace Graphics
