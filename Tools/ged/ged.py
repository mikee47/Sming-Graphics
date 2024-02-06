import os
import sys
import copy
from enum import Enum, IntEnum
from random import randrange
from dataclasses import dataclass
import tkinter as tk
from tkinter.font import Font


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480
MIN_ELEMENT_WIDTH = MIN_ELEMENT_HEIGHT = 2

GRID_ALIGNMENT = 8

# Event state modifier masks
EVS_SHIFT = 0x0001
EVS_CONTROL = 0x0004
EVS_ALTLEFT = 0x0008
EVS_ALTRIGHT = 0x0080


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

    def __post_init__(self):
        pass

    @property
    def bounds(self):
        return self.copy()

    @bounds.setter
    def bounds(self, rect):
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

def tk_rect(coords):
    return Rect(coords[0], coords[1], 1 + coords[2] - coords[0], 1 + coords[3] - coords[1])

def union(r1: Rect, r2: Rect):
    x = min(r1.x, r2.x)
    y = min(r1.y, r2.y)
    w = max(r1.x + r1.w, r2.x + r2.w) - x
    h = max(r1.y + r1.h, r2.y + r2.h) - y
    return Rect(x, y, w, h)

def tk_color(color):
    return '#%06x' % color

@dataclass
class GItem(Rect):
    line_width: int = 1
    color: int = 0xa0a0a0

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
        self.bounds = rect
        canvas.delete(self.get_tag())
        self.draw(canvas)


@dataclass
class GRect(GItem):
    radius: int = 0

    def get_min_size(self):
        sz = super().get_min_size()
        o = self.radius * 2
        return (sz[0] + o, sz[1] + o)

    def draw(self, canvas):
        x1, y1, x2, y2 = self.tk_bounds()
        w = self.line_width
        color = tk_color(self.color)
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
                fill_corner(x2-r*2, y1, 0)
                fill_corner(x2-r*2, y2-r*2, 270)
                fill_corner(x1, y2-r*2, 180)
                fill_rect(x1+r, y1, 1+x2-r, y2)
                fill_rect(x1, y1+r, x1+r, y2-r)
                fill_rect(x2-r, y1+r, x2, y2-r)
            else:
                draw_corner(x1, y1, 90)
                draw_corner(x2-r*2, y1, 0)
                draw_corner(x2-r*2, y2-r*2, 270)
                draw_corner(x1, y2-r*2, 180)
                draw_line(x1+r, y1, x2-r, y1)
                draw_line(x1+r, y2, x2-r, y2)
                draw_line(x1, y1+r, x1, y2-r)
                draw_line(x2, y1+r, x2, y2-r)
        elif w == 0:
            fill_rect(x1, y1, x2, y2)
        else:
            draw_rect(x1, y1, x2, y2)


@dataclass
class GEllipse(GItem):
    def draw(self, canvas):
        x1, y1, x2, y2 = self.tk_bounds()
        color = tk_color(self.color)
        tags = self.get_item_tags()
        if self.line_width == 0:
            canvas.create_oval(x1, y1, x2, y2, fill=color, outline='', tags=tags)
        else:
            canvas.create_oval(x1, y1, x2, y2, outline=color, width=self.line_width, tags=tags)


@dataclass
class GText(GItem):
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'text{self.get_tag()}'

    def draw(self, canvas):
        color = tk_color(self.color)
        tags = self.get_item_tags()
        canvas.create_text(self.x, self.y, width=self.w, text=self.text, fill=color, anchor=tk.NW, justify=tk.CENTER, tags=tags)


