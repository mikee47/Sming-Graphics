import sys
import os
import dataclasses
from dataclasses import dataclass
import tkinter as tk
import tkinter.font
import freetype
import PIL.Image, PIL.ImageTk, PIL.ImageOps, PIL.ImageDraw, PIL.ImageFont
from gtypes import Rect, DataObject, Color, FaceStyle, FontStyle, Align
from enum import Enum
from typing import TypeAlias
import textwrap

TkImage: TypeAlias = PIL.ImageTk.PhotoImage

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
    path: str

class SystemFonts(dict):
    def __init__(self):
        self.scan()

    def scan(self):
        self.clear()
        dirs = []
        if sys.platform == "win32":
            windir = os.environ.get("WINDIR")
            if windir:
                dirs.append(os.path.join(windir, "fonts"))
        elif sys.platform in ("linux", "linux2"):
            lindirs = os.environ.get("XDG_DATA_DIRS", "")
            if not lindirs:
                lindirs = "/usr/share"
            dirs += [os.path.join(lindir, "fonts") for lindir in lindirs.split(":")]
        elif sys.platform == "darwin":
            dirs += [
                "/Library/Fonts",
                "/System/Library/Fonts",
                os.path.expanduser("~/Library/Fonts"),
            ]
        else:
            raise SystemError("Unsupported platform: " % sys.platform)

        fontfiles = []
        for directory in [os.path.expandvars(path) for path in dirs]:
            for walkroot, walkdir, walkfilenames in os.walk(directory):
                fontfiles += [os.path.join(walkroot, name) for name in walkfilenames]

        # freetype.FT_STYLE_FLAGS
        for filename in fontfiles:
            try:
                face = freetype.Face(filename)
            except:
                continue
            name = face.family_name.decode()
            ft_italic = face.style_flags & freetype.FT_STYLE_FLAGS['FT_STYLE_FLAG_ITALIC']
            ft_bold = face.style_flags & freetype.FT_STYLE_FLAGS['FT_STYLE_FLAG_BOLD']
            if ft_italic:
                style = FaceStyle.boldItalic if ft_bold else FaceStyle.italic
            else:
                style = FaceStyle.bold if ft_bold else FaceStyle.normal
            info = FaceInfo(hex(face.style_flags), style, filename)
            self.setdefault(name, []).append(info)
            family_name = face.family_name.decode()
            style_name = face.style_name.decode()
            # print(f'{family_name} / {style_name} / {face.style_flags:x} / {style}')
            # try:
            #     varinfo = face.get_variation_info()
            #     print('  ', varinfo.axes)
            #     print('  ', varinfo.instances)
            # except:
            #     pass

system_fonts = SystemFonts()


@dataclass
class Font(Resource):
    family: str = ''
    size: int = 12
    facestyle: list[str] = ()

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

    def draw_tk_image(self, width, height, scale, fontstyle, align, color, text):
        fontstyle = {FontStyle[s] for s in fontstyle}
        align = {Align[s] for s in align}
        color = Color(color).value_str()
        img = PIL.Image.new('RGBA', (width, height))
        draw = PIL.ImageDraw.Draw(img)
        faces = system_fonts.get(self.family)
        if faces:
            if FontStyle.Italic in fontstyle:
                face_style = FaceStyle.boldItalic if FontStyle.Bold in fontstyle else FaceStyle.italic
            else:
                face_style = FaceStyle.bold if FontStyle.Bold in fontstyle else FaceStyle.normal
            # print(face_style)
            # for f in faces:
            #     print('  ', f)
            try:
                face = next(f for f in faces if f.style == face_style)
                # print(f'Found {face}')
            except StopIteration:
                try:
                    face = next(f for f in faces if f.style == FaceStyle.normal)
                except StopIteration:
                    face = faces[0]
            font = PIL.ImageFont.truetype(face.path, self.size)
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
                        if x > 0 and (x + space[0] + w) > width:
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
            if Align.Middle in align:
                y = (height - h) // 2
            elif Align.Bottom in align:
                y = height - h
            else:
                y = 0
            for w, text in lines:
                if w > 0:
                    if Align.Centre in align:
                        x = (width - w) // 2
                    elif Align.Right in align:
                        x = width - w
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
        return PIL.ImageTk.PhotoImage(img)


class FontList(ResourceList):
    def __init__(self):
        tk_def = FontList.tk_default().configure()
        font = Font(family = tk_def['family'])
        self.default = font

    @staticmethod
    def tk_default():
        return tkinter.font.nametofont('TkDefaultFont')

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
        return PIL.ImageTk.PhotoImage(img)


class ImageList(ResourceList):
    def load(self, res_dict):
        super().load(Image, res_dict)
