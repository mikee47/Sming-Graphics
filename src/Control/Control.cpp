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

Control::Colors Control::getColors() const
{
	if(!flags[Flag::enabled]) {
		return Colors{
			.border = Color::Gray,
			.back = Color::DarkGray,
			.text = Color::Gray,
		};
	}

	if(flags[Flag::active]) {
		return Colors{
			.border = Color::Red,
			.back = Color::LightGray,
			.text = Color::Black,
		};
	}

	if(flags[Flag::selected]) {
		return Colors{
			.border = Color::DarkRed,
			.back = Color::Yellow,
			.text = Color::Black,
		};
	}

	return Colors{
		.border = Color::DarkRed,
		.back = Color::Gray,
		.text = Color::White,
	};
}

} // namespace Graphics
