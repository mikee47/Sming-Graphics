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
from PIL import Image, ImageChops
from io import BytesIO


def pointsToPixels(points26):
    return round(points26 / 64)


def print_bitmap_diff(bitmap: freetype.Bitmap):
    if bitmap.width + bitmap.rows == 0:
        return
    mode = '1' if bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO else 'L'
    img = Image.frombuffer(mode, (bitmap.width, bitmap.rows), bytearray(bitmap.buffer))
    bg = Image.new(mode, (bitmap.width, bitmap.rows), 0)
    diff = ImageChops.difference(img, bg)
    diff = ImageChops.add(diff, diff, 2.0, -100)
    bbox = diff.getbbox()
    if bbox is None:
        return
    w, h = bbox[2] - bbox[0], bbox[3] - bbox[1]
    size_orig = bitmap.width * bitmap.rows
    size_new = w * h
    pix_diff = size_orig - size_new
    if pix_diff:
        print(f'glyph = ({bitmap.width}, {bitmap.rows}), bbox = ({w}, {h}), pix. diff {pix_diff} ({size_orig} -> {size_new})')


def parse_typeface(typeface: Typeface):
    face = freetype.Face(typeface.source)

    # printDetails(face)

    if typeface.font.pointSize is None:
        face.select_size(0)
    else:
        face.set_char_size(round(typeface.font.pointSize * 64))

    typeface.comment = "%s %s" % (face.family_name.decode(), face.style_name.decode())
    typeface.yAdvance = pointsToPixels(face.size.ascender + abs(face.size.descender))
    typeface.descent = abs(pointsToPixels(face.size.descender))

    for c in typeface.font.codePoints:
        index = face.get_char_index(c)
        if index == 0:
            continue # No glyph for this codepoint
        flags = freetype.FT_LOAD_RENDER
        if not typeface.font.alpha:
            flags |= freetype.FT_LOAD_MONOCHROME | freetype.FT_LOAD_TARGET_MONO
        face.load_glyph(index, flags)
        bitmap = face.glyph.bitmap
        w, h = bitmap.width, bitmap.rows
        g = Glyph(typeface)
        g.codePoint = c
        g.width = w
        g.height = h
        g.xAdvance = pointsToPixels(face.glyph.advance.x)
        g.xOffset = face.glyph.bitmap_left
        g.yOffset = 1 - face.glyph.bitmap_top

        if w + h == 0:
            g.bitmap = bytearray(0)
        elif False: #bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO:
            print_bitmap_diff(bitmap)

            g.alpha = Glyph.Alpha.L1
            g.bitmap = bytearray(bitmap.buffer)

            # Pack source bits so resulting data is as compact as possible
            stride = bitmap.pitch
            print(f'Bitmap {w} x {h}, stride {stride}')
            dstsize = (w * h + 7) // 8
            dstbuf = bytearray(dstsize)
            if stride == 1:
                stride_fmt = 'B'
            elif stride == 2:
                stride_fmt = '>H'
            elif stride == 4:
                stride_fmt = '>L'
            elif stride == 8:
                stride_fmt = '>Q'
            else:
                raise ValueError(f'Unsupported stride {stride}')
            srcoff = 0
            dstoff = 0
            dstbits = 0
            dstbitlen = 0
            lshift = 8 * stride - w
            rshift = 8 * (stride + 1)
            topshift = rshift - 8
            for y in range(h):
                srcbits, = struct.unpack(stride_fmt, g.bitmap[srcoff : srcoff+stride])
                rshift -= w
                # print(f'y {y}, srcbits {srcbits:x}, rshift {rshift}')
                srcoff += stride
                dstbits |= (srcbits >> lshift) << rshift
                dstbitlen += w
                while dstbitlen >= 8:
                    b = dstbits >> topshift
                    # print(f'y {y}, dstoff {dstoff}, dstbits {dstbits:x}, {b:x}')
                    dstbuf[dstoff] = b
                    dstbits -= (b << topshift)
                    dstbits <<= 8
                    dstbitlen -= 8
                    dstoff += 1
                    rshift += 8
            if dstoff < dstsize:
                dstbuf[dstoff] = dstbits >> topshift
            g.bitmap = dstbuf

            # rows = array.array('Q', [0 for x in range(bitmap.rows)])
            # offset = 0
            # for y in range(bitmap.rows):
            #     r = 0
            #     for i in range(bitmap.pitch):
            #         r = (r << 8) | bitmap.buffer[offset]
            #         offset += 1
            #     r >>= (8 * bitmap.pitch) - bitmap.width
            #     rows[y] = r
            # g.packBits(rows, bitmap.width)
        # elif typeface.font.alpha == Glyph.Alpha.L2:
        #     g.alpha = Glyph.Alpha.L2
        #     g.bitmap = bytearray(((g.width * g.height) + 3) // 4)
        #     i = off = 0
        #     for y in range(bitmap.rows):
        #         for x in range(bitmap.width):
        #             g.bitmap[i] = bitmap.buffer[off + x]
        #             i += 1
        #         off += bitmap.pitch
        # elif typeface.font.alpha == Glyph.Alpha.L4:
        #     g.alpha = Glyph.Alpha.L4
        #     g.bitmap = bytearray(g.width * g.height)
        #     i = off = 0
        #     for y in range(bitmap.rows):
        #         for x in range(bitmap.width):
        #             g.bitmap[i] = bitmap.buffer[off + x]
        #             i += 1
        #         off += bitmap.pitch
        else:
            # print_bitmap_diff(bitmap)

            if bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO:
                img = Image.frombuffer('1', (w, h), bytearray(bitmap.buffer), 'raw', ('1', bitmap.pitch))
                img = img.convert('L')
                imgdata = img.tobytes()
                g.alpha = Glyph.Alpha.L8
                g.bitmap = imgdata
            else:
                g.alpha = Glyph.Alpha.L8
                g.bitmap = bytearray(bitmap.buffer)

        typeface.glyphs.append(g)


from .font import parsers
parsers['.ttf'] = parse_typeface
parsers['.otf'] = parse_typeface
parsers['.pcf'] = parse_typeface
parsers['.pcf.gz'] = parse_typeface
