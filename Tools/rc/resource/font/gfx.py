#!/usr/bin/env python3
#
# gfx.py - Arduino GFX font parser
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
# GFX fonts are distributed as header files
#

import re, resource.font

def compact_string(s):
    return ''.join(s.split())

def parse_typeface(typeface):
    with open(typeface.source) as f:
        data = f.read()

    # Strip comments
    # https://blog.ostermiller.org/finding-comments-in-source-code-using-regular-expressions/
    p = re.compile('(?:/\*(?:[^*]|(?:\*+[^*/]))*\*+/)|(?://.*)')
    data = p.sub('', data)

    # Code based on https://github.com/tchapi/Adafruit-GFX-Font-Customiser/blob/master/index.html
    # Bitmap
    p = re.compile('const\ uint8\_t\ (.*)Bitmaps\[\]\ PROGMEM\ =\ {([\s\S^}]*?)};')
    m = p.search(data)
    typeface.name = m.groups()[0]
    bitmap = compact_string(m.groups()[1])
    bitmap = bitmap.rstrip(',')
    bitmap = bytearray([int(num, 0) for num in bitmap.split(',')])
    # print("name %s, bytes %u" % (typeface.name, len(typeface.bitmap)))
    # print(typeface.bitmap)

    # Typeface
    p = re.compile('const\ GFXfont\ ([\s\S^}]*)}')
    m = p.search(data)
    last_part = m.groups()[0]
    last_part = compact_string(last_part)
    parts = last_part.split(',')
    codePoint = int(parts[2], 0)
    lastCodePoint = int(parts[3], 0)
    typeface.yAdvance = int(parts[4], 0)

    # Glyph index
    p = re.compile('const\ GFXglyph\ (.*)Glyphs\[\]\ PROGMEM\ =\ {([\s\S^}]*?)};')
    m = p.search(data)
    name = m.groups()[0]
    glyphGroups = m.groups()[1]
    p = re.compile('\s*{([\s\S^}]*?)}')
    glyphGroups = p.findall(glyphGroups)
    descent = 0
    for s in glyphGroups:
        if codePoint in typeface.font.codePoints:
            g = resource.font.Glyph(typeface)
            e = [int(num, 0) for num in s.split(',')]
            g.codePoint = codePoint
            bmOffset = e[0]
            g.width = e[1]
            g.height = e[2]
            g.xAdvance = e[3]
            g.xOffset = e[4]
            g.yOffset = e[5]
            bmSize = (g.width * g.height + 7) // 8
            g.bitmap = bitmap[bmOffset:bmOffset+bmSize]
            typeface.glyphs.append(g)
            descent = max(descent, g.yOffset + g.height)
        codePoint += 1
    typeface.descent = descent


from .font import parsers
parsers['gfx/'] = parse_typeface
