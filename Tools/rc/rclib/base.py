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

import os
import enum

# Used to calculate compiled size of resource header information
class StructSize(enum.IntEnum):
    GlyphResource = 8
    GlyphBlock = 4,
    Typeface = 16,
    Font = 24,
    Image = 20,


def compact_string(s):
    return ''.join(s.split())


def fstrSize(s):
    """Calculate size of FlashString definition
    """
    n = len(s) + 1 # Include NUL terminator
    n = 4 * ((n + 3) // 4) # Align to 4-byte word boundary
    return 4 + n # Preceded by length word


class Resource(object):
    """Base resource class
    """
    def __init__(self):
        self.name = None
        self.id = None
        self.comment = None
        self.source = None

    def writeComment(self, out):
        if self.source:
            out.write("// %s\n" % self.source)
        if self.comment:
            out.write("// %s\n" % self.comment)

resourcePaths = [
    '${GRAPHICS_LIB_ROOT}/resource',
]

def findFile(filename, dirs = []):
    if os.path.exists(filename):
        return filename

    alldirs = []
    for directory in [os.path.expandvars(path) for path in resourcePaths + dirs]:
        for walkroot, walkdir, walkfilenames in os.walk(directory):
            alldirs.append(walkroot)

    for dir in alldirs:
        path = os.path.join(dir, filename)
        if os.path.exists(path):
            # status("Found '%s'" % path)
            return path

    raise FileNotFoundError(filename)
