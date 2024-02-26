import copy
import dataclasses
from enum import Enum
from dataclasses import dataclass
from PIL.ImageColor import colormap
rev_colormap = {value: name for name, value in colormap.items()}

class FaceStyle(Enum):
    normal = 0
    bold = 1
    italic = 2
    boldItalic = 3


class FontStyle(Enum):
    """Style is a set of these values, using strings here but bitfields in library"""
    # typeface
    Bold = 0
    Italic = 1
    Underscore = 2
    Overscore = 3
    Strikeout = 4
    DoubleUnderscore = 5
    DoubleOverscore = 6
    DoubleStrikeout = 7
    DotMatrix = 8
    HLine = 9
    VLine = 10


class Align(Enum):
    Left = 0
    Centre = 1
    Right = 2
    Top = 0
    Middle = 1
    Bottom = 2


@dataclass
class DataObject:
    def __post_init__(self):
        pass

    def asdict(self):
        return dataclasses.asdict(self)

    def __setattr__(self, name, value):
        fld = self.__dataclass_fields__.get(name)
        if fld:
            value = fld.type(value)
        super().__setattr__(name, value)


@dataclass
class Rect(DataObject):
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0

    @property
    def bounds(self):
        return copy.copy(self)

    @bounds.setter
    def bounds(self, rect):
        self.x, self.y, self.w, self.h = rect.x, rect.y, rect.w, rect.h

    def inflate(self, xo, yo):
        return Rect(self.x - xo, self.y - yo, self.w + xo*2, self.h + yo*2)

class ColorFormat(Enum):
    graphics = 0 # Alpha as first 2 digits
    tkinter = 1 # No alpha channel in color (tkinter)
    pillow = 2 # Alpha as last 2 digits

class Color(int):
    def __new__(cls, value):
        if isinstance(value, str):
            if value == '' or value is None:
                value = 0
            elif str.isdigit(value[0]):
                value = int(value, 0)
            elif value[0] != '#':
                try:
                    value = colormap[value]
                except KeyError:
                    raise ValueError(f'Unknown color name {value}')
                value = 0xff000000 | int(value[1:], 16)
            elif value[0] != '#':
                raise ValueError(f'Bad color {value}')
            elif len(value) < 8:
                value = 0xff000000 | int(value[1:], 16)
            else:
                value = int(value[1:], 16)
        return int.__new__(cls, value)

    def __init__(self, *args, **kwds):
        pass

    def value_str(self, format: ColorFormat):
        if format == ColorFormat.graphics:
            return '#%08x' % self
        if format == ColorFormat.pillow:
            return '#%06x%02x' % (self & 0x00ffffff, self.alpha)
        return '#%06x' % self.rgb

    @property
    def rgb(self):
        return self & 0x00ffffff

    @property
    def alpha(self):
        return self >> 24

    def __str__(self):
        if self.alpha == 0xff:
            s = self.value_str(ColorFormat.tkinter)
            return rev_colormap.get(s, s)
        return self.value_str(ColorFormat.graphics)

    def __repr__(self):
        return str(self)

