import array
from sdl2 import *
from Util import debug

class Rect:
    def __init__(self, x = 0, y = 0, w = 0, h = 0):
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    @classmethod
    def fromRect(cls, r):
        return cls(r.x, r.y, r.w, r.h)

    def __str__(self):
        return "%u, %u, %u, %u" % (self.x, self.y, self.w, self.h)

    def left(self):
        return self.x

    def right(self):
        return self.x + self.w - 1

    def top(self):
        return self.y

    def bottom(self):
        return self.y + self.h - 1

    def contains(self, x, y):
        return x >= self.left() and x <= self.right() and y >= self.top() and y <= self.bottom()

    def sdl_rect(self):
        return SDL_Rect(self.x, self.y, self.w, self.h)


def makeColor(r, g, b):
    return (r << 16) | (g << 8) | b

def blend(dst, src):
    alpha = src >> 24
    def blendChannel(shift):
        a = (src >> shift) & 0xff
        b = (dst >> shift) & 0xff
        return (alpha * a // 0xff) + ((0xff - alpha) * b // 0xff)
    r = blendChannel(16)
    g = blendChannel(8)
    b = blendChannel(0)
    return makeColor(r, g, b)

class PixelBuffer:
    def __init__(self, width, height):
        self.width, self.height = width, height
        self.pixels = array.array('I', [0 for x in range(width * height)])

    def get(self, x, y):
        if x >= self.width or y >= self.height:
            return 0
        return self.pixels[y * self.width + x]

    def set(self, x, y, color):
        if x < self.width and y < self.height:
            self.pixels[y * self.width + x] = color

    def updateTexture(self, texture, r = None):
        addr, xlen = self.pixels.buffer_info()
        if r is None:
            SDL_UpdateTexture(texture, None, addr, self.width * 4)
        else:
            r.w = min(r.w, self.width - r.x)
            r.h = min(r.h, self.height - r.y)
            addr += (r.x + r.y * self.width) * 4
            SDL_UpdateTexture(texture, r.sdl_rect(), addr, self.width * 4)

    def fill(self, rect, color):
        for y in range(0, rect.h):
            for x in range(0, rect.w):
                off = rect.x + x + (rect.y + y) * self.width
                self.pixels[off] = blend(self.pixels[off], color)

    def copy(self, src, dstx, dsty):
        def copyLine(y):
            srcoff = src.x + (src.y + y) * self.width
            dstoff = dstx + (dsty + y) * self.width
            w = min(src.w, self.width - max(src.x, dstx))
            self.pixels[dstoff : dstoff + w] = self.pixels[srcoff : srcoff + w]

        h  = min(src.h, self.height - src.y, self.height - dsty)
        if src.y < dsty:
            for y in range(h, 0, -1):
                copyLine(y - 1)
        else:
            for y in range(0, h):
                copyLine(y)

    def getLine(self, x, y, w):
        off = x + y * self.width
        return self.pixels[off : off + w]

    def setLine(self, x, y, line):
        off = x + y * self.width
        self.pixels[off : off + len(line)] = line

    def scroll(self, area, cx, cy, wrapx, wrapy, fill):
        # debug("scroll %s, (%d, %d), (%s, %s), 0x%06x" % (area, cx, cy, wrapx, wrapy, fill))

        filledRow = array.array('I', [fill for x in range(area.w)])
        fillRange = range(0,0) if wrapy else range(0, cy) if cy > 0 else range(area.h + cy, area.h)

        def getLine(y):
            return self.getLine(area.x, area.y + y, area.w)

        def setLine(y, line):
            self.setLine(area.x, area.y + y, line)

        def scrollLine(line):
            if cx < 0:
                return line[-cx:] + (line[:-cx] if wrapx else filledRow[:-cx])
            else:
                return (line[len(line)-cx:] if wrapx else filledRow[:cx]) + line[:len(line)-cx]

        y = (area.h + cy - 1) if cy < 0 else cy - 1
        line = getLine(y)
        for i in range(area.h):
            yd = y + cy
            if yd < 0:
                yd += area.h
            elif yd >= area.h:
                yd -= area.h

            if ((i + 1) * cy) % area.h == 0:
                y += 1 if cy > 0 else -1
            else:
                y += cy
            if y < 0:
                y += area.h
            elif y >= area.h:
                y -= area.h
            tmp = getLine(y)

            if yd in fillRange:
                setLine(yd, filledRow)
            else:
                setLine(yd, scrollLine(line))
            line = tmp
