#include <Graphics/Control/TimeClock.h>
#include <SystemClock.h>
#include <cmath>

namespace Graphics
{
void TimeClock::update()
{
	DateTime dt = SystemClock.now();
	update(dt);
}

void TimeClock::draw(SceneObject& scene) const
{
	Point c = bounds.centre();
	int radOuter = std::min(c.x, c.y);
	int radInner = 6;
	// scene.fillCircle(Color::Black, c, radOuter);
	scene.drawCircle(Pen(Color::DarkGray, 5), c, radOuter);
	scene.drawCircle(Pen(Color::DarkGray, 2), c, radInner);

	auto lPoint = [&](float a, int r) -> Point { return c + Point(r * std::cos(a), r * std::sin(a)); };

	auto aline = [&](const Pen& pen, int r1, int r2, int value, int maxValue) {
		float a = value * PI * 2 / maxValue;
		a -= PI / 2;
		scene.drawLine(pen, lPoint(a, r1), lPoint(a, r2));
	};

	for(int i = 0; i < 12; ++i) {
		aline(Pen(Color::DarkGray, (i % 3) ? 1 : 5), radOuter - 10, radOuter - 1, i, 12);
	}

	radInner += 3;
	radOuter -= 10;

	auto drawHour = [&](Color color, const HMS& hms) {
		if(hms.hour < 0) {
			return;
		}
		int mins = hms.min + (hms.hour % 12) * 60;
		aline(Pen(color, 5), radInner, 6 * radOuter / 8, mins, 60 * 12);
	};

	auto drawMinute = [&](Color color, const HMS& hms) {
		if(hms.min < 0) {
			return;
		}
		int secs = hms.sec + hms.min * 60;
		aline(Pen(color, 3), radInner, 7 * radOuter / 8, secs, 60 * 60);
	};
	auto drawSecond = [&](Color color, const HMS& hms) {
		if(hms.sec < 0) {
			return;
		}
		aline(color, radInner, radOuter, hms.sec, 60);
	};

	drawHour(Color::Black, active);
	drawHour(Color::White, next);
	drawMinute(Color::Black, active);
	drawMinute(Color::White, next);
	drawSecond(Color::Black, active);
	drawSecond(Color::White, next);
	active = next;
}

} // namespace Graphics
