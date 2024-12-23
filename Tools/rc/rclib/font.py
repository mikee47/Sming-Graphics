#!/usr/bin/env python3
#
# font.py - Base support for font parsing
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

from __future__ import annotations
import enum
import os
import sys
import struct
from .base import Resource, findFile, StructSize, fstrSize
from PIL import Image, ImageChops

class FontStyle(enum.Enum):
    """Style is a set of these values, using strings here but bitfields in library"""
    # typeface
    Bold = 0
    Italic = 1
    Underscore = 2
    Overscore = 3
    Strikeout = 4
    DoubleUnderscore = 5
    DoubleOverscore = 6
    DoubleStrikeout = 7
    DotMatrix = 8
    HLine = 9
    VLine = 10

    @staticmethod
    def evaluate(value: list[str]):
        n = 0
        for x in value:
            n |= 1 << FontStyle[x].value
        return n


class Glyph(Resource):
    def __init__(self, typeface: Typeface):
        super().__init__()
        self.typeface: Typeface = typeface
        self.codePoint = None
        self.bitmap: bytearray = None
        self.width = None
        self.height = None
        self.xOffset = None
        self.yOffset = None
        self.xAdvance = None
        self.alpha = 1

    def set_bitmap(self, img: Image):
        """ Convert bitmap to internal (GFX) format and set width, height.
        We remove space round the glyph, update glyph offsets and pack the resulting bits.
        """

        # Find smallest bounding box and crop
        w, h = img.width, img.height
        bg = Image.new('L', (w, h), 0)
        diff = ImageChops.difference(img, bg)
        diff = ImageChops.add(diff, diff, 2.0, -100)
        bbox = diff.getbbox()

        if bbox:
            img = img.crop(bbox)
            self.xOffset += bbox[0]
            self.yOffset += bbox[1]
        else:
            img = img.crop((0,0,0,0))
        w, h = img.width, img.height

        self.width = w
        self.height = h
        alpha = 1 if img.mode == '1' else self.typeface.font.alpha
        self.alpha = alpha
        pixels_per_byte = 8 // alpha
        dstsize = (w * h + pixels_per_byte - 1) // pixels_per_byte

        # Pack source bits so resulting data is as compact as possible
        imgdata = img.tobytes('raw', ('L', 0, 1))

        # print(f'{self.typeface.font.name} bitmap {w} x {h}, alpha {alpha}, src {len(imgdata)} bytes, dst {dstsize}')

        if pixels_per_byte == 1:
            self.bitmap = imgdata
        else:
            dstbuf = bytearray(dstsize)
            dstoff = 0
            dstbyte = 0
            shift = 8
            for pixel in imgdata:
                shift -= alpha
                dstbyte |= (pixel >> (8 - alpha)) << shift
                if shift == 0:
                    dstbuf[dstoff] = dstbyte
                    dstoff += 1
                    dstbyte = 0
                    shift = 8
            if shift != 8:
                dstbuf[dstoff] = dstbyte
            self.bitmap = dstbuf


