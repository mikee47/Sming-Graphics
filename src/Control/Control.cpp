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

} // namespace Graphics
