# Microsoft Developer Studio Project File - Name="libpng" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libpng - Win32 DLL Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libpng.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libpng.mak" CFG="libpng - Win32 DLL Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libpng - Win32 DLL Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libpng - Win32 DLL Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libpng - Win32 DLL ASM Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libpng - Win32 DLL ASM Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libpng - Win32 DLL VB" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libpng - Win32 LIB Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libpng - Win32 LIB Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libpng - Win32 LIB ASM Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libpng - Win32 LIB ASM Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "libpng - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libpng___Win32_DLL_Release"
# PROP BASE Intermediate_Dir "libpng___Win32_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_DLL_Release"
# PROP Intermediate_Dir "Win32_DLL_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /O2 /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_BUILD_DLL" /D "ZLIB_DLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:I386
# ADD LINK32 zlib1.lib /nologo /dll /machine:I386 /out:"Win32_DLL_Release\libpng13.dll" /libpath:"..\..\..\zlib\projects\visualc6\Win32_DLL_Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libpng___Win32_DLL_Release"
# PROP BASE Intermediate_Dir "libpng___Win32_DLL_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_DLL_Debug"
# PROP Intermediate_Dir "Win32_DLL_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D PNG_DEBUG=1 /D "PNG_BUILD_DLL" /D "ZLIB_DLL" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "_DEBUG" /d PNG_DEBUG=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlib1d.lib /nologo /dll /debug /machine:I386 /out:"Win32_DLL_Debug\libpng13d.dll" /libpath:"..\..\..\zlib\projects\visualc6\Win32_DLL_Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libpng___Win32_DLL_ASM_Release"
# PROP BASE Intermediate_Dir "libpng___Win32_DLL_ASM_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_DLL_ASM_Release"
# PROP Intermediate_Dir "Win32_DLL_ASM_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /O2 /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_USE_PNGVCRD" /D "PNG_BUILD_DLL" /D "ZLIB_DLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "NDEBUG" /d "PNG_USE_PNGVCRD"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:I386
# ADD LINK32 zlib1.lib /nologo /dll /machine:I386 /out:"Win32_DLL_ASM_Release\libpng13.dll" /libpath:"..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libpng___Win32_DLL_ASM_Debug"
# PROP BASE Intermediate_Dir "libpng___Win32_DLL_ASM_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_DLL_ASM_Debug"
# PROP Intermediate_Dir "Win32_DLL_ASM_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D PNG_DEBUG=1 /D "PNG_USE_PNGVCRD" /D "PNG_BUILD_DLL" /D "ZLIB_DLL" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "_DEBUG" /d PNG_DEBUG=1 /d "PNG_USE_PNGVCRD"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 zlib1d.lib /nologo /dll /debug /machine:I386 /out:"Win32_DLL_ASM_Debug\libpng13d.dll" /libpath:"..\..\..\zlib\projects\visualc6\Win32_DLL_ASM_Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL VB"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libpng___Win32_DLL_VB"
# PROP BASE Intermediate_Dir "libpng___Win32_DLL_VB"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_DLL_VB"
# PROP Intermediate_Dir "Win32_DLL_VB"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /O2 /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_BUILD_DLL" /D "ZLIB_DLL" /D PNGAPI=__stdcall /D "PNG_NO_MODULEDEF" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "NDEBUG" /dDLLFNAME_POSTFIX=""""VB"""" /dSPECIALBUILD=""""__stdcall calling convention used for exported functions""""
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /dll /machine:I386
# ADD LINK32 zlib1.lib /nologo /dll /machine:I386 /out:"Win32_DLL_VB\libpng13vb.dll" /libpath:"..\..\..\zlib\projects\visualc6\Win32_DLL_Release"
# Begin Special Build Tool
OutDir=.\Win32_DLL_VB
TargetName=libpng13vb
SOURCE="$(InputPath)"
PostBuild_Cmds=echo    Deleting $(targetname) import library and export file (Not required for VB projects)	del $(outdir)\$(targetname).lib	del $(outdir)\$(targetname).exp
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libpng___Win32_LIB_Release"
# PROP BASE Intermediate_Dir "libpng___Win32_LIB_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_LIB_Release"
# PROP Intermediate_Dir "Win32_LIB_Release"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /O2 /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libpng___Win32_LIB_Debug"
# PROP BASE Intermediate_Dir "libpng___Win32_LIB_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_LIB_Debug"
# PROP Intermediate_Dir "Win32_LIB_Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D PNG_DEBUG=1 /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Win32_LIB_Debug\libpngd.lib"

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libpng___Win32_LIB_ASM_Release"
# PROP BASE Intermediate_Dir "libpng___Win32_LIB_ASM_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Win32_LIB_ASM_Release"
# PROP Intermediate_Dir "Win32_LIB_ASM_Release"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MD /W3 /O2 /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "NDEBUG" /D "PNG_USE_PNGVCRD" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\.." /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libpng___Win32_LIB_ASM_Debug"
# PROP BASE Intermediate_Dir "libpng___Win32_LIB_ASM_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Win32_LIB_ASM_Debug"
# PROP Intermediate_Dir "Win32_LIB_ASM_Debug"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\.." /I "..\..\..\zlib" /D "WIN32" /D "_DEBUG" /D "DEBUG" /D PNG_DEBUG=1 /D "PNG_USE_PNGVCRD" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Win32_LIB_ASM_Debug\libpngd.lib"

!ENDIF 

# Begin Target

# Name "libpng - Win32 DLL Release"
# Name "libpng - Win32 DLL Debug"
# Name "libpng - Win32 DLL ASM Release"
# Name "libpng - Win32 DLL ASM Debug"
# Name "libpng - Win32 DLL VB"
# Name "libpng - Win32 LIB Release"
# Name "libpng - Win32 LIB Debug"
# Name "libpng - Win32 LIB ASM Release"
# Name "libpng - Win32 LIB ASM Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\png.c
# End Source File
# Begin Source File

SOURCE=..\..\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\..\pngget.c
# End Source File
# Begin Source File

SOURCE=..\..\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\..\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\..\pngread.c
# End Source File
# Begin Source File

SOURCE=..\..\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\..\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\..\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\..\pngset.c
# End Source File
# Begin Source File

SOURCE=..\..\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\..\pngvcrd.c

!IF  "$(CFG)" == "libpng - Win32 DLL Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL VB"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\scripts\pngw32.def

!IF  "$(CFG)" == "libpng - Win32 DLL Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL VB"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\..\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\..\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\..\pngwutil.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\png.h
# End Source File
# Begin Source File

SOURCE=..\..\pngconf.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\scripts\pngw32.rc

!IF  "$(CFG)" == "libpng - Win32 DLL Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Release"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL ASM Debug"

!ELSEIF  "$(CFG)" == "libpng - Win32 DLL VB"

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "libpng - Win32 LIB ASM Debug"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\README.txt
# End Source File
# End Target
# End Project
