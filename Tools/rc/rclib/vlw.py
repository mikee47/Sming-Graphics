#!/usr/bin/env python3
#
# vlw.py - VLW font file parser
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


# VLW files are produced by https://www.processing.org

# VLW Font file v11 layout:
#
#   VlwFontHeader hdr;
#   VlwGlyphHeader glyphs[hdr.numGlyphs];
#   uint8_t bitmapData; // Each glyph width*height bytes
#   // Footer
#   uint16_t namelen;
#   uint8_t name[];
#   uint16_t psnamelen;
#   uint8_t psname[];
#   uint8_t smooth;
#

# Header for .vlw font file
#
# Note: Byte order of .VLW file entries is MSB-first
#
# struct VlwFontHeader {
# 	uint32_t numGlyphs;
# 	uint32_t version;
# 	uint32_t pointSize;
# 	uint32_t mboxY; // Ignore
# 	uint32_t ascent;
# 	uint32_t descent;
# };

# Header for glyphs within .VLW font file
#
# struct VlwGlyphHeader {
# 	uint32_t codePoint;
# 	uint32_t height;
# 	uint32_t width;
# 	uint32_t xAdvance;   // x advance to next character position
# 	uint32_t topExtent;  // glyph y offset from baseline
# 	uint32_t leftExtent; // glyph x offset from cursor
# 	uint32_t c_ptr;		 // Ignore
# 						 // Followed by bitmap, 1 byte alpha per pixel
# 						 // uint8_t bitmap[width * height]
# };

import struct
from .font import Glyph

def parse_typeface(typeface):
    with open(typeface.source, "rb") as f:
        data = f.read()

    FONT_HEADER_SIZE = 24
    GLYPH_HEADER_SIZE = 28

    offset = 0
    numGlyphs, version, pointSize, mboxY, ascent, descent = struct.unpack_from('>6i', data, offset)
    offset += FONT_HEADER_SIZE

    bmOffset = offset + (numGlyphs * GLYPH_HEADER_SIZE)
    for i in range(numGlyphs):
        g = Glyph(typeface)
        g.codePoint, g.height, g.width, g.xAdvance, topExtent, g.xOffset, c_ptr = struct.unpack_from('>7i', data, offset)
        bmSize = g.height * g.width
        if g.codePoint in typeface.font.codePoints:
            topExtent = min(topExtent, 127)
            if g.codePoint > 0x20 and g.codePoint != 0x7F and (g.codePoint < 0xA0 or g.codePoint > 0xFF):
                descent = max(descent, g.height - topExtent)
                ascent = max(ascent, topExtent)
            g.yOffset = -topExtent
            g.flags = Glyph.Flag.alpha
            g.bitmap = data[bmOffset:bmOffset+bmSize]
            typeface.glyphs.append(g)
        offset += GLYPH_HEADER_SIZE
        bmOffset += bmSize
    offset = bmOffset

    typeface.descent = descent
    typeface.yAdvance = 2 + ascent + descent # Adds a little room for overscore

    def readName():
        nonlocal data, offset
        namelen, = struct.unpack_from('>H', data, offset)
        offset += 2
        name = data[offset:offset+namelen].decode()
        offset += namelen
        return name

    name = readName()
    psname = readName()
    smooth = data[offset]

    # status("name '%s', psname '%s', smooth %u, offset %u, len(data) %u" % (name, psname, smooth, offset, len(data)))
    # status("\tnumGlyphs %u, version %u, pointSize %u, ascent %d, descent %d"
    #     % (numGlyphs, version, pointSize, ascent, descent))


from .font import parsers
parsers['.vlw'] = parse_typeface
