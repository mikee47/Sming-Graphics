#!/usr/bin/env python3
#
# freetype.py - FreeType font parser
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
# Convert any free-type supported fonts (OpenFont, TrueType, etc.)
#

import os
import struct
import array
import freetype
from .font import Glyph, Typeface
from PIL import Image
from common import critical


def pointsToPixels(points26):
    return round(points26 / 64)


def parse_typeface(typeface: Typeface):
    paths = typeface.source if isinstance(typeface.source, list) else [typeface.source]
    faces = [freetype.Face(path) for path in paths]
    for face in faces:
        if typeface.font.pointSize is None:
            face.select_size(0)
        else:
            face.set_char_size(round(typeface.font.pointSize * 64))

    face = faces[0]
    typeface.comment = "%s %s" % (face.family_name.decode(), face.style_name.decode())
    typeface.yAdvance = pointsToPixels(face.size.ascender + abs(face.size.descender))
    typeface.descent = abs(pointsToPixels(face.size.descender))

    for c in typeface.font.codePoints:
        for face in faces:
            index = face.get_char_index(c)
            if index > 0:
                break
        if index == 0:
            critical(f'No glyph available for character {c:x}')
            continue # No glyph for this codepoint
        flags = freetype.FT_LOAD_RENDER
        if typeface.font.alpha == 1:
            flags |= freetype.FT_LOAD_MONOCHROME | freetype.FT_LOAD_TARGET_MONO
        face.load_glyph(index, flags)
        bitmap = face.glyph.bitmap
        w, h = bitmap.width, bitmap.rows
        g = Glyph(typeface)
        g.codePoint = c
        g.xAdvance = pointsToPixels(face.glyph.advance.x)
        g.xOffset = face.glyph.bitmap_left
        g.yOffset = 1 - face.glyph.bitmap_top

        mode = '1' if bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO else 'L'
        img = Image.frombuffer(mode, (w, h), bytearray(bitmap.buffer), 'raw', (mode, bitmap.pitch, 1))
        g.set_bitmap(img)
        typeface.glyphs.append(g)


from .font import parsers
parsers['.ttf'] = parse_typeface
parsers['.otf'] = parse_typeface
parsers['.pcf'] = parse_typeface
parsers['.pcf.gz'] = parse_typeface
