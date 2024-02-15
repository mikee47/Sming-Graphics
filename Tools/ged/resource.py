import dataclasses
from dataclasses import dataclass
import PIL.Image, PIL.ImageTk, PIL.ImageOps, PIL.ImageDraw

@dataclass
class Resource:
    name: str = ''

    def __post_init__(self):
        pass

    def asdict(self):
        return dataclasses.asdict(self)


@dataclass
class Font(Resource):
    family: str = ''
    size: int = 12
    # style: list[str] For now, assume all styles are available


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

    def get_tk_image(self, crop_rect, scale: int = 1):
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
