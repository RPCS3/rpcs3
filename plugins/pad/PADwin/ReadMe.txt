PADwin v1.0
--------------------

 This is an extension to use with play station2 emulators 
 such as PCSX2.
 The plugin is free open source code.

Usage:
-----
 Place the file "PADwin.dll" (Windows) or "libPADwin.so" (Linux)
 at the Plugins directory of the Emulator to use it.
 Make sure you calibrated the joystick/gamepad with Control Panel's utility!!!

Thanks
------
 My cousin Bogdan for giving me the gamepad;) (Florin)
 Dr. Hell for his WinMM PAD Driver & joyinfo tool:)
 Bositman for helping me fixing bugz;)
 Raziel for PS2 controller info

Changes:
-------
 v1.0
 * Commented out the joystic code that was causing troubles in win32 
 * Added Devc++ (4.9.9.2) project files to compile with mingw32 easy :)
 * Moved some of the PADopen code to PADinit

 v0.9
 * Added vsnet2005 beta1 files. 64bit dll should now work (not tested!)
 * Fixed bug in PADclose

 v0.8
 * Configure back to INI
 * Some fixes 

 v0.7
 * Rewritten by asadr. Analog mode with mouse 

 v0.6
 * Analog mode.
 * better compatibility

 v0.5:
 * Merged with PADxwin
 * Bugfixed PAD2
 * Rewrote PADpoll/PADstartPoll, now it has more pad commands

 v0.43:
 * Added POV support
 * I considered 0 for axis value as disabled. That was wrong. While it worked
   with analog sticks, it didn't with "more precise" digital ones:P
 * dwButtonNumber is not usable; with my gamepad it gives me the number of
   the last button pressed. this is correct according to docs. I don't have
   a driver for it so it is handeled by windows default driver. But some dudes,
   at Gravis for example, did not understood the meaning of dwButtonNumber;
   their driver returns the number of the buttons that are pressed in the same
   time, hence the J0_1 assign bug.
 * corrected back/fwrd for Z.

 v0.42:
 * added joystick/gamepad support
   2 joys; 32 buttons/joy; 6 axis; no POV

 v0.41:
 * add a pic in configuration window 

 v0.4:
 * fixed key repeating
 * updated to v0.4.0 specs

 v0.31:
 * fixed silly bug after exit

 v0.3:
 * Changed old PADreadStatus for PADstartPoll/PADpoll

 v0.2:
 * Config and About dialogs added
 * Now pads works

 v0.1:
 * First Release
 * Tested with Pcsx2

Authors:
-------

 linuzappz <linuzappz@hotmail.com>
 florin <florin@pcsx2.net>
