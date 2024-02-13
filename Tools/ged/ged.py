import os
import sys
import copy
import re
from enum import Enum, IntEnum
import random
from random import randrange, randint
import dataclasses
from dataclasses import dataclass
import json
import tkinter as tk
import tkinter.font
from tkinter import ttk, filedialog, colorchooser
from gtypes import colormap
from resource import Font, Image
from item import *

# Event state modifier masks
EVS_SHIFT = 0x0001
EVS_LOCK = 0x0002
EVS_CONTROL = 0x0004
EVS_ALTLEFT = 0x0008
EVS_MOD2 = 0x0010 # Numlock
EVS_MOD3 = 0x0020
EVS_MOD4 = 0x0040
EVS_ALTRIGHT = 0x0080 # Mod5


def align(boundary: int, *values):
    if boundary <= 1:
        return values
    def align_single(value):
        n = (value + boundary // 2) // boundary
        return n * boundary
    if len(values) == 1:
        return align_single(values[0])
    return tuple(align_single(x) for x in values)

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
    """Provides the interface for items to draw themselves"""
    def __init__(self, handler, tags):
        self.handler = handler
        self.canvas = handler.canvas
        self.tags = tags
        self.color = 'white'
        self.line_width = 1
        self.font = None

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

    def draw_image(self, rect, tk_image):
        x0, y0, x1, y1 = self.handler.tk_bounds(rect)
        self.canvas.create_image(x0, y0, image=tk_image, anchor=tk.NW, tags=self.tags)


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


class ResourceList(list):
    def get(self, name, default=None):
        try:
            return next(r for r in self if r.name == name)
        except StopIteration:
            return default

    def names(self):
        return [r.name for r in self]

    def asdict(self):
        res = {}
        for r in self:
            d = r.asdict()
            del d['name']
            res[r.name] = d
        return res

    def load(self, res_class, res_dict):
        self.clear()
        for name, rdef in res_dict.items():
            r = res_class(name=name)
            for a, v in rdef.items():
                setattr(r, a, v)
            self.append(r)


class FontAssets(ResourceList):
    def __init__(self):
        tk_def = FontAssets.tk_default().configure()
        font = Font(family = tk_def['family'])
        self.default = font

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

    def load(self, res_dict):
        super().load(Font, res_dict)

font_assets = None


class ImageAssets(ResourceList):
    def load(self, res_dict):
        super().load(Image, res_dict)

image_assets = None


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

        self.frame = ttk.Frame(tk_root)
        c = self.canvas = tk.Canvas(self.frame, background='gray')
        c.pack(side=tk.TOP, expand=True, fill=tk.BOTH)
        s = ttk.Scrollbar(self.frame, orient=tk.HORIZONTAL, command=c.xview)
        s.pack(side=tk.BOTTOM, fill=tk.X)
        c.configure(xscrollcommand=s.set)

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

    def grid_align(self, *values):
        return align(self.grid_alignment, *values)

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
        font = font_assets.get(font_name, font_assets.default)
        return tkinter.font.Font(family=font.family, size=-font.size*self.scale)

    def tk_image(self, image_name: str, crop_rect: Rect):
        return image_assets.get(image_name, Image()).get_tk_image(crop_rect, self.scale)

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

    def select(self, items: list):
        self.state = State.IDLE
        self.sel_items = items
        self.redraw()
        if self.on_sel_changed:
            self.on_sel_changed(True)

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

    def get_cursor(self, elem):
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
            Element.ITEM: 'target' if self.state == State.DRAGGING else 'hand1',
        }.get(elem)

    def canvas_move(self, evt):
        if self.state == State.DRAGGING:
            return
        elem, item = self.get_current()
        self.canvas.configure(cursor= self.get_cursor(elem))

    def canvas_drag(self, evt):
        if not self.sel_items:
            return
        self.remove_handles()
        elem = self.sel_elem
        if self.state != State.DRAGGING:
            self.state = State.DRAGGING
            self.canvas.configure(cursor=self.get_cursor(elem))
            self.orig_bounds = [x.get_bounds() for x in self.sel_items]
        off = (evt.x - self.sel_pos[0]) // self.scale, (evt.y - self.sel_pos[1]) // self.scale

        def resize_item(item, r):
            item.bounds = r
            self.canvas.delete(item.id)
            self.draw_item(item)

        if elem == Element.ITEM:
            for item, orig in zip(self.sel_items, self.orig_bounds):
                r = item.get_bounds()
                r.x, r.y = self.grid_align(orig.x + off[0], orig.y + off[1])
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
        self.canvas.configure(cursor=self.get_cursor(self.sel_elem))
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
        def add_item(itemtype):
            x, y = self.canvas_point(evt.x, evt.y)
            x, y, w, h = *self.grid_align(x, y), *self.grid_align(50, 50)
            item = GItem.create(itemtype, x=x, y=y, w=w, h=h)
            item.assign_unique_id(self.display_list)
            if hasattr(item, 'text'):
                item.text = item.id.replace('_', ' ')
            self.add_item(item)
            self.select([item])

        mod = evt.state & (EVS_CONTROL | EVS_SHIFT | EVS_ALTLEFT | EVS_ALTRIGHT)
        if mod == 0 and evt.keysym == 'Delete':
            for item in self.sel_items:
                self.display_list.remove(item)
            self.select([])
            return
        if len(evt.keysym) != 1 or (mod & EVS_CONTROL):
            return
        c = evt.keysym.upper() if (mod & EVS_SHIFT) else evt.keysym.lower()
        itemtype = {
            'r': 'Rect',
            'R': 'FilledRect',
            'e': 'Ellipse',
            'E': 'FilledEllipse',
            'i': 'Image',
            't': 'Text',
            'b': 'Button',
        }.get(c)
        if itemtype is not None:
            add_item(itemtype)


