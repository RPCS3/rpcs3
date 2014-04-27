CDVDlinuz v0.4
--------------

 This is an extension to use with play station2 emulators 
 as PCSX2 (only one right now).
 The plugin is free open source code.

  Modified by efp to work with DVDs as well

To install in Windows:
-----
 Place the file "CDVDlinuz.dll" in the "plugins" directory.

To install in Linux:
-----
 Place the file "libCDVDlinuz.so" in the "plugins/" directory.
 Place the file "cfgCDVDlinuz" in the "cfg/" directory.

To activate in PCSX2:
-----
 Start up PCSX2.
 Select "Configuration", then "Plugins and Bios".
 In the "Cdvdrom" pull-down menu, you should see:
   "EFP polling CDVD Driver v.04"
 Select it.
 Then press the "Configure" button under it.
 In the "CDVDlinuz Configuration" screen, type in where your CD or DVD
device can be found.
 (In Linux, devices look like "/dev/<device>")
 (In Windows, devices look like "D:". There are other types of references to
devices, but they haven't been tested.)
 Press to "OK" button to save your selection.
 Finally, put a CD or DVD in the selected drive, and run PCSX2.

 Keep in mind the above instructions only cover the "CD/DVD" portion of
PCSX2. All configuration options must be attended to before PCSX2 could run
correctly.
