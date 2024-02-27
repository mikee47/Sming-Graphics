#!/usr/bin/env python3
#
# rc.py - Graphics Resource Compiler
#
# Copyright 2021 mikee47 <mike@sillyhouse.net>
#
# This file is part of the Sming-Graphics Library
#
# This library is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, version 3 or later.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this library.
# If not, see <https://www.gnu.org/licenses/>.
#
# @author: July 2021 - mikee47 <mike@sillyhouse.net>
#

import os
import sys
import argparse
import rclib
import rclib.font
import rclib.image
import common
from common import json_load, status

def main():
    parser = argparse.ArgumentParser(description='Sming Resource Compiler')

    parser.add_argument('--quiet', '-q', help="Don't print non-critical status messages to stderr", action='store_true')
    parser.add_argument('input', help='Name of resource script')
    parser.add_argument('output', help='Path to output directory')

    args = parser.parse_args()
    common.quiet = args.quiet
    rclib.base.resourcePaths.append(os.path.dirname(os.path.abspath(args.input)))

    script = json_load(args.input)
    data = rclib.parse(script['resources'])

    with openOutput(os.path.join(args.output, 'resource.h'), 'w') as out:
        rclib.writeHeader(data, out)
    with openOutput(os.path.join(args.output, 'resource.bin'), 'wb') as out:
        rclib.writeBitmap(data, out)
        bitmapSize = out.tell()

    structSize = sum(item.headerSize for item in data)
    status("Resource compiled %u items, structures are %u bytes, bitmap is %u bytes"
        % (len(data), structSize, bitmapSize))


def openOutput(path, mode):
    status("Writing to '%s'" % path)
    output_dir = os.path.abspath(os.path.dirname(path))
    os.makedirs(output_dir, exist_ok=True)
    return open(path, mode)


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        raise
        print("** ERROR! %s" % e, file=sys.stderr)
        sys.exit(2)
