CDVDbin v0.67
---------------

 This is an extension to use with play station2 emulators 
 as PCSX2 (only one right now).
 The plugin is free open source code.

Using:
-----
 Place the file "CDVDbin.dll" in the Plugins directory 
 of the Emulator to use it.
 If you have a cdaudio image, go to config dialog of the plugin and check
 "Force CD Audio on detection failure". CDDA support is limmited in this
 version.

Note:
-----
-I'm using a method for detection of images that can lead to misdetections.
 So, if the plugin shows a dialog box that says that the image is an ISO
 and you KNOW it is a BIN (2352 bytes/sector) or BIN that is in fact an ISO,
 please notice me.
-The detection might work on some other image formats. You can check the
 "Debug messages" box in config dialog and then open the image using
 "All files" filter. If the image works and it is not a *known* type,
 please notice me.
-Officialy known types: (bytes/sec)
	bin(+cue),	//2352 + toc info file
	iso,		//2048 (it is not recommended to use such)
	img(+ccd+sub),	//2352 + toc info file + subchannel file
	bwi,		//2353 (i don't know much about this type)
	mdf(+mds),	//2352/2448 + toc info file (2448-2352=96 => full subchannel)
	nrg		//2048/2352/2448 as mdf type, the subq info is in the file,
					 at the end of the image there are toc infos
-A splitted image is a sequence of files that have a common name and a trailing
 number. The number starts from 0 or 1. To choose such an image in Open dialog,
 select "All Files" filter and pick one of the splits.

Thanks:
------
 Xobro, this is a port of his FPSE cd plugin
 Linuzappz

Changes:
-------
 v0.67:
 * Up to date to the latest CDVD specs v3 (0.4.3)
 * Bugfix for small files

 v0.66:
 * Added "All Files" filter
 * Added some CDDA support :P
 * Support for splitted image files
 * Support for *.mds

 v0.65:
 * Up to date to the latest CDVD specs v2 (0.4.0)
 * Added support for RAW+SUBQ images (2352+96=2448 bytes/sector)
 * Easier detection, ready for subq;)

 v0.64:
 * Up to date to the latest CDVD specs
 * Better detection of images (ISO2048/RAW2352)
   This allows one to use an incomplete/bad image
 * Added *.nrg support (Nero images)

 v0.63:
 * Added *.bwi to open dialog (Blind Write images)
 
 v0.62:
 * Small fix to file pointer in order to handle
   big files (64bit pointer). (hi bositman;)
 
 v0.61:
 * Added *.img to open dialog (CloneCD images)
 
 v0.6:
 * Changed for PS2E Definitions v0.2.9 (beta) specs; CDVDopen()
 * Fixed OFN_NOCHANGEDIR issue; thx linuzappz;)

 v0.5:
 * Added a config dialog

 v0.4:

 v0.3:
 * Automatic iso/bin detection

 v0.2:

 v0.1:
 * First Release/Port
 * Tested with Pcsx2

 Email: <florinsasu@yahoo.com>
