/****
 * Console.h
 *
 * Copyright 2021 mikee47 <mike@sillyhouse.net>
 *
 * This file is part of the Sming-Graphics Library
 *
 * This library is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, version 3 or later.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this library.
 * If not, see <https://www.gnu.org/licenses/>.
 *
 * @author: October 2021 - mikee47 <mike@sillyhouse.net>
 *
 ****/

#pragma once

#include "AbstractDisplay.h"
#include "RenderQueue.h"
#include <memory>

namespace Graphics
{
class Console : public Print
{
public:
	enum class Section {
		top,
		middle,
		bottom,
	};

	/**
	 * @brief Console constructor
	 * @param display Output device
	 * @param renderQueue Allows shared access to display
	 */
	Console(AbstractDisplay& display, RenderQueue& renderQueue) : display(display), renderQueue(renderQueue)
	{
	}

	/**
	 * @brief Use console for debug output
	 * @param enable true to divert m_puts to console, false to disable debug output
	 */
	void systemDebugOutput(bool enable);

	/**
	 * @brief Suspend/resume output to display
	 * @param state true to stop output, false to resume
	 * 
	 * While paused, content will be buffered in RAM.
	 */
	void pause(bool state);

	/**
	 * @brief Determine if output is paused
	 */
	bool isPaused() const
	{
		return paused;
	}

	/* Print methods */

	size_t write(const uint8_t* data, size_t size) override;

	size_t write(uint8_t c) override
	{
		return write(&c, 1);
	}

	void setScrollMargins(uint16_t top, uint16_t bottom)
	{
		auto item = addCommand(Command::setScrollMargins);
		item->top = top;
		item->bottom = bottom;
		update();
	}

	void setCursor(Point pt)
	{
		addCommand(Command::setCursor)->pos = pt;
		update();
	}

	void setSection(Section section)
	{
		addCommand(Command::setSection)->section = section;
		update();
	}

	Point getCursor() const
	{
		return cursor;
	}

	void clear()
	{
		addCommand(Command::clear);
		update();
	}

	Rect getSectionBounds(Section section);
	Section getSection(uint16_t line);

private:
	// Internally need to queue instructions
	enum class Command {
		writeText,
		setCursor,
		setScrollMargins,
		setSection,
		clear,
		pause, ///< Stop here until resumed
	};

	class CommandItem : public LinkedObjectTemplate<CommandItem>
	{
	public:
		CommandItem(Command command) : command(command)
		{
		}

		Command command;
		// writeText
		String text;
		union {
			// setCursor
			Point pos;
			// setSection
			Section section;
			// setScrollMargins
			struct {
				uint16_t top;
				uint16_t bottom;
			};
		};
	};

	using CommandQueue = OwnedLinkedObjectListTemplate<CommandItem>;

	CommandItem* addCommand(Command command)
	{
		auto item = new CommandItem(command);
		queue.add(item);
		lastItem = item;
		return item;
	}

	void update();
	void clearSection(Section section);
	void writeText(String&& buffer);

	AbstractDisplay& display;
	RenderQueue& renderQueue;
	CommandQueue queue;
	CommandItem* lastItem{nullptr};
	std::unique_ptr<SceneObject> scene;
	Point cursor{};
	uint16_t topMargin{0};
	uint16_t bottomMargin{0};
	uint16_t scrollOffset{0};
	bool paused{false};
};

} // namespace Graphics
