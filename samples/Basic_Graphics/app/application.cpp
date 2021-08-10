#include <SmingCore.h>
#include <Graphics.h>
#include <Graphics/Debug.h>
#include <Graphics/Display/Null.h>
#include <Graphics/Drawing/Macros.h>
#include <FlashString/Array.hpp>
#include <RapidXML.h>
#include <Storage/PartitionStream.h>
#include <resource.h>

#ifdef ENABLE_MALLOC_COUNT
#include <malloc_count.h>
#endif

#ifdef ARCH_HOST
#include <Storage/FileDevice.h>
#endif

#ifdef ENABLE_VIRTUAL_SCREEN
#include <Graphics/Display/Virtual.h>
#else
#include <Graphics/Display/ILI9341.h>
#endif

using namespace Graphics;

namespace
{
#ifdef ENABLE_VIRTUAL_SCREEN
Display::Virtual tft;
#else
HSPI::Controller spi;
Display::ILI9341 tft(spi);

// Pin setup
constexpr HSPI::PinSet TFT_PINSET{HSPI::PinSet::overlap};
constexpr uint8_t TFT_CS{2};
constexpr uint8_t TFT_RESET_PIN{4};
constexpr uint8_t TFT_DC_PIN{5};
// Not used in this sample, but needs to be kept high
constexpr uint8_t TOUCH_CS_PIN{15};
#endif

constexpr Orientation portrait{Orientation::deg180};
constexpr Orientation landscape{Orientation::deg270};

RenderQueue renderQueue(tft);
PixelFormat tftPixelFormat;
SimpleTimer guiTimer;

// Demonstrate other stuff can be done whilst rendering proceeds
SimpleTimer backgroundTimer;
OneShotFastUs interval;
OneShotFastUs sceneRenderTime;

void nextScene()
{
#ifdef ENABLE_HSPI_STATS
	debug_e("[SPI] requests %u, trans %u, wait cycles %u", spi.stats.requestCount, spi.stats.transCount,
			spi.stats.waitCycles);
#endif
	guiTimer.startOnce();
}

DEFINE_FSTR_LOCAL(TMPIMAGE_FILE, "tmpimage.raw")

class BasicGui
{
public:
	void show();

private:
	int r{0};
	int ara{4};
	int yerara{15};
	int u1{100};
	int u2 = 320 - (u1 + ara);
	int s1{0};
	int s2 = u1 + ara;

	int g{28};
	int y{90};
	int satir{6};
};

BasicGui gui;

// Re-usable assets
BitmapObject* bitmap;
RawImageObject* rawImage;
RawImageObject* heron;
constexpr Size targetSymbolSize{50, 50};
SceneObject targetSymbol(targetSymbolSize, "target");
ResourceFont fontSans(Resource::freeSans9pt);

void render(SceneObject* scene, SceneObject::Callback callback = nullptr)
{
	TextBuilder text(*scene);
	text.setScale(2);
	text.setStyle(FontStyle::HLine);
	text.setLineAlign(Align::Bottom);
	text.print(scene->name.c_str());

	text.commit(*scene);

	// MetaWriter(Serial).write(*scene);

	sceneRenderTime.start();
	if(!callback) {
		callback = [](SceneObject* scene) {
			auto elapsed = sceneRenderTime.elapsedTime();
			debug_i("Scene '%s' render complete in %s", scene->name.c_str(), elapsed.toString().c_str());
			delete scene;
			nextScene();
		};
	}
	renderQueue.render(scene, callback);
}

void imageTests(ImageObject& image, const String& name)
{
	tft.setOrientation(landscape);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, name);
	scene->clear();

	// Draw 4 images
	for(unsigned i = 0; i < 4; i++) {
		auto pt = Point(size) * i / 4;
		scene->drawImage(image, pt);
	}

	render(scene);
}

void BasicGui::show()
{
	PSTR_ARRAY(lists, "abcdef")

	tft.setOrientation(landscape);

	auto scene = new SceneObject(tft, F("Basic GUI"));
	scene->clear();
	scene->fillRect(s1, 0, u1 * 2, 48, Color::OLIVE);
	scene->fillRect((u1 * 2) + ara, 0, 318 - (u1 * 2), 48, Color::RED);
	int p1{50};
	for(int a = 0; a < satir; a++) {
		scene->fillRect(s1, p1, u1, g, Color::DARKCYAN);
		scene->fillRect(s2, p1, u2, g, Color::DARKCYAN);
		p1 += g + 4;
	}

	TextBuilder text(*scene);
	text.setCursor(22, 15);
	text.setColor(Color::WHITE);
	text.setWrap(false);
	text.setStyle(FontStyle::DotMatrix);
	text.setScale(3);
	text.print(F("Sming is the framework we all like to use"));

	text.setScale(2);
	p1 = 50;
	for(int a = 0; a < satir; a++) {
		text.setCursor(s1 + yerara, p1 + 6);
		text.print(lists[a]);
		text.setCursor(s2 + yerara, p1 + 6);
		text.print(r);
		p1 += g + 4;
	}
	r++;

	text.commit(*scene);

	// highlightText(*scene);

	render(scene);
}

void startPage()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Start Page"));
	scene->clear();

	TextBuilder text(*scene);
	text.setFont(fontSans);
	// text.setScale(2, 3);

	text.setColor(Color::Black, Color::White);
	text.setTextAlign(Align::Centre);

	constexpr FontStyles baseStyle{0}; //FontStyle::HLine};

	text.setStyle(baseStyle);

	text.setColor(Color::Yellow, Color::DarkRed);
	text.setCursor(0, 10);

	text.print("This is ");
	text.setStyle(baseStyle | FontStyle::Bold);
	text.print("bold, ");
	text.setStyle(baseStyle | FontStyle::Italic);
	text.print("italic, ");
	text.setStyle(baseStyle | FontStyle::Bold | FontStyle::Italic);
	text.print("bold-italic");
	text.setStyle(baseStyle);
	text.println(".");

	text.setColor(Color::Violet);
	text.println(F(" Sming Framework "));
	text.setColor(Color::WHITE, Color::Gray);
	text.println(SMING_VERSION);
	text.setColor(Color::CYAN);
	// text.setScale(3);
	text.println(tft.getName());

	text.setColor(Color::DarkSeaGreen, Color::BLACK);
	// text.setScale(1);
	text.print("This is ");
	text.setStyle(baseStyle | FontStyle::Bold);
	text.print("bold, ");
	text.setStyle(baseStyle | FontStyle::Italic);
	text.print("italic, ");
	text.setStyle(baseStyle | FontStyle::Bold | FontStyle::Italic);
	text.print("bold-italic");
	text.setStyle(baseStyle);
	text.println(".");

	text.setColor(Color::DarkSeaGreen);
	// text.setScale(1);
	text.print("This is ");
	text.setStyle(baseStyle | FontStyle::Bold);
	text.print("bold, ");
	text.setStyle(baseStyle | FontStyle::Italic);
	text.print("italic, ");
	text.setStyle(baseStyle | FontStyle::Bold | FontStyle::Italic);
	text.print("bold-italic");
	text.setStyle(baseStyle);
	text.println(".");

	text.commit(*scene);

	highlightText(*scene);

	render(scene);
}

