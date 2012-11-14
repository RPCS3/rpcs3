# Microsoft Developer Studio Project File - Name="tiff" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00 
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=tiff - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "tiff.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "tiff.mak" CFG="tiff - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "tiff - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "tiff - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "tiff - Win32 Release"

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
# ADD CPP /nologo /MD /w /W0 /O1 /Ob2 /I "../include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\tiff.lib"

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

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
# ADD CPP /nologo /MDd /w /W0 /Zi /Od /I "../include" /I ".." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D DEBUG=1 /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /Yu"wx/wxprec.h" /FD /c
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\tiffd.lib"

!ENDIF 

# Begin Target

# Name "tiff - Win32 Release"
# Name "tiff - Win32 Debug"
# Begin Group "tiff Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\tif_aux.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_close.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_codec.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_color.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_compress.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_dir.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_dirinfo.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_dirread.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_dirwrite.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_dumpmode.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_error.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_extension.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_fax3.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_fax3sm.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_flush.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_getimage.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_jpeg.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_luv.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_lzw.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_next.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_open.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_packbits.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_pixarlog.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_predict.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_print.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_read.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_strip.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_swab.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_thunder.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_tile.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_version.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_warning.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_win32.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_write.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tif_zip.c

!IF  "$(CFG)" == "tiff - Win32 Release"

# ADD CPP /I ".."
# SUBTRACT CPP /YX /Yc /Yu

!ELSEIF  "$(CFG)" == "tiff - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\port.h
# End Source File
# Begin Source File

SOURCE=.\t4.h
# End Source File
# Begin Source File

SOURCE=.\tif_dir.h
# End Source File
# Begin Source File

SOURCE=.\tif_fax3.h
# End Source File
# Begin Source File

SOURCE=.\tif_predict.h
# End Source File
# Begin Source File

SOURCE=.\tiff.h
# End Source File
# Begin Source File

SOURCE=.\tiffcomp.h
# End Source File
# Begin Source File

SOURCE=.\tiffconf.h
# End Source File
# Begin Source File

SOURCE=.\tiffio.h
# End Source File
# Begin Source File

SOURCE=.\tiffiop.h
# End Source File
# Begin Source File

SOURCE=.\uvcode.h
# End Source File
# End Group
# End Target
# End Project