class Editor:
    def __init__(self, root, title, field_prefix):
        self.frame = ttk.LabelFrame(root, text=title)
        self.field_prefix = field_prefix
        self.frame.columnconfigure(1, weight=1)
        self.is_updating = False
        self.on_value_changed = None
        self.fields = {}

    def clear(self):
        for w in self.frame.winfo_children():
            w.destroy()
        self.fields.clear()

    def add_label(self, text, row):
        label = ttk.Label(self.frame, text=text)
        label.grid(row=row, column=0, sticky=tk.E, padx=8)
        return label

    def add_control(self, ctrl, row):
        ctrl.grid(row=row, column=1, sticky=tk.EW, padx=4, pady=2)
        return ctrl

    @staticmethod
    def text_from_name(name: str):
        words = re.findall(r'[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', name)
        return ' '.join(words).lower()

    def add_field(self, name: str, ctrl, var_type=tk.StringVar):
        row = len(self.fields)
        self.add_label(self.text_from_name(name), row)
        var = var_type(name=self.field_prefix+name)
        ctrl.configure(textvariable=var)
        var.trace_add('write', self.tk_value_changed)
        self.add_control(ctrl, row)
        fld = self.fields[name] = (var, ctrl)
        return fld

    def add_entry_field(self, name: str, var_type=tk.StringVar):
        c = ttk.Entry(self.frame)
        return self.add_field(name, c, var_type)

    def add_combo_field(self, name: str, value_list=[], var_type=tk.StringVar):
        c = ttk.Combobox(self.frame, values=value_list)
        return self.add_field(name, c, var_type)

    def add_check_fields(self, title, names: list):
        if title:
            frame = ttk.LabelFrame(self.frame, text=title)
        else:
            frame = ttk.Frame(self.frame)
        row = len(self.fields)
        self.add_control(frame, row)
        frame.grid(column=0, columnspan=2)
        for name in names:
            var = tk.BooleanVar(name=self.field_prefix+name)
            var.trace_add('write', self.tk_value_changed)
            ctrl = ttk.Checkbutton(frame, variable=var, text=self.text_from_name(name))
            ctrl.pack(side=tk.LEFT)
            self.fields[name] = (var, ctrl)
            row += 1

    def tk_value_changed(self, name1, name2, op):
        if self.is_updating:
            return
        """See 'trace add variable' in TCL docs"""
        name = name1[len(self.field_prefix):]
        var, _ = self.fields[name]
        if var is None:
            return
        try:
            # print(f'value_changed:"{name1}", "{name2}", "{op}", "{var.get()}"')
            self.value_changed(name, var.get())
        except tk.TclError: # Can happen whilst typing if conversion fails, empty strings, etc.
            pass

    def value_changed(self, name, value):
        if self.on_value_changed:
            self.on_value_changed(name, value)

    def set_value(self, name, value):
        var, _ = self.fields[name]
        var.set(value)

    def get_value(self, name):
        var, _ = self.fields[name]
        return var.get()



class ProjectEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Project', 'proj-')
        for name in ['width', 'height', 'scale', 'grid_alignment']:
            self.add_entry_field(name, tk.IntVar)


class PropertyEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Properties', 'prop-')

    def set_field(self, name=str, values=list, options=list, callback=None):
        self.is_updating = True
        try:
            value_list = values + [o for o in options if o not in values]
            if name in self.fields:
                var, cb = self.fields[name]
                var.set(value=values[0] if len(values) == 1 else '')
                cb.configure(values=value_list)
                return

            var, cb = self.add_combo_field(name, value_list)
            if len(values) == 1:
                var.set(values[0])
            if callback:
                def handle_event(evt, callback=callback, var=var, ctrl=cb):
                    print(evt.type.name)
                    if not self.is_updating:
                        callback(var, ctrl, evt.type)
                cb.bind('<Double-1>', handle_event)
                cb.bind('<FocusIn>', handle_event)
                cb.bind('<FocusOut>', handle_event)

        finally:
            self.is_updating = False


class FontEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Font', 'font-')
        self.add_combo_field('name')
        self.add_combo_field('family', font_assets.families())
        self.add_entry_field('size', tk.IntVar)
        self.add_check_fields('Style', ['normal', 'italic', 'bold', 'boldItalic'])
        self.update()

    def value_changed(self, name, value):
        if name == 'name':
            font = font_assets.get(value)
            if font:
                self.select(font)
            return
        font_name = self.get_value('name')
        # print(f'value_changed: "{name}", "{value}", "{font_name}"')
        font = font_assets.get(font_name)
        if font is None:
            font = Font(name=font_name)
            font_assets.append(font)
        setattr(font, name, value)
        self.update()
        super().value_changed(name, value)

    def update(self):
        font_name = self.get_value('name')
        font_names = font_assets.names()
        _, c = self.fields['name']
        c.configure(values=font_names)
        if not font_name and font_names:
            font_name = font_names[0]
        self.select(font_name)

    def select(self, font):
        if font is None:
            font = font_assets.default
        elif isinstance(font, str):
            font = font_assets.get(font, font_assets.default)
        self.is_updating = True
        try:
            for f in ['name', 'family', 'size']:
                self.set_value(f, getattr(font, f))
        finally:
            self.is_updating = False


class ImageEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Image', 'img-')
        self.add_combo_field('name')
        _, cb = self.add_combo_field('source')
        cb.bind('<Double-1>', self.choose_source)
        self.add_combo_field('format')
        self.add_entry_field('width', tk.IntVar)
        self.add_entry_field('height', tk.IntVar)
        self.update()

    def choose_source(self, evt):
        image_name = self.get_value('name').strip()
        if not image_name:
            return
        IMAGE_FILTER = [('Image files', '*.*')]
        filename = filedialog.askopenfilename(title='Load project', filetypes=IMAGE_FILTER)
        if len(filename) == 0:
            return
        image = image_assets.get(image_name, None)
        if image is None:
            image = Image(name=image_name, source=filename)
            image_assets.append(image)
            self.update()

    def value_changed(self, name, value):
        if name == 'name':
            image = image_assets.get(value)
            if image:
                self.select(image)
            return
        image_name = self.get_value('name')
        # print(f'value_changed: "{name}", "{value}", "{font_name}"')
        image = image_assets.get(image_name)
        if image is None:
            return
        setattr(image, name, value)
        self.update()
        super().value_changed(name, value)

    def update(self):
        image = self.get_value('name')
        image_names = image_assets.names()
        _, c = self.fields['name']
        c.configure(values=image_names)
        if not image and image_assets:
            image = image_assets[0]
        self.select(image)

    def select(self, image):
        if isinstance(image, str):
            image = image_assets.get(image)
        if not image:
            for var, _ in self.fields.values():
                var.set('')
            return
        self.is_updating = True
        try:
            for k, v in dataclasses.asdict(image).items():
                self.set_value(k, v)
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
                'type': item.typename,
            }
            for name, value in item.asdict().items():
                if type(value) in [int, float, str]:
                    d[name] = value
                else:
                    d[name] = str(value)
            data.append(d)
        return data

    def dl_deserialise(data):
        display_list = []
        for d in data:
            item = GItem.create(d.pop('type'), id=d.pop('id'))
            for a, v in d.items():
                ac = item.fieldtype(a)
                setattr(item, a, ac(v))
            display_list.append(item)
        return display_list

    def get_project_data():
        return {
            'fonts': font_assets.asdict(),
            'images': image_assets.asdict(),
            'layout': dl_serialise(handler.display_list),
        }

    def load_project(data):
            font_assets.load(data['fonts'])
            font_editor.update()
            image_assets.load(data['images'])
            image_editor.update()
            handler.display_list = []
            display_list = dl_deserialise(data['layout'])
            handler.clear()
            handler.add_items(display_list)

    # Menus
    def fileClear():
        handler.clear()
        font_assets.clear()
        font_editor.update()
        image_assets.clear()
        image_editor.update()

    def fileAddRandom(count=10):
        display_list = []
        id_list = set(x.id for x in handler.display_list)
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randint(w_min, w_max)
            h = randint(h_min, h_max)
            x = randrange(handler.width - w)
            y = randrange(handler.height - h)
            x, y, w, h = handler.grid_align(x, y, w, h)
            itemtype = random.choice([
                GRect,
                GFilledRect,
                GEllipse,
                GFilledEllipse,
                GText,
                GButton,
            ])
            item = itemtype(x=x, y=y, w=w, h=h, color = Color(randrange(0xffffff)))
            item.assign_unique_id(id_list)
            id_list.add(item.id)
            if hasattr(item, 'line_width'):
                item.line_width = randint(1, 5)
            if hasattr(item, 'radius'):
                item.radius = randint(0, min(w, h) // 2)
            if hasattr(item, 'text'):
                item.text = item.id.replace('_', ' ')
            display_list.append(item)
        handler.add_items(display_list)


    def fileLoad():
        filename = filedialog.askopenfilename(title='Load project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            data = json_load(filename)
            load_project(data)

    def fileSave():
        filename = filedialog.asksaveasfilename(title='Save project', filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            ext = os.path.splitext(filename)[1]
            if ext != PROJECT_EXT:
                filename += PROJECT_EXT
            data = get_project_data()
            json_save(data, filename)

    def fileList():
        data = get_project_data()
        print(json_dumps(data))

    root = tk.Tk(className='GED')
    root.geometry('800x600')
    root.title('Graphical Layout Editor')

    pane = ttk.PanedWindow(root, orient=tk.HORIZONTAL)
    pane.pack(side=tk.TOP, expand=True, fill=tk.BOTH)

    # Layout editor
    handler = Handler(pane)
    handler.set_scale(2)
    handler.frame.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)
    pane.add(handler.frame, weight=2)

    # Editors to right
    edit_frame = ttk.Frame(pane, width=240)
    edit_frame.pack(fill=tk.BOTH)
    pane.add(edit_frame)

    # Toolbar
    toolbar = ttk.Frame(root)
    toolbar.pack(side=tk.TOP, before=pane, fill=tk.X)
    def addButton(text, command):
        btn = ttk.Button(toolbar, text=text, command=command)
        btn.pack(side=tk.LEFT)
    addButton('Clear', fileClear)
    addButton('Add Random', fileAddRandom)
    addButton('Load...', fileLoad)
    addButton('Save...', fileSave)
    sep = ttk.Separator(toolbar, orient=tk.VERTICAL)
    sep.pack(side=tk.LEFT)
    addButton('List', fileList)
    def changeScale():
        scale = 1 + handler.scale % 4
        handler.set_scale(scale)
    addButton('scale', changeScale)

    # Status bar
    status = tk.StringVar()
    label = ttk.Label(root, textvariable=status)
    label.pack()

    # Project
    project = ProjectEditor(edit_frame)
    project.frame.pack(fill=tk.X, ipady=4)
    for name in project.fields.keys():
        project.set_value(name, getattr(handler, name))

    # Properties
    prop = PropertyEditor(edit_frame)

    def value_selected(name, value):
        if name == 'font':
            res_frame.select(font_editor.frame)
        elif name == 'image':
            res_frame.select(image_editor.frame)
    prop.on_value_select = value_selected
    def value_changed(name, value):
        for item in handler.sel_items:
            if not hasattr(item, name):
                continue
            try:
                cls = item.fieldtype(name)
                # print(f'setattr({item}, {name}, {cls(value)})')
                setattr(item, name, cls(value))
                if name == 'font':
                    font_editor.select(value)
                elif name == 'image':
                    image_editor.select(value)
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
            prop.frame.pack_forget()
            return
        prop.frame.pack(fill=tk.X, ipady=4)
        if full_change:
            typenames = set(x.typename for x in items)
            prop.set_field('type', list(typenames), [])
        fields = {}
        for item in items:
            for name, value in dataclasses.asdict(item).items():
                values = fields.setdefault(name, set())
                if value:
                    values.add(value)
        # ID must be unique for each item, don't allow group set
        if len(items) > 1:
            del fields['id']
        for name, values in fields.items():
            callback = None
            values = list(values)
            if name == 'font':
                options = font_assets.names()
                def select_font(var, ctrl, event_type):
                    if event_type == tk.EventType.FocusIn:
                        ctrl.configure(values=font_assets.names())
                        res_frame.select(font_editor.frame)
                callback = select_font
            elif name == 'image':
                options = image_assets.names()
                def select_image(var, ctrl, event_type):
                    if event_type == tk.EventType.FocusIn:
                        ctrl.configure(values=image_assets.names())
                        res_frame.select(image_editor.frame)
                callback = select_image
            elif isinstance(values[0], Color):
                options = sorted(colormap.keys())
                def select_color(var, ctrl, event_type):
                    if event_type == tk.EventType.ButtonPress:
                        res = colorchooser.askcolor(color=var.get())
                        if res[1] is not None:
                            var.set(Color(res[1]))
                callback = select_color
            else:
                options = []
            prop.set_field(name, values, options, callback)

    handler.on_sel_changed = sel_changed

    res_frame = ttk.Notebook(edit_frame)
    res_frame.pack(fill=tk.X, ipady=4)

    # Fonts
    global font_assets
    font_assets = FontAssets()
    font_editor = FontEditor(res_frame)
    res_frame.add(font_editor.frame, text='Fonts', sticky=tk.NSEW)
    def font_value_changed(name, value):
        # print(f'font_value_changed("{name}", "{value}")')
        handler.redraw()
    font_editor.on_value_changed = font_value_changed

    # Images
    global image_assets
    image_assets = ImageAssets()
    image_editor = ImageEditor(res_frame)
    res_frame.add(image_editor.frame, text='Images', sticky=tk.NSEW)
    def image_value_changed(name, value):
        # print(f'image_value_changed("{name}", "{value}")')
        handler.redraw()
    image_editor.on_value_changed = image_value_changed

    #
    tk.mainloop()


if __name__ == "__main__":
    run()
