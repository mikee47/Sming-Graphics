import sys
import copy
from gtypes import Rect, Color
import dataclasses
from dataclasses import dataclass
import tkinter as tk

MIN_ITEM_WIDTH = MIN_ITEM_HEIGHT = 2


@dataclass
class GItem(Rect):
    id: str = None
    color: Color = Color('orange')

    @staticmethod
    def create(itemtype, **field_values):
        if isinstance(itemtype, str):
            itemtype = getattr(sys.modules[__name__], f'G{itemtype}')
        return itemtype(**field_values)

    @classmethod
    @property
    def typename(cls):
        return cls.__name__[1:]

    def assign_unique_id(self, existing_ids_or_list):
        if isinstance(existing_ids_or_list, set):
            existing_ids = existing_ids_or_list
        else:
            existing_ids = set(item.id for item in existing_ids_or_list)
        n = 1
        typename = self.typename
        while True:
            id = f'{typename}_{n}'
            if id not in existing_ids:
                self.id = id
                break
            n += 1

    def asdict(self):
        return dataclasses.asdict(self)

    def fieldtype(self, field_name: str):
        return self.__dataclass_fields__[field_name].type

    def get_min_size(self, offset=0):
        return (MIN_ITEM_WIDTH + offset, MIN_ITEM_HEIGHT + offset)

    def get_bounds(self):
        return Rect(self.x, self.y, self.w, self.h)


@dataclass
class GRect(GItem):
    line_width: int = 1
    radius: int = 0

    def get_min_size(self):
        return super().get_min_size((self.line_width + self.radius) * 2)

    def draw(self, c):
        c.color = str(self.color)
        c.line_width = self.line_width

        if self.radius > 1:
            c.draw_rounded_rect(self, self.radius)
        else:
            c.draw_rect(self)


@dataclass
class GFilledRect(GItem):
    radius: int = 0

    def get_min_size(self):
        return super().get_min_size(self.radius * 2)

    def draw(self, c):
        c.color = str(self.color)

        if self.radius > 1:
            c.fill_rounded_rect(self, self.radius)
        else:
            c.fill_rect(self)


@dataclass
class GEllipse(GItem):
    line_width: int = 1

    def get_min_size(self):
        return super().get_min_size(self.line_width * 2)

    def draw(self, c):
        c.color = str(self.color)
        c.line_width = self.line_width
        c.draw_ellipse(self)


@dataclass
class GFilledEllipse(GItem):
    def draw(self, c):
        c.color = str(self.color)
        c.fill_ellipse(self)


@dataclass
class GText(GItem):
    font: str = ''
    text: str = ''

    def draw(self, c):
        c.color = str(self.color)
        c.font = self.font
        r = self.get_bounds()
        M = 8
        r.inflate(-M, -M)
        c.draw_text(r, self.text, tk.CENTER, tk.CENTER)


@dataclass
class GImage(GItem):
    image: str = ''
    xoff: int = 0
    yoff: int = 0
    tk_image = None  # cached image, tkinter won't show it otherwise

    def draw(self, c):
        self.tk_image = c.handler.tk_image(self.image, Rect(self.xoff, self.yoff, self.w, self.h))
        c.draw_image(self, self.tk_image)


@dataclass
class GButton(GItem):
    back_color: Color = Color('gray')
    font: str = ''
    text: str = ''

    def draw(self, c):
        radius = min(self.w, self.h) // 8
        M = radius // 2
        r = self.get_bounds()
        r.inflate(-M, -M)
        c.color = str(self.back_color)
        c.fill_rounded_rect(self, radius)
        c.color = str(self.color)
        c.font = self.font
        c.draw_text(r, self.text, tk.CENTER, tk.CENTER)

@dataclass
class GLabel(GItem):
    back_color: Color = Color('gray')
    font: str = ''
    text: str = ''

    def draw(self, c):
        c.color = str(self.back_color)
        c.fill_rect(self)
        c.color = str(self.color)
        c.font = self.font
        c.draw_text(self, self.text, tk.CENTER, tk.CENTER)