void textTests()
{
	tft.setOrientation(landscape);

	auto scene = new SceneObject(tft, F("Text tests"));
	scene->clear();

	Rect r(10, 20, 120, 90);
	r.inflate(5);
	scene->fillEllipse(Color::Maroon, r);
	r.inflate(3);
	scene->drawEllipse(Pen{Color::Yellow, 3}, r);
	r.inflate(-7);
	TextBuilder text(*scene);
	text.setClip(r);
	text.setFont(fontSans);
	text.setColor(Color::White);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.addStyle(FontStyle::Underscore);
	text.print(F("This is some centred text"));
	text.removeStyle(FontStyle::Underscore);

	r = Rect(180, 120, 110, 110);
	text.setClip(r);
	text.setTextAlign(Align::Left);
	text.print(F("This is some text which should be wrapped."));
	r.inflate(8);
	scene->drawRect(Pen(Color::Cyan, 3), r, 10);

	r = Rect(150, 10, 100, 80);
	text.setClip(r);
	text.setTextAlign(Align::Right);
	text.setLineAlign(Align::Bottom);
	text.print(F("Text at\r\nBottom"));
	r.inflate(7);
	scene->drawRect(Pen(Color::Red, 2), r);

	r = Rect(10, 150, 100, 50);
	text.setClip(r);
	text.setFont(lcdFont);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(F("Text\r\n\n"));
	text.addStyle(FontStyle::Underscore);
	text.print(F("Middle Empty"));
	r.inflate(5);
	scene->drawRect(Color::LightSeaGreen, r);

	text.commit(*scene);

	highlightText(*scene);

	render(scene);
}

void parse(SceneObject& scene, TextBuilder& text, XML::Node* node)
{
	CStringArray tags(F("b\0"
						"i\0"
						"u\0"
						"br\0"
						"p\0"));

	auto oldOptions = text.getOptions();

	if(node->type() == XML::NodeType::node_element) {
		switch(tags.indexOf(node->name())) {
		case 0:
			text.addStyle(FontStyle::Bold);
			break;
		case 1:
			text.addStyle(FontStyle::Italic);
			break;
		case 2:
			text.addStyle(FontStyle::Underscore);
			break;
		case 3:
			text.print("\r\n");
			break;
		case 4:
			text.print("\r\n");
			text.moveCursor(0, 5);
			break;
		}
		for(auto attr = node->first_attribute(); attr != nullptr; attr = attr->next_attribute()) {
			String s(attr->value(), attr->value_size());
			s.replace(';', '\0');
			CStringArray sz(std::move(s));
			for(auto e : sz) {
				auto sep = strchr(e, ':');
				if(sep == nullptr) {
					continue;
				}
				String tag(e, sep - e);
				auto value = sep + 1;
				if(tag.equalsIgnoreCase(F("color"))) {
					Color color;
					if(fromString(value, color)) {
						text.setForeColor(color);
					}
				} else if(tag.equalsIgnoreCase(F("background-color"))) {
					Color color;
					if(fromString(value, color)) {
						text.setBackColor(color);
					}
				}
				debug_i("ELEM: %s = %s", tag.c_str(), value);
			}
		}
	} else {
		text.write(node->value(), node->value_size());
	}

	node = node->first_node();
	while(node != nullptr) {
		parse(scene, text, node);
		node = node->next_sibling();
	}

	text.setStyle(oldOptions.style);
	text.setColor(oldOptions.fore);
};

void htmlTextTest()
{
	tft.setOrientation(landscape);

	DEFINE_FSTR_LOCAL(
		FS_HTML, //
		"<html>"
		"Excerpt from <u style=\"color:lightyellow\">https://www.rapidtables.com/web/color/RGB_Color.html</u>."
		"<p/>"
		"<b>RGB color space</b> or <b>RGB color system</b>, constructs all the colors from "
		"the combination of the "
		"<b style=\"color:red;\">Red</b>, "
		"<b style=\"color:green;\">Green</b> and "
		"<b style=\"color:blue;\">Blue</b> colors."
		"<p/>"
		"Text <i style=\"background-color:slateblue;\">breaks</i> are always fun. LOL. "
		"For example, a <b>BOLD</b> first letter, or even <b><i>Bold-italic</i></b>, "
		"should only be <u>broken</u> at the appropriate characters."
		"</html>")

	auto scene = new SceneObject(tft, F("HTML text test"));
	scene->clear();

	XML::Document doc;
	if(!XML::deserialize(doc, FS_HTML)) {
		debug_w("XML::deserialize failed");
		return nextScene();
	}

	auto font = new ResourceFont(Resource::ubuntu);
	// auto font = new ResourceFont(Resource::notoSans15);
	scene->addAsset(font);
	TextBuilder text(*scene);
	text.setFont(font);
	text.setColor(Color::LightBlue);
	auto node = doc.first_node("html")->first_node();
	while(node != nullptr) {
		parse(*scene, text, node);
		node = node->next_sibling();
	}

	text.commit(*scene);

	// highlightText(*scene);
	render(scene);
}

static unsigned scrollCount;
constexpr Point step(2, 2);
Rect scrollRect;

void doScroll(SceneObject* scene)
{
	auto scroll = reinterpret_cast<ScrollObject*>(scene->objects.head());
	switch(scrollCount) {
	case 24:
		scroll->shift.y = -scroll->shift.y;
		break;
	case 48:
		scroll->shift.x = -scroll->shift.x;
		break;
	case 72:
		scroll->wrapx = false;
		// scroll->wrapy = false;
		break;
	case 96:
		delete scene;
		nextScene();
		return;
	}

	// scene->objects.clear();
	// Canvas canvas(scene, tft.getSize());
	// auto& r = scrollRect;
	// auto sz = tft.getSize();
	// sz.w /= 2;
	// sz.h /= 2;
	// auto s = sz;
	// s.w -= r.x;
	// s.h -= r.y;
	// scene->scroll(Rect(r.x, r.y, s), step.x, step.y, Color::Green);
	// scene->scroll(Rect(sz.w, r.y, s), -step.x, step.y, Color::Green);
	// scene->scroll(Rect(r.x, sz.h, s), step.x, -step.y, Color::Green);
	// scene->scroll(Rect(sz.w, sz.h, s), -step.x, -step.y, Color::Green);
	// r.inflate(-step.x, -step.y);

	renderQueue.render(scene, doScroll, 50);
	++scrollCount;
}

