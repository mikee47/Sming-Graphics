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
    # underscore
    Underscore = 2
    DoubleUnderscore = 3
    # overscore
    Overscore = 4
    DoubleOverscore = 5
    # strikeout
    Strikeout = 6
    DoubleStrikeout = 7
    # extra
    DotMatrix = 8
    HLine = 9
    VLine = 10


class Align(Enum):
    Left = 0
    Centre = 1
    Right = 2
    Top = 3
    Middle = 4
    Bottom = 5


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


class Color(int):
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