class Typeface(Resource):
    def __init__(self, font: Font, style):
        super().__init__()
        self.font: Font = font
        self.style = style
        self.yAdvance = None
        self.descent = None
        self.glyphs: list[Glyph] = []
        self.headerSize = 0

    def serialize(self, bmOffset, res_offset, ptr64: bool):
        glyph_data = self.serialize_glyphs()
        block_data = self.serialize_glyph_blocks()

        # `struct TypefaceResource'
        typefaceSize = StructSize.Typeface64 if ptr64 else StructSize.Typeface
        face_res = struct.pack('<IBBBBQQ' if ptr64 else '<IBBBBII',
            bmOffset,
            FontStyle.evaluate(self.style),
            self.yAdvance,
            self.descent,
            len(block_data) // StructSize.GlyphBlock,
            res_offset + typefaceSize,
            res_offset + typefaceSize + len(glyph_data)
        )
        assert(len(face_res) == typefaceSize)

        return face_res + glyph_data + block_data

    def serialize_glyphs(self):
        # Array of `struct GlyphResource`
        resdata = b''
        bmOffset = 0
        for g in self.glyphs:
            glyph_res = struct.pack('<HBBbbBB',
                bmOffset,
                g.width,
                g.height,
                g.xOffset,
                g.yOffset,
                g.xAdvance,
                g.flags)
            assert(len(glyph_res) == StructSize.GlyphResource)
            resdata += glyph_res
            bmOffset += len(g.bitmap)
        return resdata

    def serialize_glyph_blocks(self):
        # Array of `struct GlyphBlock`
        resdata = b''
        cp = -1
        count = 0
        length = 0

        def writeBlock():
            nonlocal resdata
            resdata += struct.pack('<HH', cp, length)

        for g in self.glyphs:
            if cp >= 0 and g.codePoint == cp + length:
                length += 1
                continue
            if cp >= 0:
                writeBlock()
                count += 1
            cp = g.codePoint
            length = 1
        writeBlock()
        return resdata

    def writeGlyphRecords(self, out):
        # Array of glyph definitions
        bmOffset = 0
        out.write("const GlyphResource %s_glyphs[] PROGMEM {\n" % self.name)
        for g in self.glyphs:
            c = chr(g.codePoint)
            if not c.isprintable():
                c = ''
            elif c == '\\':
                c = "'\\'"
            out.write("\t{ 0x%04x, %3u, %3u, %3d, %3d, %3u, GlyphResource::L%u }, // #0x%04x %s \n" %
                (bmOffset, g.width, g.height, g.xOffset, g.yOffset, g.xAdvance, g.alpha, g.codePoint, c))
            bmOffset += len(g.bitmap)
            self.headerSize += StructSize.GlyphResource
        out.write("};\n\n")
        return bmOffset

    def writeGlyphBlocks(self, out):
        # Array defining blocks of glyphs with consecutive code points
        cp = -1
        count = 0
        length = 0

        def writeBlock():
            out.write("\t{ 0x%04x, %u },\n" % (cp, length))
            self.headerSize += StructSize.GlyphBlock

        out.write("const GlyphBlock %s_blocks[] PROGMEM {\n" % self.name)
        for g in self.glyphs:
            if cp >= 0 and g.codePoint == cp + length:
                length += 1
                continue
            if cp >= 0:
                writeBlock()
                count += 1
            cp = g.codePoint
            length = 1
        writeBlock()
        out.write("};\n\n")
        count += 1
        return count

    def get_bitmap_size(self):
        return sum(len(g.bitmap) for g in self.glyphs)

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        bmSize = self.writeGlyphRecords(out)
        numBlocks = self.writeGlyphBlocks(out)
        # The main typeface record
        super().writeComment(out)
        out.write("const TypefaceResource %s_typeface PROGMEM {\n" % self.name)
        out.write("\t.bmOffset = 0x%08x,\n" % bmOffset)
        bmSize = self.get_bitmap_size()
        out.write("\t.bmSize = %u,\n" % bmSize)
        format = FontStyle.evaluate(self.style)
        out.write("\t.format = 0x%02x,\n" % format)
        out.write("\t.yAdvance = %u,\n" % self.yAdvance)
        out.write("\t.descent = %u,\n" % self.descent)
        out.write("\t.numBlocks = %u,\n" % numBlocks)
        out.write("\t.glyphs = %s_glyphs,\n" % self.name)
        out.write("\t.blocks = %s_blocks,\n" % self.name)
        out.write("};\n\n")
        self.headerSize += StructSize.Typeface
        return bmOffset + bmSize

    def writeBitmap(self, out):
        for g in self.glyphs:
            out.write(g.bitmap)


