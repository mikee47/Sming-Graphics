import copy
import dataclasses
from dataclasses import dataclass
from PIL.ImageColor import colormap
rev_colormap = {value: name for name, value in colormap.items()}

@dataclass
class DataObject:
    def __post_init__(self):
        pass

    def asdict(self):
        return dataclasses.asdict(self)

    def fieldtype(self, field_name: str):
        return self.__dataclass_fields__[field_name].type



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

