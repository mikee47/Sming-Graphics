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
	class List : public LinkedObjectListTemplate<Control>
	{
	public:
		Control* find(Point pos);
	};

	enum class Flag {
		enabled,  ///< Can be interacted with
		active,   ///< i.e. pressed
		selected, ///< e.g. ON
		dirty,	///< Requires repainting
	};

	enum class Element {
		border,
		back,
		text,
	};

	Control() : bounds(0, 0, 100, 50)
	{
	}

	Control(const Rect& bounds) : bounds(bounds)
	{
	}

	Control(const Rect& bounds, const String& caption) : bounds(bounds), caption(caption)
	{
	}

	Renderer* createRenderer(const Location& location) const override;

	virtual void draw(SceneObject& scene) const = 0;

	void write(MetaWriter& meta) const override
	{
	}

	String getCaption() const
	{
		return caption.c_str();
	}

	void setCaption(const String& value)
	{
		if(caption == value) {
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

	void setDirty()
	{
		flags += Flag::dirty;
	}

	virtual Font* getFont() const
	{
		return nullptr;
	}

	virtual Color getColor(Element element) const;

	virtual Align getTextAlign() const
	{
		return Align::Near;
	}

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
	CString caption;
	mutable BitSet<uint8_t, Flag> flags;
};

} // namespace Graphics