void scrollTests()
{
	auto scene = new SceneObject(tft, F("Scroll Test"));
	Rect r{60, 50, 120, 120};
	scene->scroll(r, -3, 1, true, false, Color::Green);

	// TextBuilder text = scene->addText(r);
	// text.setTextAlign(Align::Centre);
	// text.setScale({2, 2});
	// text.print(F("This is a region of scrolling text.\r\n"
	// 			 "It's supposed to scroll.\r\n"
	// 			 "Let's hope that works out, eh?!"));
	// scene->addText();

	// render(scene);
	// return;

	// text.setClip(r);
	// text.setTextAlign(Align::Centre);
	// text.setScale({3, 2});
	// text.print(F("This is a region of scrolling text.\r\n"
	// 			   "It's supposed to scroll.\r\n"
	// 			   "Let's hope that works out, eh?!"));

	scrollRect = tft.getSize();
	scrollCount = 0;
	doScroll(scene);
}

void copyTests()
{
	tft.setOrientation(landscape);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, F("Copy Tests"));
	scene->clear(makeColor(50, 50, 50));

	/*
	 * Draw a small scene onto the display, top-left, with centre circle filled a random colour,
	 * then duplicate it using copy operation.
	 */
	scene->fillCircle(25, 25, 10, ColorRange::random());
	scene->drawCircle(25, 25, 20, Color::WHITE);
	scene->drawLine(0, 25, 49, 25, Color::YELLOW);
	scene->drawLine(25, 0, 25, 49, Color::YELLOW);

	int16_t x{60};
	for(int16_t y = 0; y < size.h; y += 60) {
		for(; x < size.w; x += 60) {
			scene->copy({0, 0, 50, 50}, {x, y});
		}
		x = 0;
	}

	render(scene);
}

void showFonts()
{
	static uint8_t fontIndex;
	if(fontIndex >= Resource::fontTable.length()) {
		fontIndex = 0;
		return nextScene();
	}

	tft.setOrientation(portrait);

	auto scene = new SceneObject(tft, F("Fonts"));

	// auto brush = new ImageBrush(bitmap);
	auto brush = new GradientBrush(BrushStyle::FullScreen, Color::Yellow, Color::Red);
	scene->addAsset(brush);
	TextBuilder text(*scene);
	text.setColor(brush);
	text.setWrap(false);
	static Point cursor;
	text.setCursor(cursor);

	auto print = [&](Font* font) {
		scene->addAsset(font);
		text.setFont(font);
		auto brush = new GradientBrush(BrushStyle::FullScreen, ColorRange::random(), ColorRange::random());
		scene->addAsset(brush);
		text.setColor(brush);
		debug_i("Font: %s", font->name().c_str());
		text.println(font->name());
	};

	if(fontIndex == 0) {
		scene->clear(Color::BLACK);
		print(new LcdFont);
		for(; fontIndex < Resource::fontTable.length(); ++fontIndex) {
			cursor = text.getCursor();
			auto font = new ResourceFont(Resource::fontTable[fontIndex]);
			Rect r{text.getCursor(), text.getClip().w, font->height()};
			scene->fillRect(ColorRange::random(), r);
			print(font);
			if(text.getCursor().y >= text.getClip().h) {
				break;
			}
		}
	} else {
		auto sz = scene->getSize();
		auto font = new ResourceFont(Resource::fontTable[fontIndex]);
		int cy = (sz.h - font->height()) - cursor.y;
		debug_i("cursor (%s), font.height %u, Scroll(%s, %d)", cursor.toString().c_str(), font->height(),
				sz.toString().c_str(), cy);
		scene->scroll(sz, 0, cy, ColorRange::random());
		cursor.y = sz.h - font->height();
		text.setCursor(cursor);
		print(font);
		cursor = text.getCursor();
		++fontIndex;
	}

	text.commit(*scene);

	// MetaWriter(Serial).write(*scene);

	auto callback = [](SceneObject* scene) {
		delete scene;
		showFonts();
	};
	renderQueue.render(scene, callback, 1000);
}

using GetFont = Font* (*)();
GetFont getFont;

void showFont()
{
	static uint8_t state;
	static Font* font;
	if(state == 0) {
		delete font;
		font = getFont();
		if(font == nullptr) {
			return nextScene();
		}
	}

	auto scene = new SceneObject(tft, F("Show Font"));
	scene->clear(Color::BLACK);

	TextBuilder text(*scene);

	text.setWrap(false);
	// text.setColor(Color::White);

	auto brush = new GradientBrush(BrushStyle::FullScreen, Color::White, Color::Blue);
	// auto brush = new ImageBrush(bitmap);
	scene->addAsset(brush);

	text.setColor(brush, Color::Black);

	auto print = [&](FontStyles style) {
		text.setStyle(style);
		auto& options = text.getOptions();
		String s = font->name();
		if(options.style) {
			s += ' ';
			s += toString(options.style);
		}
		s += ' ';
		s += toString(options.scale);
		text.println(s);
	};

	auto printSet = [&](FontStyles base) {
		print(base);
		print(base | FontStyle::Italic);
		print(base | FontStyle::Bold);
		print(base | FontStyle::Bold | FontStyle::Italic);
	};

	String title = font->name();

	text.setFont(font);
	switch(state) {
	case 0:
		printSet(0);
		++state;
		break;
	case 1:
		text.setScale(2);
		printSet(FontStyle::DotMatrix);
		title += F(" DotMatrix");
		++state;
		break;
	case 2:
		text.setScale(2);
		printSet(FontStyle::HLine);
		title += F(" HLine");
		++state;
		break;
	case 3:
		text.setScale(2);
		printSet(FontStyle::VLine);
		title += F(" VLine");
		state = 0;
		break;
	}

	text.setFont(nullptr);
	text.setColor(Color::White, Color::Brown);
	text.setScale(2);
	text.setStyle(FontStyle::HLine);
	text.setLineAlign(Align::Bottom);
	text.print(title);

	text.commit(*scene);
	auto callback = [](SceneObject* scene) {
		delete scene;
		showFont();
	};
	renderQueue.render(scene, callback, 1500);
}

void showResourceFonts()
{
	tft.setOrientation(landscape);

	static uint8_t index;
	index = 0;
	getFont = []() -> Font* {
		auto& def = Resource::fontTable[index++];
		return def ? new ResourceFont(def) : nullptr;
	};
	showFont();
}

void drawLineTest(SceneObject* scene)
{
	scene->clear(makeColor(50, 50, 50));

	scene->drawRoundRect(0, 0, 320, 240, 100, Color::BLUE);
	scene->fillRoundRect(110, 80, 100, 80, 20, makeColor(Color::PURPLE, 128));

	scene->drawLine(Pen(Color::WHITE, 3), Point{0, 50}, Point{100, 0});

	scene->drawTriangle(0, 0, 50, 50, 100, 20, Color::GREEN);
	scene->drawTriangle(Pen(makeColor(Color::ORANGE, 128), 3), {10, 10}, {150, 150}, {330, 20});

	scene->drawLine(0, 150, 319, 239, Color::MAGENTA);
	scene->drawLine(319, 150, 0, 239, Color::WHITE);

	scene->drawLine(0, 150, 319, 150, Color::GREEN);
	scene->drawLine(160, 150, 160, 239, Color::GREENYELLOW);

	scene->drawCircle(160, 120, 20, makeColor(Color::WHITE, 128));
	scene->fillCircle(160, 120, 18, Color::RED);
}

