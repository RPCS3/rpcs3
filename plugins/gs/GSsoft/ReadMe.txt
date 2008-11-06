GSsoft v1.0
-----------

 This is an extension to use with play station2 emulators 
 such as PCSX2.
 The plugin is free open source code.

Using:
-----
 Place the file "GSsoft.dll" (Windows) or "GSsoft.so" (Linux) 
 at the Plugins directory of the Emulator to use it.

Configuration:
-------------
 Display Resolution:
  Select the Resolution mode you want to use.

 Fullscreen:
  Sets fullscreen mode (else windowed).

 Display FPS Count:
  This will show on the start the frame per second count, 
  also you can change settings from here.

 Frameskip:
  You can use this to improve a little speed (will skip some
  frames).

Quick Keys:
----------  
 Delete - Show/Hide FPS Count.
 End    - Change selected setting in FPS Count.
 PageUp/
 PageDn - Move up/down in the FPS Count.

Changes:
-------
 v1.0
 * Fixed GIFprocessReg with RGBAQ/STQ handling

 v0.9
 * Lots of fixes, implemented most psm's, thx to gabest
   for helping me with some stuff
 * Rewrote triangle algorithm, based on the 3DFC algo,
   http://www.geocities.com/SiliconValley/Bay/1704
 * Both CRTs now are blended when both enabled

 v0.8 
 * Up to 0.5.5 specs
 * Several fixes
 * Up to SDL 1.2.7
 * added sdl_gfxprimitives 1.5 inside source. No need to load external dll
 * savings are now done in ini.
 * cleanup some stuff

 v0.6
 * Added savestates support
 * Partially fixed the local memory structure, big thanks to asadr/tyranid 
 * Fixed some bugs 
 * This version is a bit untested, so you may   expect some crashes ;P
 v0.5:
 * Added Color.c/Texts.c
 * Most _Z primitives are implemented
 * Fixed lots of bugs
 * Implemented Alpha Blending
 * Implemented colorclamping
 * New PS2Edefs v0.4.0

 v0.41:
 * Corrected VSync interlace bit
 * Added FINISH handling
 * Added From VRam transfers

 v0.4:
 * Added Linux-SDL dir, thanks to Muad
 * Added bugfix for ps2mame (thanks to asad)
 * STQ stuff starts to work (thanks to Absolute0)
 * Added GSgifTransfer2 support
 * Also some internal changes

 v0.31:
 * Fixed interlace modes

 v0.3:
 * Added Draw.c/h for blits.
 * Added ShowFullVRam option on FPS Count.
 * Added some zBuffer support.
 * Last vertex rgba is now ok.
 * Added more stuff on GSwrite/read32/64

 v0.2:
 * 16bit mode should be fine always now
 * Cursor will be disabled in windowed mode too
 * Improved lots of stuff, psms now works (8bit textures)
 * Added non-stretched blits
 * Linux support under XWindows (GSsoftx)

 v0.1:
 * First Release
 * Tested with Pcsx2

Authors:
-------

 linuzappz <linuzappz@hotmail.com>
 [TyRaNiD]
 shadow   <shadowpcsx2@yahoo.gr>


