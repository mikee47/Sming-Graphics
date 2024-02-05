import os
import sys
import copy
from enum import Enum, IntEnum
from random import randrange
from dataclasses import dataclass
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

# Top 4 bits identify hit class
ELEMENT_CLASS_MASK = 0xf0
# Lower 4 bits for direction
ELEMENT_DIR_MASK = 0x0f
# Hit mask for handles at corners and midpoints of bounding rect
EC_HANDLE = 0x10
# Hit mask for item elements
EC_ITEM = 0x20

# Directions
DIR_N = 0x01
DIR_E = 0x02
DIR_S = 0x04
DIR_W = 0x08
DIR_NE = DIR_N | DIR_E
DIR_SE = DIR_S | DIR_E
DIR_SW = DIR_S | DIR_W
DIR_NW = DIR_N | DIR_W

class Element(IntEnum):
    HANDLE_N = EC_HANDLE | DIR_N
    HANDLE_NE = EC_HANDLE | DIR_NE
    HANDLE_E = EC_HANDLE | DIR_E
    HANDLE_SE = EC_HANDLE | DIR_SE
    HANDLE_S = EC_HANDLE | DIR_S
    HANDLE_SW = EC_HANDLE | DIR_SW
    HANDLE_W = EC_HANDLE | DIR_W
    HANDLE_NW = EC_HANDLE | DIR_NW

    ITEM = EC_ITEM | 0x01

    def is_handle(self):
        return (self & ELEMENT_CLASS_MASK) == EC_HANDLE


