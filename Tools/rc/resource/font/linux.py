#
# Linux font parser
#
# https://github.com/torvalds/linux/tree/master/lib/fonts
#
# These are distributed as source C files
#

import re, resource.font, array, struct
from resource import compact_string

def parse_typeface(typeface):
    with open(typeface.source) as f:
        data = f.read()

    # Strip comments
    # https://blog.ostermiller.org/finding-comments-in-source-code-using-regular-expressions/
    p = re.compile('(?:/\*(?:[^*]|(?:\*+[^*/]))*\*+/)|(?://.*)')
    data = p.sub('', data)

    # Bitmap
    p = re.compile('const struct font_data (.+) = {\s+{.*},\s+{([\s\S^}]*?)}')
    m = p.search(data)
    bitmap = compact_string(m.groups()[1])
    bitmap = bitmap.rstrip(',')
    bitmap = bytearray([int(num, 0) for num in bitmap.split(',')])

    # Typeface
    p = re.compile('const struct font_desc (.+) = {([\s\S^}]*)};')
    m = p.search(data)
    desc = compact_string(m.groups()[1])
    # print("desc = %s" % desc)
    width = 0
    height = 0
    charcount = 256
    for p in desc.split(','):
        (key, value) = p.split('=')
        if key == '.width':
            width = int(value)
        elif key == '.height':
            height = int(value)
        elif key == '.charcount':
            charcount = int(value)
        elif key == '.data':
            break

    typeface.yAdvance = 5 + height
    typeface.descent = 2

    # Convert bitmap to internal (GFX) format
    # 1. Identify defined area, exclude surround empty region
    # 2. Pack bits
    colBytes = (width + 7) // 8

    def getRows(off):
        # Convert src bytearray to array of words
        rows = array.array('Q', [0 for x in range(height)])
        for a in range(height):
            w = 0
            for b in bitmap[off:off+colBytes]:
                w = (w << 8) | b
            s = width % 8
            if s != 0:
                w >>= 8 - s
            rows[a] = w
            off += colBytes
        return rows

    # Build the glyphs
    codePoint = 0
    offset = 0
    for codePoint in range(charcount):
        if codePoint in typeface.font.codePoints:
            g = resource.font.Glyph(typeface)
            g.codePoint = codePoint
            rows = getRows(offset)
            g.packBits(rows, width)
            g.xAdvance = 1 + width
            typeface.glyphs.append(g)
        offset += height * colBytes


from .font import parsers
parsers['linux/'] = parse_typeface
