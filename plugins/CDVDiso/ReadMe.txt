CDVDiso v0.5
------------

 This is an extension to use with play station2 emulators 
 as PCSX2 (only one right now).
 The plugin is free open source code.

Linux requeriments:
------------------
 You need the GTK library, compiled with GTK v1.2.5, 
 the Zlib library (v1.1.3) and the bz2 library (v1.0.0).

Usage:
-----
 Place the file "libCDVDiso.so" (linux) or "CDVDiso.dll" (win32) at the Plugin 
 directory of the Emulator to use it.

 Linux only: Also place the cfgCDVDiso file at your $HOME directory or at the 
 Emulator directory.

Configuration:
-------------
 You can either write the iso you want to use in the config dialog, or everytime
 you run the emu open it, if you're doing this don't write anything in the dialog.

Creating an iso (linux only):
---------------
 To create an iso you can use the buttons 'Create Iso' or 'Create Compressed Iso',
 the file will be the one in the Iso Edit, and the Cdrom Device is the cdrom that
 will be used as source.
 The compression method is specified by the Compression Method Combo.

 Note: This will fail if the file in the Iso Edit already exists (if it's
 compressed the .Z or .bz suffix will be added).

Compressed isos:
---------------
 You must create them by the Compress Iso button, this will create a .Z or .bz
 file (the compressed image) and a .Z.table or .bz.index file (this is a table,
 for speed reasons), both will be created in the same dir the selected iso 
 image, the original image will not be deleted and/or changed, and also you can
 decompress the compressed iso with Decompress Iso button.
 The compression method is specified by the Compression Method Combo.

 Note: you only can decompress the images with the Decompress button, not with
 compress or bzip2.

Compression Method:
------------------
 .Z  - compress faster: this will compress faster, but not as good as the .bz.
 .bz - compress better: will compress better, but slower.

Changes:
-------
 v0.5:
 * Added block dumping code
 * Added BZ2/Z2 format ;)
 * Added optimaze asm code of zlib with vsnet2003 only. Compression / Uncompression should be faster now (shadow)
 * Added Vsnet2005 beta1 project files + amd64 asm optimaze code for zlib (shadow)

 v0.4:
 * Rewrote mostly ;)
 * .bz is still unsupported

 v0.3:
 * Better Iso detection, thx florin :)
 * Updated to PS2Edefs v0.4.5

 v0.2:
 * Added support for isos using 2048 blocksize
 * Nero images are supported
 * Better extension filtering

 v0.1:
 * First Release
 * Tested with Pcsx2

 Email: <linuzappz@hotmail.com>