@dataclass
class Rect:
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0

    def set_rect(self, rect):
        self.x, self.y, self.w, self.h = rect.x, rect.y, rect.w, rect.h

    def align(self):
        return Rect(align(self.x), align(self.y), align(self.w), align(self.h))

    def inflate(self, off):
        return Rect(self.x - off[0], self.y - off[1], self.w + off[0]*2, self.h + off[1]*2)

    def handle_pos(self, elem: Element):
        return {
            Element.HANDLE_N: (self.x + self.w // 2, self.y),
            Element.HANDLE_NE: (self.x + self.w, self.y),
            Element.HANDLE_E: (self.x + self.w, self.y + self.h // 2),
            Element.HANDLE_SE: (self.x + self.w, self.y + self.h),
            Element.HANDLE_S: (self.x + self.w // 2, self.y + self.h),
            Element.HANDLE_SW: (self.x, self.y + self.h),
            Element.HANDLE_W: (self.x, self.y + self.h // 2),
            Element.HANDLE_NW: (self.x, self.y),
        }.get(elem)

    def element_rect(self, elem):
        HR = 4, 4
        ER = 1, 1
        if elem == Element.ITEM:
            return Rect(self.x, self.y, self.w, self.h).inflate(ER)
        pt = self.handle_pos(elem)
        return Rect(pt[0], pt[1], 1, 1).inflate(HR)

    def tk_bounds(self):
        return (self.x, self.y, self.x + self.w, self.y + self.h)


class GItem(Rect):
    def __init__(self, bounds: Rect):
        super().__init__(bounds.x, bounds.y, bounds.w, bounds.h)

    def __post_init__(self):
        super().__post_init__()
        self.line_width = 1
        self.color = 0xa0a0a0

    def get_bounds(self):
        return Rect(self.x, self.y, self.w, self.h)

    def get_tag(self):
        return 'G%08x' % id(self)

    def get_item_tags(self):
        return ('item', str(Element.ITEM), self.get_tag())

    def get_min_size(self):
        o = self.line_width * 2
        return (MIN_ELEMENT_WIDTH + o, MIN_ELEMENT_HEIGHT + o)

    def resize(self, canvas, rect):
        self.set_rect(rect)
        canvas.delete(self.get_tag())
        self.draw(canvas)


class GRect(GItem):
    def __post_init__(self):
        super().__post_init__()
        self.radius = 0

    def get_min_size(self):
        sz = super().get_min_size()
        o = self.radius * 2
        return (sz[0] + o, sz[1] + o)

    def draw(self, canvas):
        # pygame.draw.rect(surface, self.color, self, self.line_width, self.radius)
        x1, y1, x2, y2 = self.tk_bounds()
        w = self.line_width
        color = '#%06x' % self.color
        tags = self.get_item_tags()

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


class GEllipse(GItem):
    def draw(self, canvas):
        # pygame.draw.ellipse(surface, self.color, self, self.line_width)
        x1, y1, x2, y2 = self.tk_bounds()
        color = '#%06x' % self.color
        tags = self.get_item_tags()
        if self.line_width == 0:
            canvas.create_oval(x1, y1, x2, y2, fill=color, tags=tags)
        else:
            canvas.create_oval(x1, y1, x2, y2, outline=color, width=self.line_width, tags=tags)

class State(Enum):
    IDLE = 0
    DRAGGING = 1
    SELECTING = 2

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
        self.state = State.IDLE

    def add_random_shapes(self, count):
        self.display_list = []
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randrange(w_min, w_max)
            h = randrange(h_min, h_max)
            x = randrange(DISPLAY_WIDTH - w)
            y = randrange(DISPLAY_HEIGHT - h)
            r = Rect(x, y, w, h).align()
            kind = randrange(5)
            if kind == 1:
                item = GEllipse(r)
            else:
                item = GRect(r)
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
        elem = Element(int(tags[1]))
        if elem == Element.ITEM:
            item = next(x for x in self.display_list if x.get_tag() == tags[2])
        else:
            item = None
        return elem, item

    def draw_handles(self, rect):
        for e in Element:
            if not e.is_handle():
                continue
            r = rect.element_rect(e).tk_bounds()
            tags = ('handle', str(e))
            self.canvas.create_rectangle(r[0], r[1], r[2], r[3], outline='white', width=1, tags=tags)

    def remove_handles(self):
        self.canvas.delete('handle')

    def canvas_select(self, evt):
        elem, item = self.get_current()
        if not elem:
            if self.sel_item:
                self.remove_handles()
                self.sel_item = None
            return

        self.sel_pos = (evt.x, evt.y)
        self.sel_elem = elem
        if item and item != self.sel_item:
            self.remove_handles()
            self.draw_handles(item)
            self.sel_item = item
        self.orig_item = copy.copy(self.sel_item)

    @staticmethod
    def get_cursor(elem):
        if elem is None:
            return ''
        return {
            Element.HANDLE_NW: 'top_left_corner',
            Element.HANDLE_SE: 'bottom_right_corner',
            Element.HANDLE_NE: 'top_right_corner',
            Element.HANDLE_SW: 'bottom_left_corner',
            Element.HANDLE_N: 'sb_v_double_arrow',
            Element.HANDLE_S: 'sb_v_double_arrow',
            Element.HANDLE_E: 'sb_h_double_arrow',
            Element.HANDLE_W: 'sb_h_double_arrow',
            Element.ITEM: 'target',
        }.get(elem)

    def canvas_move(self, evt):
        if self.state == State.DRAGGING:
            return
        elem, item = self.get_current()
        self.canvas.configure({'cursor': self.get_cursor(elem)})

    def canvas_drag(self, evt):
        item = self.sel_item
        if not item:
            return
        if self.state != State.DRAGGING:
            self.remove_handles()
            self.state = State.DRAGGING
        elem = self.sel_elem
        orig = self.orig_item
        r = item.get_bounds()
        off = evt.x - self.sel_pos[0], evt.y - self.sel_pos[1]
        if elem == Element.ITEM:
            r.x, r.y = align((orig.x + off[0], orig.y + off[1]))
        else:
            if elem & DIR_N:
                r.y = align(orig.y + off[1])
                r.h = orig.y + orig.h - r.y
            if elem & DIR_E:
                r.w = align(orig.w + off[0])
            if elem & DIR_S:
                r.h = align(orig.h + off[1])
            if elem & DIR_W:
                r.x = align(orig.x + off[0])
                r.w = orig.x + orig.w - r.x
            min_size = item.get_min_size()
            if r.w < min_size[0] or r.h < min_size[1]:
                return
        item.resize(self.canvas, r)

    def canvas_end_move(self, evt):
        if self.state != State.DRAGGING:
            return
        self.state = State.IDLE
        self.remove_handles()
        self.draw_handles(self.sel_item)


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
