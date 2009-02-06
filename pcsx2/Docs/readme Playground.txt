About PCSX2 Playground:

pcsx2-playground is originally based off the c/c++ official pcsx2 trunk code. It has since been modified heavily in an on-going attempt to clean up the code, fix bugs, and improve overall emulation ability.


Devs (alphabetically)	-	Comments/Specialty

arcum42			-	Linux compatibility and porting.
cottonvibes		-	FPU and VU recompilers, general coding.
drkIIRaziel		-	Memory management, emulation theory/principals, recompiler design.
gigaherz		-	General coding,  spu2ghz, cdvdGigaherz, works on stuff until he gets 'bored' :p
Jake.Stine		-	MTGS, counters, timing/syncing, general coding.
ramapcsx2		-	pcsx2 tweaker, always finds those magic values that make the game work. :p

Beta Testers (alphabetically):

Bositman		-	Setup our Playground forums and helps moderate them. Also beta tests for us and actively contributes in Playground discussion.
Crushtest		-	Helps with Playground beta testing and discussion.
Krakatos		-	Project Management and Playground forums moderation. Devotes several hours of his day to playground beta-testing and discussion.

Additional Credits:

Special thanks to the official pcsx2 team for making ps2 emulation a reality!
Also a big special-thanks to the pcsx2 team for hosting our Playground forums and builds!
More info about the official pcsx2 team can be found in "readme 0.9.4.txt"


Added Features and Improvements:

    * Improved Frameskip/VU-skip
    * Special Game Fixes Section
    * Advanced Options Section for custom tweaking VU/FPU behavior.
    * Rewritten Multithreaded GS (MTGS) mode, with 15% speedup and fixes many instabilities.
    * Improved VU/FPU Flags and Clamping support (helps fix odd behaviors and SPS in some games)
    * Improved EE/IOP synchronization (fixes many freeze-ups and vmhacks).
    * Improved CDVD support. 


Performance:

Emulation speeds when using Playground's default settings will be slower than the official 0.9.4 or 0.9.5 builds because pcsx2-playground is doing more work to provide your games with a more correct emulated environment. Properly emulating all of the VU clamping and flags is very time-consuming in particular. So various speedhacks were made so people can disable some of these slower emulation features for games that do not need them. When set-up properly, pcsx2-playground is usually faster than the official build.

The speed hacks offered are still a work-in-progress, and so for now there isn't much documentation on how to use them. Every game is different and so you'll just have to play around and see what works best. You can also try visiting the Pcsx2 forums, where members of the Playground team and other knowledgeable contributors should be able to help you get the most out of the emulator.


The Future of Pcsx2 Playground

There is still alot of room for improvement, but we're trying hard to fix stuff as best we can. Our todo/wish lists are extensive but we're optimistic that the emulator can continue to improve. :)

Visit our forums at Pcsx2.net, or come to the #pcsx2 IRC channel on efnet if you want to contribute!


System Requirements:

Minimum

    * Windows/Linux OS
    * CPU that supports SSE2 (Pentium 4 and up, Athlon64 and up)
    * GPU that supports Pixel Shaders 2.0
    * 512mb RAM 

Recommended

    * Windows Vista 32bit/64bit
    * CPU: Intel Core 2 Duo @ 3.2ghz or better
    * GPU: 8600gt or better
    * 1gb RAM (2gb if on Vista) 

Note: pcsx2 only takes advantage of 2 cores so far; so a quad-core processor will not help with speed.

