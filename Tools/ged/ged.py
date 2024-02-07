import os
import sys
import copy
from enum import Enum, IntEnum
from random import randrange
from dataclasses import dataclass, field, asdict
import json
import tkinter as tk
from tkinter.font import Font
from tkinter import ttk, filedialog
from PIL.ImageColor import colormap

rev_colormap = {value: name for name, value in colormap.items()}

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


class Canvas:
    def __init__(self, tk_canvas, tags=()):
        self.canvas = tk_canvas
        self.tags = tags
        self.color = 'white'
        self.width = 1

    def draw_rect(self, x0, y0, x1, y1):
        self.canvas.create_rectangle(x0, y0, x1-1, y1-1, outline=self.color, width=self.width, tags=self.tags)

    def fill_rect(self, x0, y0, x1, y1):
        self.canvas.create_rectangle(x0, y0, x1, y1, fill=self.color, outline='', width=0, tags=self.tags)

    def draw_line(self, x0, y0, x1, y1):
        self.canvas.create_line(x0, y0, x1, y1, fill=self.color, width=self.width, tags=self.tags)

    def draw_corner(self, x, y, r, start_angle):
        self.canvas.create_arc(x, y, x+r*2, y+r*2, start=start_angle, extent=90, outline=self.color, width=self.width, style='arc', tags=self.tags)

    def fill_corner(self, x, y, r, start_angle):
        self.canvas.create_arc(x, y, x+r*2, y+r*2, start=start_angle, extent=90, fill=self.color, outline='', tags=self.tags)

    def draw_ellipse(self, x0, y0, x1, y1):
        self.canvas.create_oval(x0, y0, x1, y1, fill=self.color, outline='', width=0, tags=self.tags)

    def fill_ellipse(self, x0, y0, x1, y1):
        self.canvas.create_oval(x0, y0, x1, y1, outline=self.color, width=self.width, tags=self.tags)


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

    def inflate(self, xo, yo):
        return Rect(self.x - xo, self.y - yo, self.w + xo*2, self.h + yo*2)

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


def union(r1: Rect, r2: Rect):
    x = min(r1.x, r2.x)
    y = min(r1.y, r2.y)
    w = max(r1.x + r1.w, r2.x + r2.w) - x
    h = max(r1.y + r1.h, r2.y + r2.h) - y
    return Rect(x, y, w, h)

def tk_color(color):
    return color if isinstance(color, str) else '#%06x' % color

def tk_inflate(bounds, xo, yo):
    return (bounds[0]-xo, bounds[1]-yo, bounds[2]+xo, bounds[3]+yo)

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

    def resize(self, handler, rect):
        self.bounds = rect
        handler.canvas.delete(self.get_tag())
        self.draw(handler)


@dataclass
class GRect(GItem):
    radius: int = 0

    def get_min_size(self):
        sz = super().get_min_size()
        o = self.radius * 2
        return (sz[0] + o, sz[1] + o)

    def draw(self, handler):
        x1, y1, x2, y2 = handler.tk_bounds(self)
        c = Canvas(handler.canvas, self.get_item_tags())
        c.color = tk_color(self.color)
        c.width = self.line_width * handler.scale

        if self.radius > 1:
            r = self.radius * handler.scale
            if c.width == 0:
                c.fill_corner(x1, y1, r, 90)
                c.fill_corner(x2-r*2, y1, r, 0)
                c.fill_corner(x2-r*2, y2-r*2, r, 270)
                c.fill_corner(x1, y2-r*2, r, 180)
                c.fill_rect(x1+r, y1, 1+x2-r, y2)
                c.fill_rect(x1, y1+r, x1+r, y2-r)
                c.fill_rect(x2-r, y1+r, x2, y2-r)
            else:
                c.draw_corner(x1, y1, r, 90)
                c.draw_corner(x2-r*2, y1, r, 0)
                c.draw_corner(x2-r*2, y2-r*2, r, 270)
                c.draw_corner(x1, y2-r*2, r, 180)
                c.draw_line(x1+r, y1, x2-r, y1)
                c.draw_line(x1+r, y2, x2-r, y2)
                c.draw_line(x1, y1+r, x1, y2-r)
                c.draw_line(x2, y1+r, x2, y2-r)
        elif c.width == 0:
            c.fill_rect(x1, y1, x2, y2)
        else:
            c.draw_rect(x1, y1, x2, y2)


@dataclass
class GEllipse(GItem):
    def draw(self, handler):
        r = handler.tk_bounds(self)
        c = Canvas(handler.canvas, self.get_item_tags())
        c.color = tk_color(self.color)
        c.width = self.line_width * handler.scale
        if c.width == 0:
            c.draw_ellipse(*r)
        else:
            c.fill_ellipse(*r)


