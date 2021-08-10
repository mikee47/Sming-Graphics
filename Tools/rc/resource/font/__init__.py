#
# Font resource parsers
#

from . import gfx, linux, vlw, freetype, pfi
from .font import *

resource.parsers['font'] = parse_item
