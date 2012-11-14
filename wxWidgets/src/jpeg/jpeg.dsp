# Microsoft Developer Studio Project File - Name="jpeg" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00 
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=jpeg - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "JpegVC.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "JpegVC.mak" CFG="jpeg - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "jpeg - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "jpeg - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "jpeg - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W1 /O1 /Ob2 /I "../include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\jpeg.lib"

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W1 /Zi /Od /I "../include" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D DEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\jpegd.lib"

!ENDIF 

# Begin Target

# Name "jpeg - Win32 Release"
# Name "jpeg - Win32 Debug"
# Begin Group "JPEG Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\jcapimin.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcapistd.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jccoefct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jccolor.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcdctmgr.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jchuff.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcinit.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcmainct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcmarker.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcmaster.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcomapi.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcparam.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcphuff.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcprepct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jcsample.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jctrans.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdapimin.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdapistd.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdatadst.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdatasrc.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdcoefct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdcolor.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jddctmgr.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdhuff.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdinput.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmainct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmarker.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmaster.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdmerge.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdphuff.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdpostct.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdsample.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jdtrans.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jerror.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctflt.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctfst.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jfdctint.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctflt.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctfst.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctint.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jidctred.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jmemmgr.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jmemnobs.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jquant1.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jquant2.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\jutils.c

!IF  "$(CFG)" == "jpeg - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "jpeg - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\jchuff.h
# End Source File
# Begin Source File

SOURCE=.\jconfig.h
# End Source File
# Begin Source File

SOURCE=.\jdct.h
# End Source File
# Begin Source File

SOURCE=.\jdhuff.h
# End Source File
# Begin Source File

SOURCE=.\jerror.h
# End Source File
# Begin Source File

SOURCE=.\jinclude.h
# End Source File
# Begin Source File

SOURCE=.\jmemsys.h
# End Source File
# Begin Source File

SOURCE=.\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=.\jpegint.h
# End Source File
# Begin Source File

SOURCE=.\jpeglib.h
# End Source File
# Begin Source File

SOURCE=.\jversion.h
# End Source File
# End Group
# End Target
# End Project
