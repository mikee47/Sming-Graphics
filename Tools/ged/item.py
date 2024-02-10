import copy
from dataclasses import dataclass
from PIL.ImageColor import colormap
import tkinter as tk

rev_colormap = {value: name for name, value in colormap.items()}

MIN_ITEM_WIDTH = MIN_ITEM_HEIGHT = 2


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
        return copy.copy(self)

    @bounds.setter
    def bounds(self, rect):
        self.x, self.y, self.w, self.h = rect.x, rect.y, rect.w, rect.h

    def inflate(self, xo, yo):
        return Rect(self.x - xo, self.y - yo, self.w + xo*2, self.h + yo*2)


@dataclass
class GFont:
    family: str = ''
    size: int = 12
    # style: list[str] For now, assume all styles are available


class GColor(int):
    def __new__(cls, value):
        if isinstance(value, str):
            if value == '' or str.isdigit(value[0]):
                value = int(value, 0)
            else:
                if value[0] != '#':
                    try:
                        value = colormap[value]
                    except KeyError:
                        raise ValueError(f'Unknown color name {value}')
                if value[0] != '#':
                    raise ValueError(f'Bad color {value}')
                value = int(value[1:], 16)
        return int.__new__(cls, value)

    def __init__(self, *args, **kwds):
        pass

    def value_str(self):
        return '#%06x' % self

    def __str__(self):
        s = self.value_str()
        return rev_colormap.get(s, s)

    def __repr__(self):
        return str(self)



@dataclass
class GItem(Rect):
    color: GColor = GColor('orange')

    def itemtype(self):
        return self.__class__.__name__[1:]

    def get_min_size(self, offset=0):
        return (MIN_ITEM_WIDTH + offset, MIN_ITEM_HEIGHT + offset)

    def get_bounds(self):
        return Rect(self.x, self.y, self.w, self.h)

    @property
    def id(self):
        return 'G%08x' % id(self)


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
    font: str = 'default'
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'Text {self.id}'

    def draw(self, c):
        c.color = str(self.color)
        c.font = self.font
        r = self.get_bounds()
        M = 8
        r.inflate(-M, -M)
        c.draw_text(r, self.text, tk.CENTER, tk.CENTER)


@dataclass
class GButton(GItem):
    font: str = 'default'
    text: str = None

    def __post_init__(self):
        super().__post_init__()
        if self.text is None:
            self.text = f'button {self.id}'

    def draw(self, c):
        radius = min(self.w, self.h) // 8
        M = radius // 2
        r = self.get_bounds()
        r.inflate(-M, -M)
        c.color = str(self.color)
        c.fill_rounded_rect(self, radius)
        c.color = 'white'
        c.font = self.font
        c.draw_text(r, self.text, tk.CENTER, tk.CENTER)
