import os
import sys
import io
import binascii
import copy
from enum import Enum, IntEnum, StrEnum
import random
from random import randrange, randint
import dataclasses
from dataclasses import dataclass
import json
import tkinter as tk
import tkinter.font
from tkinter import ttk, filedialog, colorchooser
from gtypes import colormap, ColorFormat
import resource
from item import *
from typing import TypeAlias
from widgets import *
import remote

TkVarType: TypeAlias = str | int | float # python types used for tk.StringVar, tk.IntVar, tk.DoubleVar
CanvasPoint: TypeAlias = tuple[int, int] # x, y on canvas
CanvasRect: TypeAlias = tuple[int, int, int, int] # x0, y0, x1, y1 on canvas
DisplayPoint: TypeAlias = tuple[int, int] # x, y on display
ItemList: TypeAlias = list[GItem]

class Tag(StrEnum):
    """TK Canvas tags to identify class of item"""
    HANDLE = '@@handle'
    ITEM = '@@item'
    BORDER = '@@border'
    BACKGROUND = '@@background'

# Event state modifier masks
EVS_SHIFT = 0x0001
EVS_LOCK = 0x0002
EVS_CONTROL = 0x0004
EVS_MOD1 = 0x0008 # Linux: Left ALT; Windows: numlock
EVS_MOD2 = 0x0010 # Linux: Numlock
EVS_MOD3 = 0x0020 # Windows: scroll lock
EVS_MOD4 = 0x0040
EVS_MOD5 = 0x0080 # Linux: Right ALT
EVS_BUTTON1 = 0x0100
EVS_BUTTON2 = 0x0200
EVS_BUTTON3 = 0x0400
EVS_BUTTON4 = 0x0800
EVS_BUTTON5 = 0x1000

# Windows-specific codes
EVS_WIN_LEFTALT = 0x20000
EVS_WIN_RIGHTALT = 0x20000 | EVS_CONTROL

# MAC codes not checked


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

# Globals
font_assets = None
image_assets = None

class Canvas:
    """Provides the interface for items to draw themselves"""
    def __init__(self, layout, tags):
        self.layout = layout
        self.tags = tags
        self.back_color = 0
        self.color = 'white'
        self.line_width = 1
        self.scale = 1
        self.font = None
        self.fontstyle = set()
        self.halign = Align.Left
        self.valign = Align.Top

    def draw_rect(self, rect):
        layout = self.layout
        x0, y0, x1, y1 = layout.tk_bounds(rect)
        w = self.line_width * layout.scale
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_rectangle(x0, y0, x1-1, y1-1, outline=color, width=w, tags=self.tags)

    def fill_rect(self, rect):
        layout = self.layout
        x0, y0, x1, y1 = layout.tk_bounds(rect)
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_rectangle(x0, y0, x1, y1, fill=color, outline='', width=0, tags=self.tags)

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
        layout = self.layout
        x0, y0 = layout.tk_point(x0, y0)
        x1, y1 = layout.tk_point(x1, y1)
        w = self.line_width * layout.scale
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_line(x0, y0, x1, y1, fill=color, width=w, tags=self.tags)

    def draw_corner(self, x, y, r, start_angle):
        layout = self.layout
        x0, y0 = layout.tk_point(x, y)
        x1, y1 = layout.tk_point(x+r*2, y+r*2)
        w = self.line_width * layout.scale
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_arc(x0, y0, x1, y1, start=start_angle, extent=90, outline=color, width=w, style='arc', tags=self.tags)

    def fill_corner(self, x, y, r, start_angle):
        layout = self.layout
        x0, y0 = layout.tk_point(x, y)
        x1, y1 = layout.tk_point(x+r*2, y+r*2)
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_arc(x0, y0, x1, y1, start=start_angle, extent=90, fill=color, outline='', tags=self.tags)

    def draw_ellipse(self, rect):
        layout = self.layout
        x0, y0, x1, y1 = layout.tk_bounds(rect)
        w = self.line_width * layout.scale
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_oval(x0, y0, x1, y1, outline=color, width=w, tags=self.tags)

    def fill_ellipse(self, rect):
        layout = self.layout
        x0, y0, x1, y1 = layout.tk_bounds(rect)
        color = self.color.value_str(ColorFormat.tkinter)
        layout.canvas.create_oval(x0, y0, x1, y1, fill=color, outline='', width=0, tags=self.tags)

    def draw_text(self, rect, text):
        layout = self.layout
        font = font_assets.get(self.font, font_assets.default)
        tk_image = font.draw_tk_image(
            width=rect.w,
            height=rect.h,
            scale=layout.scale,
            fontstyle=self.fontstyle,
            fontscale=self.scale,
            halign=self.halign,
            valign=self.valign,
            back_color=self.back_color,
            color=self.color,
            text=text)
        x, y = layout.tk_point(rect.x, rect.y)
        layout.canvas.create_image(x, y, image=tk_image, anchor=tk.NW, tags=self.tags)
        return tk_image

    def draw_image(self, rect, image: str, offset: CanvasPoint):
        """Important: Caller must retain copy of returned image otherwise it gets disposed"""
        layout = self.layout
        x0, y0, x1, y1 = layout.tk_bounds(rect)
        crop_rect = Rect(*offset, rect.w, rect.h)
        image_asset = image_assets.get(image, resource.Image())
        tk_image = image_asset.get_tk_image(crop_rect, layout.scale)
        layout.canvas.create_image(x0, y0, image=tk_image, anchor=tk.NW, tags=self.tags)
        return tk_image


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


