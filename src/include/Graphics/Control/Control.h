#pragma once

#include "../Scene.h"

namespace Graphics
{
/**
 * @brief Basic interactive button on screen
 *
 * Responsible for drawing area within designated rectangle
 */
class Control : public CustomObject
{
public:
	using List = LinkedObjectListTemplate<Control>;

	enum class Flag {
		enabled,  ///< Can be interacted with
		active,   ///< i.e. pressed
		selected, ///< e.g. ON
		dirty,	///< Requires repainting
	};

	struct Colors {
		Color border;
		Color back;
		Color text;
	};

	Control() : bounds(0, 0, 100, 50)
	{
	}

	Control(const Rect& bounds) : bounds(bounds)
	{
	}

	Renderer* createRenderer(const Location& location) const override;

	virtual void draw(SceneObject& scene) const = 0;

	void write(MetaWriter& meta) const override
	{
	}

	const String& getCaption() const
	{
		return caption;
	}

	void setCaption(const String& value)
	{
		if(value == caption) {
			return;
		}
		this->caption = value;
		flags += Flag::dirty;
	}

	void enable(bool state)
	{
		setFlag(Flag::enabled, state);
	}

	void select(bool state)
	{
		setFlag(Flag::selected, state);
	}

	void setPos(Point pos)
	{
		setBounds(Rect{pos, bounds.size()});
	}

	void resize(Size size)
	{
		setBounds(Rect{bounds.topLeft(), size});
	}

	void setBounds(const Rect& r)
	{
		if(r == bounds) {
			return;
		}
		bounds = r;
		flags += Flag::dirty;
	}

	Rect getBounds() const
	{
		return bounds;
	}

	bool isEnabled() const
	{
		return flags[Flag::enabled];
	}

	bool isSelected() const
	{
		return flags[Flag::selected];
	}

	bool isDirty() const
	{
		return flags[Flag::dirty];
	}

	Colors getColors() const;

protected:
	friend class Screen;

	void setFlag(Flag flag, bool state)
	{
		if(state == flags[flag]) {
			return;
		}
		flags[flag] = state;
		flags += Flag::dirty;
	}

	Rect bounds;
	String caption;
	mutable BitSet<uint8_t, Flag> flags;
};

} // namespace Graphics
