Sming Graphics Library
======================

.. highlight:: bash

An asynchronous graphics management library intended for SPI-based displays.
See the :sample:`Basic_Graphics` sample application for demonstration of how to use it.

For a discussion about multitasking and how Sming works, see :doc:`/information/multitasking`.

Getting started
---------------

On first use several required python modules must be installed.
Try building a sample application::

    cd $SMING_HOME
    make fetch Graphics
    cd Libraries/Graphics/samples/Basic_Graphics
    make python-requirements
    make


Virtual Screen
--------------

This is a display device for use by the Host Emulator. The 'display' is a python application which communicates
with the Sming :cpp:class:`Graphics::Display::Virtual` display driver using a TCP socket.
Graphics processing is handled using SDL2.

To start the virtual screen server type ``make virtual-screen`` from your project directory.
The default TCP port is 7780. If you need to change this, use::

    make virtual-screen VSPORT=7780

The screen server's IP address is shown in the caption bar.
Build and run your project in host mode as follows::

    make SMING_ARCH=Host
    make run VSADDR=192.1.2.3


Some definitions
----------------

:cpp:class:`Graphics::Object`
    Defines a specific graphical item, such as a line, circle or image.

:cpp:class:`Graphics::SceneObject`
    List of objects describing the components of a displayed image (line, rectangle, circle, image, text, etc.)
    Also contains a list of assets which allows objects to be re-used within a scene.

    Applications use methods such as `drawCircle` to construct a scene.

:cpp:class:`Graphics::TextBuilder`
    Helper object to simplify text layout, handling wrapping, alignment etc.

:cpp:class:`Graphics::Surface`
    Rectangular array of pixels representing a display or offscreen bitmap

:cpp:class:`Graphics::Device`
    A display device. This provides a single ``Surface`` for access to the screen.

:cpp:class:`Graphics::Renderer`
    Class which performs background rendering tasks such as drawing a sequence of lines, filling a circle, etc.
    Each renderer performs a single, specific-task. The renderers are created by the Surface which allows
    display devices to provide customised implementations.

:cpp:class:`Graphics::PixelFormat`
    Describes the format of pixel data required by a Surface.

    For example, the ILI9341 display uses RGB565 colours (2 bytes) with the MSB first.
    It can use other pixel formats (RGB 6:6:6), but this is how the driver sets the display up during initialisation.

    Applications do not need to be aware of this, however, and simply express colours in a 32-bit
    :cpp:class:`Graphics::Color` type. This includes an alpha channel, where 255 is fully opaque
    and 0 is fully transparent.
    Conversion is handled in a single step (via :cpp:func:`Graphics::pack`) so further manipulation
    (even byte-swapping) is not required by the display driver.

    Note that the ILI9341 display has another quirk in that display data is always read back in 24-bit format
    (18 significant bits). This detail is managed by the display driver which does the conversion during read
    operations. See :cpp:func:`Graphics::Surface::readDataBuffer`.

:cpp:class:`Graphics::Blend`
    This is a virtual base class used to implement custom colour blending operations.
    Currently only supported for memory images using :cpp:class:`Graphics::ImageSurface`.


Resources
---------

Projects describe which fonts and image resources they require in a JSON resource script.

This is compiled to produce two files:
    $(OUT_BASE)/resource.h
        A header file describing the resources and containing indices and other lookup tables.
        Include this ONCE from a source file.
    $(OUT_BASE)/resource.bin
        Contains image data and font glyph bitmaps.
        This file can be linked into the application or stored in a file.
        A better approach is to store it in a dedicated partition as this avoids any
        filing system overhead.
        The library accesses this as a stream - applications must call :cpp:func:`Graphics::Resource::init`.

The goal is to enable an optimal set of resources to be produced for the target display from
original high-quality assets.


Usage
~~~~~

    1.  Create a directory to contain project resources, e.g. ``resource``.
    2.  Place any custom images, fonts, etc. into this directory
    3.  Create a resource script file, e.g. ``resource/graphics.rc``.
        See below for details on editing this file.
    3.  Add an entry to the project ``component.mk`` file::
            RESOURCE_SCRIPT := resource/graphics.rc

The general structure of a resource script is:

.. code-block:: json

    {
        "resources": {
            "<type>": {
                "<name>": {
                    ...
                }
            }
        }
    }


Fonts
~~~~~

A :cpp:class:`Graphics::Font` contains up to four typefaces which correspond
to the selected style (normal, italic, bold or bold+italic).

The default font is :cpp:class:`Graphics::LcdFont`, a simple Arduino fixed Adafruit GFX font.
Only the 'normal' typeface is defined.
This is linked into the program image.

