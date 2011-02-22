The Simple DirectMedia Layer (SDL for short) is a cross-platform 
library designed to make it easy to write multi-media software, 
such as games and emulators.

The Simple DirectMedia Layer library source code is available from: 
http://www.libsdl.org/

This library is distributed under the terms of the GNU LGPL license: 
http://www.gnu.org/copyleft/lesser.html


This packages contains the SDL.framework for OS X. 
Conforming with Apple guidelines, this framework 
contains both the SDL runtime component and development header files.


To Install:
Copy the SDL.framework to /Library/Frameworks

You may alternatively install it in <Your home directory>/Library/Frameworks 
if your access privileges are not high enough. 
(Be aware that the Xcode templates we provide in the SDL Developer Extras 
package may require some adjustment for your system if you do this.)


Known Issues:
???


Additional References:

 - Screencast tutorials for getting started with OpenSceneGraph/Mac OS X are 
 	available at:
	http://www.openscenegraph.org/projects/osg/wiki/Support/Tutorials/MacOSXTips
	Though these are OpenSceneGraph centric, the same exact concepts apply to 
	SDL, thus the videos are recommended for everybody getting started with
	developing on Mac OS X. (You can skim over the PlugIns stuff since SDL
	doesn't have any PlugIns to worry about.)



(Partial) History of PB/Xcode projects:
2009-09-21 - Added 64-bit for Snow Leopard. 10.4 is the new minimum requirement.
	Removed 'no X11' targets as 
	new codebase will assume you have them. Also removed specific #defines
	for X11, but needed to add search path to /usr/X11R6/include
	
2007-12-31 - Enabled strip -x in the Xcode settings and removed it 
	from the Build DMG script.
	Added a per-arch setting for the Deployment targets for OTHER_LDFLAGS_ppc
	to re-enable prebinding.
	Need to remember to copy these changes to the SDL satellite projects.

2007-12-30 - Updated documentation to reflect new installation paths for
	Xcode project templates under Leopard (Xcode 2.5/3.0).

????-??-?? - Added extra targets for building formal releases against the
	10.2 SDK so we don't have to keep modifying the settings.

????-??-?? - Added fancy DMG (background logo) support with automation.

2006-05-09 - Added shell script phase to deal with new SDL_config.h 
	behavior. Encountered what seems to be an Xcode bug with 
	multiple files of the same name, even when conditional compiling
	is controlled by custom #defines (SDL_sysloadso.c). Multiple or
	undefined symbols are the result of this.
	Recommended that macosx/SDL_sysloadso.c be modified to directly 
	include the dlopen version of the file via #ifdef's so only
	one version needs to exist. Filed a formal bug report with Apple 
	about this (4542369).

2006-03-22 - gcc 4 visibility features have been added to the code base so I 
	enabled the switch in Xcode to take advantage of it. Be aware that only
	our x86 builds will be exposed to this feature as we still build ppc 
	with gcc 3.3.
	
	Christian Walther has sent me some great feedback on things that are 
	broken, so I have made some of these fixes. Among the issues are
	compatibility and current library versions are not set to 1 (breaks
	backwards compatibility), documentation errors, resource copying 
	location problems for the SDLTest apps, missing HAVE_OPENGL and
	OpenGL.framework linking in testgl.
	(Eric Wing)

2006-03-17 - Because the X11 headers are not installed by default with Xcode,
	we decided to offer two variants of the same targets (one with X11 stuff
	and one without). By default, since the X11 stuff does not necessarily 
	conflict with the native stuff, we build the libraries with the X11 stuff 
	so advanced developers can access it by default. However, in the case
	that a developer did not install X11 (or just doesn't want the extra bloat),
	the user may directly select those targets and build those instead.
	
	Once again, we are attempting to remove the exported symbols file. If 
	I recall correctly, the clashing symbol problems we got were related
	to the CD-ROM code which was formerly in C++. Now that the C++ code
	has been purged, we are speculating that we might be able to remove
	the exports file safely. The long term solution is to utilize gcc 4's
	visibility features.

	For the developer extras package, I changed the package format 
	from a .pkg based installer to a .dmg to avoid requiring 
	administrator/root to access contents, for better 
	transparency, and to allow users to more easily control which components 
	they actually want to install.
	I also made changes and updates to the PB/Xcode project templates (see Developer ReadMe).
	(Eric Wing)

2006-03-07 - The entire code base has been reorganized and platform specific 
	defines have been pushed into header files (SDL_config_*.h). This means 
	that defines that previously had to be defined in the Xcode projects can 
	be removed (which I have started doing). Furthermore, it appears that the 
	MMX/SSE code has been rewritten and refactored so it now compiles without 
	nasm and without making us do strange things to support OS X. However, this 
	Xcode project still employs architecture specific build options in order to 
	achieve the mandated 10.2 compatibility. As a result of the code base changes, 
	there are new public headers. But also as a result of these changes, there are 
	also new headers that qualify as "PrivateHeaders". Private Headers are headers 
	that must be exported because a public header includes them, but users shouldn't 
	directly invoke these. SDL_config_macosx.h and SDL_config_dreamcast.h are 
	examples of this. We have considered marking these headers as Private, but it 
	requires that the public headers invoke them via framework conventions, i.e.
	#include <FrameworkName/Header.h>
	e.g.
	#include <SDL/SDL_config_macosx.h>
	and not
	#include "SDL_config_macosx.h"
	However this imposes the restriction that non-framework distributions must
	 place their headers in a directory called SDL/ (and not SDL11/ like FreeBSD). 
	 Currently, I do not believe this would pose a problem for any of the current 
	 distributions (Fink, DarwinPorts). Or alternatively, users could be 
	 expected/forced to also include the header path: 
	 -I/Library/Frameworks/SDL.framework/PrivateHeaders, 
	 but most people would probably not read the documentation on this. 
	 But currently, we have decided to be conservative and have opted not to 
	 use the PrivateHeaders feature.
	(Eric Wing)

2006-01-31 - Updates to build Universal Binaries while retaining 10.2 compatibility. 
	We were unable to get MMX/SSE support enabled. It is believed that a rewrite of 
	the assembly code will be necessary to make it position independent and not 
	require nasm. Altivec has finally been enabled for PPC. (Eric Wing)

2005-09-?? - Had to add back the exports file because it was causing build problems
	for some cases. (Eric Wing)

2005-08-21 - First entry in history. Updated for SDL 1.2.9 and Xcode 2.1. Getting 
	ready for Universal Binaries. Removed the .pkg system for .dmg for due to problems 
	with broken packages in the past several SDL point releases. Removed usage of SDL 
	exports file because it has become another point of failure. Introduced new documentation 
	about SDLMain and how to compile in an devel-lite section of the SDL.dmg. (Eric Wing)

Before history:
SDL 1.2.6? to 1.2.8
Started updating Project Builder projects to Xcode for Panther and Tiger. Also removed 
the system that split the single framework into separate runtime and headers frameworks. 
This is against Apple conventions and causes problems on multiuser systems. 
We now distribute a single framework.
The .pkg system has repeatedly been broken with every new release of OS X. 
With 1.2.8, started migrating stuff to .dmg based system to simplify distribution process. 
Tried updating the exports file and Perl script generation system for changing syntax. (Eric Wing)

Pre-SDL 1.2.6 
Created Project Builder projects for SDL and .pkg based distribution system. (Darrell Walisser)