void lineTests()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Line Drawing Tests"));
	drawLineTest(scene);
	render(scene);
}

void arcAnimation(SceneObject* scene = nullptr)
{
	static OneShotFastMs* timeout;

	auto size = tft.getSize();

	if(scene == nullptr) {
		tft.setOrientation(landscape);
		auto scene = new SceneObject(tft, F("Arc animation"));
		scene->clear(Color::BLACK);

		timeout = new OneShotFastMs;
		timeout->reset<5000>();
		return render(scene, arcAnimation);
	}

	if(timeout == nullptr) {
		delete scene;
		return nextScene();
	}

	constexpr int diffAngle{90};
	constexpr int stateAngle{120};

	static int startAngle;
	static int endAngle = diffAngle;
	static int state;

	scene->objects.clear();

	Rect r(size);
	r.h -= 30;

	// 42 OK, 43 fails
	r.w = r.h + 42;

	const uint8_t step{10};
	scene->drawArc(Pen(ColorRange::random(), 8), r, endAngle, endAngle + step);
	scene->drawArc(Pen(Color::Black, 7), r, startAngle, startAngle + step);
	switch(state) {
	case 0:
		startAngle += step;
		endAngle += step;
		if(startAngle % stateAngle == 0) {
			++state;
		}
		break;
	case 1:
		startAngle += step;
		if(startAngle == endAngle) {
			++state;
		}
		break;
	default:
		endAngle += step;
		if(endAngle - startAngle == diffAngle) {
			state = 0;
		}
	}

	if(startAngle == 360) {
		startAngle = 0;
		endAngle -= 360;
	}

	TextBuilder text(scene->assets, r);
	text.setScale({4, 4});
	text.setStyle(FontStyle::DotMatrix);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);

	unsigned msRemain = timeout->remainingTime();
	char s[10];
	m_snprintf(&s[1], sizeof(s) - 1, "%-04u", msRemain);
	s[0] = s[1];
	s[1] = '.';
	text.write(s, 4);
	text.commit(*scene);

	if(msRemain == 0) {
		delete timeout;
		timeout = nullptr;
	}

	renderQueue.render(scene, arcAnimation, 20);
}

void filledArcAnimation(SceneObject* scene = nullptr)
{
	static int endAngle;
	static unsigned arcCount;

	if(scene == nullptr) {
		tft.setOrientation(landscape);
		scene = new SceneObject(tft, F("Filled Arc Animation"));
		scene->clear(Color::BLACK);
		return render(scene, filledArcAnimation);
	}

	if(arcCount++ == 10) {
		arcCount = 0;
		endAngle = 0;
		delete scene;
		return nextScene();
	}

	scene->objects.clear();

	auto startAngle = endAngle;
	endAngle += Range(5, 360).random();

	Rect r(scene->getSize());
	r.h -= 30;
	scene->fillArc(ColorRange::random(), r, startAngle, endAngle);

	renderQueue.render(scene, filledArcAnimation, 500);
}

void timeRender(Object::Kind kind, TextBuilder& text)
{
	Display::NullDevice device;
	device.setOrientation(landscape);
	auto size = device.getSize();
	Location loc{size};
	std::unique_ptr<Renderer> renderer;
	Point centre(size.w / 2, size.h / 2);
	const uint16_t r{50};
	Rect rect(centre, size, Origin::Centre);
	Pen pen(Color::Blue, 3);
	switch(kind) {
	case Object::Kind::Circle:
		renderer.reset(new CircleRenderer(loc, CircleObject(pen, centre, r)));
		break;
	case Object::Kind::FilledCircle:
		renderer.reset(new FilledCircleRenderer(loc, FilledCircleObject(pen, centre, r)));
		break;
	case Object::Kind::Ellipse:
		renderer.reset(new EllipseRenderer(loc, EllipseObject(pen, rect)));
		break;
	case Object::Kind::FilledEllipse:
		renderer.reset(new FilledEllipseRenderer(loc, FilledEllipseObject(pen, rect)));
		break;
	case Object::Kind::Rect:
		renderer.reset(new RoundedRectRenderer(loc, RectObject(pen, rect, 10)));
		break;
	case Object::Kind::FilledRect:
		renderer.reset(new FilledRoundedRectRenderer(loc, FilledRectObject(pen, rect, 10)));
		break;
	default:;
	}

	auto surface = device.createSurface();
	CpuCycleTimer timer;
	bool complete = renderer->execute(*surface);
	auto ticks = timer.elapsedTicks();
	delete surface;

	String s = timer.ticksToTime(ticks).as<NanoTime::Milliseconds>().toString();
	debug_i("Render %scomplete, %s took %u ticks, %s", complete ? "" : "NOT ", toString(kind).c_str(), ticks,
			s.c_str());

	text.print(toString(kind));
	text.print(F(": ticks "));
	text.print(ticks);
	text.print(F(", time "));
	text.println(s);
}

void RenderSpeedComparison()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Render Speed Comparison"));
	// scene->clear();
	TextBuilder text(*scene);
	text.setLineAlign(Align::Centre);
	text.setFont(fontSans);
	text.setColor(makeColor(Color::White, 128));

	timeRender(Object::Kind::Circle, text);
	timeRender(Object::Kind::Ellipse, text);
	timeRender(Object::Kind::FilledCircle, text);
	timeRender(Object::Kind::FilledEllipse, text);
	timeRender(Object::Kind::Rect, text);
	timeRender(Object::Kind::FilledRect, text);

	text.commit(*scene);

	render(scene);
}

void sceneTests()
{
	tft.setOrientation(landscape);

	// Now draw it several times
	auto scene = new SceneObject(tft, F("Multi-Scene Tests"));
	scene->clear(makeColor(50, 50, 50));
	scene->drawObject(targetSymbol, Rect{50, 50, targetSymbolSize});
	scene->drawObject(targetSymbol, Rect{200, 50, targetSymbolSize});
	scene->drawObject(targetSymbol, Rect{50, 150, targetSymbolSize});
	scene->drawObject(targetSymbol, Rect{200, 150, targetSymbolSize});

	render(scene);
}

