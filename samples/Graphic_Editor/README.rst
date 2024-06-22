Graphic Editor
==============

Run this application on target hardware or in the host emulator to allow GED (Graphical Editor) to control it.

It is intended to be a companion application which can be run on the target hardware
to evaluate **exactly** how the screen will appear in the final application.

See :doc:`../../Tools/ged/README` for further details.


Getting started
---------------

Run::

    make graphic-editor

If this doesn't work, see :library:`Graphics` for installation requirements.

To demonstrate using the host emulator:

1.  Build this application::

        make SMING_SOC=host

2.  Run the virtual screen::

        make virtual-screen

3.  Run this application using the virtual screen address (in the title bar)::

        make run VSADDR=192.168.1.40

4.  Run the graphical editor, if it isn't already::

        make graphic-editor

5.  Hit the ``load`` button and navigate to the ``samples/Graphic_Editor/projects`` directory.
    Load the ``images.ged`` sample project.

6.  Hit the ``Send resources`` button.
    You should see messages on the console indicating successful resource upload.

7.  Hit the ``Send layout`` button to update the virtual screen.

The ``Send resouces`` only needs to be done once in a session unless new images are added
or their sizes changed.
