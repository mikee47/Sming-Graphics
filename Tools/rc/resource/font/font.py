#
# font.py
#

import resource, enum, os, sys
from resource import InputError


class Glyph(resource.Resource):
    class Flag(enum.IntEnum):
        alpha = 0x01,

    def __init__(self, typeface):
        super().__init__()
        self.typeface = typeface
        self.codePoint = None
        self.bitmap = None
        self.width = None
        self.height = None
        self.xOffset = None
        self.yOffset = None
        self.xAdvance = None
        self.flags = 0

    def packBits(self, rows, width):
        """ Convert bitmap to internal (GFX) format
            Source bitmap source data is array.array('I') of row bitmap data,
            with last bit in position 0.
            We identify defined area, exclude surround empty region, then pack bits
            and update glyph details.
        """
        if self.flags & resource.font.Glyph.Flag.alpha:
            return

        height = len(rows)

        # Identify leading/trailing blank rows and columns
        leadingRows = trailingRows = 0
        leadingCols = trailingCols = width
        started = False
        for row in rows:
            if row == 0:
                if started:
                    trailingRows += 1
                else:
                    leadingRows += 1
            else:
                started = True
                trailingRows = 0
                # Examine columns
                n = 0
                mask = 1 << (width - 1)
                for i in range(width):
                    if row & mask != 0:
                        break
                    n += 1
                    mask >>= 1
                leadingCols = min(leadingCols, n)
                mask = 0x01
                n = 0
                for i in range(width):
                    if row & mask != 0:
                        break
                    n += 1
                    mask <<= 1
                trailingCols = min(trailingCols, n)

        # print("leadingRows %u, trailingRows %u, height %u" % (leadingRows, trailingRows, height))

        if leadingRows == height:
            self.width = self.height = self.xOffset = self.yOffset = 0
            self.bitmap = bytearray(0)
            return

        # Existing glyph attributes should only be adjusted
        if self.width is None:
            self.width = width
        self.width -= leadingCols + trailingCols
        if self.height is None:
            self.height = height
        self.height -= leadingRows + trailingRows
        if self.xOffset is None:
            self.xOffset = 0
        self.xOffset += leadingCols
        if self.yOffset is None:
            self.yOffset = -height
        self.yOffset += leadingRows

        # print("glyph %d x %d, leadingRows %u, trailingRows %u, leadingCols %u, trailingCols %u"
        #     % (self.width, self.height, leadingRows, trailingRows, leadingCols, trailingCols))

        n = self.height
        destBytes = (self.width * self.height + 7) // 8
        self.bitmap = bytearray(destBytes)
        off = 0
        dstmask = 0x80
        for i in range(n):
            row = rows[leadingRows + i]
            srcmask = 1 << (width - leadingCols - 1)
            for j in range(self.width):
                if row & srcmask != 0:
                    self.bitmap[off] |= dstmask
                srcmask >>= 1
                dstmask >>= 1
                if dstmask == 0:
                    dstmask = 0x80
                    off += 1
        # print("packBits %u; width %u, height %u, leading %u, trailing %u" % (len(src), width, height, leading, trailing))