All other fonts are loaded from resources. These are parsed from their original format and
converted to an efficient internal format based on the Arduino GFX font library.

Resource script entries look like this:

.. code-block: json

    "font": {
        "<name>": {
            "codepoints": "<filter>", // Which character glyphs to include. See below.
            "chars": "<text string>", // List of required character codepoints
            "normal": "<filename>",
            "italic": "<filename>",
            "bold": "<filename>",
            "boldItalic": "<filename>"
        }
    }

Styles are optional but a font must have at least one typeface.

By default, all ASCII characters from 0x20 (space) to 0x7e (~).
The ``codepoints`` parameter is a comma-separated list of ranges:

    a-z,A-Z,0-9,0x4000-0x4050

This overrides the default and includes only characters and digits, plus unicode characters in the given range.
The ``chars`` parameter is a simple list of characters, e.g. "Include these chars".
Both lists are combined, de-duplicated and sorted in ascending order.

The following font classes are currently supported:

    GFX
        Adafruit GFX library font files.
        These are identified using a "gfx/" prefix for the font files.
        See ``resource/fonts/GFX``.
    Linux
        Linux console bitmap fonts. These are in .c format, identified by "linux/" path prefix.
        See ``resource/fonts/Linux``.
    PFI
        The ".pfi" file extension is a Portable Font Index.
        Font glyph bitmap information is contained in an associated .pbm file.
        See ``resource/fonts/PFI``.
    VLW
        Smoothed fonts produced by the https://processing.org/ library with a ".vlw" file extension.
        These are from the `TFT_eSPI <https://github.com/Bodmer/TFT_eSPI>`__ library.
        See ``resource/fonts/VLW``.

        Note that TTF/OTF scalable vector fonts are supported directly by this library
        so is the preferred format for new fonts.
    freetype
        Font type is identified from the file extension:
            .ttf
                TrueType
            .otf
                OpenType
            .pcf, .pcf.gz
                X11 bitmap font
        The ``freetype`` library supports other types so if required these are easily added.

        These fonts have some additional parameters:
            "mono": <True/False>
                Whether to produce monochrome (1-bit) or grayscale (8-bit alpha) glyphs.
                If not specified, defaults to grayscale.
            "size": <Point size of font>
                e.g. 16, 14.5


Images
~~~~~~

Resource script entries look like this:

.. code-block:: json

    "image": {
        "<name>": {
            "format": "<target format>", // RGB565, RGB24 or BMP
            "source": "filename" | "url",
            // Optional list of transformations to apply
            "transform": {
                "width": target width,      // Height will be auto-scaled
                "height": target height,    // Width will be auto-scaled
                "crop": "width,height",     // Crop image to given size from (0, 0)
                "crop": "x,y,width,height", // Crop to selected area of image
                "resize": "width,height",   // Resize image
                "flip": "left_right" | "top_bottom", // Flip the image horizontally or vertically
                "rotate": angle in degrees  // Rotate the image
            }
        }
    }

The Python ``pillow`` library is used to read and process images.

The ``format`` parameter is intended to produce a raw, un-compressed image compatible with the
target display device.
Alternatively, specify "BMP" to output a standared .bmp file or omit to store the original
image contents un-processed.


Scene construction
------------------

Instead of writing directly to the display, applications create a :cpp:class:`Graphics::SceneObject`
which will contain a description of the scene to be drawn.

The scene can then be constructed using methods such as ``drawLine``, ``drawRect``, etc.
These create an object describing the operation and adds it to the scene.

When the scene is fully described, the application calls the display device's ``render`` method
and the rendering begins in the background.
A callback function may be provided which will be invoked when the scene has been drawn.
The application is free to start creating further scenes, etc.

Note: Scenes are never modified during rendering so can be drawn multiple times if required.


Rendering
---------

This is the process of converting the scene objects into pixels which are then drawn into a :cpp:class:`Graphics::Surface`.

Computation and primitive rendering is done in small, manageable chunks via the task queue.
Data is sent to the screen using the appropriate display driver.
The ILI9341 driver uses interrupts and hardware (SPI) transfers to do its work.

If the primitive object is simple (e.g. block fill, horizontal/vertical line) then it can be written
to the display buffer immediately.

More complex shapes (images, text, diagonal lines, rectangles, triangles, circles, etc.) create a
specific :cpp:class:`Graphics::Renderer` instance to do the work.

Transparency (alpha-blending) involves a read-modify-write cycle.
The ILI9341 driver can also handle small transparent rectangles (including indivudual pixels)
to assist with line drawing.
Larger areas are handled by the appropriate renderer.

