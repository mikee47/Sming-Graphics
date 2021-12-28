#!/usr/bin/env python3
#
# rc.py - Graphics Resource Compiler
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

import os, sys, json, argparse
from resource import *
import resource.font
import resource.image

def openOutput(path):
    if path == '-':
        try:
            stdout_binary = sys.stdout.buffer  # Python 3
        except AttributeError:
            stdout_binary = sys.stdout
        return stdout_binary
    status("Writing to '%s'" % path)
    output_dir = os.path.abspath(os.path.dirname(path))
    os.makedirs(output_dir, exist_ok=True)
    return open(path, 'wb')


def main():
    parser = argparse.ArgumentParser(description='Sming Resource Compiler')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Name of resource script')
    parser.add_argument('output', help='Path to output directory')

    args = parser.parse_args()
    common.quiet = args.quiet
    resource.resourcePaths.append(os.path.dirname(os.path.abspath(args.input)))

    script = json_load(args.input)
    list = resource.parse(script['resources'])

    with openOutput(os.path.join(args.output, 'resource.h'), 'w') as out:
        writeHeader(list, out)
    with openOutput(os.path.join(args.output, 'resource.bin'), 'wb') as out:
        writeBitmap(list, out)
        bitmapSize = out.tell()

    structSize = sum(item.headerSize for item in list)
    status("Resource compiled %u items, structures are %u bytes, bitmap is %u bytes"
        % (len(list), structSize, bitmapSize))


def openOutput(path, mode):
    if path == '-':
        return sys.stdout
    status("Writing to '%s'" % path)
    output_dir = os.path.abspath(os.path.dirname(path))
    os.makedirs(output_dir, exist_ok=True)
    return open(path, mode)


def writeHeader(list, out):
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
    for item in list:
        item.bmOffset = bmOffset
        bmOffset = item.writeHeader(bmOffset, out)
        item.bmSize = bmOffset - item.bmOffset

    out.write("DEFINE_FSTR_VECTOR(fontTable, FontResource,\n")
    for item in list:
        if type(item) is resource.font.Font:
            out.write("\t&%s,\n" % item.name)
    out.write(");\n\n")

    out.write("DEFINE_FSTR_VECTOR(imageTable, ImageResource,\n")
    for item in list:
        if type(item) is resource.image.Image:
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
    for item in list:
        out.write("0x%08x     %6u  %6u  %-8s  %s\n"
            % (item.bmOffset, item.bmSize, item.headerSize, type(item).__name__, item.name))
        headerSize += item.headerSize
    out.write("----------     ------  ------  --------  ---------\n")
    out.write("Total:         %6u  %6u\n*/\n\n" % (bmOffset, headerSize))


def writeBitmap(list, out):
    for item in list:
        item.writeBitmap(out)


if __name__ == '__main__':
    try:
        main()
    except InputError as e:
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
