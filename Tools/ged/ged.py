import os
import sys
import copy
from enum import IntEnum
from random import randrange
import tkinter as tk


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480
MIN_ELEMENT_WIDTH = MIN_ELEMENT_HEIGHT = 2

GRID_ALIGNMENT = 8

def align(value):
    a = GRID_ALIGNMENT
    if a <= 1:
        return value
    if isinstance(value, (tuple, list)):
        return (align(x) for x in value)
    n = (value + a // 2) // a
    return n * a


class Rect:
    def __init__(self, x=0, y=0, w=0, h=0):
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def inflate(self, off):
        return Rect(self.x - off[0], self.y - off[1], self.w + off[0]*2, self.h + off[1]*2)

    def bounds(self):
        return (self.x, self.y, self.x + self.w, self.y + self.h)


# Top 4 bits identify hit class
HIT_CLASS_MASK = 0xf0
# Lower 4 bits for direction
HIT_DIR_MASK = 0x0f
# Hit mask for handles at corners and midpoints of bounding rect
HIT_HANDLE = 0x10
# Hit mask for edges of bounding rectangle
HIT_EDGE = 0x20
HIT_BODY = 0x30

# Directions
DIR_N = 0x01
DIR_E = 0x02
DIR_S = 0x04
DIR_W = 0x08
DIR_NE = DIR_N | DIR_E
DIR_SE = DIR_S | DIR_E
DIR_SW = DIR_S | DIR_W
DIR_NW = DIR_N | DIR_W

def hit_class(e):
    return e & HIT_CLASS_MASK

def hit_dir(e):
    return e & HIT_DIR_MASK

class Element(IntEnum):
    HANDLE_N = HIT_HANDLE | DIR_N
    HANDLE_NE = HIT_HANDLE | DIR_NE
    HANDLE_E = HIT_HANDLE | DIR_E
    HANDLE_SE = HIT_HANDLE | DIR_SE
    HANDLE_S = HIT_HANDLE | DIR_S
    HANDLE_SW = HIT_HANDLE | DIR_SW
    HANDLE_W = HIT_HANDLE | DIR_W
    HANDLE_NW = HIT_HANDLE | DIR_NW

    BODY = HIT_BODY


class GElement(Rect):
    def __init__(self, x=0, y=0, w=0, h=0):
        super().__init__(x, y, w, h)
        self.init()

    def init(self):
        self.line_width = 1
        self.color = 0xa0a0a0

    def get_tag(self):
        return 'G%08x' % id(self)

    def get_cursor(self, elem):
        if elem is None:
            return None
        return {
            Element.HANDLE_NW: 'top_left_corner',
            Element.HANDLE_SE: 'bottom_right_corner',
            Element.HANDLE_NE: 'top_right_corner',
            Element.HANDLE_SW: 'bottom_left_corner',
            Element.HANDLE_N: 'sb_v_double_arrow',
            Element.HANDLE_S: 'sb_v_double_arrow',
            Element.HANDLE_E: 'sb_h_double_arrow',
            Element.HANDLE_W: 'sb_h_double_arrow',
            Element.BODY: 'target',
        }.get(elem)

    def remove_handles(self, canvas):
        canvas.delete('handle')

    def draw_handles(self, canvas):
        for e in Element:
            if hit_class(e) != HIT_HANDLE:
                continue
            r = self.element_rect(e).bounds()
            tags = (self.get_tag(), str(e), 'handle')
            canvas.create_rectangle(r[0], r[1], r[2], r[3], outline='white', width=1, tags=tags)

    def element_rect(self, elem):
        HR = 4, 4
        ER = 1, 1
        return {
            Element.HANDLE_N: Rect(self.x + self.w // 2, self.y, 1, 1).inflate(HR),
            Element.HANDLE_NE: Rect(self.x + self.w, self.y, 1, 1).inflate(HR),
            Element.HANDLE_E: Rect(self.x + self.w, self.y + self.h // 2, 1, 1).inflate(HR),
            Element.HANDLE_SE: Rect(self.x + self.w, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_S: Rect(self.x + self.w // 2, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_SW: Rect(self.x, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_W: Rect(self.x, self.y + self.h // 2, 1, 1).inflate(HR),
            Element.HANDLE_NW: Rect(self.x, self.y, 1, 1).inflate(HR),
            Element.BODY: Rect(self.x, self.y, self.w, self.h).inflate(ER),
        }.get(elem)

    def get_min_size(self):
        o = self.line_width * 2
        return (MIN_ELEMENT_WIDTH + o, MIN_ELEMENT_HEIGHT + o)

    def adjust(self, canvas, elem, orig, off):
        x, y, w, h = self.x, self.y, self.w, self.h
        if elem == Element.BODY:
            x, y = orig.x + off[0], orig.y + off[1]
        else:
            if elem & DIR_N:
                y, h = orig.y + off[1], orig.h - off[1]
            if elem & DIR_E:
                w = orig.w + off[0]
            if elem & DIR_S:
                h = orig.h + off[1]
            if elem & DIR_W:
                x, w = orig.x + off[0], orig.w - off[0]
            min_size = self.get_min_size()
            w, h = max(w, min_size[0]), max(h, min_size[1])
        self.resize(canvas, x, y, w, h)

    def resize(self, canvas, x, y, w, h):
        self.x, self.y, self.w, self.h = align((x, y, w, h))
        canvas.delete(self.get_tag())
        self.draw(canvas)


class GRect(GElement):
    def init(self):
        super().init()
        self.radius = 0

    def get_min_size(self):
        sz = super().get_min_size()
        o = self.radius * 2
        return (sz[0] + o, sz[1] + o)

    def draw(self, canvas):
        # pygame.draw.rect(surface, self.color, self, self.line_width, self.radius)
        x1, y1, x2, y2 = self.bounds()
        w = self.line_width
        color = '#%06x' % self.color
        tags = (self.get_tag(), str(Element.BODY), 'item')

        def draw_rect(x0, y0, x1, y1):
            canvas.create_rectangle(x0, y0, x1, y1, outline=color, width=w, tags=tags)
        def fill_rect(x0, y0, x1, y1):
            canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline='', width=0, tags=tags)
        def draw_line(x0, y0, x1, y1):
            canvas.create_line(x0, y0, x1, y1, fill=color, width=w, tags=tags)
        def draw_corner(x, y, start):
            canvas.create_arc(x, y, x+r*2, y+r*2, start=start, extent=90, outline=color, width=w, style='arc', tags=tags)
        def fill_corner(x, y, start):
            canvas.create_arc(x, y, x+r*2, y+r*2, start=start, extent=90, fill=color, outline='', width=0, tags=tags)

        r = self.radius
        if r > 1:
            if w == 0:
                fill_corner(x1, y1, 90)
                fill_corner(x2-r*2, y1, start=0)
                fill_corner(x2-r*2, y2-r*2, start=270)
                fill_corner(x1, y2-r*2, start=180)
                fill_rect(x1+r, y1, 1+x2-r, y2)
                fill_rect(x1, y1+r, x1+r, y2-r)
                fill_rect(x2-r, y1+r, x2, y2-r)
            else:
                draw_corner(x1, y1, 90)
                draw_corner(x2-r*2, y1, start=0)
                draw_corner(x2-r*2, y2-r*2, start=270)
                draw_corner(x1, y2-r*2, start=180)
                draw_line(x1+r, y1, x2-r, y1)
                draw_line(x1+r, y2, x2-r, y2)
                draw_line(x1, y1+r, x1, y2-r)
                draw_line(x2, y1+r, x2, y2-r)
        elif w == 0:
            fill_rect(x1, y1, x2, y2)
        else:
            draw_rect(x1, y1, x2, y2)


class GEllipse(GElement):
    def draw(self, canvas):
        # pygame.draw.ellipse(surface, self.color, self, self.line_width)
        x1, y1, x2, y2 = self.bounds()
        color = '#%06x' % self.color
        tags = (self.get_tag(), str(Element.BODY), 'item')
        if self.line_width == 0:
            canvas.create_oval(x1, y1, x2, y2, fill=color, tags=tags)
        else:
            canvas.create_oval(x1, y1, x2, y2, outline=color, width=self.line_width, tags=tags)


class Handler:
    def __init__(self, tk_root, size):
        c = self.canvas = tk.Canvas(tk_root, width=size[0], height=size[1], background='black')
        c.pack(side=tk.TOP)
        c.bind('<1>', self.canvas_select)
        c.bind('<Motion>', self.canvas_move)
        c.bind('<B1-Motion>', self.canvas_drag)
        c.bind('<ButtonRelease-1>', self.canvas_end_move)
        self.display_list = []
        self.sel_item = None
        self.is_dragging = False

    def add_random_shapes(self, count):
        self.display_list = []
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randrange(w_min, w_max)
            h = randrange(h_min, h_max)
            x = randrange(DISPLAY_WIDTH - w)
            y = randrange(DISPLAY_HEIGHT - h)
            x, y, w, h = align(x), align(y), align(w), align(h)
            kind = randrange(5)
            if kind == 1:
                item = GEllipse(x, y, w, h)
            else:
                item = GRect(x, y, w, h)
                item.radius = randrange(0, min(w,h)//2)
            item.color = randrange(0xffffff)
            item.line_width = randrange(5)
            self.add_item(item)

    def add_item(self, item):
        self.display_list.append(item)
        item.draw(self.canvas)

    def remove_item(self, item):
        canvas.delete(item.get_tag())
        self.display_list.remove(item)

    def get_current(self):
        tags = self.canvas.gettags('current')
        if not tags:
            return None, None
        item = next(x for x in self.display_list if x.get_tag() == tags[0])
        elem = Element(int(tags[1]))
        return item, elem

    def canvas_select(self, evt):
        item, elem = self.get_current()
        if not item:
            if self.sel_item:
                self.sel_item.remove_handles(self.canvas)
                self.sel_item = None
            return

        self.sel_pos = (evt.x, evt.y)
        self.orig_item = copy.copy(item)
        self.sel_elem = elem
        if item != self.sel_item:
            if self.sel_item:
                self.sel_item.remove_handles(self.canvas)
            item.draw_handles(self.canvas)
            self.sel_item = item

    def canvas_move(self, evt):
        if self.is_dragging:
            return
        item, elem = self.get_current()
        if not item:
            self.canvas.configure({'cursor': ''})
            return
        csr = item.get_cursor(elem)
        if csr:
            self.canvas.configure({'cursor': csr})

    def canvas_drag(self, evt):
        e = self.sel_item
        if not e:
            return
        if not self.is_dragging:
            e.remove_handles(self.canvas)
            self.is_dragging = True
        e.adjust(self.canvas, self.sel_elem, self.orig_item, (evt.x - self.sel_pos[0], evt.y - self.sel_pos[1]))

    def canvas_end_move(self, evt):
        if not self.is_dragging:
            return
        self.is_dragging = False
        e = self.sel_item
        e.remove_handles(self.canvas)
        e.draw_handles(self.canvas)


def run():
    root = tk.Tk(className='GED')
    root.title('Graphical Layout Editor')
    # root.geometry(f'{DISPLAY_WIDTH}x{DISPLAY_HEIGHT}')
    def btn_click():
        print('Button clicked')
    btn = tk.Button(root, text='Hello', command=btn_click)
    btn.pack(side=tk.TOP)
    handler = Handler(root, DISPLAY_SIZE)
    handler.add_random_shapes(20)

    sel_item = None
    sel_elem = None
    cap_item = None
    mouse_pos = None
    sel_pos = None

    tk.mainloop()


if __name__ == "__main__":
    run()
