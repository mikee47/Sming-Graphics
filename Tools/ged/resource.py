import sys
import os
import dataclasses
from dataclasses import dataclass
import freetype
import PIL.Image, PIL.ImageOps, PIL.ImageDraw, PIL.ImageFont
from PIL.ImageTk import PhotoImage as TkImage
from gtypes import Rect, DataObject, Color, ColorFormat, FaceStyle, FontStyle, Align
from enum import Enum
import textwrap

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../rc')))
import rclib


@dataclass
class Resource(DataObject):
    name: str = ''


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


@dataclass
class FaceInfo:
    flags: int
    style: str
    facestyle: FaceStyle
    filename: str


# @dataclass
# class SystemFont:
#     faces: dict[FaceInfo] = dataclasses.field(default_factory=dict)


class SystemFonts(dict):
    def __init__(self):
        self.scan()

    def scan(self):
        self.clear()

        fontfiles = set()
        for path in rclib.font.system_font_directories:
            for walkroot, walkdirs, walkfilenames in os.walk(os.path.expandvars(path)):
                fontfiles |= {os.path.join(walkroot, name) for name in walkfilenames}

        for filename in fontfiles:
            _, ext = os.path.splitext(filename)
            if ext not in rclib.font.parsers:
                continue
            try:
                face = freetype.Face(filename)
            except:
                continue
            name = face.family_name.decode()
            ft_italic = face.style_flags & freetype.FT_STYLE_FLAGS['FT_STYLE_FLAG_ITALIC']
            ft_bold = face.style_flags & freetype.FT_STYLE_FLAGS['FT_STYLE_FLAG_BOLD']
            if ft_italic:
                facestyle = FaceStyle.boldItalic if ft_bold else FaceStyle.italic
            else:
                facestyle = FaceStyle.bold if ft_bold else FaceStyle.normal
            style_name = face.style_name.decode()
            faceinfo = FaceInfo(hex(face.style_flags), style_name, facestyle, filename)
            font = self.setdefault(name, {})
            font[style_name] = faceinfo


system_fonts = SystemFonts()


@dataclass
class Font(Resource):
    family: str = ''
    size: int = 12
    mono: bool = False
    normal: str = ''
    bold: str = ''
    italic: str = ''
    boldItalic: str = ''

    def draw_tk_image(self, width: int, height: int, scale: int, fontstyle, fontscale: int, halign: Align, valign: Align, back_color, color, text):
        fontstyle = {FontStyle[s] for s in fontstyle}
        back_color = Color(back_color).value_str(ColorFormat.pillow)
        color = Color(color).value_str(ColorFormat.pillow)
        w_box, h_box = width // fontscale, height // fontscale
        img = PIL.Image.new('RGBA', (w_box, h_box))
        draw = PIL.ImageDraw.Draw(img)
        draw.fontmode = '1' if self.mono else 'L'
        try:
            sysfont = system_fonts[self.family]
            face_name = ''
            if FontStyle.Italic in fontstyle:
                if FontStyle.Bold in fontstyle:
                    face_name = self.boldItalic
                face_name = face_name or self.italic
            elif FontStyle.Bold in fontstyle:
                face_name = self.bold
            face_name = face_name or self.normal
            font = PIL.ImageFont.truetype(sysfont[face_name].filename, self.size)
        except:
            font = PIL.ImageFont.load_default(self.size)
        draw.font = font
        ascent, descent = font.getmetrics()
        line_height = ascent + descent
        # Break text up into words for wrapping
        paragraphs = [textwrap.wrap(para, 1, break_long_words = False,
                replace_whitespace = False, drop_whitespace=False)
                for para in text.splitlines()]
        # Measure and build list of lines and their length in pixels
        lines = []
        for para in paragraphs:
            text = ''
            x = 0
            space = (0, '')
            for word in para:
                w = draw.textlength(word)
                if word.isspace():
                    space = (w, word)
                else:
                    if x > 0 and (x + space[0] + w) > w_box:
                        lines.append((x, text))
                        text = ''
                        x = 0
                    else:
                        x += space[0]
                        text += space[1]
                    space = (0, '')
                    x += w
                    text += word
            if x > 0 or not para:
                lines.append((x, text))
        h = line_height * len(lines)
        if valign == Align.Middle:
            y = (h_box - h) // 2
        elif valign == Align.Bottom:
            y = h_box - h
        else:
            y = 0
        for w, text in lines:
            if w > 0:
                if halign == Align.Centre:
                    x = (w_box - w) // 2
                elif halign == Align.Right:
                    x = w_box - w
                else:
                    x = 0
                draw.text((x, y), text, fill=color)
                # draw.rectangle((x,y,x+w,y+line_height), outline='white')
                def hline(y):
                    draw.line((x, y, x + w, y), fill=color)
                if FontStyle.Underscore in fontstyle or FontStyle.DoubleUnderscore in fontstyle:
                    hline(y + ascent)
                if FontStyle.DoubleUnderscore in fontstyle:
                    hline(y + ascent + 3)
                if FontStyle.Overscore in fontstyle or FontStyle.DoubleOverscore in fontstyle:
                    hline(y + 3)
                if FontStyle.DoubleOverscore in fontstyle:
                    hline(y)
                yc = y + line_height // 2
                if FontStyle.Strikeout in fontstyle:
                    hline(yc)
                if FontStyle.DoubleStrikeout in fontstyle:
                    hline(yc - 1)
                    hline(yc + 1)
            y += ascent + descent

        img = img.resize((round(width * scale), round(height * scale)),
            resample=PIL.Image.Resampling.NEAREST)
        return TkImage(img)


class FontList(ResourceList):
    def __init__(self):
        self.default = Font(family=next(iter(system_fonts)))

    @staticmethod
    def families():
        return sorted(system_fonts)

    def load(self, res_dict):
        super().load(Font, res_dict)


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
            img = img.resize((round(img.width * scale), round(img.height * scale)),
                resample=PIL.Image.Resampling.NEAREST)
        else:
            w, h = round(crop_rect.w * scale), round(crop_rect.h * scale)
            img = PIL.Image.new('RGB', (w, h), color='red')
            draw = PIL.ImageDraw.Draw(img)
            draw.line((0, 0, w, h), fill=128, width=3)
            draw.line((0, h, w, 0), fill=128, width=3)
        return TkImage(img)


class ImageList(ResourceList):
    def load(self, res_dict):
        super().load(Image, res_dict)