class State(Enum):
    IDLE = 0
    DRAGGING = 1
    PANNING = 2
    SCALING = 3

class LayoutEditor(ttk.Frame):
    def __init__(self, master, width: int = 320, height: int = 240, scale: float = 1.0, grid_alignment: int = 8):
        super().__init__(master)
        self.width = width
        self.height = height
        self.orientation = 0 # Not used internally
        self.scale = scale
        self.grid_alignment: int = grid_alignment
        self.display_list = []
        self.sel_items = []
        self.on_sel_changed = None
        self.on_scale_changed = None
        self.state = State.IDLE

        c = self.canvas = tk.Canvas(self)
        hs = ttk.Scrollbar(self, orient=tk.HORIZONTAL, command=c.xview)
        hs.pack(side=tk.BOTTOM, fill=tk.X)
        vs = ttk.Scrollbar(self, orient=tk.VERTICAL, command=c.yview)
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
            orientation=self.orientation,
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

    def clear(self):
        self.display_list.clear()
        self.sel_items.clear()
        self.state = State.IDLE
        self.redraw()
        self.sel_changed(True)

    def add_items(self, items: ItemList):
        self.display_list.extend(items)
        for item in items:
            self.draw_item(item)

    def add_item(self, item: GItem):
        self.display_list.append(item)
        self.draw_item(item)

    def draw_item(self, item: GItem):
        tags = (Tag.ITEM, int(Element.ITEM), item.id)
        c = Canvas(self, tags)
        item.draw(c)

    def remove_item(self, item: GItem):
        self.canvas.delete(item.id)
        self.display_list.remove(item)

    def get_current(self) -> tuple[Element, GItem]:
        tags = self.canvas.gettags(tk.CURRENT)
        if len(tags) < 2 or tags[0] not in [Tag.HANDLE, Tag.ITEM]:
            return None, None
        elem = Element(int(tags[1]))
        if tags[0] == Tag.HANDLE:
            return elem, None
        item = next(x for x in self.display_list if x.id == tags[2])
        return elem, item

    def draw_handles(self):
        self.canvas.delete(Tag.HANDLE)
        if not self.sel_items:
            return
        hr = None
        tags = (Tag.HANDLE, str(Element.ITEM))
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
            tags = (Tag.HANDLE, str(e))
            pt = get_handle_pos(hr, e)
            r = self.tk_bounds(Rect(pt[0], pt[1]))
            r = tk_inflate(r, 4, 4)
            self.canvas.create_rectangle(r, outline='', fill='white', tags=tags)
        if self.state == State.IDLE:
            self.sel_bounds = hr

    def select(self, items: ItemList):
        self.state = State.IDLE
        self.sel_items = items
        self.draw_handles()
        self.sel_changed(True)

    def canvas_select(self, evt):
        self.canvas.focus_set()
        self.sel_pos = (evt.x, evt.y)
        is_multi = (evt.state & EVS_CONTROL) != 0
        elem, item = self.get_current()
        if not elem:
            if not is_multi and self.sel_items:
                self.sel_items = []
                self.draw_handles()
                self.sel_changed(True)
            self.canvas.scan_mark(evt.x, evt.y)
            return

        self.sel_elem = elem
        sel_changed = False
        if item and not item in self.sel_items:
            if is_multi:
                self.sel_items.append(item)
            else:
                self.sel_items = [item]
            sel_changed = True
            self.draw_handles()
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
            delta = 1 if evt.delta > 0 else -1
        if control and not shift:
            scale = round(self.scale + delta / 10, 2)
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
        elem = self.sel_elem
        if self.state != State.DRAGGING:
            self.state = State.DRAGGING
            self.set_cursor(self.get_cursor(elem))
            self.orig_bounds = [x.get_bounds() for x in self.sel_items]
        off = round((evt.x - self.sel_pos[0]) / self.scale), round((evt.y - self.sel_pos[1]) / self.scale)

        def resize_item(item, r):
            item.bounds = r
            self.canvas.addtag_withtag('updating', item.id)
            self.draw_item(item)
            self.canvas.tag_raise(item.id, 'updating')
            self.canvas.delete('updating')

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

    def redraw(self):
        self.canvas.delete(tk.ALL)
        r = self.tk_bounds(Rect(0, 0, self.width, self.height))
        self.canvas.create_rectangle(r, outline='', fill='black', tags=(Tag.BACKGROUND))
        for item in self.display_list:
            self.draw_item(item)
        self.canvas.create_rectangle(r[0]-1, r[1]-1, r[2]+1, r[3]+1, outline='dimgray', tags=(Tag.BORDER))
        self.draw_handles()

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
                self.canvas.delete(item.id)
            self.select([])

        def shift_items(xo, yo):
            for item in self.sel_items:
                item.x += xo
                item.y += yo
            self.redraw()
            self.sel_changed(False)

        mod = evt.state & (EVS_CONTROL | EVS_SHIFT)
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
            'home': (self.z_move, 'top'),
            'end': (self.z_move, 'bottom'),
            'prior': (self.z_move, 'raise'),
            'next': (self.z_move, 'lower'),
        }.get(c)
        if opt:
            opt[0](*opt[1:])

    def z_move(self, cmd: str):
        """Move item with highest Z-order to top, stack others immediately below it"""
        if not self.sel_items:
            return
        dl = self.display_list
        z_list = self.sel_items
        z_list.sort(reverse=cmd in ['bottom', 'lower'], key=lambda x: dl.index(x))
        # z = [(self.display_list.index(x), x) for x in items]
        # z.sort(key=lambda e: e[0])
        i = {
            'top': len(dl),
            'bottom': 0,
            'raise': min(dl.index(z_list[-1]) + 1, len(dl) - 1),
            'lower': max(dl.index(z_list[-1]) - 1, 0),
        }[cmd]
        for item in z_list:
            dl.remove(item)
            dl.insert(i, item)
        self.redraw()
        self.sel_changed(False)


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
        self.draw_handles()
        self.sel_changed(True)


