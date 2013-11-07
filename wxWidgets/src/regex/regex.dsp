# Microsoft Developer Studio Project File - Name="regex" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=regex - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "regex.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "regex.mak" CFG="regex - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "regex - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Release DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Debug DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Release Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Debug Unicode" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Release With Debug Info" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Debug Unicode DLL" (based on "Win32 (x86) Static Library")
!MESSAGE "regex - Win32 Release Unicode DLL" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "regex - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W4 /O1 /I "." /I "../../lib/msw" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\regex.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /Zi /Od /I "." /I "../../lib/mswd" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\regexd.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Release DLL"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W4 /O1 /I "." /I "../../lib/msw" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\regex.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Debug DLL"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /Zi /Od /I "." /I "../../lib/mswd" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\lib\regexd.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Release Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "regex___Win32_Release_Unicode"
# PROP BASE Intermediate_Dir "regex___Win32_Release_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "regex___Win32_Release_Unicode"
# PROP Intermediate_Dir "regex___Win32_Release_Unicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /O1 /I "." /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W4 /O1 /I "." /I "../../lib/mswu" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\regex.lib"
# ADD LIB32 /nologo /out:"..\..\lib\regexu.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Debug Unicode"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "regex___Win32_Debug_Unicode"
# PROP BASE Intermediate_Dir "regex___Win32_Debug_Unicode"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "regex___Win32_Debug_Unicode"
# PROP Intermediate_Dir "regex___Win32_Debug_Unicode"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /Gm /Zi /Od /I "." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /Zi /Od /I "." /I "../../lib/mswud" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\regexd.lib"
# ADD LIB32 /nologo /out:"..\..\lib\regexud.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Release With Debug Info"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "regex___Win32_Release_With_Debug_Info"
# PROP BASE Intermediate_Dir "regex___Win32_Release_With_Debug_Info"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "regex___Win32_Release_With_Debug_Info"
# PROP Intermediate_Dir "regex___Win32_Release_With_Debug_Info"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /O1 /I "." /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /MD /W4 /Zi /O1 /I "." /I "../../lib/msw" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\regex.lib"
# ADD LIB32 /nologo /out:"..\..\lib\regex.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Debug Unicode DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "regex___Win32_Debug_Unicode_DLL"
# PROP BASE Intermediate_Dir "regex___Win32_Debug_Unicode_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "regex___Win32_Debug_Unicode_DLL"
# PROP Intermediate_Dir "regex___Win32_Debug_Unicode_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W4 /Gm /Zi /Od /I "." /I "../../lib/mswud" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /Zi /Od /I "." /I "../../lib/mswud" /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "__WINDOWS__" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WXMSW__" /D "__WXDEBUG__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\regexud.lib"
# ADD LIB32 /nologo /out:"..\..\lib\regexud.lib"

!ELSEIF  "$(CFG)" == "regex - Win32 Release Unicode DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "regex___Win32_Release_Unicode_DLL"
# PROP BASE Intermediate_Dir "regex___Win32_Release_Unicode_DLL"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "regex___Win32_Release_Unicode_DLL"
# PROP Intermediate_Dir "regex___Win32_Release_Unicode_DLL"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /O1 /I "." /I "../../lib/mswu" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD CPP /nologo /MD /W4 /O1 /I "." /I "../../lib/mswu" /I "../../include" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE" /D "wxUSE_UNICODE" /D "__WINDOWS__" /D "__WXMSW__" /D "__WIN95__" /D "__WIN32__" /D WINVER=0x0400 /D "STRICT" /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"..\..\lib\regexu.lib"
# ADD LIB32 /nologo /out:"..\..\lib\regexu.lib"

!ENDIF 

# Begin Target

# Name "regex - Win32 Release"
# Name "regex - Win32 Debug"
# Name "regex - Win32 Release DLL"
# Name "regex - Win32 Debug DLL"
# Name "regex - Win32 Release Unicode"
# Name "regex - Win32 Debug Unicode"
# Name "regex - Win32 Release With Debug Info"
# Name "regex - Win32 Debug Unicode DLL"
# Name "regex - Win32 Release Unicode DLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\regcomp.c
# End Source File
# Begin Source File

SOURCE=.\regerror.c
# End Source File
# Begin Source File

SOURCE=.\regexec.c
# End Source File
# Begin Source File

SOURCE=.\regfree.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\regcustom.h
# End Source File
# Begin Source File

SOURCE=.\regerrs.h
# End Source File
# Begin Source File

SOURCE=.\regex.h
# End Source File
# Begin Source File

SOURCE=.\regguts.h
# End Source File
# Begin Source File

SOURCE=..\..\include\wx\msw\setup.h

!IF  "$(CFG)" == "regex - Win32 Release"

# Begin Custom Build - Creating ..\..\lib\msw\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/msw/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\msw\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Debug"

# Begin Custom Build - Creating ..\..\lib\mswd\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/mswd/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\mswd\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Release Unicode"

# Begin Custom Build - Creating ..\..\lib\mswu\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/mswu/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\mswu\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Debug Unicode"

# Begin Custom Build - Creating ..\..\lib\mswud\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/mswud/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\mswud\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Release With Debug Info"

# Begin Custom Build - Creating ..\..\lib\msw\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/msw/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\msw\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Debug Unicode DLL"

# Begin Custom Build - Creating ..\..\lib\mswud\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/mswud/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\mswud\wx\setup.h

# End Custom Build

!ELSEIF  "$(CFG)" == "regex - Win32 Release Unicode DLL"

# Begin Custom Build - Creating ..\..\lib\mswu\wx\setup.h from $(InputPath)
InputPath=..\..\include\wx\msw\setup.h

"../../lib/mswu/wx/setup.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy "$(InputPath)" ..\..\lib\mswu\wx\setup.h

# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
