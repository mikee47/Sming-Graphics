/****
 * Meta.h
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
 * @author: May 2021 - mikee47 <mike@sillyhouse.net>
 *
 ****/

#pragma once

#include "Types.h"
#include <Data/LinkedObjectList.h>
#include <Data/CString.h>
#include <Data/Stream/DataSourceStream.h>
#include <Print.h>

namespace Graphics
{
class MetaWriter;

/**
 * @brief Empty base class to support object enumeration
 * Non-virtual to avoid bloat.
 */
class Meta
{
	// 	String getTypeStr() const;
	// 	void write(MetaWriter& meta) const;
};

/**
 * @brief Writes object content in readable format for debugging
 */
class MetaWriter
{
public:
	MetaWriter(Print& out) : out(out)
	{
	}

	template <typename T>
	typename std::enable_if<std::is_base_of<Meta, T>::value, void>::type write(const String& name, const T& value)
	{
		writeIndent();
		if(name) {
			out.print(name);
			out.print(": ");
		}
		out.print(value.getTypeStr());
		out.println(" {");
		++indent;
		value.write(*this);
		--indent;
		println("};");
	}

	template <typename T> typename std::enable_if<std::is_base_of<Meta, T>::value, void>::type write(const T& value)
	{
		write(nullptr, value);
	}

	void write(const String& name, const CString& value)
	{
		writeIndent();
		out.print(name);
		out.print(": ");
		out.println(value.c_str());
	}

	void write(const String& name, IDataSourceStream& stream)
	{
		writeIndent();
		out.print(name);
		out.print(": ");
		stream.seekFrom(0, SeekOrigin::Start);
		char buffer[1024];
		auto len = stream.readBytes(buffer, sizeof(buffer));
		out.write(buffer, len);
		out.println();
	}

	template <typename T>
	typename std::enable_if<std::is_arithmetic<T>::value || std::is_base_of<String, T>::value, void>::type
	write(const String& name, const T& value)
	{
		writeIndent();
		out.print(name);
		out.print(": ");
		out.println(value);
	}

	template <typename T>
	typename std::enable_if<!std::is_arithmetic<T>::value && !std::is_base_of<Meta, T>::value &&
								!std::is_base_of<String, T>::value && !std::is_base_of<CString, T>::value &&
								!std::is_base_of<Stream, T>::value,
							void>::type
	write(const String& name, const T& value)
	{
		writeIndent();
		out.print(name);
		out.print(": ");
		out.println(::toString(value));
	}

	void beginArray(const String& name, const String& type)
	{
		String s;
		s += name;
		s += ": ";
		s += type;
		s += "[] {";
		println(s);
		++indent;
	}

	void endArray()
	{
		--indent;
		println("}");
	}

	template <typename T> void writeArray(const String& name, const String& type, const T* values, unsigned count)
	{
		beginArray(name, type);
		for(unsigned i = 0; i < count; ++i) {
			println(toString(values[i]));
		}
		endArray();
	}

	template <typename T>
	void writeArray(const String& name, const String& type, const LinkedObjectListTemplate<T>& list)
	{
		beginArray(name, type);
		for(auto& obj : list) {
			write(obj);
		}
		endArray();
	}

private:
	void writeIndent()
	{
		auto n = indent * 2;
		char buf[n];
		memset(buf, ' ', n);
		out.write(buf, n);
	}

	template <typename T> void println(const T& value)
	{
		writeIndent();
		out.println(value);
	}

	Print& out;
	uint8_t indent{0};
};

} // namespace Graphics
