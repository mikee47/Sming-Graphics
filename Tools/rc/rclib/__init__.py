#!/usr/bin/env python3
#
# __init__.py - Base resource class and parsing support
#
# Copyright 2021 mikee47 <mike@sillyhouse.net>
#
# This file is part of the Sming-Graphics Library
#
# This library is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, version 3 or later.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this library.
# If not, see <https://www.gnu.org/licenses/>.
#
# @author: July 2021 - mikee47 <mike@sillyhouse.net>
#

from . import font, gfx, linux, vlw, freetype, pfi
from . import image

# Dictionary of registered resource type parsers
#   'type': parse_item(item, name)
parsers = {
    'font': font.parse_item,
    'image': image.parse_item,
}


def parse(resources):
    list = []
    for type, entries in resources.items():
        parse = parsers.get(type)
        if not parse:
            raise InputError("Unsupported resource type '%s'" % type)
        for name, item in entries.items():
            # status("Parsing %s '%s'..." % (type, name))
            obj = parse(item, name)
            list.append(obj)
    return list


def writeHeader(data: list, out):
    out.write(
        "/**\n"
        " * Auto-generated file\n"
        " */\n"
        "\n"
        "#include <Graphics/resource.h>\n"
        "#include <FlashString/Vector.hpp>\n"
        "\n"
        "namespace Graphics {\n"
        "namespace Resource {\n"
        "\n"
    )
    bmOffset = 0
    for item in data:
        item.bmOffset = bmOffset
        bmOffset = item.writeHeader(bmOffset, out)
        item.bmSize = bmOffset - item.bmOffset

    out.write("DEFINE_FSTR_VECTOR(fontTable, FontResource,\n")
    for item in data:
        if isinstance(item, font.Font):
            out.write("\t&%s,\n" % item.name)
    out.write(");\n\n")

    out.write("DEFINE_FSTR_VECTOR(imageTable, ImageResource,\n")
    for item in data:
        if isinstance(item, image.Image):
            out.write("\t&%s,\n" % item.name)
    out.write(");\n\n")

    out.write(
        "} // namespace Resource\n"
        "} // namespace Graphics\n"
    )

    out.write("\n/*\nSummary\n")
    out.write("Bitmap Offset  Size    Header  Type      Name\n")
    out.write("----------     ------  ------  --------  ---------\n")
    headerSize = 0
    for item in data:
        out.write("0x%08x     %6u  %6u  %-8s  %s\n"
            % (item.bmOffset, item.bmSize, item.headerSize, type(item).__name__, item.name))
        headerSize += item.headerSize
    out.write("----------     ------  ------  --------  ---------\n")
    out.write("Total:         %6u  %6u\n*/\n\n" % (bmOffset, headerSize))


def writeBitmap(data: list, out):
    for item in data:
        item.writeBitmap(out)