class Font(Resource):
    def __init__(self):
        super().__init__()
        self.typefaces = []
        self.yAdvance = 0
        self.descent = 0
        self.headerSize = 0
        self.alpha = 1

    def serialize(self, bmOffset, res_offset, ptr64: bool):
        resdata = b''
        face_offsets = []
        fontSize = StructSize.Font64 if ptr64 else StructSize.Font
        for typeface in self.typefaces:
            # print(typeface.name)
            offset = res_offset + fontSize + len(resdata)
            face_offsets.append(offset)
            resdata += typeface.serialize(bmOffset, offset, ptr64)
            bmOffset += typeface.get_bitmap_size()
        while len(face_offsets) < 4:
            face_offsets.append(0)

        # `struct FontResource`
        font_res = struct.pack('<QBB2B4Q' if ptr64 else '<IBB2B4I',
            0,
            self.yAdvance,
            self.descent,
            0, 0,
            *face_offsets)

        assert(len(font_res) == fontSize)

        return font_res + resdata

    def get_bitmap_size(self):
        return sum(face.get_bitmap_size() for face in self.typefaces)

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        super().writeComment(out)
        for face in self.typefaces:
            bmOffset = face.writeHeader(bmOffset, out)
            self.headerSize += face.headerSize
        out.write("DEFINE_FSTR_LOCAL(%s_name, \"%s\")\n" % (self.name, self.name))
        self.headerSize += fstrSize(self.name)
        out.write("FontResource %s {\n" % self.name)
        out.write("\t.name = &%s_name,\n" % self.name)
        out.write("\t.yAdvance = %u,\n" % self.yAdvance)
        out.write("\t.descent = %u,\n" % self.descent)
        out.write("\t.faces = {\n")
        for face in self.typefaces:
            out.write("\t\t&%s_typeface,\n" % face.name)
        out.write("\t},\n")
        out.write("};\n\n")
        self.headerSize += StructSize.Font
        return bmOffset

    def writeBitmap(self, out):
        for face in self.typefaces:
            face.writeBitmap(out)


# Dictionary of registered resource type parsers
#   'type': parse_typeface(typeface)
#
# The following fields are available:
#
#   typeface.source
#   typeface.font
#   typeface.font.name
#   typeface.font.pointSize
#   typeface.font.alpha
#   typeface.font.codePoints
#   typeface.style
#
parsers = {}

def getParser(resname):
    s = resname.lower()
    for tag, parser in parsers.items():
        if s.endswith(tag) or s.startswith(tag):
            return parser

    raise LookupError("Cannot determine type of font '%s'" % resname)


def parse_item(item, name):
    """Parse a font definition
    """
    codePoints = [ord(c) for c in item.get('chars', "")]
    cplist = item.get('codepoints', '0x20-0x7e')
    def val(s):
        return eval(s) if s.startswith('0x') else ord(s)
    for e in cplist.split(','):
        if e == '':
            continue
        r = e.split('-')
        if len(r) == 2:
            codePoints += range(val(r[0]), val(r[1]) + 1)
        else:
            codePoints.append(val(r[0]))
    codePoints.sort()
    codePoints = list(dict.fromkeys(codePoints))


    font = Font()
    font.name = name
    font.pointSize = item.get('size')
    mono = item.get('mono', False)
    font.alpha = item.get('alpha', 1 if mono else 8)
    font.codePoints = codePoints

    def add(name, style):
        resname = item.get(name)
        if resname is None:
            return

        path = findFont(resname)
        parse = getParser(resname)

        typeface = Typeface(font, style)
        typeface.name = font.name + '_' + name
        typeface.source = path
        # status("  typeface: '%s'..." % resname)

        parse(typeface)
        font.descent = max(font.descent, typeface.descent)
        font.yAdvance = max(font.yAdvance, typeface.yAdvance)
        font.typefaces.append(typeface)
    add('normal', [])
    add('italic', ['Italic'])
    add('bold', ['Bold'])
    add('boldItalic', ['Bold', 'Italic'])
    return font


def get_system_font_directories():
    font_dirs = set()

    if sys.platform == "win32":
        windir = os.environ.get("WINDIR")
        if windir:
            font_dirs.add(os.path.join(windir, "fonts"))
    elif sys.platform in ("linux", "linux2"):
        lindirs = os.environ.get("XDG_DATA_DIRS", "")
        if lindirs:
            font_dirs |= {os.path.join(lindir, "fonts") for lindir in lindirs.split(":")}
        font_dirs |= {
            "/usr/share/fonts",
            "/usr/share/x11/fonts",
            "$HOME/.local/share/fonts",
        }
    elif sys.platform == "darwin":
        font_dirs |= {
            "/Library/Fonts",
            "/System/Library/Fonts",
            os.path.expanduser("~/Library/Fonts"),
        }
    else:
        raise SystemError("Unsupported platform: " % sys.platform)

    return sorted(list(font_dirs))


def findFont(filename):
    return findFile(filename, system_font_directories)


system_font_directories = get_system_font_directories()
