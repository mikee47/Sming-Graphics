import dataclasses
from dataclasses import dataclass
import tkinter as tk
import tkinter.font
import PIL.Image, PIL.ImageTk, PIL.ImageOps, PIL.ImageDraw
from gtypes import Rect, DataObject
from enum import Enum
from typing import TypeAlias

TkImage: TypeAlias = PIL.ImageTk.PhotoImage

@dataclass
class Resource(DataObject):
    name: str = ''


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


@dataclass
class Font(Resource):
    family: str = ''
    size: int = 12
    style: list[str] = ()

    def get_tk_font(self, scale, fontstyle: tuple[str]):
        style = {FontStyle[s] for s in fontstyle}
        args = {}
        if style & {FontStyle.Underscore, FontStyle.DoubleUnderscore}:
            args['underline'] = True
        if style & {FontStyle.Strikeout, FontStyle.DoubleStrikeout}:
            args['overstrike'] = True
        if FontStyle.Italic in style:
            args['slant'] = 'italic'
        if FontStyle.Bold in style:
            args['weight'] = 'bold'
        return tk.font.Font(family=self.family, size=-round(self.size * scale), **args)



@dataclass
class Image(Resource):
    source: str = ''
    format: str = ''
    width: int = 64
    height:int = 64
    image = None

    def __post_init__(self):
        super().__post_init__()
        if self.source:
            self.reset_size()

    def reset_size(self):
        try:
            img = PIL.Image.open(self.source)
            self.width, self.height = img.size
        except:
            pass

    def get_tk_image(self, crop_rect: Rect, scale: float = 1) -> TkImage:
        w, h = self.width, self.height
        img = self.image
        if img and (img.width != w or img.height != h):
            img = None
        if img is None:
            try:
                img = PIL.Image.open(self.source)
                if w == 0:
                    if h == 0:
                        w, h = img.width, img.height
                    else:
                        w = img.width * h // img.height
                elif h == 0:
                    h = img.height * w // img.width
                if w != img.width or h != img.height:
                    img = img.resize((w, h))
                self.width = w
                self.height = h
            except:
                img = None
            self.image = img

        if img:
            box = crop_rect.x, crop_rect.y, crop_rect.x + crop_rect.w, crop_rect.y + crop_rect.h
            img = self.image.crop(box)
            img = img.resize((round(img.width * scale), round(img.height * scale)))
        else:
            w, h = round(crop_rect.w * scale), round(crop_rect.h * scale)
            img = PIL.Image.new('RGB', (w, h), color='red')
            draw = PIL.ImageDraw.Draw(img)
            draw.line((0, 0, w, h), fill=128, width=3)
            draw.line((0, h, w, 0), fill=128, width=3)
        return PIL.ImageTk.PhotoImage(img)
