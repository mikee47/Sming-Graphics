Color Test
==========

Simple test that should draw from left to right rectangles with red, green, blue and white color. 

Useful for display driver diagnostics.
The TFT_eSPI project describes quite nicely the different switches that can affect the colors in MIPI displays::

	Different hardware manufacturers use different colour order
	configurations at the hardware level.  This may result in
	incorrect colours being displayed.
	Incorrectly displayed colours could also be the result of
	using the wrong display driver...
	Typically displays have a control register (MADCTL) that can
	be used to set the Red Green Blue (RGB) colour order to RGB
	or BRG so that red and blue are swapped on the display.
	This control register is also used to manage the display
	rotation and coordinate mirroring. The control register
	typically has 8 bits, for the ILI9341 these are:
	Bit Function
	7   Mirror Y coordinate (row address order)
	6   Mirror X coordinate (column address order)
	5   Row/column exchange (for rotation)
	4   Refresh direction (top to bottom or bottom to top in portrait orientation)
	3   RGB order (swaps red and blue)
	2   Refresh direction (top to bottom or bottom to top in landscape orientation)
	1   Not used
	0   Not used
	The control register bits can be written with this example command sequence:
	
	   tft.writecommand(TFT_MADCTL);
	   tft.writedata(0x48);          // Bits 6 and 3 set
	
	0x48 is the default value for ILI9341 (0xA8 for ESP32 M5STACK)
	in rotation 0 orientation.
	
	Another control register can be used to "invert" colours,
	this swaps black and white as well as other colours (e.g.
	green to magenta, red to cyan, blue to yellow).

Source: https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Test%20and%20diagnostics/Colour_Test/Colour_Test.ino
