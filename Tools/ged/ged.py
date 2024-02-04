import os
import sys
import copy
from enum import IntEnum
from random import randrange
import tkinter as tk


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480
MIN_ELEMENT_WIDTH = MIN_ELEMENT_HEIGHT = 2

class Rect:
    def __init__(self, x=0, y=0, w=0, h=0):
        self.x = x
        self.y = y
        self.w = w
        self.h = h

    def inflate(self, off):
        return Rect(self.x - off[0], self.y - off[1], self.w + off[0]*2, self.h + off[1]*2)

    def pos(self, anchor=tk.NW):
        if anchor == tk.N:
            return (self.x + self.w // 2, self.y)
        if anchor == tk.NE:
            return (self.x + self.w, self.y)
        if anchor == tk.E:
            return (self.x + self.w, self.y + self.h // 2)
        if anchor == tk.SE:
            return (self.x + self.w, self.y + self.h)
        if anchor == tk.S:
            return (self.x + self.w // 2, self.y + self.h)
        if anchor == tk.SW:
            return (self.x, self.y + self.h)
        if anchor == tk.W:
            return (self.x, self.y + self.h // 2)
        if anchor == tk.CENTER:
            return (self.x + self.w // 2, self.y + self.h // 2)
        # Default and NW
        return (self.x, self.y)

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

    def test(self, pt):
        for e in Element:
            r = self.element_rect(e)
            if r and r.collidepoint(pt):
                return e
        return None

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

    def hide_handles(self, canvas):
        canvas.itemconfigure('handle', {'state': 'hidden'})

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
            w = max(w, MIN_ELEMENT_WIDTH + self.line_width*2)
            h = max(h, MIN_ELEMENT_HEIGHT + self.line_width*2)
        self.resize(canvas, x, y, w, h)

    def resize(self, canvas, x, y, w, h):
        self.x, self.y, self.w, self.h = x, y, w, h
        r = self.bounds()
        canvas.coords(self.id, r[0], r[1], r[2], r[3])


class GRect(GElement):
    def init(self):
        super().init()
        self.radius = 0

    def draw(self, canvas):
        # pygame.draw.rect(surface, self.color, self, self.line_width, self.radius)
        x1, y1, x2, y2 = self.bounds()
        color = '#%06x' % self.color
        tags = (self.get_tag(), str(Element.BODY), 'item')
        if self.line_width == 0:
            self.id = canvas.create_rectangle(x1, y1, x2, y2, fill=color, tags=tags)
        else:
            self.id = canvas.create_rectangle(x1, y1, x2, y2, outline=color, width=self.line_width, tags=tags)


class GEllipse(GElement):
    def draw(self, canvas):
        # pygame.draw.ellipse(surface, self.color, self, self.line_width)
        x1, y1, x2, y2 = self.bounds()
        color = '#%06x' % self.color
        tags = (self.get_tag(), str(Element.BODY), 'item')
        if self.line_width == 0:
            self.id = canvas.create_oval(x1, y1, x2, y2, fill=color, tags=tags)
        else:
            self.id = canvas.create_oval(x1, y1, x2, y2, outline=color, width=self.line_width, tags=tags)


class Handler:
    def __init__(self):
        self.canvas = None
        self.display_list = []
        self.sel_item = None
        self.is_dragging = False

    def get_random_shapes(self, count):
        self.display_list = []
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randrange(w_min, w_max)
            h = randrange(h_min, h_max)
            x = randrange(DISPLAY_WIDTH - w)
            y = randrange(DISPLAY_HEIGHT - h)
            kind = randrange(5)
            if kind == 1:
                r = GEllipse(x, y, w, h)
            else:
                r = GRect(x, y, w, h)
                r.radius = randrange(0, 20)
            r.color = randrange(0xffffff)
            r.line_width = randrange(5)
            self.display_list.append(r)

    def build_canvas(self):
        for e in self.display_list:
            e.draw(self.canvas)
        self.canvas.tag_bind('item', '<Enter>', self.elem_enter)
        self.canvas.tag_bind('item', '<Leave>', self.elem_leave)
        self.canvas.tag_bind('item', '<1>', self.elem_select)
        self.canvas.tag_bind('item', '<B1-Motion>', self.elem_move)
        self.canvas.tag_bind('item', '<ButtonRelease-1>', self.elem_end_move)

        self.canvas.tag_bind('handle', '<Enter>', self.elem_enter)
        self.canvas.tag_bind('handle', '<Leave>', self.elem_leave)
        self.canvas.tag_bind('handle', '<1>', self.handle_select)
        self.canvas.tag_bind('handle', '<B1-Motion>', self.elem_move)
        self.canvas.tag_bind('handle', '<ButtonRelease-1>', self.elem_end_move)

        self.canvas.bind('<1>', self.canvas_select)

    def get_elem(self, tag):
        return next(x for x in self.display_list if x.get_tag() == tag)

    def get_current_elem(self):
        tag = self.canvas.gettags('current')[0]
        return self.get_elem(tag)

    def elem_enter(self, evt):
        if self.is_dragging:
            return
        self.orig_item = copy.copy(self.sel_item)
        tags = self.canvas.gettags('current')
        e = self.get_elem(tags[0])
        csr = e.get_cursor(int(tags[1]))
        if csr:
            self.canvas.configure({'cursor': csr})

    def elem_leave(self, evt):
        if self.is_dragging:
            return
        self.canvas.configure({'cursor': ''})

    def elem_select(self, evt):
        if self.sel_item:
            self.sel_item.remove_handles(self.canvas)
        e = self.get_current_elem()
        self.sel_item = e
        self.orig_item = copy.copy(e)
        self.sel_elem = Element.BODY
        self.sel_pos = (evt.x, evt.y)
        e.draw_handles(self.canvas)

    def handle_select(self, evt):
        tags = self.canvas.gettags('current')
        self.orig_item = copy.copy(self.sel_item)
        self.sel_elem = int(tags[1])
        self.sel_pos = (evt.x, evt.y)

    def canvas_select(self, evt):
        e = self.sel_item
        if not e or self.canvas.gettags('current'):
            return
        e.remove_handles(self.canvas)
        self.sel_item = None

    def elem_move(self, evt):
        self.is_dragging = True
        e = self.sel_item
        e.hide_handles(self.canvas)
        e.adjust(self.canvas, self.sel_elem, self.orig_item, (evt.x - self.sel_pos[0], evt.y - self.sel_pos[1]))

    def elem_end_move(self, evt):
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
    handler = Handler()
    handler.canvas = tk.Canvas(root, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, background='black')
    handler.canvas.pack(side=tk.TOP)
    root.update()

    handler.get_random_shapes(20)
    handler.build_canvas()

    sel_item = None
    sel_elem = None
    cap_item = None
    mouse_pos = None
    sel_pos = None

    tk.mainloop()


if __name__ == "__main__":
    run()
