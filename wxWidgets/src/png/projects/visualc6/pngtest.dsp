# Microsoft Developer Studio Project File - Name="pngtest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=pngtest - Win32 DLL Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pngtest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pngtest.mak" CFG="pngtest - Win32 DLL Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pngtest - Win32 DLL Release" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 DLL Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 DLL ASM Release" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 DLL ASM Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 LIB Release" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 LIB Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 LIB ASM Release" (based on "Win32 (x86) Console Application")
!MESSAGE "pngtest - Win32 LIB ASM Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pngtest - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "pngtest___Win32_DLL_Release"
# PROP BASE Intermediate_Dir "pngtest___Win32_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_DLL_Release"
# PROP Intermediate_Dir "Win32_DLL_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_DLL" /D "PNG_NO_STDIO" /D "PNG_NO_GLOBAL_ARRAYS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 Win32_DLL_Release\libpng13.lib ..\..\..\zlib\projects\visualc6\Win32_DLL_Release\zlib1.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
OutDir=.\Win32_DLL_Release
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=set path=$(outdir);..\..\..\zlib\projects\visualc6\Win32_DLL_Release;	$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "pngtest___Win32_DLL_Debug"
# PROP BASE Intermediate_Dir "pngtest___Win32_DLL_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_DLL_Debug"
# PROP Intermediate_Dir "Win32_DLL_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "PNG_DLL" /D "PNG_NO_STDIO" /D "PNG_NO_GLOBAL_ARRAYS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Win32_DLL_Debug\libpng13d.lib ..\..\..\zlib\projects\visualc6\Win32_DLL_Debug\zlib1d.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Win32_DLL_Debug
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=set path=$(outdir);..\..\..\zlib\projects\visualc6\Win32_DLL_Debug;	$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 DLL ASM Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "pngtest___Win32_DLL_ASM_Release"
# PROP BASE Intermediate_Dir "pngtest___Win32_DLL_ASM_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_DLL_ASM_Release"
# PROP Intermediate_Dir "Win32_DLL_ASM_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_DLL" /D "PNG_NO_STDIO" /D "PNG_NO_GLOBAL_ARRAYS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 Win32_DLL_ASM_Release\libpng13.lib ..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Release\zlib1.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
OutDir=.\Win32_DLL_ASM_Release
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=set path=$(outdir);..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Release;	$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 DLL ASM Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "pngtest___Win32_DLL_ASM_Debug"
# PROP BASE Intermediate_Dir "pngtest___Win32_DLL_ASM_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_DLL_ASM_Debug"
# PROP Intermediate_Dir "Win32_DLL_ASM_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "PNG_DLL" /D "PNG_NO_STDIO" /D "PNG_NO_GLOBAL_ARRAYS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Win32_DLL_ASM_Debug\libpng13d.lib ..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Debug\zlib1d.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Win32_DLL_ASM_Debug
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=set path=$(outdir);..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Debug;	$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 LIB Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "pngtest___Win32_LIB_Release"
# PROP BASE Intermediate_Dir "pngtest___Win32_LIB_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_LIB_Release"
# PROP Intermediate_Dir "Win32_LIB_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 Win32_LIB_Release\libpng.lib ..\..\..\zlib\projects\visualc6\Win32_LIB_Release\zlib.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
OutDir=.\Win32_LIB_Release
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 LIB Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "pngtest___Win32_LIB_Debug"
# PROP BASE Intermediate_Dir "pngtest___Win32_LIB_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_LIB_Debug"
# PROP Intermediate_Dir "Win32_LIB_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Win32_LIB_Debug\libpngd.lib ..\..\..\zlib\projects\visualc6\Win32_LIB_Debug\zlibd.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Win32_LIB_Debug
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 LIB ASM Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "pngtest___Win32_LIB_ASM_Release"
# PROP BASE Intermediate_Dir "pngtest___Win32_LIB_ASM_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_LIB_ASM_Release"
# PROP Intermediate_Dir "Win32_LIB_ASM_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /machine:I386
# ADD LINK32 Win32_LIB_ASM_Release\libpng.lib ..\..\..\zlib\projects\visualc6\Win32_LIB_ASM_Release\zlib.lib /nologo /subsystem:console /machine:I386
# Begin Special Build Tool
OutDir=.\Win32_LIB_ASM_Release
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pngtest - Win32 LIB ASM Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "pngtest___Win32_LIB_ASM_Debug"
# PROP BASE Intermediate_Dir "pngtest___Win32_LIB_ASM_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_LIB_ASM_Debug"
# PROP Intermediate_Dir "Win32_LIB_ASM_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Win32_LIB_ASM_Debug\libpngd.lib ..\..\..\zlib\projects\visualc6\Win32_LIB_ASM_Debug\zlibd.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
OutDir=.\Win32_LIB_ASM_Debug
SOURCE="$(InputPath)"
PostBuild_Desc=[Run Test]
PostBuild_Cmds=$(outdir)\pngtest.exe ..\..\pngtest.png
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "pngtest - Win32 DLL Release"
# Name "pngtest - Win32 DLL Debug"
# Name "pngtest - Win32 DLL ASM Release"
# Name "pngtest - Win32 DLL ASM Debug"
# Name "pngtest - Win32 LIB Release"
# Name "pngtest - Win32 LIB Debug"
# Name "pngtest - Win32 LIB ASM Release"
# Name "pngtest - Win32 LIB ASM Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\pngtest.c
# End Source File
# End Group
# End Target
# End Project