void memoryImageDrawing()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft);
	scene->clear();
	// scene->drawCircle(100, 100, 40, makeColor(Color::WHITE, 128));
	uint8_t a{128};
	uint16_t r{120};
	auto pt = Rect(scene->getSize()).centre();
	auto brush = scene->addAsset(new GradientBrush(BrushStyle::FullScreen, Color::Red, Color::White));
	scene->fillCircle(brush, pt + Point(0, -r / 2), r);
	scene->fillCircle(makeColor(Color::GREEN, a), pt + Point(-r / 2, r / 2), r);
	scene->fillCircle(makeColor(Color::BLUE, a), pt + Point(r / 2, r / 2), r);

	auto font = new ResourceFont(Resource::notoSans36);
	scene->addAsset(font);
	TextBuilder text(*scene);
	text.setFont(font);
	text.setScale(2);
	// Font* font;
	// text.setScale(4);
	text.setColor(makeColor(Color::White, 160));
	text.setClip({pt, {160, 160}, Origin::Centre});
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.print(F("Crazy Paving"));

	text.setScale(0);
	text.resetClip();
	text.setColor(makeColor(Color::Yellow, 128), makeColor(Color::Blue, 150));
	text.setLineAlign(Align::Bottom);
	text.print(F(" Sming Rocks! "));

	text.commit(*scene);

	auto& asset = *scene->addAsset(new TextAsset("Sming"));
	TextParser parser(tft.getSize());
	parser.setFont(font);
	parser.setWrap(false);
	parser.setColor(makeColor(Color::Black, 20));
	for(int16_t y = -5; y < 240; y += 30) {
		parser.setCursor(0, y);
		do {
			parser.parse(asset, 0, 5);
		} while(parser.getCursor().x < 320);
	}
	parser.commit(*scene);

	// highlightText(*scene);
	MetaWriter(Serial).write(*scene);

	render(scene);
}

void surfaceTests()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Surface Tests"));
	scene->clear(makeColor(50, 50, 50)); // Different background to symbol

	// Render symbol to a memory image (black background)
	auto image = new MemoryImageObject(tftPixelFormat, targetSymbolSize);
	auto surface = image->createSurface();
	surface->render(targetSymbol, targetSymbolSize);
	delete surface;

	scene->addAsset(image);
	scene->drawImage(*image, {50, 50});
	scene->drawImage(*image, {200, 50});
	scene->drawImage(*image, {50, 150});
	scene->drawImage(*image, {200, 150});

	render(scene);
}

void surfaceTests2()
{
	tft.setOrientation(landscape);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, F("Surface Tests #2"));

	/*
	 * Draw a small scene onto the display, top-left, with centre circle filled a random colour.
	 * We're going to copy this to a file image the first time its run, then draw it multiple times.
	 * The first time this is run, all the symbols will be the same, but on subsequent runs the
	 * centre circle colours will differ so we can see which is the original and which are the copies.
	 */
	scene->clear(makeColor(50, 50, 50));
	scene->fillCircle(25, 25, 10, ColorRange::random());
	scene->drawCircle(25, 25, 20, Color::WHITE);
	scene->drawLine(0, 25, 49, 25, Color::YELLOW);
	scene->drawLine(25, 0, 25, 49, Color::YELLOW);

	// A surface copy is linear so is an efficient way to store to file
	bool exist = fileExist(TMPIMAGE_FILE);
	// auto fs = new FileStream(TMPIMAGE_FILE, File::Create | File::ReadWrite);
	// auto image = new FileImageObject(fs, tftPixelFormat, {50, 50});
	auto image = new MemoryImageObject(tftPixelFormat, {50, 50});
	scene->addAsset(image);
	// if(!exist) {
	// Make a copy
	auto surface = image->createSurface();
	scene->addAsset(surface);
	scene->copySurface(*surface, {0, 0, 50, 50}, {0, 0});
	// }

	// Render the copied image multiple times
	int16_t x{60};
	for(int16_t y = 0; y < size.h; y += 60) {
		for(; x < size.w; x += 60) {
			scene->drawImage(*image, {x, y});
		}
		x = 0;
	}

	render(scene);
}

/*
 * Alpha rendering 
 *
 * Operation should be performed using Color alpha channel.
 * For alpha=255 item is drawn normally.
 *
 * Objects are usually drawn in rows. Ellipses and circles draw multiple quadrants at the same time.
 * These are drawn as a sequence of rectangles.
 *
 * Consider leaving renderers unchanged and using an `AlphaSurface` attached to a display.
 * This would have an internal buffer (say, 4K) and its own DisplayLists.
 * Calling `render` would then execute the display lists
 *
 * One problem here is that packed color buffers do not contain alpha information.
 * We can fix this by using 32-bit colour for our AlphaSurface.
 *
 * 
 * 
 * The simplest example is for glyphs and small areas where the entire target fits in a single buffer.
 * Larger objects can be renderered in rows or columns (i.e. horizontal or vertical lines).
 *
 * In both cases rendering is managed by an AlphaRenderer, initialised with the bounding box
 * for the object to be drawn. Settings control render style (block, HLine or VLine).
 *
 * Rendering starts with a read operation. The CopyRenderer calls `getLine`:
 *
 *		// @param line Location and dimensions of line to read
 *		// @retval bool true to read the line, false to skip
 * 		bool getLine(Rect& line);
 *
 * The object renderer can adjust the position to avoid un-necessary reads
 * 
 * 
 * by reading the first line from target surface.
 * 
 *  The location
 * and orientation of this is determined by the object renderer.
 * 
 * 
 * When it arrives, `processLine()` is called to combine with object pixel data:
 * 
 *		processLine(uint8_t* data, uint16_t length);
 *
 * The result is then written back.
 *
 * Lines may be of irregular length, so this approach will result in more reads and writes
 * than strictly necessary. So we need a way to determine how much 
 * 
 * 
 * 
 * Circles, ellipses, etc. may be improved as each row length differs.
 * Another callback can provide the appropriate line length.
 * 
 * 
 */
void blendTests()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Blend Tests"));
	scene->clear();

	/*
		Create a memory Image object
		Create a blend surface
		Read target surface area into Image
		Draw symbol to blend surface
		Render image

		Support alpha in packed colours is probably not a good idea, unless hardware supports it.
		Can set alpha factor MemoryObject to start with.


		NB. Update memory object to deal with alpha
	*/
	auto alphaBlend = new BlendAlpha(70);
	scene->addAsset(alphaBlend);

	auto draw = [&](Object& object, Point pos) {
		scene->drawImage(*bitmap, pos);
		scene->drawObject(object, Rect{0, 0, targetSymbolSize} + pos, alphaBlend);
		scene->drawObject(object, Rect{0, 78, targetSymbolSize} + pos, alphaBlend);
		scene->drawObject(object, Rect{78, 0, targetSymbolSize} + pos, alphaBlend);
		scene->drawObject(object, Rect{78, 78, targetSymbolSize} + pos, alphaBlend);
		scene->drawObject(object, Rect{{64, 64}, targetSymbolSize, Origin::Centre} + pos, alphaBlend);
	};

	// Draw symbol with each element separately blended as it is drawn
	Point pos;
	draw(targetSymbol, pos);

	// Draw symbol to image, then blend to display
	auto image = new MemoryImageObject(tft.getPixelFormat(), targetSymbolSize);
	scene->addAsset(image);
	auto surface = image->createSurface();
	surface->render(targetSymbol, targetSymbolSize);
	delete surface;
	pos.x += bitmap->width() + 20;
	draw(*image, pos);

	auto trans = new BlendTransparent(makeColor(255, 180, 180));
	scene->addAsset(trans);
	pos.y += bitmap->height() - 10;
	scene->drawImage(*bitmap, pos, trans);
	pos.x = 0;
	scene->drawImage(*bitmap, pos, trans);

	render(scene);
}