class Editor(ttk.LabelFrame):
    def __init__(self, master, title: str):
        super().__init__(master, text=title)
        self.columnconfigure(1, weight=1)
        self.is_updating = False
        self.on_value_changed = None
        self.fields = {}

    @property
    def name(self):
        return self.cget('text')

    def reset(self):
        for w in self.winfo_children():
            w.destroy()
        self.fields.clear()

    @staticmethod
    def text_from_name(name: str) -> str:
        words = re.findall(r'[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', name)
        return ' '.join(words).lower()

    def add_field(self, name: str, widget_type, **kwargs):
        row = len(self.fields)
        LabelWidget(self, self.text_from_name(name)).set_row(row)
        w = widget_type(self, name, callback=self.value_changed, **kwargs).set_row(row)
        self.fields[name] = w
        return w

    def add_entry_field(self, name: str, vartype=int):
        return self.add_field(name, EntryWidget, vartype=vartype)

    def add_spinbox_field(self, name: str, from_: int, to: int, increment: int = 1):
        return self.add_field(name, SpinboxWidget, from_=from_, to=to, increment=increment)

    def add_text_field(self, name: str):
        return self.add_field(name, TextWidget)

    def add_scale_field(self, name: str, vartype: type, from_: int | float, to: int | float, resolution: int | float = 1):
        return self.add_field(name, ScaleWidget, vartype=vartype, from_=from_, to=to, resolution=resolution)

    def add_combo_field(self, name: str, values=[], vartype=str):
        return self.add_field(name, ComboboxWidget, vartype=vartype, values=values)

    def add_check_field(self, name: str):
        return self.add_field(name, CheckFieldWidget)

    def add_check_fields(self, name: str, is_set: bool, values: list):
        return self.add_field(name, CheckFieldsWidget, is_set=is_set, values=values)

    def add_grouped_check_fields(self, name: str, groups: dict):
        w = GroupedCheckFieldsWidget(self, name, groups, self.value_changed)
        self.fields[name] = w
        return w

    def value_changed(self, name: str, value: TkVarType):
        if not self.is_updating and self.on_value_changed:
            self.on_value_changed(name, value)

    def get_value(self, name: str) -> TkVarType:
        return self.fields[name].get_value()

    def load_values(self, object):
        self.is_updating = True
        try:
            for name, widget in self.fields.items():
                widget.set_value(getattr(object, name))
        finally:
            self.is_updating = False


class ProjectEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Project')
        self.filename = None
        self.add_entry_field('width')
        self.add_entry_field('height')
        self.add_check_fields('orientation', False, ['0:0°', '1:90°','2:180°', '3:270°'])
        self.add_scale_field('scale', float, MIN_SCALE, MAX_SCALE, 0.1)
        self.add_entry_field('grid_alignment')


class PropertyEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Properties')

    def set_field(self, name=str, values=list, options=list, callback=None):
        self.is_updating = True
        try:
            value_list = values + [o for o in options if o not in values]
            if name in self.fields:
                widget = self.fields[name]
                widget.set_value(value=values[0] if len(values) == 1 else '')
                widget.set_choices(value_list)
                return

            if name == 'fontstyle':
                widget = self.add_grouped_check_fields(name,
                    {
                        'faceStyle:set': ('Bold', 'Italic'),
                        'underscore': ('Single:Underscore', 'Double:DoubleUnderscore'),
                        'overscore': ('Single:Overscore', 'Double:DoubleOverscore'),
                        'strikeout': ('Single:Strikeout', 'Double:DoubleStrikeout'),
                        'extra': ('DotMatrix', 'HLine', 'VLine')
                    })
            elif name == 'halign':
                widget = self.add_check_fields(name, False, ('Left', 'Centre', 'Right'))
            elif name == 'valign':
                widget = self.add_check_fields(name, False, ('Top', 'Middle', 'Bottom'))
            elif name == 'text':
                widget = self.add_text_field(name)
            elif name == 'fontscale':
                widget = self.add_spinbox_field(name, 1, 16)
            elif name in ['xoff', 'yoff']:
                widget = self.add_spinbox_field(name, -100, 100)
            elif name == 'radius':
                widget = self.add_spinbox_field(name, 0, 255)
            else:
                widget = self.add_combo_field(name, value_list)
                if callback:
                    def handle_event(evt, callback=callback, widget=widget):
                        if not self.is_updating:
                            callback(widget, evt.type)
                    widget.bind('<Double-1>', handle_event)
                    widget.bind('<FocusIn>', handle_event)
                    widget.bind('<FocusOut>', handle_event)
            if len(values) == 1:
                widget.set_value(values[0])
        finally:
            self.is_updating = False


class ItemEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Items')
        self.on_select = None
        def select_changed():
            if self.on_select:
                self.on_select()
        tree = self.tree = TreeviewWidget(self, select_changed)
        tree.pack(expand=True, fill=tk.BOTH)

    def bind(self, *args):
        self.tree.bind(*args)

    def update(self, items):
        self.tree.set_choices(reversed(items))
 
    def select(self, sel_items):
        self.tree.select(sel_items)

    @property
    def sel_items(self):
        return self.tree.get_selection()


class FontEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Font')
        self.add_combo_field('name')
        self.add_combo_field('family', font_assets.families())
        self.add_spinbox_field('size', 5, 100)
        self.add_check_field('mono')
        for x in resource.FaceStyle:
            self.add_combo_field(x.name)
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
            font = resource.Font(name=font_name)
            font_assets.append(font)
        curvalue = getattr(font, name)
        if value == curvalue:
            return
        setattr(font, name, value)
        if name == 'family':
            sysfont = resource.system_fonts[font.family]
            for fs in resource.FaceStyle:
                style = ''
                for x in sysfont.values():
                    if fs == x.facestyle:
                        style = x.style
                        break
                self.fields[fs.name].set_value(style)
        self.update()
        super().value_changed(name, value)

    def update(self):
        font_name = self.get_value('name')
        font_names = font_assets.names()
        self.fields['name'].set_choices(font_names)
        if not font_name and font_names:
            font_name = font_names[0]
        self.select(font_name)

    def select(self, font: str | resource.Font):
        if font is None:
            font = font_assets.default
        elif isinstance(font, str):
            font = font_assets.get(font, font_assets.default)
        sysfont = resource.system_fonts.get(font.family, [])
        choices = list(sysfont)
        for x in resource.FaceStyle:
            self.fields[x.name].set_choices(choices)
        self.load_values(font)


