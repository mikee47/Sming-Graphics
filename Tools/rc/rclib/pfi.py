#!/usr/bin/env python3
#
# pfi.py - Portable Font Index files
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
import PIL
import array
from .font import Glyph

def parse_typeface(typeface):
    with open(typeface.source) as f:
        pfi = f.read()
    pfi = pfi.split('\n')

    if pfi[0] != 'F1':
        raise ValueError('Require a text .PFI file')
    while pfi[1].startswith('#'):
        del pfi[1]
    # print("Name: %s" % pfi[1])
    dims = pfi[2].split('x')
    width = 0 if dims[0] == '?' else int(dims[0])
    height = int(dims[1])
    # print("Dims %u x %u" % (width, height))

    typeface.yAdvance = 5 + height
    typeface.descent = 2

    pbmFilename = os.path.splitext(typeface.source)[0] + '.pbm'
    pbm = PIL.Image.open(pbmFilename)
    # print("%s: %s, mode %s" % (os.path.basename(pbmFilename), pbm.size, pbm.mode))

    for line in pfi[3:]:
        line = line.split(' ')
        c = line[0]
        if c == '':
            break
        del line[0]
        if len(c) == 1:
            c = ord(c)
        else:
            c = int(c, 16)

        # if not c in typeface.font.codePoints:
        #     continue

        g = Glyph(typeface)
        g.codePoint = c
        if line[0] != '' and (width == 0 or len(line) == 3): # Proportional
            w = int(line[0])
            del line[0]
        else:
            w = width
        g.xAdvance = 1 + w

        rows = array.array('Q', [0 for y in range(height)])
        if line and line[0] != '':
            xo, yo = int(line[0]), int(line[1])
            for y in range(height):
                mask = 1 << (w - 1)
                for x in range(w):
                    xx = xo + x
                    try:
                        if pbm.getpixel((xx, yo + y)) == 0:
                            rows[y] |= mask
                    except IndexError:
                        pass # Empty pixels
                    mask >>= 1
        g.packBits(rows, w)
        typeface.glyphs.append(g)

    def sortkey(g):
        return g.codePoint
    typeface.glyphs.sort(key=sortkey)

from .font import parsers
parsers['.pfi'] = parse_typeface
