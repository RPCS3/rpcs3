CDVDiso v0.6
------------

   This is an extension to use with play station2 emulators 
 as PCSX2 (only one right now).
 The plugin is free open source code.

Linux requirements:
------------------
   You need the GTK library, compiled with GTK v2.6.1 (at least).

Usage:
-----
   For those using Windows, place the file "CDVDisoEFP.dll" in the Plugin
 directory of the Emulator to use it.

   For Linux users, place the file "libCDVDisoEFP.so" in the plugin
 directory of the Emulator. Also, place the file "cfgCDVDisoEFP" in the "cfg"
 directory of the Emulator to use it.

Configuration:
-------------
   You must select the iso you want to use in the Configuration dialog. You
 will come back to this dialog should the Emulator want another disc.

Creating an iso (linux, Windows XP only):
---------------
   To create an iso you can use the button 'Make Image'. The file
 will be the one in the Iso Edit, and the Source Device is the CDRom or
 DVD drive that will have the disc you want to make an image of.
 The compression method is specified by the Compression Method Combo.

 Note: This will fail if the file already exists (if it's compressed
   a .z or .bz2 suffix will be added).

Compressed isos:
---------------
   You can create them using the Convert button. This will create a .Z or .bz
 file (the compressed image) and a .z.table or .bz2.table file (this is a
 table, for speed reasons.) Both will be created in the same dir as the
 selected image. The original image will not be deleted and/or changed. Also,
 you can decompress a compressed image by selecting "None" in the 
 Compression Method Combo.

 Note: you only can decompress the images with this program; not with
 compress or bzip2.

Compression Method:
------------------
 .z speed - fast compression, small blocks, table in memory.
 .bz2 speed - average compression, 32k - 40k blocks, table in memory.
 .bz2 size - best compression, 60k - 62k blocks, table on file.


 Email: <linuzappz@hotmail.com>
