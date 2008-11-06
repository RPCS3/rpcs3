# Microsoft Developer Studio Project File - Name="GSsoftdx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=GSsoftdx - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "GSsoftdx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "GSsoftdx.mak" CFG="GSsoftdx - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "GSsoftdx - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSSOFTDX_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../" /I "./" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GSSOFTDX_EXPORTS" /D "__WIN32__" /D "__MSCW32__" /D "__i386__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x2c0a /d "NDEBUG"
# ADD RSC /l 0x2c0a /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ddraw.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib sdl.lib sdl_gfx.lib /nologo /dll /pdb:none /machine:I386
# Begin Target

# Name "GSsoftdx - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\Cache.c
# End Source File
# Begin Source File

SOURCE=..\Color.c
# End Source File
# Begin Source File

SOURCE=.\Conf.c
# End Source File
# Begin Source File

SOURCE=..\Draw.c
# End Source File
# Begin Source File

SOURCE=..\GS.c
# End Source File
# Begin Source File

SOURCE=.\GSsoftdx.def
# End Source File
# Begin Source File

SOURCE=..\Mem.c
# End Source File
# Begin Source File

SOURCE=..\Page.c
# End Source File
# Begin Source File

SOURCE=..\Prim.c
# End Source File
# Begin Source File

SOURCE=..\Rec.c
# End Source File
# Begin Source File

SOURCE=..\Regs.c
# End Source File
# Begin Source File

SOURCE=..\scale2x.c
# End Source File
# Begin Source File

SOURCE=..\SDL.c
# End Source File
# Begin Source File

SOURCE=..\SDL_gfxPrimitives.c
# End Source File
# Begin Source File

SOURCE=..\Soft.c
# End Source File
# Begin Source File

SOURCE=..\Texts.c
# End Source File
# Begin Source File

SOURCE=..\Transfer.c
# End Source File
# Begin Source File

SOURCE=.\Win32.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\Draw.h
# End Source File
# Begin Source File

SOURCE=..\GS.h
# End Source File
# Begin Source File

SOURCE=..\Mem.h
# End Source File
# Begin Source File

SOURCE=.\plugin.h
# End Source File
# Begin Source File

SOURCE=..\PS2Edefs.h
# End Source File
# Begin Source File

SOURCE=..\PS2Etypes.h
# End Source File
# Begin Source File

SOURCE=..\Rec.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=..\Soft.h
# End Source File
# Begin Source File

SOURCE=..\Texts.h
# End Source File
# Begin Source File

SOURCE=.\Win32.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\GSsoftdx.rc
# End Source File
# Begin Source File

SOURCE=.\Pcsx2.ico
# End Source File
# End Group
# Begin Group "Docs"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\ReadMe.txt
# End Source File
# End Group
# End Target
# End Project
