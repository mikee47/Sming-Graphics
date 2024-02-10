import os
import sys
import copy
from enum import Enum, IntEnum
from random import randrange, randint
import dataclasses
from dataclasses import dataclass
import json
import tkinter as tk
import tkinter.font
from tkinter import ttk, filedialog
from item import *

# Event state modifier masks
EVS_SHIFT = 0x0001
EVS_CONTROL = 0x0004
EVS_ALTLEFT = 0x0008
EVS_ALTRIGHT = 0x0080


def align(value: int, boundary: int):
    if boundary <= 1:
        return value
    if isinstance(value, (tuple, list)):
        return (align(x, boundary) for x in value)
    n = (value + boundary // 2) // boundary
    return n * boundary

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
    def __init__(self, handler, tags):
        self.handler = handler
        self.canvas = handler.canvas
        self.tags = tags
        self.color = 'white'
        self.line_width = 1
        self.font = 'default'

    def draw_rect(self, rect):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        self.canvas.create_rectangle(x0, y0, x1-1, y1-1, outline=self.color, width=self.line_width, tags=self.tags)

    def fill_rect(self, rect):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        self.canvas.create_rectangle(x0, y0, x1, y1, fill=self.color, outline='', width=0, tags=self.tags)

    def draw_rounded_rect(self, rect, r):
        x0, y0, x1, y1 = rect.x, rect.y, rect.x + rect.w, rect.y + rect.h
        self.draw_corner(x0, y0, r, 90)
        self.draw_corner(x1-r*2, y0, r, 0)
        self.draw_corner(x1-r*2, y1-r*2, r, 270)
        self.draw_corner(x0, y1-r*2, r, 180)
        self.draw_line(x0+r, y0, x1-r, y0)
        self.draw_line(x0+r, y1, x1-r, y1)
        self.draw_line(x0, y0+r, x0, y1-r)
        self.draw_line(x1, y0+r, x1, y1-r)

    def fill_rounded_rect(self, rect, r):
        x0, y0, x1, y1 = rect.x, rect.y, rect.x + rect.w, rect.y + rect.h
        self.fill_corner(x0, y0, r, 90)
        self.fill_corner(x1-r*2, y0, r, 0)
        self.fill_corner(x1-r*2, y1-r*2, r, 270)
        self.fill_corner(x0, y1-r*2, r, 180)
        self.fill_rect(Rect(x0+r, y0, rect.w - 2*r, rect.h))
        self.fill_rect(Rect(x0, y0+r, r, rect.h-2*r))
        self.fill_rect(Rect(x0+rect.w-r, y0+r, r, rect.h-2*r))

    def draw_line(self, x0, y0, x1, y1):
        x0, y0 = self.handler.tk_point(x0, y0)
        x1, y1 = self.handler.tk_point(x1, y1)
        self.canvas.create_line(x0, y0, x1, y1, fill=self.color, width=self.line_width, tags=self.tags)

    def draw_corner(self, x, y, r, start_angle):
        x0, y0 = self.handler.tk_point(x, y)
        x1, y1 = self.handler.tk_point(x+r*2, y+r*2)
        self.canvas.create_arc(x0, y0, x1, y1, start=start_angle, extent=90, outline=self.color, width=self.line_width, style='arc', tags=self.tags)

    def fill_corner(self, x, y, r, start_angle):
        x0, y0 = self.handler.tk_point(x, y)
        x1, y1 = self.handler.tk_point(x+r*2, y+r*2)
        self.canvas.create_arc(x0, y0, x1, y1, start=start_angle, extent=90, fill=self.color, outline='', tags=self.tags)

    def draw_ellipse(self, rect):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        self.canvas.create_oval(x0, y0, x1, y1, outline=self.color, width=self.line_width, tags=self.tags)

    def fill_ellipse(self, rect):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        self.canvas.create_oval(x0, y0, x1, y1, fill=self.color, outline='', width=0, tags=self.tags)

    def draw_text(self, rect, text, halign, valign):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        id = self.canvas.create_text(x0, y0, width=1+x1-x0,
            font=self.handler.tk_font(self.font),
            text=text, fill=self.color,
            anchor=tk.NW, justify=tk.CENTER, tags=self.tags)
        _, _, x2, y2 = self.canvas.bbox(id)
        x0 += (x1 - x2) // 2
        y0 += (y1 - y2) // 2
        self.canvas.coords(id, x0, y0)


def union(r1: Rect, r2: Rect):
    x = min(r1.x, r2.x)
    y = min(r1.y, r2.y)
    w = max(r1.x + r1.w, r2.x + r2.w) - x
    h = max(r1.y + r1.h, r2.y + r2.h) - y
    return Rect(x, y, w, h)


def tk_inflate(bounds, xo, yo):
    return (bounds[0]-xo, bounds[1]-yo, bounds[2]+xo, bounds[3]+yo)


def get_handle_pos(r: Rect, elem: Element):
    return {
        Element.HANDLE_N: (r.x + r.w // 2, r.y),
        Element.HANDLE_NE: (r.x + r.w, r.y),
        Element.HANDLE_E: (r.x + r.w, r.y + r.h // 2),
        Element.HANDLE_SE: (r.x + r.w, r.y + r.h),
        Element.HANDLE_S: (r.x + r.w // 2, r.y + r.h),
        Element.HANDLE_SW: (r.x, r.y + r.h),
        Element.HANDLE_W: (r.x, r.y + r.h // 2),
        Element.HANDLE_NW: (r.x, r.y),
    }.get(elem)


class FontAssets(dict):
    def __init__(self):
        super().__init__()
        self.clear()

    def clear(self):
        super().clear()
        font = self['default'] = GFont()
        tk_def = FontAssets.tk_default().configure()
        font.family = tk_def['family']

    @staticmethod
    def tk_default():
        return tkinter.font.nametofont('TkDefaultFont')

    @staticmethod
    def families():
        # Not all fonts are listed by TK, so include the 'guaranteed supported' ones
        font_families = list(tk.font.families())
        tk_def = FontAssets.tk_default().configure()
        font_families += ['Courier', 'Times', 'Helvetica', tk_def['family']]
        font_families = list(set(font_families))
        return sorted(font_families, key=str.lower)

    def names(self):
        return list(self.keys())

    def asdict(self):
        d = {}
        for name, font_def in self.items():
            d[name] = {
                'family': font_def.family,
                'size': font_def.size,
            }
        return d

    def load(self, font_defs):
        self.clear()
        for name, font_def in font_defs.items():
            self[name] = GFont(family=font_def['family'], size=font_def['size'])

font_assets = None


class State(Enum):
    IDLE = 0
    DRAGGING = 1
    SELECTING = 2

class Handler:
    def __init__(self, tk_root, width=320, height=240, scale=1, grid_alignment=8):
        self.width = width
        self.height = height
        self.scale = scale
        self.grid_alignment = grid_alignment
        self.display_list = []
        self.sel_items = []
        self.on_sel_changed = None
        self.state = State.IDLE

        c = self.canvas = tk.Canvas(tk_root, background='gray')
        c.bind('<1>', self.canvas_select)
        c.bind('<Motion>', self.canvas_move)
        c.bind('<B1-Motion>', self.canvas_drag)
        c.bind('<ButtonRelease-1>', self.canvas_end_move)
        c.bind('<Any-KeyRelease>', self.canvas_key)

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

    def grid_align(self, value):
        return align(value, self.grid_alignment)

    def tk_scale(self, *values):
        if len(values) == 1:
            return value * self.scale
        return tuple(x * self.scale for x in values)

    def tk_point(self, x, y):
        xo, yo = self.draw_offset
        x, y = self.tk_scale(x, y)
        return (xo + x, yo + y)

    def canvas_point(self, x, y):
        xo, yo = self.draw_offset
        return ((x - xo) // self.scale, (y - yo) // self.scale)

    def tk_bounds(self, rect):
        xo, yo = self.draw_offset
        b = self.tk_scale(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h)
        return (xo + b[0], yo + b[1], xo + b[2], yo + b[3])

    def tk_font(self, font_name: str):
        font = font_assets[font_name]
        return tkinter.font.Font(family=font.family, size=-font.size*self.scale)

    def clear(self):
        self.display_list.clear()
        self.sel_items.clear()
        self.state = State.IDLE
        self.redraw()
        if self.on_sel_changed:
            self.on_sel_changed(True)

    def add_items(self, item_list):
        self.display_list.extend(item_list)
        self.redraw()

    def add_item(self, item):
        self.display_list.append(item)
        self.draw_item(item)

    def draw_item(self, item):
        tags = ('item', str(Element.ITEM), item.id)
        c = Canvas(self, tags)
        item.draw(c)

    def remove_item(self, item):
        self.canvas.delete(item.id)
        self.display_list.remove(item)

    def get_current(self):
        tags = self.canvas.gettags(tk.CURRENT)
        if len(tags) < 2:
            return None, None
        elem = Element(int(tags[1]))
        if tags[0] == 'handle':
            return elem, None
        item = next(x for x in self.display_list if x.id == tags[2])
        return elem, item

    def draw_handles(self):
        hr = None
        tags = ('handle', str(Element.ITEM))
        for item in self.sel_items:
            hr = union(hr, item) if hr else item.bounds
            r = tk_inflate(self.tk_bounds(item), 1, 1)
            self.canvas.create_rectangle(r, outline='white', width=1, tags=tags)
        if len(self.sel_items) > 1:
            r = tk_inflate(self.tk_bounds(hr), 1, 1)
            self.canvas.create_rectangle(r, outline='white', width=1, tags=tags)
        for e in Element:
            if e == Element.ITEM:
                continue
            tags = ('handle', str(e))
            pt = get_handle_pos(hr, e)
            r = self.tk_bounds(Rect(pt[0], pt[1]))
            r = tk_inflate(r, 4, 4)
            self.canvas.create_rectangle(r, outline='', fill='white', tags=tags)
        return hr

    def remove_handles(self):
        self.canvas.delete('handle')

    def canvas_select(self, evt):
        self.canvas.focus_set()
        self.sel_pos = (evt.x, evt.y)
        is_multi = (evt.state & EVS_CONTROL) != 0
        elem, item = self.get_current()
        if not elem:
            if not is_multi and self.sel_items:
                self.remove_handles()
                self.sel_items = []
                if self.on_sel_changed:
                    self.on_sel_changed(True)
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
            self.on_sel_changed(True)

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

        def resize_item(item, r):
            item.bounds = r
            self.canvas.delete(item.id)
            self.draw_item(item)

        if elem == Element.ITEM:
            for item, orig in zip(self.sel_items, self.orig_bounds):
                r = item.get_bounds()
                r.x, r.y = self.grid_align((orig.x + off[0], orig.y + off[1]))
                resize_item(item, r)
        elif len(self.sel_items) == 1:
            item, orig = self.sel_items[0], self.orig_bounds[0]
            r = item.get_bounds()
            if elem & DIR_N:
                r.y = self.grid_align(orig.y + off[1])
                r.h = orig.y + orig.h - r.y
            if elem & DIR_E:
                r.w = self.grid_align(orig.w + off[0])
            if elem & DIR_S:
                r.h = self.grid_align(orig.h + off[1])
            if elem & DIR_W:
                r.x = self.grid_align(orig.x + off[0])
                r.w = orig.x + orig.w - r.x
            min_size = item.get_min_size()
            if r.w >= min_size[0] and r.h >= min_size[1]:
                resize_item(item, r)
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
                    r.x = self.grid_align(cur_bounds.x + (orig.x - orig_bounds.x) * cur_bounds.w // orig_bounds.w)
                    r.y = self.grid_align(cur_bounds.y + (orig.y - orig_bounds.y) * cur_bounds.h // orig_bounds.h)
                    if do_size:
                        r.w = self.grid_align(orig.w * cur_bounds.w // orig_bounds.w)
                        r.h = self.grid_align(orig.h * cur_bounds.h // orig_bounds.h)
                        min_size = item.get_min_size()
                        if r.w < min_size[0] or r.h < min_size[1]:
                            continue
                    resize_item(item, r)

        self.draw_handles()
        if self.on_sel_changed:
            self.on_sel_changed(False)


    def canvas_end_move(self, evt):
        if self.state != State.DRAGGING:
            return
        self.state = State.IDLE
        self.redraw() # Fix Z-ordering and ensure consistency


    def redraw(self):
        self.canvas.delete(tk.ALL)
        r = self.tk_bounds(Rect(0, 0, self.width, self.height))
        self.canvas.create_rectangle(r, outline='', fill='black')
        for item in self.display_list:
            self.draw_item(item)
        self.canvas.create_rectangle(r[0]-1, r[1]-1, r[2]+1, r[3]+1, outline='dimgray')
        if self.sel_items:
            self.sel_bounds = self.draw_handles()

    def canvas_key(self, evt):
        # print(evt)
        def add_item(cls):
            x, y = self.canvas_point(evt.x, evt.y)
            item = cls(x, y, 50, 50)
            self.add_item(item)
            self.state = State.DRAGGING
            self.canvas.dtag('current', 'current')
            self.canvas.addtag_withtag('current', item.id)
            self.canvas_select(evt)
            self.canvas_drag(evt)

        if evt.state == 0 and evt.keysym == 'Delete':
            for item in self.sel_items:
                self.display_list.remove(item)
            self.sel_items.clear()
            self.redraw()
        elif evt.state in [0, EVS_SHIFT]:
            cls = {
                'r': GRect,
                'e': GEllipse,
                'R': GFilledRect,
                'E': GFilledEllipse,
                't': GText,
                'b': GButton,
            }.get(evt.keysym)
            if cls is not None:
                add_item(cls)


class Editor:
    def __init__(self, root, title):
        self.frame = ttk.LabelFrame(root, text=title)

    def addLabel(self, text, row):
        label = ttk.Label(self.frame, text=text)
        label.grid(row=row, column=0, sticky=tk.E, padx=8)
        return label


class PropertyEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Properties')
        self.on_value_changed = None
        self.fields = {}
        self.is_updating = False

    def clear(self):
        for w in self.frame.winfo_children():
            w.destroy()
        self.fields.clear()

    def set_field(self, name=str, values=list, options=list):
        value_list = values + [o for o in options if o not in values]
        if name in self.fields:
            var, cb = self.fields[name]
            self.is_updating = True
            var.set(value=values[0] if len(values) == 1 else '')
            self.is_updating = False
            cb.configure(values=value_list)
            return

        row = len(self.fields)
        self.addLabel(name.replace('_', ' '), row)
        var = tk.StringVar(name=name, value=values[0] if len(values) == 1 else '')
        var.trace_add('write', self.value_changed)
        cb = ttk.Combobox(self.frame, textvariable=var, values=value_list)
        cb.grid(row=row, column=1)
        self.fields[name] = (var, cb)

    def value_changed(self, name1, name2, op):
        if self.is_updating:
            return
        """See 'trace add variable' in TCL docs"""
        fld = self.fields.get(name1)
        if fld is None:
            return
        var = fld[0]
        # print(f'value_changed:"{name1}", "{name2}", "{op}", "{var.get()}"')
        if self.on_value_changed:
            self.on_value_changed(name1, var.get())

    def get_value(self, name):
        var, _ = self.fields[name]
        return var.get()


class FontEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Font')
        self.on_value_changed = None
        self.is_updating = False
        row = 0
        self.addLabel('Name', row)
        self.font_name = tk.StringVar()
        self.font_name.trace_add('write', self.sel_changed)
        self.fontsel = ttk.Combobox(self.frame, textvariable=self.font_name)
        self.fontsel.grid(row=row, column=1)
        row += 1
        self.addLabel('Family', row)
        self.family = tk.StringVar()
        self.family.trace_add('write', self.value_changed)
        c = ttk.Combobox(self.frame, textvariable=self.family, values=font_assets.families())
        c.grid(row=row, column=1)
        row += 1
        self.addLabel('Size', row)
        self.size = tk.IntVar()
        self.size.trace_add('write', self.value_changed)
        c = ttk.Entry(self.frame, textvariable=self.size)
        c.grid(row=row, column=1)
        row += 1
        styleFrame = ttk.Labelframe(self.frame, text='Styles')
        styleFrame.grid(row=row, column=0, columnspan=2)
        self.styles = {}
        def addCheck(name):
            v = self.styles[name] = tk.BooleanVar()
            c = ttk.Checkbutton(styleFrame, variable=v, text=name)
            c.pack()
        for style in ('normal', 'italic', 'bold', 'bold-italic'):
            addCheck(style)
        self.update()

        # self.on_value_changed = None
        # self.fields = {}
        # self.is_updating = False

    def sel_changed(self, name1, name2, op):
        font_name = self.font_name.get()
        # print('sel_changed', name1, name2, op, font_name)
        self.select(font_name)

    def value_changed(self, name1, name2, op):
        if self.is_updating:
            return
        font_name = self.font_name.get()
        # print('value_changed', name1, name2, op, font_name)
        font = font_assets[font_name]
        font.family = self.family.get()
        font.size = self.size.get()
        if self.on_value_changed:
            self.on_value_changed(font_name)

    def update(self):
        font_names = font_assets.names()
        self.fontsel.configure(values=font_names)
        self.select(font_names[0])

    def select(self, font_name):
        self.is_updating = True
        try:
            font = font_assets[font_name]
            self.font_name.set(font_name)
            self.family.set(font.family)
            self.size.set(font.size)
        finally:
            self.is_updating = False


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
                'tag': item.id,
            }
            d.update(dataclasses.asdict(item))
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
                ac = item.__dataclass_fields__[a].type
                setattr(item, a, ac(v))
            display_list.append(item)
        return display_list

    # Menus
    def fileClear():
        handler.clear()
        font_assets.clear()
        font_editor.update()

    def fileAddRandom(count=10):
        display_list = []
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randint(w_min, w_max)
            h = randint(h_min, h_max)
            x = randrange(handler.width - w)
            y = randrange(handler.height - h)
            r = Rect(*handler.grid_align((x, y, w, h)))
            line_width = randint(1, 5)
            radius = randint(0, min(r.w, r.h) // 2)
            kind = randrange(6)
            if kind == 0:
                item = GEllipse(line_width=line_width)
            elif kind == 1:
                item = GFilledEllipse()
            elif kind == 2:
                item = GText()
            elif kind == 3:
                item = GButton()
            elif kind == 4:
                item = GRect(line_width=line_width, radius=radius)
            elif kind == 5:
                item = GFilledRect(radius=radius)
            item.bounds = r
            item.color = GColor(randrange(0xffffff))
            display_list.append(item)
        handler.add_items(display_list)


    def fileLoad():
        filename = filedialog.askopenfilename(title='Load project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            data = json_load(filename)
            font_assets.load(data['fonts'])
            font_editor.update()
            handler.display_list = []
            display_list = dl_deserialise(data['layout'])
            handler.clear()
            handler.add_items(display_list)

    def fileSave():
        filename = filedialog.asksaveasfilename(title='Save project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            ext = os.path.splitext(filename)[1]
            if ext != PROJECT_EXT:
                filename += PROJECT_EXT
            data = {
                'fonts': font_assets.asdict(),
                'layout': dl_serialise(handler.display_list),
            }
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

    # Layout editor
    handler = Handler(root)
    handler.set_scale(2)
    handler.canvas.grid(row=1, column=0, rowspan=2, sticky=tk.NSEW)

    # Toolbar
    toolbar = ttk.Frame(root)
    toolbar.grid(row=0, column=0, columnspan=2, sticky=tk.W)
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

    # Status bar
    status = tk.StringVar()
    label = ttk.Label(root, textvariable=status)
    label.grid(row=3, column=0, columnspan=2, sticky=tk.W)
    # grip = ttk.Sizegrip(root)
    # grip.grid(row=3, column=1, sticky=tk.SE)

    # Properties
    prop = PropertyEditor(root)
    prop.frame.grid(row=1, column=1, sticky=tk.NW)

    def value_changed(name, value):
        for item in handler.sel_items:
            try:
                cls = item.__dataclass_fields__[name].type
                # print(f'setattr({item}, {name}, {cls(value)})')
                setattr(item, name, cls(value))
            except ValueError as e:
                status.set(str(e))
        handler.redraw()
    prop.on_value_changed = value_changed

    # Selection handling
    def sel_changed(full_change: bool):
        if full_change:
            prop.clear()
        items = handler.sel_items
        if not items:
            return
        if full_change:
            types = set(x.itemtype() for x in items)
            prop.set_field('type', list(types), [])
        fields = {}
        for item in items:
            for name, value in dataclasses.asdict(item).items():
                values = fields.setdefault(name, set())
                values.add(value)
        for name, values in fields.items():
            if name == 'font':
                options = font_assets.names()
            elif name == 'color':
                options = sorted(colormap.keys())
            else:
                options = []
            prop.set_field(name, list(values), options)

    handler.on_sel_changed = sel_changed

    # Fonts
    global font_assets
    font_assets = FontAssets()
    font_editor = FontEditor(root)
    def font_value_changed(font_name):
        handler.redraw()
    font_editor.on_value_changed = font_value_changed
    font_editor.frame.grid(row=2, column=1, sticky=tk.NW)

    #
    tk.mainloop()


if __name__ == "__main__":
    run()