void imageBrushTests()
{
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, F("Image Brush Test"));
	scene->clear();

	auto brush = new ImageBrush(BrushStyle::FullScreen, *heron);
	scene->addAsset(brush);

	Rect r = scene->getSize();
	r.inflate(-50);
	scene->fillRect(brush, r, 20);
	scene->fillRect(makeColor(Color::Red, 20), r, 20);

	TextBuilder text(*scene);
	text.setTextAlign(Align::Centre);
	auto font = scene->addAsset(new ResourceFont(Resource::notoSans36));
	text.setFont(*font);
	text.setScale(3);
	text.setColor(makeColor(Color::Green, 80));
	text.print("Sming");
	text.commit(*scene);

	render(scene);
}

void placementTests()
{
	tft.setOrientation(landscape);
	auto size = tft.getSize();
	auto scene = new SceneObject(tft, F("Placement Tests"));
	scene->clear(makeColor(50, 50, 50));

	TextBuilder text(*scene);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.setColor(Color::White);

	Rect rc(Point(size.w / 2, size.h / 2), {60, 60}, Origin::Centre);
	for(unsigned i = 0; i < 9; ++i) {
		auto o = Origin(i);
		auto pt = rc[o];
		Rect r(pt, {50, 50}, opposite(o));
		scene->drawRect(Pen{ColorRange::random(), 3}, r);
		text.setClip(r);
		text.print(toString(o));
	}

	text.commit(*scene);

	highlightText(*scene);

	render(scene);
}

void printStream(IDataSourceStream& stream)
{
	String s = stream.readString(0xffff);
	debug_i("Stream has %u bytes", s.length());
	m_printHex("DATA", s.c_str(), s.length(), 0, 16);
}

void printDrawing(DrawingObject& drawing)
{
	MetaWriter{Serial}.write(drawing);
}

void drawingTest()
{
	constexpr Size size{320, 240};

	// Low-level drawing API
	{
		auto mem = new MemoryDataStream;
		Drawing::Writer w(*mem);
		w.reset();
		w.setPenColor(Color::Green);
		w.setPenWidth(3);
		w.drawCircle({10, 10}, 50);
		w.flush();
		printStream(*mem);
		DrawingObject drawing(mem);
		printDrawing(drawing);
	}

	// Using canvas
	{
		SceneObject scene(size);
		scene.drawCircle(Pen{Color::Green, 3}, Point{10, 10}, 50);
		MemoryDataStream mem;
		DrawingTarget target(mem);
		target.render(scene, size);
		printStream(mem);
	}

	auto mem = new MemoryDataStream;
	auto drawing = new DrawingObject(mem);
	{
		// Draw a new scene
		SceneObject scene(size);
		drawLineTest(&scene);
		// Add some arcs
		Rect r(size);
		r.h -= 30;
		const uint8_t step{10};
		for(unsigned angle = 0; angle + step <= 360; angle += step) {
			scene.drawArc(Pen(ColorRange::random(128), 30), r, angle, angle + step);
		}
		// Render the scene to our drawing object
		DrawingTarget target(*drawing);
		target.render(scene, size);
		printStream(*mem);
		// printDrawing(*drawing);
	}

	//  03 0a 00
	//  04 0a 00
	//  0b
	//  05 00 80 00 00
	//  06 03
	//  08 32 00
	//  10

	// Now render the drawing object
	tft.setOrientation(landscape);
	auto scene2 = new SceneObject(tft, "Drawing Test");
	scene2->clear();
	scene2->addObject(drawing);

	render(scene2);
}

void drawingTest2()
{
	constexpr Size size{320, 240};
	constexpr Size drawingSize{60, 60};

	auto mem = new MemoryDataStream;
	auto drawing = new DrawingObject(mem);
	{
		// Draw a new scene
		SceneObject scene(size);
		// Add some arcs
		Rect r(drawingSize);
		const uint8_t step{15};
		for(unsigned angle = 0; angle + step <= 360; angle += step) {
			scene.drawArc(Pen(ColorRange::random(), 4), r, angle, angle + step);
		}
		// Render the scene to our drawing object
		DrawingTarget target(*drawing);
		target.render(scene, size);
		// printStream(*mem);
		// printDrawing(*drawing);
	}

	// Now render the drawing object
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, "Drawing Test #2");
	scene->clear();
	scene->addAsset(drawing);
	TextBuilder text(*scene);
	text.setTextAlign(Align::Centre);
	text.setLineAlign(Align::Centre);
	text.setColor(Color::White);
	text.setScale(0);
	unsigned i{0};
	constexpr Point stride = Point(drawingSize) * 2 / 3;
	constexpr Point reps = (Point(size) + stride - Point(drawingSize)) / stride;
	Point off = (Point(size) - stride * (reps - Point(1, 1)) - Point(drawingSize)) / 2;
	for(int16_t y = 0; y + drawingSize.h <= size.h; y += stride.y) {
		for(int16_t x = 0; x + drawingSize.w <= size.w; x += stride.x) {
			Rect rc{x, y, drawingSize};
			rc += off;
			scene->drawObject(*drawing, rc);
			text.setClip(rc);
			text.print(i++);
		}
	}

	text.commit(*scene);

	// highlightText(*scene);

	render(scene);
}

/*

TODO:

How to define re-useable sections in a drawing?
	Add a call/sub mechanism:

			sub 0
			...
			end
			call 0

		These are not executed but the reader keeps a list of these to index
		ID by start position. Then use `call` to execute it. The reader will need
		a stack to track stream positions, nesting depth, etc.

	3) functions and `call`

		SVG has the `use` element to draw an object reference.
		It uses an `id` tag on an object.
		Storing an object in `defs` means it won't be displayed.
		This allows both reuse and compression. We don't want to have to tag every
		object although autogenerated files could suppoort `goto` and `call` commands
		which change the stream pointer. This n

	So we'll use (1) then.

How to reference external assets, e.g. bitmap, font
	Bitmaps could be specified by filename. It only gets loaded during rendering.
	Resident assets could also be registered by ID and/or name.

Drawing text
	Canvas deals with alignment, etc. so drawing deals with a single text segment,
	specifying typeface and other options.
		typeface

External objects
	These can be added to a list with ID then drawn using a `draw` command.
	Objects could be other drawings, images, etc.

Colours, pallettes, filling vs drawing
	Brushes are for fills, pens are for drawing
		PenColor, PenWidth, PenBrush
		Brush

	Brush is an index into the palette.
	Can build the palette manually in code, but this could also be scripted.

	Still want to be able to specify absolute colours, though dynamic building would
	optimise using palette entries.

OTHER:

	Scene `references` only there for printing meta information.
	This list can be built dynamically during output so not required.
	This facility is really only for debugging.

*/