Shapes such as rectangles, triangles, circles, etc. are described as a set of connected ``lines`` and ``points``
which is then iterated through and rendered using ``fill`` operations.

The standard :cpp:class:`Graphics::Surface` implementation uses a standard set of renderers to do this work,
however these can be overridden by display devices to make use of available hardware features.


Transparency
------------

With sufficient RAM this is a trivial exercise as we just render everything to a memory surface then copy
it to the display.
For updating small areas of the screen this may be the best approach.
However, even a small 240x320 pixel display would require 150kBytes with RGB565.

The more general approach adopted by this library is to read a small block of pixels from the display,
combine them with new data as required then write the block back to the display.
The :cpp:class:`Graphics::TextRenderer` does this by default. The algorithm also ensures that text can be rendered
correctly with filled backgrounds (solid, transparent or custom brushes).

.. note::

   It may not be possible to read from some displays.
   For example, a small 1-inch display is commonly available using the :cpp:class:`Graphics::Display::ST7789V`
   controller connected in 4-wire mode, but with the SDO line unconnected internally.

   In such cases transparency must be handled by the application using memory surfaces.


Display driver
--------------

The ILI9341 display driver buffers requests into a set of hardware-specific commands with associated data.
When the request buffer is full, it is sent it to the hardware using the :library:`HardwareSPI` library.
This particular display is typically configured in 4-wire SPI mode which requires an additional GPIO
to select between command and data transfers. The driver manages this within its interrupt service routines.

Two request buffers are used so that when one buffer has completed transfer the next can begin immediately.
This switch is handled within the interrupt service routine, which also schedules a task callback to the renderer
so it may re-fill the first request buffer.


Configuration variables
-----------------------

.. envvar:: VSADDR

    e.g. 192.168.1.105:7780

    TCP address of the virtual screen server application, as shown in the title bar.


.. envvar:: VSPORT

    default: 7780

    Port number to use for virtual screen.


.. envvar:: ENABLE_GRAPHICS_DEBUG

    default 0 (off)

    Set to '1' to enable additional debug output from renderers


.. envvar:: ENABLE_GRAPHICS_RAM_TRACKING

    default 0 (off)

    Set to '1' to enable additional diagnistics to assist with tracking RAM usage


Further work
------------

The list of possible updates to the library is endless. Some thoughts:

Text wrapping
    Currently wraps at common punctuation characters.
    Add options to specify how to break text (character boundaries, word boundaries, break characters, etc.)

Rich Text
    Add support for HTML, markdown and other suitable formats.
    This could be handled by the resource compiler by converting to `Graphics::Drawing` resources.

Transparent bitmaps
    Simple transparency can be handled using :cpp:class:`Blend`
    Images with per-pixel alpha are not currently implemented.
    The resource compiler would be extended to support creation of images in ARGB format, with appropriate renderer updates.

Text glyph alignment
    Text is drawn using the upper-left corner as reference.
    It may be desirable to change this, using the baseline or bottom corner instead.
    This can be added as an option.

Metafiles
    Rather than describing scenes in code, we should be able to draw them using other tools and export
    the layouts as metafiles. Similarly, we might like to export our scenes as metafiles for later re-use.

    An example use case is for a 'scribble' application, recording pen strokes a touch-enabled screen.

    - `Windows Metafile Format <https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-wmf/4813e7fd-52d0-4f42-965f-228c8b7488d2>`__
    - `WebCGM <https://www.w3.org/TR/2010/REC-webcgm21-20100301>`__

    The ``Drawing`` API is intended for this use, providing a simple Graphics Description Language (GDL).
    The provided ``GDRAW_xxx`` macros allow embedding of these scripts within programs.
    This feature is restricted so appropriate only for very simple constructions.

    Drawings could be described in textual resource files and compiled during build.

    A simple utility program can be written to convert between text and binary GDL formats.
    Other conversion programs then only need to generate the text equivalent.
    This will allow the binary format to change as required.

    It would be highly desirable to give subroutines textual identifiers.
    These would be resolved into numeric identifiers for the binary format.

Interactive objects
    Drawing buttons, for example, might be done by defining the basic button in GDL.

    The button routine could use the following::

            pen #0: Inactive surround
            pen #1: Active surround
            brush #0: Inactive fill
            brush #1: Active fill

    These slots can be set depending on the desired state by program code.
    It would be more efficient if GDL can do this based on other parameters,
    or perhaps by registering callbacks or control objects to make these decisions.

    For example, a ``ButtonObject`` could use GDL to draw itself and provide callbacks to handle
    state control, etc. This would also be required for control purposes.
    
    To support use of touch screens we need the ability to query scenes and identify objects at a specific
    position. Those objects may then be adjusted and redrawn to reflect 'push' operations, etc.

    For example, we might create a button control by defining a scene describing how the button looks, then
    create an InteractiveObject containing a reference to that scene plus some kind of user identifier
    and location of the control 'hot spot'.

