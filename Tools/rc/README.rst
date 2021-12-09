Resource Compiler
=================

Goals:

-   Project defines all required graphical resources in a script file
-   Fonts defined from external source and converted into internal format
-   Images processed and converted

The compiler produces two output files:

Index / lookup tables (resource.h)
    -   A header file defining font glyphs, image information, etc.
        Defines symbolic identifiers for each resource.
        Uses FlashString routines to take advantage of caching.

Bitmap data (resource.bin)
    Single BLOB containing font bitmaps, image content, text.
    Resource manager is initialised with a single stream object for this data,
    so it could be stored in a file.
    Ideally stored in a dedicated partition to avoid file management overheads.


Fonts
-----

Tool can load multiple font types then output in standard internal format.
This is based on the Arduino GFX format as it produces the most compact layout.
Supports alpha-based fonts with 8 bits per glyph pixel.
Code points expressed in unicode.

Arduino GFX
    Standard .h format so just parse it directly.

    Sample code for reading/converting here:
    https://github.com/chall3ng3r/Squix-Display-FontConverterV3

    Online tool here:
    http://oleddisplay.squix.ch/#/home

Seiko-Epson 'PFI' fonts
    Port code to python.

X11 bitmap fonts in adobe .BDF format
    https://github.com/Tecate/bitmap-fonts

TTF / ODF files
    See https://processing.org/ for code which generates .VLM files from these.
    The compiler will allow .ttf and .odf files (or any other supported format)
    to be converted into a file similar to .VLW, with improvements.
    Storing each glyph pixel as 8-bit alpha is good, but the index information can
    be improved, e.g. by calculating maxDescent.


images
------

Convert images into format suitable for target display.
e.g. RGB565 raw pixel format.

Bitmap information is stored in index with identifier, image size and format.

vector graphics
---------------

SVG, postscript, etc. must be converted into a suitable format for the display device:

1. internal Graphics::Drawing vector resource
2. raw bitmap (with or without alpha channel)

Which of these is more appropriate will depend on the application and the dimensions.

Drawings are fitted to a specific screen size.

Scalable graphics could be implemented by creating a high-resolution drawing
and then scaling it down.
This could be done by overriding the Drawing::Reader class to scale coordinates appropriately.
