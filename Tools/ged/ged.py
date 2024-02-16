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
from resource import Font, Image, TkImage
from item import *

type TkVarType = str | int | float # python types used for tk.StringVar, tk.IntVar, tk.DoubleVar
type CanvasPoint = typle[int, int] # x, y on canvas
type CanvasRect = tuple[int, int, int, int] # x0, y0, x1, y1 on canvas
type DisplayPoint = tuple[int, int] # x, y on display
type ItemList = list[GItem]

# Event state modifier masks
EVS_SHIFT = 0x0001
EVS_LOCK = 0x0002
EVS_CONTROL = 0x0004
EVS_MOD1 = 0x0008 # Left ALT
EVS_MOD2 = 0x0010 # Numlock
EVS_MOD3 = 0x0020
EVS_MOD4 = 0x0040
EVS_MOD5 = 0x0080 # Right ALT
EVS_BUTTON1 = 0x0100
EVS_BUTTON2 = 0x0200
EVS_BUTTON3 = 0x0400
EVS_BUTTON4 = 0x0800
EVS_BUTTON5 = 0x1000

# Display scaling
MIN_SCALE, MAX_SCALE = 1.0, 5.0

def align(boundary: int, *values):
    def align_single(value):
        if boundary <= 1:
            return value
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
    PANNING = 2
    SCALING = 3

