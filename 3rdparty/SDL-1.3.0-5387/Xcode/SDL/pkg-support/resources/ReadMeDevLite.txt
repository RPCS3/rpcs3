This directory is for developers. This directory contains some basic essentials you will need for developing SDL based applications on OS X. The SDL-devel package contains all of this stuff plus more, so you can ignore this if you install the SDL-devel.pkg. The SDL-devel package contains Project Builder/Xcode templates, SDL documentation, and different variations of SDLmain and NIB files for SDL.

To compile an SDL based application on OS X, SDLMain.m must be compiled into your program. (See the SDL FAQ). The SDL-devel.pkg includes Project Builder/Xcode templates which already do this for you. But for those who may not want to install the dev package, an SDLMain is provided here as a convenience. Be aware that there are different variations of SDLMain.m depending on what class of SDL application you make and they are intended to work with NIB files. Only one SDLMain variant is provided here and without any NIB files. You should look to the SDL-devel package for the others. We currently do not provide a SDLMain.a file, partly to call to attention that there are different variations of SDLmain.

To build from the command line, your gcc line will look something like this:

gcc -I/Library/Frameworks/SDL.framework/Headers MyProgram.c SDLmain.m -framework SDL -framework Cocoa

An SDL/OpenGL based application might look like:

gcc -I/Library/Frameworks/SDL.framework/Headers -I/System/Library/Frameworks/OpenGL.framework/Headers MyProgram.c SDLmain.m -framework SDL -framework Cocoa -framework OpenGL