class Typeface(resource.Resource):
    def __init__(self, font, style):
        super().__init__()
        self.font = font
        self.style = style
        self.bitmap = None
        self.yAdvance = None
        self.descent = None
        self.glyphs = []
        self.headerSize = 0

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
            out.write("\t{ 0x%04x, %3u, %3u, %3d, %3d, %3u, 0x%02x }, // #0x%04x %s \n" %
                (bmOffset, g.width, g.height, g.xOffset, g.yOffset, g.xAdvance, g.flags, g.codePoint, c))
            bmOffset += len(g.bitmap)
            self.headerSize += resource.StructSize.GlyphResource
        out.write("};\n\n")
        return bmOffset

    def writeGlyphBlocks(self, out):
        # Array defining blocks of glyphs with consecutive code points
        cp = -1
        count = 0
        length = 0

        def writeBlock():
            out.write("\t{ 0x%04x, %u },\n" % (cp, length))
            self.headerSize += resource.StructSize.GlyphBlock

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

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        bmSize = self.writeGlyphRecords(out)
        numBlocks = self.writeGlyphBlocks(out)
        # The main typeface record
        super().writeComment(out)
        out.write("const TypefaceResource %s_typeface PROGMEM {\n" % self.name)
        out.write("\t.bmOffset = 0x%08x,\n" % bmOffset)
        bmSize = sum(len(g.bitmap) for g in self.glyphs)
        out.write("//\t.bmSize = %u,\n" % bmSize)
        if self.style != []:
            out.write("\t.style = uint8_t(FontStyles(%s).value()),\n" % ' | '.join('FontStyle::' + style for style in self.style))
        out.write("\t.yAdvance = %u,\n" % self.yAdvance)
        out.write("\t.descent = %u,\n" % self.descent)
        out.write("\t.numBlocks = %u,\n" % numBlocks)
        out.write("\t.glyphs = %s_glyphs,\n" % self.name)
        out.write("\t.blocks = %s_blocks,\n" % self.name)
        out.write("};\n\n")
        self.headerSize += resource.StructSize.Typeface
        return bmOffset + bmSize

    def writeBitmap(self, out):
        for g in self.glyphs:
            out.write(g.bitmap)


class Font(resource.Resource):
    def __init__(self):
        super().__init__()
        self.typefaces = []
        self.yAdvance = 0
        self.descent = 0
        self.headerSize = 0

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        super().writeComment(out)
        for face in self.typefaces:
            bmOffset = face.writeHeader(bmOffset, out)
            self.headerSize += face.headerSize
        out.write("DEFINE_FSTR_LOCAL(%s_name, \"%s\")\n" % (self.name, self.name))
        self.headerSize += resource.fstrSize(self.name)
        out.write("FontResource %s {\n" % self.name)
        out.write("\t.name = &%s_name,\n" % self.name)
        out.write("\t.yAdvance = %u,\n" % self.yAdvance)
        out.write("\t.descent = %u,\n" % self.descent)
        out.write("\t.faces = {\n")
        for face in self.typefaces:
            out.write("\t\t&%s_typeface,\n" % face.name)
        out.write("\t},\n")
        out.write("};\n\n")
        self.headerSize += resource.StructSize.Font
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
#   typeface.font.mono
#   typeface.font.codePoints
#   typeface.style
#
parsers = {}

def getParser(resname):
    s = resname.lower()
    for tag, parser in parsers.items():
        if s.endswith(tag) or s.startswith(tag):
            return parser

    raise InputError("Cannot determine type of font '%s'" % resname)


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
    font.mono = item.get('mono', False)
    font.codePoints = codePoints

    def add(name, style):
        resname = item.get(name)
        if resname is None:
            return

        path = findFont(resname)
        parse = getParser(resname)

        typeface = resource.font.Typeface(font, style)
        typeface.name = font.name + '_' + name
        typeface.source = path
        resource.status("  typeface: '%s'..." % resname)

        parse(typeface)
        font.descent = max(font.descent, typeface.descent)
        font.yAdvance = max(font.yAdvance, typeface.yAdvance)
        font.typefaces.append(typeface)
    add('normal', [])
    add('italic', ['Italic'])
    add('bold', ['Bold'])
    add('boldItalic', ['Bold', 'Italic'])
    return font


def findFont(filename):
    dirs = []
    if sys.platform == "win32":
        windir = os.environ.get("WINDIR")
        if windir:
            dirs.append(os.path.join(windir, "fonts"))
    elif sys.platform in ("linux", "linux2"):
        lindirs = os.environ.get("XDG_DATA_DIRS", "")
        if not lindirs:
            lindirs = "/usr/share"
        dirs += [os.path.join(lindir, "fonts") for lindir in lindirs.split(":")]
    elif sys.platform == "darwin":
        dirs += [
            "/Library/Fonts",
            "/System/Library/Fonts",
            os.path.expanduser("~/Library/Fonts"),
        ]
    else:
        raise SystemError("Unsupported platform: " % sys.platform)

    return resource.findFile(filename, dirs)