class ImageEditor(Editor):
    def __init__(self, root):
        super().__init__(root, 'Image')
        self.add_combo_field('name')
        widget = self.add_combo_field('source')
        widget.bind('<Double-1>', self.choose_source)
        self.add_combo_field('format', values=['BMP', 'RGB24', 'RGB565'])
        self.add_entry_field('width')
        self.add_entry_field('height')
        self.update()

    def choose_source(self, evt):
        image_name = self.get_value('name').strip()
        if not image_name:
            return
        IMAGE_FILTER = [('Image files', '*.*')]
        filename = filedialog.askopenfilename(
            title='Load project',
            initialfile=self.get_value('source'),
            filetypes=IMAGE_FILTER
        )
        if len(filename) == 0:
            return
        filename = os.path.relpath(filename)
        image = image_assets.get(image_name, None)
        if image is None:
            image = resource.Image(name=image_name, source=filename)
            image_assets.append(image)
        else:
            image.source = filename
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
        self.fields['name'].set_choices(image_names)
        if not image and image_assets:
            image = image_assets[0]
        self.select(image)

    def select(self, image: str | resource.Image):
        if isinstance(image, str):
            image = image_assets.get(image)
        if not image:
            for widget in self.fields.values():
                widget.set_value('')
            return
        self.load_values(image)


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
        data = {}
        for item in display_list:
            d = {
                'type': item.typename,
            }
            for name, value in item.asdict().items():
                if name == 'id':
                    continue
                if type(value) in [int, float, str, list]:
                    d[name] = value
                elif isinstance(value, set):
                    d[name] = list(value)
                else:
                    d[name] = str(value)
            data[item.id] = d
        return data

    def dl_deserialise(data: dict) -> ItemList:
        display_list = []
        for id, d in data.items():
            item = GItem.create(d.pop('type'), id=id)
            for a, v in d.items():
                setattr(item, a, v)
            display_list.append(item)
        return display_list

    def get_project_data() -> dict:
        return {
            'project': layout.asdict(),
            'fonts': font_assets.asdict(),
            'images': image_assets.asdict(),
            'layout': dl_serialise(layout.display_list),
        }

    def load_project(data: dict):
        fileClear()
        layout.load(data['project'])
        for k, v in data['project'].items():
            project_editor.fields[k].set_value(v)
        font_assets.load(data['fonts'])
        font_editor.update()
        image_assets.load(data['images'])
        image_editor.update()
        layout.display_list = []
        display_list = dl_deserialise(data['layout'])
        layout.clear()
        layout.add_items(display_list)
        item_editor.update(layout.display_list)

    # Menus
    def fileClear():
        layout.clear()
        font_assets.clear()
        font_editor.update()
        image_assets.clear()
        image_editor.update()
        project_editor.filename = None

    def fileAddRandom(count=10):
        display_list = []
        id_list = set(x.id for x in layout.display_list)
        for i in range(count):
            w_min, w_max = 10, 200
            h_min, h_max = 10, 100
            w = randint(w_min, w_max)
            h = randint(h_min, h_max)
            x = randrange(layout.width - w)
            y = randrange(layout.height - h)
            x, y, w, h = layout.grid_align(x, y, w, h)
            itemtype = random.choice([
                GRect,
                GFilledRect,
                GEllipse,
                GFilledEllipse,
                GText,
                GButton,
            ])
            item = itemtype(x=x, y=y, w=w, h=h, color = random.choice(list(colormap)))
            item.assign_unique_id(id_list)
            id_list.add(item.id)
            if hasattr(item, 'line_width'):
                item.line_width = randint(1, 5)
            if hasattr(item, 'radius'):
                item.radius = randint(0, min(w, h) // 2)
            if hasattr(item, 'text'):
                item.text = item.id.replace('_', ' ')
            display_list.append(item)
        layout.add_items(display_list)
        layout.sel_changed(False)


    def fileLoad():
        filename = filedialog.askopenfilename(
            title='Load project',
            initialfile=project_editor.filename,
            filetypes=PROJECT_FILTER
        )
        if len(filename) != 0:
            os.chdir(os.path.dirname(filename))
            data = json_load(filename)
            load_project(data)
            project_editor.filename = filename

    def fileSave():
        filename = filedialog.asksaveasfilename(
            title='Save project',
            initialfile=project_editor.filename,
            filetypes=PROJECT_FILTER)
        if len(filename) != 0:
            ext = os.path.splitext(filename)[1]
            if ext != PROJECT_EXT:
                filename += PROJECT_EXT
            data = get_project_data()
            json_save(data, filename)
            project_editor.filename = filename

    def fileList():
        data = get_project_data()
        print(json_dumps(data))
        # layout.canvas.delete(Tag.HANDLE, Tag.BORDER)
        # layout.canvas.update()
        # x, y = layout.draw_offset
        # w, h = layout.width * layout.scale, layout.height * layout.scale
        # B = 8
        # layout.canvas.postscript(file='test.ps', x=x-B, y=y-B, width=w+B*2, height=h+B*2)

    def fileGenerateResource():
        from resource import rclib, FaceStyle

        images = {}
        for img in image_assets:
            d = dict(
                source = img.source,
                transform = dict(
                    resize=f'{img.width},{img.height}'
                ),
            )
            if img.format:
                d['format'] = img.format
            images[img.name] = d

        fonts = {}
        for font in font_assets:
            d = dict(
                mono = font.mono,
                size = font.size,
            )
            sysfont = resource.system_fonts[font.family]
            for fs in FaceStyle:
                style = getattr(font, fs.name)
                if style:
                    d[fs.name] = os.path.basename(sysfont[style].filename)
            fonts[font.name] = d

        resources = dict(
            image = images,
            font = fonts,
        )

        print(json_dumps(resources))

        data = rclib.parse(resources)

        def base64(data):
            return binascii.b2a_base64(data, newline=False)

        client = remote.Client('192.168.13.10', 23)

        bmOffset = 0
        for item in data:
            print(type(item))
            if isinstance(item, rclib.image.Image):
                # struct ImageResource
                rec = item.serialize(bmOffset)
                print(rec.hex())
                line = f'r:image;{item.name};'.encode() + base64(rec)
                # item.headerSize = rclib.base.StructSize.Image
                bmOffset += item.get_bitmap_size()
                client.send_line(line)
                continue
            if isinstance(item, rclib.font.Font):
                rec = item.serialize(bmOffset)
                bmOffset += item.get_bitmap_size()
                print(rec)

                # line = f'r:font;{item.name};' + base64(font_resource)

                # client.send_line('@:font-begin;')
                # buf.seek(0)
                # while blk := buf.read(64):
                #     client.send_line(b'b:;' + base64(blk))
                # client.send_line('@:resource-end;')


        buf = io.BytesIO()
        rclib.writeBitmap(data, buf)

        client.send_line('@:resource-begin;')
        buf.seek(0)
        while blk := buf.read(64):
            client.send_line(b'b:;' + base64(blk))
        client.send_line('@:resource-end;')
    

    def fileSend():
        client = remote.Client('192.168.13.10', 23)
        data = remote.serialise(layout.display_list)
        client.send_line(f'@:size;w={layout.width};h={layout.height};orient={layout.orientation};')
        client.send_line(f'@:clear')
        for line in data:
            client.send_line(line)
        client.send_line('@:render;')


    root = tk.Tk(className='GED')
    root.geometry('1000x600')
    root.title('Graphical Layout Editor')

    pane = ttk.PanedWindow(root, orient=tk.HORIZONTAL)
    pane.pack(side=tk.TOP, expand=True, fill=tk.BOTH)

    # Layout editor
    layout = LayoutEditor(pane)
    layout.set_scale(2)
    layout.pack(side=tk.LEFT, expand=True, fill=tk.BOTH)
    pane.add(layout, weight=2)

    # Editors to right
    edit_frame = ttk.PanedWindow(pane, orient=tk.VERTICAL, width=320)
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
    addButton('Gen .rc', fileGenerateResource)
    addButton('Send', fileSend)

    # Status bar
    status = tk.StringVar()
    label = ttk.Label(root, textvariable=status)
    label.pack()

    res_frame = ttk.Notebook(edit_frame)
    edit_frame.add(res_frame, weight=1)
    def add_editor(editor_class):
        editor = editor_class(res_frame)
        res_frame.add(editor, text=editor.name, sticky=tk.NSEW)
        return editor

    # Project
    project_editor = add_editor(ProjectEditor)
    project_editor.load_values(layout)
    def project_value_changed(name, value):
        if name == 'width':
            layout.set_size(value, layout.height)
        elif name == 'height':
            layout.set_size(layout.width, value)
        elif name == 'orientation':
            layout.orientation = int(value)
        elif name == 'scale':
            layout.set_scale(value)
        else:
            setattr(layout, name, value)
    project_editor.on_value_changed = project_value_changed
    def scale_changed(layout):
        project_editor.fields['scale'].set_value(layout.scale)
    layout.on_scale_changed = scale_changed

    # Properties
    prop = PropertyEditor(edit_frame)
    edit_frame.add(prop, weight=2)

    def value_changed(name: str, value: TkVarType):
        if name == 'type':
            # Must defer this as caller holds reference to var which messes things up when destroying fields
            def change_type():
                new_sel_items = []
                for item in layout.sel_items:
                    new_item = item.copy_as(value)
                    i = layout.display_list.index(item)
                    layout.display_list[i] = new_item
                    new_sel_items.append(new_item)
                layout.select(new_sel_items)
            root.after(100, change_type)
            return

        for item in layout.sel_items:
            if not hasattr(item, name):
                continue
            try:
                setattr(item, name, value)
                if name == 'font':
                    font_editor.select(value)
                elif name == 'image':
                    image_editor.select(value)
            except ValueError as e:
                status.set(str(e))
        layout.redraw()
    prop.on_value_changed = value_changed

    # Selection handling
    def sel_changed(full_change: bool):
        if full_change:
            prop.reset()
        items = layout.sel_items
        item_editor.update(layout.display_list)
        item_editor.select(layout.sel_items)
        if not items:
            return
        if full_change:
            typenames = set(x.typename for x in items)
            prop.set_field('type', list(typenames), TYPENAMES)
        fields = {}
        for item in items:
            for name, value in item.asdict().items():
                if isinstance(value, list):
                    values = fields.setdefault(name, [])
                    if value:
                        values.append(value)
                else:
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
                def select_font(widget, event_type):
                    if event_type == tk.EventType.FocusIn:
                        widget.set_choices(font_assets.names())
                        res_frame.select(font_editor)
                callback = select_font
            elif name == 'image':
                options = image_assets.names()
                def select_image(widget, event_type):
                    if event_type == tk.EventType.FocusIn:
                        widget.set_choices(image_assets.names())
                        res_frame.select(image_editor)
                callback = select_image
            elif values and isinstance(values[0], Color):
                options = sorted(colormap.keys())
                def select_color(widget, event_type):
                    if event_type == tk.EventType.ButtonPress:
                        res = colorchooser.askcolor(color=widget.get_value())
                        if res[1] is not None:
                            widget.set_value(Color(res[1]))
                callback = select_color
            # elif name == 'fontstyle':
            #     pass
            else:
                options = []
            prop.set_field(name, values, options, callback)

    layout.on_sel_changed = sel_changed

    # Items
    item_editor = add_editor(ItemEditor)
    def item_select():
        layout.select(item_editor.sel_items)
    item_editor.on_select = item_select
    item_editor.bind('<Key-Home>', lambda _: layout.z_move('top'))
    item_editor.bind('<Key-End>', lambda _: layout.z_move('bottom'))
    item_editor.bind('<Key-Prior>', lambda _: layout.z_move('raise'))
    item_editor.bind('<Key-Next>', lambda _: layout.z_move('lower'))

    # Fonts
    global font_assets
    font_assets = resource.FontList()
    font_editor = add_editor(FontEditor)
    def font_value_changed(name: str, value: TkVarType):
        # print(f'font_value_changed("{name}", "{value}")')
        layout.redraw()
    font_editor.on_value_changed = font_value_changed

    # Images
    global image_assets
    image_assets = resource.ImageList()
    image_editor = add_editor(ImageEditor)
    def image_value_changed(name: str, value: TkVarType):
        # print(f'image_value_changed("{name}", "{value}")')
        layout.redraw()
    image_editor.on_value_changed = image_value_changed

    #
    tk.mainloop()


if __name__ == "__main__":
    run()