Brushes, Gradient fills, etc.
    Rectangular gradient fills can be done by creating a custom ImageObject and drawing it.
    The object just generates the pixel data as requested according to configured parameters.

    This is the basis for Brush objects which would be used to render circles, rounded rectangles and glyphs.
    That would allow for all sorts of useful effects, including clipping images to other shapes.

    Tiling an image can be done by filling a rectangle with an ImageBrush, which deals with co-ordinate
    wrapping and returns the corresponding region from a source ImageObject.

Cursors, sprites and priority drawing
    Response to user input should be prioritised. For example, if a cursor or mouse pointer is present
    on the screen then waiting for a complex screen update is going to affect useability.

    Cursors are drawn over everything else. Although some hardware may support cursors directly,
    for now we'll assume not.
    Similarly text carets (e.g. '_', '>' prompts) can be drawn with priority so that changes in their
    position are updated ahead of other items.

    A read-xor-write pattern is simple enough to implement within a Command List,
    so that the XOR operation is done in interrupt context. This should provide sufficient priority.

    Sprites can be managed using a Surface filter layer which tracks Address Window operations.
    If data is written to an area occupied by a sprite, then that data can be modified (using XOR)
    before passing through to provide flicker-free drawing.
    
    Repeat fills make this a little tricky though.
    An alternative is to identify when a sprite will be affected and, if so, remove it before
    drawing the new information then re-draw it afterwards.
    This could be implemented using Regions so that only the affect portion is updated.

    Each sprite must store both its own contents (foreground), plus the original display contents (background)
    *excluding* any other sprites. Multiple overlapping are combined at point of rendering.


References
----------

File formats
    -   `BMP File Format <https://en.wikipedia.org/wiki/BMP_file_format>`__ (Wikipedia)

Fonts
    -   `Bitmap fonts <https://github.com/Tecate/bitmap-fonts>`__ Collection of monospaced bitmap fonts for X11
    -   `BDF Parser Python library <https://github.com/tomchen/bdfparser>`__
    -   `Arduino font converter <https://github.com/chall3ng3r/Squix-Display-FontConverterV3>`__
    -   `Processing <https://processing.org/>`__
    -   `OpenType Specification <https://docs.microsoft.com/en-us/typography/opentype/spec/>`__
    -   `FreeType library <https://www.freetype.org/index.html>`__
    -   `Adafruit GFX font converter <https://github.com/adafruit/Adafruit-GFX-Library/tree/master/fontconvert>`__
    -   `Adafruit GFX Pixel font customiser <https://tchapi.github.io/Adafruit-GFX-Font-Customiser/>`__

Graphics libraries
    -   `Simple and Fast Multimedia Library <https://github.com/SFML/SFML>`__
    -   `SDL Wiki <https://wiki.libsdl.org/>`__
    -   `ADA Industrial Control Widget Library <http://www.dmitry-kazakov.de/ada/aicwl.htm>`__
    -   `Cairo 2D graphics library <https://www.cairographics.org/>`__
    -   `GDI+ <https://docs.microsoft.com/en-us/windows/win32/gdiplus/>`__
    -   `Clipper <http://angusj.com/delphi/clipper.php>`__ for clipping and offsetting lines and polygons
    -   `FabGL <https://github.com/fdivitto/FabGL>`__ Display Controller, PS/2 Mouse and Keyboard Controller,
        Graphics Library, Sound Engine, Graphical User Interface (GUI), Game Engine and ANSI/VT Terminal for the ESP32
    -   `TFT_eSPI <https://github.com/Bodmer/TFT_eSPI>`__
    -   `LVGL <https://docs.lvgl.io/master/index.html>`__ Light and Versatile Graphics Library

Metafiles
    -   `WebCGM 2.1 <https://www.w3.org/TR/2010/REC-webcgm21-20100301/>`__ Computer Graphics Metafile standard
    -   `Windows Metafile <https://en.wikipedia.org/wiki/Windows_Metafile>`__


Papers
    -   `The Beauty of Bresenham's Algorithm <https://zingl.github.io/bresenham.html>`__ Discuss anti-aliasing techniques



API
---

.. doxygennamespace:: Graphics
   :members:
