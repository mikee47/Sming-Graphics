#pragma once

#include "Control.h"
#include <DateTime.h>

namespace Graphics
{
class TimeClock : public Control
{
public:
	struct HMS {
		int8_t hour{0};
		int8_t min{0};
		int8_t sec{0};

		HMS()
		{
		}

		HMS(int8_t h, int8_t m, int8_t s) : hour(h), min(m), sec(s)
		{
		}

		HMS(const DateTime& dt) : hour(dt.Hour), min(dt.Minute), sec(dt.Second)
		{
		}

		bool operator==(const HMS& other) const
		{
			return hour == other.hour && min == other.min && sec == other.sec;
		}
	};

	using Control::Control;

	void update(const HMS& hms)
	{
		if(hms == next) {
			return;
		}
		next = hms;
		flags += Flag::dirty;
	}

	void update();

	void draw(SceneObject& scene) const override;

private:
	mutable HMS active;
	HMS next{};
};

} // namespace Graphics
