Basic Graphics
==============

.. highlight:: bash

Demonstration of how to use the :library:`Graphics` library with the Host Virtual Screen
or displays such as the `Adafruit 2.8” Touch Shield V2 (SPI) <https://www.adafruit.com/product/1651>`__.

To try this out from the Host Emulator you can use the virtual screen::

    make virtual-screen

The IP address:port will be shown at the command prompt and in the title bar, for example 192.168.1.105:7780.
Build the application::

    make SMING_ARCH=Host VSADDR=192.168.1.105 VSPORT=7780

Note: The screen server address and port are passed by command line so the application doesn't require rebuilding.
For example::

    out/Host/debug/firmware/app vsaddr=192.168.1.105 vsport=7780
