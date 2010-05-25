
-/- MemoryCardFile SVN Status -/-

Currently the MemoryCardFile "plugin" is integrated into the main UI of PCSX2.  It provides
standard raw/block emulation of a memorycard device.  The plugin-ification of the MemoryCard
system has not yet been completed, which is why the plugin as of right now is "integrated"
into the UI.  As the new PS2E.v2 plugin API is completed, the MemoryCardFile provider will
be moved into an external DLL.

Due to these future plans, and due to the number of files involved in implementing the
user interface for memorycard management, I have arranged the MemoryCardFile plugin files
in their own sub-folder inside the Visual Studio project.