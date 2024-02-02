Graphical Editor
================

This is a simple GUI layout tool to assist with placement and setting attributes, etc. for objects.

Initial design goals:

- Platform portability
    - Uses python 'pygame' library which is essentially SDL2 with good graphic primitive support
- Simplicity
    - quick and easy to use
    - requires minimal display parameters (size, pixel format)
- Accurate
    - Pixel layout corresponds to actual hardware, not just some idealised representation
- Fast and responsive
    - use hardware acceleration for drawing (SDL2)
    - no compilation required
- Easy to modify/extend, e.g.
    - adding additional export formats

TODO:
    - Need format for project file (JSON probably). Should this be the .rc file?
    - code generation
    - 

Preferably generate data blocks which can be imported into applications.
Some will be static, e.g. screen backgrounds, 
May require code generation but if so keep to an absolute minimum.
Data blocks can incorporate logic (Graphics::Drawing).

Features to add:

- Font/text support
- Grid layout
    - simplify placement
    - alignment tools
- multiple selection
    - select multiple elements
- cut & paste
- undo / redo
- Bitmap support
    - import, place, size, adjust bitmaps
    - specify format
- Grouping / overlays / scenes
    - e.g. common page background ('master page')
    - concept of 'scene library' perhaps
- Resource script integration
    - export/import to/from .rc files
    - select fonts
- Live display
    - update live display (or virtual display) in real time, probably via serial interface
    - perhaps a simple wrapper protocol so user can provide pipe, file, whatever for this