@dataclass
class GButton(GRect):
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'button {self.get_tag()}'

    def draw(self, canvas):
        self.radius = min(self.w, self.h) // 8
        self.line_width = 0
        super().draw(canvas)
        color = tk_color(self.color)
        tags = self.get_item_tags()
        font_px = min(self.w, self.h) // 4
        font = Font(family='Helvetica', size=-font_px)
        M = font_px
        x, y, w = self.x + M, self.y, self.w - 2*M
        id = canvas.create_text(x, y, width=w,
            font=font, text=self.text, fill='white',
            anchor=tk.NW, justify=tk.CENTER, tags=tags)
        r = tk_rect(canvas.bbox(id))
        x += (w - r.w) // 2
        y += (self.h - r.h) // 2
        canvas.coords(id, x, y)


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
        self.sel_items = []
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
                item = GEllipse()
            elif kind == 2:
                item = GText()
            elif kind == 3:
                item = GButton()
            else:
                item = GRect()
                item.radius = randrange(0, min(r.w, r.h) // 2)
            item.bounds = r
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
        tags = self.canvas.gettags(tk.CURRENT)
        if not tags:
            return None, None
        elem = Element(int(tags[1]))
        if tags[0] == 'handle':
            return elem, None
        item = next(x for x in self.display_list if x.get_tag() == tags[2])
        return elem, item

    def draw_handles(self):
        hr = self.sel_items[0].get_bounds()
        for item in self.sel_items[1:]:
            hr = union(hr, item)
        for e in Element:
            r = hr.element_rect(e).tk_bounds()
            tags = ('handle', str(e))
            self.canvas.create_rectangle(r[0], r[1], r[2], r[3], outline='white', width=1, tags=tags)
        return hr

    def remove_handles(self):
        self.canvas.delete('handle')

    def canvas_select(self, evt):
        self.sel_pos = (evt.x, evt.y)
        is_multi = (evt.state & EVS_CONTROL) != 0
        elem, item = self.get_current()
        if not elem:
            if not is_multi and self.sel_items:
                self.remove_handles()
                self.sel_items = []
            return

        self.sel_elem = elem
        if item and not item in self.sel_items:
            self.remove_handles()
            if is_multi:
                self.sel_items.append(item)
            else:
                self.sel_items = [item]
            self.sel_bounds = self.draw_handles()
        self.orig_bounds = [x.get_bounds() for x in self.sel_items]

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
        if not self.sel_items:
            return
        self.remove_handles()
        if self.state != State.DRAGGING:
            self.state = State.DRAGGING
            self.orig_bounds = [x.get_bounds() for x in self.sel_items]
        elem = self.sel_elem
        off = evt.x - self.sel_pos[0], evt.y - self.sel_pos[1]

        if elem == Element.ITEM:
            for item, orig in zip(self.sel_items, self.orig_bounds):
                r = item.get_bounds()
                r.x, r.y = align((orig.x + off[0], orig.y + off[1]))
                item.resize(self.canvas, r)
        elif len(self.sel_items) == 1:
            item, orig = self.sel_items[0], self.orig_bounds[0]
            r = item.get_bounds()
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
            if r.w >= min_size[0] and r.h >= min_size[1]:
                item.resize(self.canvas, r)
        else:
            # Scale the bounding rectangle
            orig = orig_bounds = self.sel_bounds
            r = copy.copy(orig)
            if elem & DIR_N:
                r.y = orig.y + off[1]
                r.h = orig.y + orig.h - r.y
            elif elem & DIR_S:
                r.h = orig.h + off[1]
            if elem & DIR_E:
                r.w = orig.w + off[0]
            elif elem & DIR_W:
                r.x = orig.x + off[0]
                r.w = orig.x + orig.w - r.x
            if r.w > 0 and r.h > 0:
                cur_bounds = r
                # Now use scaled rectangle to move/resize items
                do_size = (evt.state & EVS_SHIFT) != 0
                for item, orig in zip(self.sel_items, self.orig_bounds):
                    r = copy.copy(orig)
                    r.x = align(cur_bounds.x + (orig.x - orig_bounds.x) * cur_bounds.w // orig_bounds.w)
                    r.y = align(cur_bounds.y + (orig.y - orig_bounds.y) * cur_bounds.h // orig_bounds.h)
                    if do_size:
                        r.w = align(orig.w * cur_bounds.w // orig_bounds.w)
                        r.h = align(orig.h * cur_bounds.h // orig_bounds.h)
                        min_size = item.get_min_size()
                        if r.w < min_size[0] or r.h < min_size[1]:
                            continue
                    item.resize(self.canvas, r)

        self.draw_handles()


    def canvas_end_move(self, evt):
        if self.state != State.DRAGGING:
            return
        self.state = State.IDLE
        self.remove_handles()
        self.redraw() # Fix Z-ordering and ensure consistency
        self.sel_bounds = self.draw_handles()


    def redraw(self):
        self.canvas.delete(tk.ALL)
        for item in self.display_list:
            item.draw(self.canvas)


def run():
    root = tk.Tk(className='GED')
    root.title('Graphical Layout Editor')
    def btn_click():
        for item in handler.display_list:
            print(repr(item))
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