@dataclass
class GText(GItem):
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'Text {self.get_tag()}'

    def draw(self, handler):
        color = tk_color(self.color)
        tags = self.get_item_tags()
        x1, y1, x2, y2 = handler.tk_bounds(self)
        M = 10
        w = x2 - x1 - M*2
        handler.canvas.create_text(x1, y1, width=w, text=self.text, fill=color, anchor=tk.NW, justify=tk.CENTER, tags=tags)


@dataclass
class GButton(GRect):
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'button {self.get_tag()}'

    def draw(self, handler):
        self.radius = min(self.w, self.h) // 8
        self.line_width = 0
        super().draw(handler)
        color = tk_color(self.color)
        tags = self.get_item_tags()
        font_px = min(self.w, self.h) // 4
        font = Font(family='Helvetica', size=font_px * -handler.scale)
        M = font_px * handler.scale
        x1, y1, x2, y2 = handler.tk_bounds(self)
        x1 += M
        x2 -= M
        id = handler.canvas.create_text(x1, y1, width=1+x2-x1,
            font=font, text=self.text, fill='white',
            anchor=tk.NW, justify=tk.CENTER, tags=tags)
        _, _, x3, y3 = handler.canvas.bbox(id)
        x1 += (x2 - x3) // 2
        y1 += (y2 - y3) // 2
        handler.canvas.coords(id, x1, y1)


class State(Enum):
    IDLE = 0
    DRAGGING = 1
    SELECTING = 2

