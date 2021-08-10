#
# FreeType font parser
#
# Convert any free-type supported fonts (OpenFont, TrueType, etc.)
#

import resource.font, freetype, os, array

def pointsToPixels(points26):
    return round(points26 / 64)


def parse_typeface(typeface):
    face = freetype.Face(typeface.source)

    # printDetails(face)

    if typeface.font.pointSize is None:
        face.select_size(0)
    else:
        face.set_char_size(round(typeface.font.pointSize * 64))

    typeface.comment = "%s %s" % (face.family_name.decode(), face.style_name.decode())
    typeface.yAdvance = pointsToPixels(face.size.height)
    typeface.descent = -pointsToPixels(face.size.descender)

    for c in typeface.font.codePoints:
        index = face.get_char_index(c)
        if index == 0:
            continue # No glyph for this codepoint
        flags = freetype.FT_LOAD_RENDER
        if typeface.font.mono:
            flags |= freetype.FT_LOAD_MONOCHROME | freetype.FT_LOAD_TARGET_MONO
        face.load_glyph(index, flags)
        bitmap = face.glyph.bitmap
        g = resource.font.Glyph(typeface)
        g.codePoint = c
        g.width = bitmap.width
        g.height = bitmap.rows
        g.xAdvance = pointsToPixels(face.glyph.advance.x)
        g.xOffset = face.glyph.bitmap_left
        g.yOffset = 1 - face.glyph.bitmap_top

        if bitmap.pixel_mode == freetype.FT_PIXEL_MODE_MONO:
            rows = array.array('Q', [0 for x in range(bitmap.rows)])
            offset = 0
            for y in range(bitmap.rows):
                r = 0
                for i in range(bitmap.pitch):
                    r = (r << 8) | bitmap.buffer[offset]
                    offset += 1
                r >>= (8 * bitmap.pitch) - bitmap.width
                rows[y] = r
            g.packBits(rows, bitmap.width)
        else:
            g.flags = resource.font.Glyph.Flag.alpha
            g.bitmap = bytearray(g.width * g.height)
            i = off = 0
            for y in range(bitmap.rows):
                for x in range(bitmap.width):
                    g.bitmap[i] = bitmap.buffer[off + x]
                    i += 1
                off += bitmap.pitch
        typeface.glyphs.append(g)


from .font import parsers
parsers['.ttf'] = parse_typeface
parsers['.otf'] = parse_typeface
parsers['.pcf'] = parse_typeface
parsers['.pcf.gz'] = parse_typeface
