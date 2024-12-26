#!/usr/bin/env python3
#
# evf.py - EVE ROM fonts
#
# Copyright 2024 mikee47 <mike@sillyhouse.net>
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
# @author: Dec 2024 - mikee47 <mike@sillyhouse.net>
#

import os
from PIL import Image, ImageOps
from .font import Glyph, Typeface
from dataclasses import dataclass
from enum import IntEnum
import json

RAW_FORMATS = dict([
    (1, ('1', 1)),
    (2, ('L;2', 2)),
    (4, ('L;4', 4)),
    (8, ('L', 8)),
])

@dataclass
class FontMetrics:
    char_width: list[int]   # Width of each character glyph in pixels
    bpp: int                # Bits per pixel for glyph bitmap
    stride: int             # Font line stride (bytes)
    width: int              # Font width in pixels
    height: int             # Font height in pixels
    descent: int

    def __init__(self, data: dict):
        for k, v in data.items():
            setattr(self, k, v)


def parse_typeface(typeface: Typeface):
    basename = os.path.splitext(typeface.source)[0]
    handle = int(basename[-2:])
    is_extended = handle in [17, 19]

    with open(typeface.source, 'rb') as f:
        metrics = FontMetrics(json.load(f))
    with open(basename + '.raw', 'rb') as f:
        bitmap = f.read()

    glyph_size = metrics.height * metrics.stride

    ch = 128 if is_extended else 0
    bmpoff = 0

    typeface.descent = metrics.descent
    typeface.yAdvance = metrics.height + 4

    for w in metrics.char_width:
        if w != 0:
            g = Glyph(typeface)
            g.codePoint = ch
            g.xAdvance = w
            g.xOffset = 0
            g.yOffset = typeface.descent - metrics.height

            bmp = bitmap[bmpoff : bmpoff + glyph_size]
            rawfmt, g.alpha = RAW_FORMATS[metrics.bpp]
            img = Image.frombuffer(rawfmt[0], (metrics.width, metrics.height), bmp, 'raw', (rawfmt, metrics.stride))

            g.set_bitmap(img)
            typeface.glyphs.append(g)

            bmpoff += glyph_size

        ch += 1


from .font import parsers
parsers['.evf'] = parse_typeface