class Handler:
    def __init__(self, tk_root, width=320, height=240, scale=1):
        self.width = width
        self.height = height
        self.scale = scale
        self.display_list = []
        self.sel_items = []
        self.on_sel_changed = None
        self.state = State.IDLE

        c = self.canvas = tk.Canvas(tk_root, background='gray')
        c.bind('<1>', self.canvas_select)
        c.bind('<Motion>', self.canvas_move)
        c.bind('<B1-Motion>', self.canvas_drag)
        c.bind('<ButtonRelease-1>', self.canvas_end_move)

        # Respond to size changes but slow them down a bit as full redraw is expensive
        self.size_change_pending = False
        def canvas_configure(evt):
            if not self.size_change_pending:
                self.canvas.after(200, self.size_changed)
                self.size_change_pending = True
        c.bind('<Configure>', canvas_configure)

    def set_size(self, width, height):
        if width == self.width and height == self.height:
            return
        self.width = width
        self.height = height
        self.size_changed()
    
    def set_scale(self, scale):
        if scale == self.scale:
            return
        self.scale = scale
        self.size_changed()

    def size_changed(self):
        self.size_change_pending = False
        w, h = self.canvas.winfo_width(), self.canvas.winfo_height()
        self.draw_offset = ((w - self.width * self.scale) // 2, (h - self.height * self.scale) // 2)
        self.redraw()

    def tk_bounds(self, rect):
        xo, yo = self.draw_offset
        return (
            xo + rect.x * self.scale,
            yo + rect.y * self.scale,
            xo + (rect.x + rect.w) * self.scale,
            yo + (rect.y + rect.h) * self.scale )

    def clear(self):
        self.display_list.clear()
        self.sel_items.clear()
        self.state = State.IDLE
        self.redraw()

    def add_items(self, item_list):
        self.display_list.extend(item_list)
        self.redraw()

    def add_item(self, item):
        self.display_list.append(item)
        item.draw(self)

    def remove_item(self, item):
        self.canvas.delete(item.get_tag())
        self.display_list.remove(item)

    def get_current(self):
        tags = self.canvas.gettags(tk.CURRENT)
        if len(tags) < 2:
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
            tags = ('handle', str(e))
            if e == Element.ITEM:
                r = self.tk_bounds(hr)
                r = tk_inflate(r, 1, 1)
                self.canvas.create_rectangle(r, outline='white', width=1, tags=tags)
            else:
                pt = hr.handle_pos(e)
                r = self.tk_bounds(Rect(pt[0], pt[1]))
                r = tk_inflate(r, 4, 4)
                self.canvas.create_rectangle(r, outline='', fill='white', tags=tags)
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
                if self.on_sel_changed:
                    self.on_sel_changed()
            return

        self.sel_elem = elem
        sel_changed = False
        if item and not item in self.sel_items:
            self.remove_handles()
            if is_multi:
                self.sel_items.append(item)
            else:
                self.sel_items = [item]
            sel_changed = True
            self.sel_bounds = self.draw_handles()
        self.orig_bounds = [x.get_bounds() for x in self.sel_items]
        if sel_changed and self.on_sel_changed:
            self.on_sel_changed()

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
        off = (evt.x - self.sel_pos[0]) // self.scale, (evt.y - self.sel_pos[1]) // self.scale

        if elem == Element.ITEM:
            for item, orig in zip(self.sel_items, self.orig_bounds):
                r = item.get_bounds()
                r.x, r.y = align((orig.x + off[0], orig.y + off[1]))
                item.resize(self, r)
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
                item.resize(self, r)
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
                    item.resize(self, r)

        self.draw_handles()


    def canvas_end_move(self, evt):
        if self.state != State.DRAGGING:
            return
        self.state = State.IDLE
        self.redraw() # Fix Z-ordering and ensure consistency


    def redraw(self):
        self.canvas.delete(tk.ALL)
        r = self.tk_bounds(Rect(0, 0, self.width, self.height))
        c = Canvas(self.canvas)
        c.color = 'black'
        c.fill_rect(*r)
        for item in self.display_list:
            item.draw(self)
        c.color = 'white'
        c.draw_rect(r[0]-1, r[1]-1, r[2]+1, r[3]+1)
        if self.sel_items:
            self.sel_bounds = self.draw_handles()


def run():
    PROJECT_EXT = '.ged'
    PROJECT_FILTER = [('GED Project', '*' + PROJECT_EXT)]

    def json_loads(s):
        return json.loads(s)#, object_pairs_hook=OrderedDict)

    def json_load(filename):
        with open(filename) as f:
            return json_loads(f.read())

    def json_save(data, filename):
        with open(filename, "w") as f:
            json.dump(data, f, indent=4)

    def json_dumps(obj):
        return json.dumps(obj, indent=4)

    def dl_serialise(display_list):
        data = []
        for item in display_list:
            d = {
                'class': str(item.__class__.__name__),
                'tag': item.get_tag(),
            }
            d.update(asdict(item))
            data.append(d)
        return data

    def dl_deserialise(data):
        display_list = []
        for d in data:
            class_name = d.pop('class')
            cls = getattr(sys.modules[__name__], class_name)
            item = cls()
            tag = d.pop('tag')
            for a, v in d.items():
                setattr(item, a, v)
            display_list.append(item)
        return display_list

    # Menus
    def fileClear():
        handler.clear()

    def fileAddRandom(count = 10):
        display_list = []
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randrange(w_min, w_max)
            h = randrange(h_min, h_max)
            x = randrange(handler.width - w)
            y = randrange(handler.height - h)
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
            display_list.append(item)
        handler.add_items(display_list)


    def fileLoad():
        filename = filedialog.askopenfilename(title='Load project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            data = json_load(filename)
            handler.display_list = []
            display_list = dl_deserialise(data)
            handler.clear()
            handler.add_items(display_list)

    def fileSave():
        filename = filedialog.asksaveasfilename(title='Save project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            ext = os.path.splitext(filename)[1]
            if ext != PROJECT_EXT:
                filename += PROJECT_EXT
            data = dl_serialise(handler.display_list)
            json_save(data, filename)

    def fileList():
        data = dl_serialise(handler.display_list)
        print(json_dumps(data))

    root = tk.Tk(className='GED')
    root.geometry('800x600')
    root.title('Graphical Layout Editor')
    root.grid()
    root.columnconfigure(0, weight=2)
    root.rowconfigure(1, weight=2)
    handler = Handler(root)
    handler.set_scale(2)
    handler.canvas.grid(row=1, column=0, sticky=tk.NSEW)

    # Toolbar
    toolbar = ttk.Frame(root)
    toolbar.grid(row=0, column=0, columnspan=2)
    col = 0
    def addButton(text, command):
        btn = ttk.Button(toolbar, text=text, command=command)
        nonlocal col
        btn.grid(row=0, column=col)
        col += 1
    addButton('Clear', fileClear)
    addButton('Add Random', fileAddRandom)
    addButton('Load...', fileLoad)
    addButton('Save...', fileSave)
    sep = ttk.Separator(toolbar, orient=tk.VERTICAL)
    sep.grid(row=0, column=col, sticky=tk.NS)
    col += 1
    addButton('List', fileList)
    def changeScale():
        scale = 1 + handler.scale % 4
        handler.set_scale(scale)
    addButton('scale', changeScale)
    # addButton('Edit Config', self.editConfig)
    # addButton('Add Device', self.addDevice)

    # Properties
    frame = ttk.Frame(root)
    frame.grid(row=1, column=1)
    var = tk.StringVar(value='blue')
    cb = ttk.Combobox(frame, textvariable=var)
    cb.pack()
    def set_color(evt):
        color = var.get()
        color_name = rev_colormap.get(color)
        if color_name:
            var.set(color_name)
            color = color_name
        for item in handler.sel_items:
            item.color = color
        handler.redraw()
    cb.bind('<Return>', set_color)

    def sel_changed():
        items = handler.sel_items
        if not items:
            return
        color = tk_color(items[0].color)
        var.set(rev_colormap.get(color, color))
    handler.on_sel_changed = sel_changed

    tk.mainloop()


if __name__ == "__main__":
    run()
