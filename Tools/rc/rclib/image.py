#
# Image resource parser
#

import PIL.Image
import PIL.ImageOps
import os
import io
import struct
import requests
from .base import Resource, findFile, fstrSize, StructSize, PixelFormat

class Image(Resource):
    def __init__(self):
        super().__init__()
        self.bitmap = None
        self.width = None
        self.height = None
        self.format = None
        self.pixel_format = None
        self.headerSize = 0

    def serialize(self, bmOffset, res_offset, ptr64: bool):
        """struct ImageResource"""
        pixel_format = PixelFormat[self.pixel_format.upper()].value
        print(f'image {self.name} pixel_format {self.pixel_format} {fmt}')
        return struct.pack('<QIIHHI' if ptr64 else '<IIIHHI',
            0, # FSTR::String* name
            bmOffset,
            len(self.bitmap),
            self.width,
            self.height,
            pixel_format)

    def get_bitmap_size(self):
        return len(self.bitmap)

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        super().writeComment(out)
        out.write("DEFINE_FSTR_LOCAL(%s_name, \"%s\")\n" % (self.name, self.name))
        self.headerSize += fstrSize(self.name)
        out.write("const ImageResource %s PROGMEM = {\n" % self.name)
        out.write("\t.name = &%s_name,\n" % self.name)
        out.write("\t.bmOffset = 0x%08x,\n" % bmOffset)
        out.write("\t.bmSize = 0x%06x,\n" % len(self.bitmap))
        out.write("\t.width = %u,\n" % self.width)
        out.write("\t.height = %u,\n" % self.height)
        out.write("\t.format = ImageFormat::%s,\n" % self.format)
        out.write("\t.pixelFormat = PixelFormat::%s,\n" % self.pixel_format)
        out.write("};\n\n")
        self.headerSize += StructSize.Image
        return bmOffset + self.get_bitmap_size()

    def writeBitmap(self, out):
        out.write(self.bitmap)


def convert(image, source, format):
    def convert_raw(bytesPerPixel: int, callback):
        image.format = 'RAW'
        image.pixel_format = format
        data = bytearray(image.width * image.height * bytesPerPixel)
        i = 0
        for p in source.getdata():
            data[i:i+bytesPerPixel] = bytearray(callback(p))
            i += bytesPerPixel
        image.bitmap = data
        return True


    if format == 'RGB24':
        def rgb24(src):
            return src[0], src[1], src[2]
        return convert_raw(3, rgb24)

    if format == 'RGB565':
        def rgb565(src):
            r, g, b = src[0] >> 3, src[1] >> 2, src[2] >> 3
            color = (r << 11) | (g << 5) | b
            return (color >> 8, color & 0xff)
        return convert_raw(2, rgb565)

    if format == 'ARGB1555':
        def argb1555(src):
            r, g, b, a = src[0] >> 3, src[1] >> 3, src[2] >> 3, src[3] >> 7
            color = (a << 15) | (r << 10) | (g << 5) | b
            return (color >> 8, color & 0xff)
        return convert_raw(2, argb1555)

    if format == 'ARGB2':
        def argb2(src):
            r, g, b, a = src[0] >> 6, src[1] >> 6, src[2] >> 6, src[3] >> 6
            color = (a << 6) | (r << 4) | (g << 2) | b
            return (color,)
        return convert_raw(1, argb2)

    if format == 'ARGB4':
        def argb4(src):
            r, g, b, a = src[0] >> 4, src[1] >> 4, src[2] >> 4, src[3] >> 4
            color = (a << 12) | (r << 8) | (g << 4) | b
            return (color >> 8, color & 0xff)

    if format in ['BMP', 'JPEG', 'PNG']:
        image.format = format
        image.pixel_format = 'None'
        bytes = io.BytesIO()
        source.save(bytes, format)
        image.bitmap = bytes.getbuffer()
        return True

    return False


# Crop image to "x, y, w, h"
def crop_image(img, args):
    args = args.split(',')
    if len(args) == 2:
        # Crop evenly around centre of image using (x, y) as reference
        (x, y) = (int(num, 0) for num in args)
        (w, h) = (img.width - x*2, img.height - y*2)
    else:
        (x, y, w, h) = (int(num, 0) for num in args)
    # status('Crop image to (%u, %u, %u, %u)' % (x, y, w, h))
    return img.crop((x, y, x + w, y + h))

# Resize image to "w, h"
def resize_image(img, args):
    (w, h) = (int(num, 0) for num in args.split(','))
    # status('Resize image to (%u, %u)' % (w, h))
    return img.resize((w, h))

# Resize image to given width, maintaining aspect ratio
def set_image_width(img, args):
    w = args
    h = round(w * img.height / img.width)
    # status("Resize image to (%u, %u)" % (w, h))
    return img.resize((w, h))

# Resize image to given height, maintaining aspect ratio
def set_image_height(img, args):
    h = args
    w = round(h * img.width / img.height)
    # status("Resize image to (%u, %u)" % (w, h))
    return img.resize((w, h))

# Flip an image either "left-right" or "top-bottom"
def flip_image(img, args):
    # status('Flip image %s' % args)
    if args == 'left-right':
        return img.transpose(PIL.Image.FLIP_LEFT_RIGHT)
    if args == 'top-bottom':
        return img.transpose(PIL.Image.FLIP_TOP_BOTTOM)
    raise InputError("Unknown argument to flip '%s'" % args)

# Rotate an image, angle given in degrees
def rotate_image(img, args):
    angle = args
    # status('Rotate image %u degrees' % angle)
    return img.rotate(angle)


def colorise_image(img, args):
    # status(f"args: {args}")
    args = list(args.items())
    if len(args) == 3:
        black = args[0][0]
        blackpoint = args[0][1]
        mid = args[1][0]
        midpoint = args[1][1]
        white = args[2][0]
        whitepoint = args[2][1]
    else:
        black = args[0][0]
        blackpoint = args[0][1]
        white = args[1][0]
        whitepoint = args[1][1]
        mid = None
        midpoint = (whitepoint + blackpoint) / 2

    gimg = PIL.ImageOps.grayscale(img)
    return PIL.ImageOps.colorize(gimg, black, white, mid, blackpoint, whitepoint, midpoint)

transforms = {
    'crop': crop_image,
    'resize': resize_image,
    'width': set_image_width,
    'height': set_image_height,
    'flip': flip_image,
    'rotate': rotate_image,
    'color': colorise_image,
}

def parse_item(item, name):
    """Parse an image
    """
    resname = item['source']
    if resname.startswith("http://") or resname.startswith("https://"):
        headers = {'user-agent': 'resource-compiler/1.0'}
        r = requests.get(resname, headers=headers)
        imgdata = r.content
        img = PIL.Image.open(io.BytesIO(r.content))
        imgsize = len(r.content)
    else:
        filename = findFile(resname)
        img = PIL.Image.open(filename)
        imgsize = os.path.getsize(filename)
        imgdata = None

    # status("Source image %s: '%s': %s %s, %u bytes" % (name, resname, img.format, img.size, imgsize))

    image = Image()
    image.name = name
    image.format = img.format

    transform = item.get('transform')
    if transform is not None:
        for op, value in transform.items():
            img = transforms[op](img, value)

    (image.width, image.height) = img.size
    if not convert(image, img, item.get('format')):
        if imgdata:
            image.bitmap = imgdata
        else:
            with open(filename, 'rb') as f:
                image.bitmap = f.read()

    # status("Image %s: %s %s, %u bytes" % (name, image.format, img.size, len(image.bitmap)))

    return image