void drawingTest3()
{
	// clang-format off
DEFINE_FSTR_ARRAY_LOCAL(myDrawing, uint8_t,
	// Draw circle and set next X position
	GDRAW_BEGIN_SUB(0)
		GDRAW_CIRCLE(9)
		GDRAW_XREL(20)
	GDRAW_END_SUB()
	// Step draw 3 circles
	GDRAW_BEGIN_SUB(1)
		GDRAW_SELECT_PEN(1)
		GDRAW_CALL(0)
		GDRAW_SELECT_PEN(2)
		GDRAW_CALL(0)
		GDRAW_SELECT_PEN(3)
		GDRAW_CALL(0)
	GDRAW_END_SUB()
	// Draw 15 circles in a row, move cursor, rotate pens
	GDRAW_BEGIN_SUB(2)
		GDRAW_XABS(20)
		GDRAW_CALL(1)
		GDRAW_CALL(1)
		GDRAW_CALL(1)
		GDRAW_CALL(1)
		GDRAW_CALL(1)
		GDRAW_YREL(20)
		GDRAW_SELECT_PEN(3)
		GDRAW_STORE_PEN(4)
		GDRAW_SELECT_PEN(2)
		GDRAW_STORE_PEN(3)
		GDRAW_SELECT_PEN(1)
		GDRAW_STORE_PEN(2)
		GDRAW_SELECT_PEN(4)
		GDRAW_STORE_PEN(1)
	GDRAW_END_SUB()
	// Draw 4 rows of 15 circles
	GDRAW_BEGIN_SUB(3)
	GDRAW_CALL(2)
	GDRAW_CALL(2)
	GDRAW_CALL(2)
	GDRAW_CALL(2)
	GDRAW_END_SUB()
	// Setup initial pens
	GDRAW_PEN_WIDTH(3)
	GDRAW_PEN_COLOR(makeColor(Color::Red, 128))
	GDRAW_STORE_PEN(1)
	GDRAW_PEN_COLOR(makeColor(Color::Green, 128))
	GDRAW_STORE_PEN(2)
	GDRAW_PEN_COLOR(makeColor(Color::Blue, 128))
	GDRAW_STORE_PEN(3)
	// Draw 9 rows of 12 circles
	GDRAW_CALL(3)
	GDRAW_CALL(3)
	GDRAW_CALL(3)
	// Filled orange circle
	GDRAW_XABS(120)
	GDRAW_YABS(120)
	GDRAW_SELECT_PEN(0)
	GDRAW_BRUSH_COLOR(makeColor(Color::Orange, 150))
	GDRAW_FILL_CIRCLE(30)
	GDRAW_PEN_COLOR(makeColor(Color::Black, 150))
	GDRAW_XABS(125)
	GDRAW_YABS(115)
	GDRAW_DRAW_CHARS(5, 'x', 'a', ' ', 'i', '#')
	GDRAW_YREL(10)
	GDRAW_XABS(10)
	GDRAW_YABS(180)
	GDRAW_MOVE()
	GDRAW_XREL(150)
	GDRAW_YREL(20)
	GDRAW_FONT_STYLE(1, FontStyle::Underscore | FontStyle::Bold | FontStyle::Italic)
	GDRAW_OFFSET_LENGTH(0, 4)
	GDRAW_DRAW_TEXT(100)
	GDRAW_FONT_STYLE(1, 0)
	GDRAW_OFFSET_LENGTH(4, 99)
	GDRAW_DRAW_TEXT(100)
	GDRAW_BRUSH_COLOR(makeColor(Color::White, 100))
	GDRAW_FILL_RECT(0)

	// Draw radial lines (redefine sub #0)
	GDRAW_BEGIN_SUB(0)
		GDRAW_XREL(30)
		GDRAW_LINE()
		GDRAW_YREL(30)
		GDRAW_LINE()
		GDRAW_XREL(-30)
		GDRAW_LINE()
		GDRAW_XREL(-30)
		GDRAW_LINE()
		GDRAW_YREL(-30)
		GDRAW_LINE()
		GDRAW_YREL(-30)
		GDRAW_LINE()
		GDRAW_XREL(30)
		GDRAW_LINE()
		GDRAW_XREL(30)
		GDRAW_LINE()
	GDRAW_END_SUB()
	GDRAW_BEGIN_SUB(1)
		GDRAW_MOVE()
		GDRAW_CALL(0)
		GDRAW_XREL(5)
		GDRAW_YREL(5)
	GDRAW_END_SUB()
	GDRAW_XABS(180)
	GDRAW_YABS(180)
	GDRAW_SELECT_PEN(1)
	GDRAW_CALL(1)
	GDRAW_SELECT_PEN(2)
	GDRAW_CALL(1)
	GDRAW_SELECT_PEN(3)
	GDRAW_CALL(1)
)
	// clang-format on

	constexpr Size size{320, 240};

	auto drawing = new DrawingObject(myDrawing);
	drawing->assets.store(new TextAsset(100, F("This is some text")));
	drawing->assets.store(new ResourceFont(1, Resource::freeSans9pt));
	printStream(drawing->getStream());
	// printDrawing(*drawing);

	// Render the drawing
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, "Drawing Test 3");
	scene->clear();
	scene->addObject(drawing);

	render(scene);
}

void drawingTest4()
{
	// clang-format off
DEFINE_FSTR_ARRAY_LOCAL(buttonLayout, uint8_t,
	// Draw a button
	GDRAW_BEGIN_SUB(0)
		GDRAW_SAVE()
		GDRAW_XREL(50)
		GDRAW_YREL(30)
		GDRAW_BRUSH_COLOR(Color::Gray)
		GDRAW_FILL_RECT(4)
		GDRAW_XREL(-48)
		GDRAW_YREL(-28)
		GDRAW_MOVE()
		GDRAW_XREL(46)
		GDRAW_YREL(26)
		GDRAW_BRUSH_COLOR(Color::White)
		GDRAW_FILL_RECT(3)
		GDRAW_RESTORE()
		GDRAW_XREL(60)
		GDRAW_MOVE()
	GDRAW_END_SUB()

	// Draw a line of buttons
	GDRAW_BEGIN_SUB(1)
		GDRAW_CALL(0)
		GDRAW_CALL(0)
		GDRAW_CALL(0)
		GDRAW_CALL(0)
	GDRAW_END_SUB()

	GDRAW_CALL(1)
)
	// clang-format on

	constexpr Size size{320, 240};

	auto drawing = new DrawingObject(buttonLayout);
	drawing->assets.store(new TextAsset(100, F("This is some text")));
	printStream(drawing->getStream());
	// printDrawing(*drawing);

	// Render the drawing
	tft.setOrientation(landscape);
	auto scene = new SceneObject(tft, "Drawing Test 4");
	scene->clear();
	scene->addObject(drawing);

	render(scene);
}

