#
# Image resource parser
#

import PIL.Image, PIL.ImageOps, resource, os, io, requests
from resource import status

class Image(resource.Resource):
    def __init__(self):
        super().__init__()
        self.bitmap = None
        self.width = None
        self.height = None
        self.format = None
        self.headerSize = 0

    def writeHeader(self, bmOffset, out):
        self.headerSize = 0
        super().writeComment(out)
        out.write("DEFINE_FSTR_LOCAL(%s_name, \"%s\")\n" % (self.name, self.name))
        self.headerSize += resource.fstrSize(self.name)
        out.write("const ImageResource %s PROGMEM = {\n" % self.name)
        out.write("\t.name = &%s_name,\n" % self.name)
        out.write("\t.bmOffset = 0x%08x,\n" % bmOffset)
        out.write("\t.bmSize = 0x%06x,\n" % len(self.bitmap))
        out.write("\t.width = %u,\n" % self.width)
        out.write("\t.height = %u,\n" % self.height)
        out.write("\t.format = PixelFormat::%s,\n" % self.format)
        out.write("};\n\n")
        self.headerSize += resource.StructSize.Image
        return bmOffset + len(self.bitmap)

    def writeBitmap(self, out):
        out.write(self.bitmap)


def convert_rgb24(image, source):
    image.format = 'RGB24'
    data = bytearray(image.width * image.height * 3)
    i = 0
    for p in source.getdata():
        data[i+0] = p[0]
        data[i+1] = p[1]
        data[i+2] = p[2]
        i += 3
    return data


def convert_rgb565(image, source):
    image.format = 'RGB565'
    data = bytearray(image.width * image.height * 2)
    i = 0
    for p in source.getdata():
        r, g, b = p[0] >> 3, p[1] >> 2, p[2] >> 3
        color = (r << 11) | (g << 5) | b
        data[i+0] = color >> 8
        data[i+1] = color & 0xff
        i += 2
    return data


def convert_bmp(image, source):
    image.format = 'None'
    bytes = io.BytesIO()
    source.convert('RGB').save(bytes, 'BMP')
    return bytes.getbuffer()


converters = {
    'RGB24': convert_rgb24,
    'RGB565': convert_rgb565,
    'BMP': convert_bmp,
}

# Crop image to "x, y, w, h"
def crop_image(img, args):
    args = args.split(',')
    if len(args) == 2:
        # Crop evenly around centre of image using (x, y) as reference
        (x, y) = (int(num, 0) for num in args)
        (w, h) = (img.width - x*2, img.height - y*2)
    else:
        (x, y, w, h) = (int(num, 0) for num in args)
    status('Crop image to (%u, %u, %u, %u)' % (x, y, w, h))
    return img.crop((x, y, x + w, y + h))

# Resize image to "w, h"
def resize_image(img, args):
    (w, h) = (int(num, 0) for num in args.split(','))
    status('Resize image to (%u, %u)' % (w, h))
    return img.resize((w, h))

# Resize image to given width, maintaining aspect ratio
def set_image_width(img, args):
    w = args
    h = round(w * img.height / img.width)
    status("Resize image to (%u, %u)" % (w, h))
    return img.resize((w, h))

# Resize image to given height, maintaining aspect ratio
def set_image_height(img, args):
    h = args
    w = round(h * img.width / img.height)
    status("Resize image to (%u, %u)" % (w, h))
    return img.resize((w, h))

# Flip an image either "left-right" or "top-bottom"
def flip_image(img, args):
    status('Flip image %s' % args)
    if args == 'left-right':
        return img.transpose(PIL.Image.FLIP_LEFT_RIGHT)
    if args == 'top-bottom':
        return img.transpose(PIL.Image.FLIP_TOP_BOTTOM)
    raise InputError("Unknown argument to flip '%s'" % args)

# Rotate an image, angle given in degrees
def rotate_image(img, args):
    angle = args
    status('Rotate image %u degrees' % angle)
    return img.rotate(angle)


def colorise_image(img, args):
    status(f"args: {args}")
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
        img = PIL.Image.open(io.BytesIO(r.content))
        imgsize = len(r.content)
    else:
        filename = resource.findFile(resname)
        img = PIL.Image.open(filename)
        imgsize = os.path.getsize(filename)

    status("Source image %s: '%s': %s %s, %u bytes"
        % (name, resname, img.format, img.size, imgsize))

    transform = item.get('transform')
    if transform is not None:
        for op, value in transform.items():
            img = transforms[op](img, value)

    image = Image()
    image.name = name
    (image.width, image.height) = img.size
    format = item.get('format')
    if format:
        convert = converters[format]
        image.bitmap = convert(image, img)
    else:
        image.format = 'None'
        with open(filename, 'rb') as f:
            image.bitmap = f.read()

    status("Image %s: %s %s, %u bytes"
        % (name, image.format, img.size, len(image.bitmap)))

    return image