class Handler:
    def __init__(self, tk_root, width: int = 320, height: int = 240, scale: float = 1.0, grid_alignment: int = 8):
        self.width = width
        self.height = height
        self.scale = scale
        self.grid_alignment: int = grid_alignment
        self.display_list = []
        self.sel_items = []
        self.on_sel_changed = None
        self.on_scale_changed = None
        self.state = State.IDLE

        self.frame = ttk.Frame(tk_root)
        c = self.canvas = tk.Canvas(self.frame)
        hs = ttk.Scrollbar(self.frame, orient=tk.HORIZONTAL, command=c.xview)
        hs.pack(side=tk.BOTTOM, fill=tk.X)
        vs = ttk.Scrollbar(self.frame, orient=tk.VERTICAL, command=c.yview)
        vs.pack(side=tk.RIGHT, fill=tk.Y)
        c.configure(xscrollcommand=hs.set, yscrollcommand=vs.set)
        c.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)

        def set_default_scroll_positions():
            c.xview_moveto(0.22)
            c.yview_moveto(0.22)
        c.after(100, set_default_scroll_positions)

        # Windows
        c.bind('<MouseWheel>', self.canvas_mousewheel)
        # Linux
        c.bind('<4>', self.canvas_mousewheel)
        c.bind('<5>', self.canvas_mousewheel)

        c.bind('<1>', self.canvas_select)
        c.bind('<Motion>', self.canvas_move)
        c.bind('<B1-Motion>', self.canvas_drag)
        c.bind('<ButtonRelease-1>', self.canvas_end_move)
        c.bind('<3>', self.canvas_pan_mark)
        c.bind('<B3-Motion>', self.canvas_pan)
        c.bind('<ButtonRelease-3>', self.canvas_end_pan)
        c.bind('<Any-KeyPress>', self.canvas_key)
        c.bind('<Control-a>', self.canvas_select_all)
        c.bind('<Control-d>', self.canvas_duplicate_selection)

    def load(self, data: dict):
        for k, v in data.items():
            setattr(self, k, v)
        self.size_changed()

    def asdict(self):
        return dict(
            width=self.width,
            height=self.height,
            scale=self.scale,
            grid_alignment=self.grid_alignment,
        )

    def set_size(self, width: int, height: int):
        if width == self.width and height == self.height:
            return
        self.width = width
        self.height = height
        self.size_changed()
    
    def set_scale(self, scale: float, refpos: CanvasPoint = None):
        if scale == self.scale or scale < MIN_SCALE or scale > MAX_SCALE:
            return
        if refpos is None:
            # Use current displayed centre
            x, y = self.canvas.winfo_width() // 2, self.canvas.winfo_height() // 2
        else:
            x, y = refpos
        # Adjust scrollbars so given point remains unchanged with new scale
        s = scale / self.scale
        x = round((self.canvas.canvasx(x) * s) - x)
        y = round((self.canvas.canvasy(y) * s) - y)
        self.scale = scale
        self.size_changed()
        _, _, w, h = self.tk_canvas_size()
        self.canvas.xview_moveto(x / w)
        self.canvas.yview_moveto(y / h)
        if self.on_scale_changed:
            self.on_scale_changed(self)

    def size_changed(self):
        xo, yo, w, h = self.tk_canvas_size()
        self.draw_offset = xo, yo
        self.canvas.configure(scrollregion=(0, 0, w, h))
        self.redraw()

    def grid_align(self, *values):
        return align(self.grid_alignment, *values)

    def tk_scale(self, *values):
        if len(values) == 1:
            return round(values[0] * self.scale)
        return tuple(round(x * self.scale) for x in values)

    def tk_canvas_size(self):
        """Return 4-tuple (x, y, w, h) where (x,y) is top-left of display area, (w,h) is total canvas size"""
        w, h = 2 * self.width * self.scale, 2 * self.height * self.scale
        x, y = w // 4, h // 4
        return x, y, w, h

    def tk_point(self, x: int, y: int):
        xo, yo = self.draw_offset
        x, y = self.tk_scale(x, y)
        return (xo + x, yo + y)

    def canvas_point(self, x: int, y: int) -> DisplayPoint:
        xo, yo = self.draw_offset
        x1 = round((self.canvas.canvasx(x) - xo) / self.scale)
        y1 = round((self.canvas.canvasy(y) - yo) / self.scale)
        return x1, y1

    def tk_bounds(self, rect: Rect) -> CanvasRect:
        xo, yo = self.draw_offset
        b = self.tk_scale(rect.x, rect.y, rect.x + rect.w, rect.y + rect.h)
        return (xo + b[0], yo + b[1], xo + b[2], yo + b[3])

    def tk_font(self, font_name: str) -> tk.font.Font:
        font = font_assets.get(font_name, font_assets.default)
        return tk.font.Font(family=font.family, size=-self.tk_scale(font.size))

    def tk_image(self, image_name: str, crop_rect: Rect) -> TkImage:
        image_resource = image_assets.get(image_name, Image())
        return image_resource.get_tk_image(crop_rect, self.scale)

    def clear(self):
        self.display_list.clear()
        self.sel_items.clear()
        self.state = State.IDLE
        self.redraw()
        self.sel_changed(True)

    def add_items(self, items: ItemList):
        self.display_list.extend(items)
        self.redraw()

    def add_item(self, item: GItem):
        self.display_list.append(item)
        self.draw_item(item)

    def draw_item(self, item: GItem):
        tags = ('item', str(Element.ITEM), item.id)
        c = Canvas(self, tags)
        item.draw(c)

    def remove_item(self, item: GItem):
        self.canvas.delete(item.id)
        self.display_list.remove(item)

    def get_current(self) -> tuple[Element, GItem]:
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

    def select(self, items: ItemList):
        self.state = State.IDLE
        self.sel_items = items
        self.redraw()
        self.sel_changed(True)

    def canvas_select(self, evt):
        self.canvas.focus_set()
        self.sel_pos = (evt.x, evt.y)
        is_multi = (evt.state & EVS_CONTROL) != 0
        elem, item = self.get_current()
        if not elem:
            if not is_multi and self.sel_items:
                self.remove_handles()
                self.sel_items = []
                self.sel_changed(True)
            self.canvas.scan_mark(evt.x, evt.y)
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
        if sel_changed:
            self.sel_changed(True)

    def get_cursor(self, elem: Element):
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

    def canvas_mousewheel(self, evt):
        shift = evt.state & EVS_SHIFT
        control = evt.state & EVS_CONTROL
        if evt.state & EVS_BUTTON3:
            if self.state != State.SCALING:
                self.set_cursor('circle')
            self.state = State.SCALING
            control = True
        if evt.num == 5:
            delta = -1
        elif evt.num == 4:
            delta = 1
        else:
            delta = evt.delta
        if control and not shift:
            scale = round(self.scale + delta / 5, 2)
            self.set_scale(scale, (evt.x, evt.y))
        elif shift and not control:
            self.canvas.yview_scroll(delta, tk.UNITS)
        elif not (control or shift):
            self.canvas.xview_scroll(delta, tk.UNITS)

    def set_cursor(self, cursor: str):
        self.canvas.configure(cursor=cursor)

    def canvas_move(self, evt):
        if self.state != State.IDLE:
            return
        elem, item = self.get_current()
        self.set_cursor(self.get_cursor(elem))

    def canvas_pan_mark(self, evt):
        self.canvas.scan_mark(evt.x, evt.y)
        self.state = State.PANNING
        self.set_cursor('sizing')

    def canvas_pan(self, evt):
        if self.state == State.IDLE:
            self.state = State.PANNING
            self.set_cursor('sizing')
        elif self.state != State.PANNING:
            return
        self.canvas.scan_dragto(evt.x, evt.y, gain=1)

    def canvas_end_pan(self, evt):
        self.state = State.IDLE
        self.set_cursor('')

    def canvas_drag(self, evt):
        if not self.sel_items:
            self.canvas_pan(evt)
            return
        def align(*values):
            if evt.state & EVS_SHIFT:
                return values[0] if len(values) == 1 else values
            return self.grid_align(*values)
        self.remove_handles()
        elem = self.sel_elem
        if self.state != State.DRAGGING:
            self.state = State.DRAGGING
            self.set_cursor(self.get_cursor(elem))
            self.orig_bounds = [x.get_bounds() for x in self.sel_items]
        off = round((evt.x - self.sel_pos[0]) / self.scale), round((evt.y - self.sel_pos[1]) / self.scale)

        def resize_item(item, r):
            item.bounds = r
            self.canvas.delete(item.id)
            self.draw_item(item)

        if elem == Element.ITEM:
            x, y = self.sel_bounds.x, self.sel_bounds.y
            off = align(x + off[0]) - x, align(y + off[1]) - y
            for item, orig in zip(self.sel_items, self.orig_bounds):
                r = item.get_bounds()
                r.x, r.y = orig.x + off[0], orig.y + off[1]
                resize_item(item, r)
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
                resize_item(item, r)
        else:
            # Scale the bounding rectangle
            orig = self.sel_bounds
            r = copy.copy(orig)
            if elem & DIR_N:
                r.y += off[1]
                r.h += orig.y - r.y
            elif elem & DIR_S:
                r.h += off[1]
            if elem & DIR_E:
                r.w += off[0]
            elif elem & DIR_W:
                r.x += off[0]
                r.w += orig.x - r.x
            cur_bounds = r
            orig_bounds = orig
            if evt.state & EVS_CONTROL:
                if r.w > 0 and r.h > 0:
                    # Use scaled rectangle to resize items
                    for item, orig in zip(self.sel_items, self.orig_bounds):
                        r = Rect(
                            align(cur_bounds.x + (orig.x - orig_bounds.x) * cur_bounds.w // orig_bounds.w),
                            align(cur_bounds.y + (orig.y - orig_bounds.y) * cur_bounds.h // orig_bounds.h),
                            align(orig.w * cur_bounds.w // orig_bounds.w),
                            align(orig.h * cur_bounds.h // orig_bounds.h) )
                        min_size = item.get_min_size()
                        if r.w >= min_size[0] and r.h >= min_size[1]:
                            resize_item(item, r)
            else:
                # Adjust item positions only within new bounding rectangle
                for item, orig in zip(self.sel_items, self.orig_bounds):
                    r = copy.copy(orig)
                    if elem & (DIR_N | DIR_S):
                        r.y = align(cur_bounds.y + (orig.y - orig_bounds.y) * (cur_bounds.h - orig.h) // (orig_bounds.h - orig.h))
                    if elem & (DIR_E | DIR_W):
                        r.x = align(cur_bounds.x + (orig.x - orig_bounds.x) * (cur_bounds.w - orig.w) // (orig_bounds.w - orig.w))
                    resize_item(item, r)

        self.draw_handles()
        self.sel_changed(False)


    def sel_changed(self, full: bool):
        if self.on_sel_changed:
            self.on_sel_changed(full)

    def canvas_end_move(self, evt):
        if self.state == State.PANNING:
            self.state = State.IDLE
            self.set_cursor('')
        elif self.state == State.DRAGGING:
            self.state = State.IDLE
            self.set_cursor(self.get_cursor(self.sel_elem))
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

        def delete_items():
            for item in self.sel_items:
                self.display_list.remove(item)
            self.select([])

        def shift_items(xo, yo):
            for item in self.sel_items:
                item.x += xo
                item.y += yo
            self.redraw()
            self.sel_changed(False)

        mod = evt.state & (EVS_CONTROL | EVS_SHIFT | EVS_MOD1 | EVS_MOD5)
        if mod & EVS_CONTROL:
            return
        c = evt.keysym.upper() if (mod & EVS_SHIFT) else evt.keysym.lower()
        g = self.grid_alignment
        opt = {
            'delete': (delete_items,),
            'left': (shift_items, -g, 0),
            'right': (shift_items, g, 0),
            'up': (shift_items, 0, -g),
            'down': (shift_items, 0, g),
            'LEFT': (shift_items, -1, 0),
            'RIGHT': (shift_items, 1, 0),
            'UP': (shift_items, 0, -1),
            'DOWN': (shift_items, 0, 1),
            'r': (add_item, 'Rect'),
            'R': (add_item, 'FilledRect'),
            'e': (add_item, 'Ellipse'),
            'E': (add_item, 'FilledEllipse'),
            'i': (add_item, 'Image'),
            't': (add_item, 'Text'),
            'b': (add_item, 'Button'),
            'l': (add_item, 'Label'),
        }.get(c)
        if opt:
            opt[0](*opt[1:])

    def canvas_select_all(self, evt):
        self.sel_items = list(self.display_list)
        self.redraw()
        self.sel_changed(True)

    def canvas_duplicate_selection(self, evt):
        items = [copy.copy(x) for x in self.sel_items]
        off = self.grid_align(20)
        id_list = set(x.id for x in self.display_list)
        for item in items:
            item.assign_unique_id(id_list)
            id_list.add(item.id)
            item.x += off
            item.y += off
        self.sel_items = items
        self.add_items(items)
        self.sel_changed(True)


class Editor:
    def __init__(self, root, title: str, field_prefix: str):
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

    def add_label(self, text: str, row: int):
        label = ttk.Label(self.frame, text=text)
        label.grid(row=row, column=0, sticky=tk.E, padx=8)
        return label

    def add_control(self, ctrl: tk.Widget, row: int):
        ctrl.grid(row=row, column=1, sticky=tk.EW, padx=4, pady=2)
        return ctrl

    @staticmethod
    def text_from_name(name: str) -> str:
        words = re.findall(r'[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', name)
        return ' '.join(words).lower()

    def add_field(self, name: str, ctrl: tk.Widget, var_type=tk.StringVar):
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

    def add_check_fields(self, title: str, names: list):
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

    def value_changed(self, name: str, value: TkVarType):
        if self.on_value_changed:
            self.on_value_changed(name, value)

    def set_value(self, name: str, value: TkVarType):
        var, _ = self.fields[name]
        var.set(value)

    def get_value(self, name: str) -> TkVarType:
        var, _ = self.fields[name]
        return var.get()



class ProjectEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Project', 'proj-')


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

    def value_changed(self, name: str, value: TkVarType):
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

    def select(self, font: str | Font):
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

    def value_changed(self, name: str, value: TkVarType):
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

    def select(self, image: str | Image):
        if isinstance(image, str):
            image = image_assets.get(image)
        if not image:
            for var, _ in self.fields.values():
                var.set('')
            return
        self.is_updating = True
        try:
            for k, v in image.asdict().items():
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

    def json_save(data: dict, filename):
        with open(filename, "w") as f:
            json.dump(data, f, indent=4)

    def json_dumps(obj):
        return json.dumps(obj, indent=4)

    def dl_serialise(display_list: ItemList) -> dict:
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

    def dl_deserialise(data: dict) -> ItemList:
        display_list = []
        for d in data:
            item = GItem.create(d.pop('type'), id=d.pop('id'))
            for a, v in d.items():
                ac = item.fieldtype(a)
                setattr(item, a, ac(v))
            display_list.append(item)
        return display_list

    def get_project_data() -> dict:
        return {
            'project': handler.asdict(),
            'fonts': font_assets.asdict(),
            'images': image_assets.asdict(),
            'layout': dl_serialise(handler.display_list),
        }

    def load_project(data: dict):
        if 'project' in data:
            handler.load(data['project'])
            for k, v in data['project'].items():
                project.set_value(k, v)
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
    root.geometry('1000x600')
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
    for k, v in handler.asdict().items():
        var, _ = project.add_entry_field(k, tk.DoubleVar if k == 'scale' else tk.IntVar)
        var.set(v)
    def project_value_changed(name, value):
        if name == 'width':
            handler.set_size(value, handler.height)
        elif name == 'height':
            handler.set_size(handler.width, value)
        elif name == 'scale':
            handler.set_scale(value)
        else:
            setattr(handler, name, value)
    project.on_value_changed = project_value_changed
    def scale_changed(handler):
        project.set_value('scale', handler.scale)
    handler.on_scale_changed = scale_changed

    # Properties
    prop = PropertyEditor(edit_frame)

    def value_changed(name: str, value: TkVarType):
        if name == 'type':
            # Must defer this as caller holds reference to var which messes things up when destroying fields
            def change_type():
                new_sel_items = []
                for item in handler.sel_items:
                    new_item = GItem.create(value)
                    for a, v in item.asdict().items():
                        if hasattr(new_item, a):
                            setattr(new_item, a, v)
                    i = handler.display_list.index(item)
                    handler.display_list[i] = new_item
                    new_sel_items.append(new_item)
                handler.select(new_sel_items)
            root.after(100, change_type)
            return

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
            prop.set_field('type', list(typenames), TYPENAMES)
        fields = {}
        for item in items:
            for name, value in item.asdict().items():
                values = fields.setdefault(name, set())
                if str(value):
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
            elif values and isinstance(values[0], Color):
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
    def font_value_changed(name: str, value: TkVarType):
        # print(f'font_value_changed("{name}", "{value}")')
        handler.redraw()
    font_editor.on_value_changed = font_value_changed

    # Images
    global image_assets
    image_assets = ImageAssets()
    image_editor = ImageEditor(res_frame)
    res_frame.add(image_editor.frame, text='Images', sticky=tk.NSEW)
    def image_value_changed(name: str, value: TkVarType):
        # print(f'image_value_changed("{name}", "{value}")')
        handler.redraw()
    image_editor.on_value_changed = image_value_changed

    #
    tk.mainloop()


if __name__ == "__main__":
    run()
