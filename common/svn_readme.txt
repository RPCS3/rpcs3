
Folder: vsprops/

  Contains Win32-specific build scripts and .vsprops files for use by Win32 projects
  of our many (many!) plugins.

Folder: include/

  Houses the common include files for various shared portions of Pcsx2 code.  The most
  significant are the PS2Edefs / PS2Etypes files (plugin APIs).  Other code snippets\
  and generic usefuls may be added at a later date.

Folder: src/

  Source code for snippets and libs maintained by the Pcsx2 Team.  Code is compiled into
  libs, which are found in /deps.  (automatically included into your plugin linker
  search path if you use the common/vsprops/)