void regionTests()
{
	tft.setOrientation(landscape);
	auto size = tft.getSize();
	auto scene = new SceneObject(size, "Region Tests");
	scene->clear();

	struct Rect2 {
		Rect r1;
		Rect r2;
	};
	const Rect2 list[]{
		{
			{0, 0, 40, 40},
			{10, 10, 20, 20},
		},
		{
			{0, 0, 40, 40},
			{0, 5, 40, 30},
		},
		{
			{0, 0, 40, 40},
			{0, 5, 30, 30},
		},
		{
			{0, 0, 40, 40},
			{30, 10, 40, 30},
		},
		{
			{0, 0, 40, 40},
			{30, 0, 40, 40},
		},
		{
			{0, 0, 40, 40},
			{0, 30, 40, 40},
		},
		{
			{0, 30, 40, 40},
			{30, 0, 40, 40},
		},
		{
			{0, 0, 40, 40},
			{30, 30, 40, 40},
		},
	};

	constexpr uint8_t lineAlpha{200};
	constexpr uint8_t fillAlpha{128};
	constexpr size_t margin{5};
	Size lineSize{};
	Point pos{};
	auto test = [&](const Rect& r1, const Rect& r2) {
		Rect u = r1 + r2;
		if(pos.x + u.w > size.w) {
			pos.x = 0;
			pos.y += lineSize.h + margin;
			lineSize.h = u.h;
		} else {
			lineSize.h = std::max(lineSize.h, u.h);
		}

		auto rgn = r1 - r2;
		debug_i("(%s) - (%s) = %s", r1.toString().c_str(), r2.toString().c_str(), rgn.toString().c_str());
		Color colours[]{Color::Red, Color::Green, Color::Blue, Color::Magenta};
		for(unsigned i = 0; i < 4; ++i) {
			auto& r = rgn.rects[i];
			if(r) {
				scene->fillRect(makeColor(colours[i], fillAlpha), r + pos);
			}
		}

		scene->drawRect(makeColor(Color::White, lineAlpha), r1 + pos);
		scene->drawRect(makeColor(Color::Aqua, lineAlpha), r2 + pos);

		pos.x += u.w + margin;
	};
	for(auto& t : list) {
		test(t.r1, t.r2);
		test(t.r2, t.r1);
	}

	render(scene);
}

const InterruptCallback functionList[] PROGMEM{
	startPage,
	scrollTests,
	showFonts,
	showResourceFonts,
	lineTests,
	sceneTests,
	memoryImageDrawing,
	surfaceTests,
	surfaceTests2,
	blendTests,
	[]() { gui.show(); },
	[]() { imageTests(*bitmap, "Bitmap tests"); },
	[]() { imageTests(*rawImage, "Raw image tests"); },
	imageBrushTests,
	textTests,
	htmlTextTest,
	placementTests,
	[]() { arcAnimation(); },
	[]() { filledArcAnimation(); },
	RenderSpeedComparison,
	drawingTest,
	drawingTest2,
	drawingTest3,
	drawingTest4,
	copyTests,
	regionTests,
};

void run()
{
	static uint8_t state;

	if(state >= ARRAY_SIZE(functionList)) {
		state = 0;
	}
	functionList[state++]();
}

} // namespace

void init()
{
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); // Allow debug output to serial

	// MallocCount::enableLogging(true);
	// MallocCount::setLogThreshold(256);

#ifndef DISABLE_WIFI
	//WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(false);
	WifiAccessPoint.enable(false);
#endif

#ifdef ARCH_HOST
	setDigitalHooks(nullptr);
#endif

	spiffs_mount();

	auto part = Storage::findPartition(F("resource"));
	Graphics::Resource::init(new Storage::PartitionStream(part));

	// readBitmap();

	bitmap = new BitmapObject(Resource::sming_bmp);
	if(!bitmap->init()) {
		debug_e("Invalid bitmap");
	}
	rawImage = new RawImageObject(Resource::sming_raw);
	heron = new RawImageObject(Resource::heron_raw);

	// Create a target symbol, used in various tests
	Rect r(targetSymbolSize);
	targetSymbol.fillCircle(Color::RED, r.centre(), 10);
	targetSymbol.drawCircle(Color::WHITE, r.centre(), 20);
	targetSymbol.drawLine(Color::YELLOW, r[Origin::W], r[Origin::E]);
	targetSymbol.drawLine(Color::YELLOW, r[Origin::N], r[Origin::S]);
	targetSymbol.drawRect(Pen(Color::Gray, 3), r);

	Serial.println("Display start");
#ifdef ENABLE_VIRTUAL_SCREEN
	tft.begin();
	// tft.setMode(Display::Virtual::Mode::Debug);
#else

	// Touch CS
	pinMode(TOUCH_CS_PIN, OUTPUT);
	digitalWrite(TOUCH_CS_PIN, HIGH);

	spi.begin();

	/*
	 * ILI9341 min. clock cycle is 100ns write, 150ns read.
	 * In practice, writes work at 40MHz, reads at 27MHz.
	 * Attempting to read at 40MHz results in colour corruption.
	 */
	tft.begin(TFT_PINSET, TFT_CS, TFT_DC_PIN, TFT_RESET_PIN, 27000000);

	Serial.printf(_F("Speed: %u\r\n"), tft.getSpeed());
	Serial.printf(_F("DisplayID: 0x%06x\r\n"), tft.readDisplayId());
	Serial.printf(_F("Status: 0x%08x\r\n"), tft.readDisplayStatus());
	Serial.printf(_F("MADCTL: 0x%02x\r\n"), tft.readMADCTL());
	Serial.printf(_F("PixelFormat: 0x%02x\r\n"), tft.readPixelFormat());
	Serial.printf(_F("ImageFormat: 0x%02x\r\n"), tft.readImageFormat());
	Serial.printf(_F("SignalMode: 0x%02x\r\n"), tft.readSignalMode());
	Serial.printf(_F("SelfDiag: 0x%02x\r\n"), tft.readSelfDiag());
	Serial.printf(_F("NVMemStatus: 0x%04x\r\n"), tft.readNvMemStatus());
#endif

	tftPixelFormat = tft.getPixelFormat();

	// copyTmpImage();

	backgroundTimer.initializeMs<500>([]() {
		auto ticks = interval.elapsedTicks();
		Serial.print(F("Background timer: ticks "));
		Serial.print(ticks);
		Serial.print(F(", time "));
		Serial.print(interval.ticksToTime(ticks).as<NanoTime::Milliseconds>().toString());
		Serial.print(F(", heap free "));
		Serial.print(system_get_free_heap_size());
#ifdef ENABLE_MALLOC_COUNT
		Serial.print(F(", used "));
		Serial.print(MallocCount::getCurrent());
		Serial.print(F(", peak "));
		Serial.print(MallocCount::getPeak());
#endif
		Serial.println();
		interval.start();
	});
	backgroundTimer.start();

	guiTimer.initializeMs<5000>(run);
	run();
}
