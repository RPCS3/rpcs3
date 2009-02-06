SSSPSX Pad - An Open Source Pad plugin for PSX and PS2 emulators
Author: Nagisa
Homepage: http://www.asahi-net.or.jp/~bz7t-skmt/

Overview:
-Small executable program
-Source code under 1000 step,20kb binary
-Open Source,under the GPL Licence


Features:

For PS1 emulators
- Force feedback support (PCSX only)
  Delete the following PCSX sorce code.
  File: sio.c Line: 138
　---------------------
　if (buf[parp] == 0x41) {
　　switch (value) {
　　　case 0x43:
　　　　buf[1] = 0x43;
　　　　break;
　　　case 0x45:
　　　　buf[1] = 0xf3;
　　　　break;
　　}
　}
　---------------------

For PCSX2
-Force feedback support (maybe)
-PADKeyEvent API support
-Using DirectInput 9 (game controller and keyboard)

Thanks to:
http://www.hm5.aitai.ne.jp/~takuya/index.html#ds2_analisys for the valuable info
PCSX2 team for the PadWinKeyb source code
bositman for some report

Version History:
v1.0: -Initial Release
v1.1: -Changed to DirectInput 9
v1.2: -PADKeyEvent API support added
v1.3: -DirectInput collision problem fixed
v1.4: -Added timeout on settings dialog.If the countdown ends, the key will be set to "NONE".
      -Changed "ESC" key action on settings dialog.If you press the "ESC" key, the setting will keep the previous one.
      -Fixed silly bug. (dont ask me about it).
v1.5: -Fixed 0x4D packet.
