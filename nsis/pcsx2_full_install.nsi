
; PCSX2 Full/Complete Install Package!
; (a NSIS installer script)
;
; Copyright 2009-2010  PCSX2 Dev Team
;

!ifndef INC_CRT_2008
  ; Set to 0 to disable inclusion of Visual Studio 2008 SP1 CRT Redists
  !define INC_CRT_2008  1
!endif

!ifndef INC_CRT_2010
  ; Set to 0 to disable inclusion of Visual Studio 2010 CRT Redists
  !define INC_CRT_2010  1
!endif

!define OUTFILE_POSTFIX "setup"
!include "SharedBase.nsh"

; Reserve features for improved performance with solid archiving.
;  (uncomment if we add our own install options ini files)
;!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS
;!insertmacro MUI_RESERVEFILE_LANGDLL

!insertmacro MUI_PAGE_COMPONENTS 
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

!include "ApplyExeProps.nsh"
!include "SharedRedtape.nsh"

; =======================================================================
;                            Installer Sections
; =======================================================================

; -----------------------------------------------------------------------
; Basic section (emulation proper)
Section "!${APP_NAME} (required)" SEC_CORE

  SectionIn RO

  !include "SectionCoreReqs.nsh"

  ; ------------------------------------------
  ;          -- Plugins Section --
  ; ------------------------------------------

!if ${INC_PLUGINS} > 0

  SetOutPath "$INSTDIR\Plugins"
  !insertmacro UNINSTALL.LOG_OPEN_INSTALL

    File /nonfatal /oname=gsdx-sse2-r${SVNREV_GSDX}.dll    ..\bin\Plugins\gsdx-sse2.dll
    File /nonfatal /oname=gsdx-ssse3-r${SVNREV_GSDX}.dll   ..\bin\Plugins\gsdx-ssse3.dll 
    File /nonfatal /oname=gsdx-sse4-r${SVNREV_GSDX}.dll    ..\bin\Plugins\gsdx-sse4.dll
    File /nonfatal /oname=zerogs-r${SVNREV_ZEROGS}.dll     ..\bin\Plugins\zerogs.dll
  
    File /nonfatal /oname=spu2-x-r${SVNREV_SPU2X}.dll      ..\bin\Plugins\spu2-x.dll
    File /nonfatal /oname=zerospu2-r${SVNREV_ZEROSPU2}.dll ..\bin\Plugins\zerospu2.dll
  
    File /nonfatal /oname=cdvdiso-r${SVNREV_CDVDISO}.dll   ..\bin\Plugins\cdvdiso.dll
    File                                                   ..\bin\Plugins\cdvdGigaherz.dll
  
    File /nonfatal /oname=lilypad-r${SVNREV_LILYPAD}.dll   ..\bin\Plugins\lilypad.dll
    File                                                   ..\bin\Plugins\PadSSSPSX.dll
  
  ;File                                                   ..\bin\Plugins\FWlinuz.dll

  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL

!endif

SectionEnd

!include "SectionShortcuts.nsh"

; -----------------------------------------------------------------------
; MSVC Redistributable - required if the user does not already have it
; Note: if your NSIS generates an error here it means you need to download the latest
; visual studio redist package from microsoft.  Any redist 2008/SP1 or newer will do.
;
; IMPORTANT: Online references for how to detect the presence of the VS2008 redists LIE.
; None of the methods are reliable, because the registry keys placed by the MSI installer
; vary depending on operating system *and* MSI installer version (youch).
;
!if ${INC_CRT_2008} > 0
Section "Microsoft Visual C++ 2008 SP1 Redist (required)"  SEC_CRT2008

  SectionIn RO

  ; Downloaded from:
  ;  http://download.microsoft.com/download/d/d/9/dd9a82d0-52ef-40db-8dab-795376989c03/vcredist_x86.exe
 
  SetOutPath "$TEMP"
  File "vcredist_2008_sp1_x86.exe"
  DetailPrint "Running Visual C++ 2008 SP1 Redistributable Setup..."
  ExecWait '"$TEMP\vcredist_2008_sp1_x86.exe" /qb'
  DetailPrint "Finished Visual C++ 2008 SP1 Redistributable Setup"
  
  Delete "$TEMP\vcredist_2008_sp1_x86.exe"

SectionEnd
!endif

!if ${INC_CRT_2010} > 0
Section "Microsoft Visual C++ 2010 Redist (recommended)" SEC_CRT2010

  ;SectionIn RO

  ; Detection made easy: Unlike previous redists, VC2010 now generates a platform
  ; independent key for checking availability.
  
  ; Downloaded from:
  ;   http://download.microsoft.com/download/5/B/C/5BC5DBB3-652D-4DCE-B14A-475AB85EEF6E/vcredist_x86.exe

  ReadRegDword $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\10.0\VC\VCRedist\x86" "Installed"
  IfErrors done
  StrCmp $R0 "1" done

  SetOutPath "$TEMP"
  File "vcredist_2010_x86.exe"
  DetailPrint "Running Visual C++ 2010 SP1 Redistributable Setup..."
  ExecWait '"$TEMP\vcredist_2010_x86.exe" /qb'
  DetailPrint "Finished Visual C++ 2010 SP1 Redistributable Setup"
  
  Delete "$TEMP\vcredist_2010_x86.exe"

done:
SectionEnd
!endif

; -----------------------------------------------------------------------
; This section needs to be last, so that in case it fails, the rest of the program will
; be installed cleanly.
; 
; This section could be optional, but why not?  It's pretty painless to double-check that
; all the libraries are up-to-date.
;
Section "DirectX Web Setup (recommended)" SEC_DIRECTX
                                                                              
 ;SectionIn RO

 SetOutPath "$TEMP"
 File "dxwebsetup.exe"
 DetailPrint "Running DirectX Web Setup..."
 ExecWait '"$TEMP\dxwebsetup.exe" /Q' $DirectXSetupError
 DetailPrint "Finished DirectX Web Setup"                                     
                                                                              
 Delete "$TEMP\dxwebsetup.exe"

SectionEnd

!include "SectionUninstaller.nsh"

LangString DESC_CORE       ${LANG_ENGLISH} "Core components (binaries, plugins, languages, etc)."

LangString DESC_STARTMENU  ${LANG_ENGLISH} "Adds shortcuts for PCSX2 to the start menu (all users)."
LangString DESC_DESKTOP    ${LANG_ENGLISH} "Adds a shortcut for PCSX2 to the desktop (all users)."

LangString DESC_CRT2008    ${LANG_ENGLISH} "Required by the PCSX2 binaries packaged in this installer."
LangString DESC_CRT2010    ${LANG_ENGLISH} "This will be used by future PCSX2 plugins and updates, but is not (yet) necessary."
LangString DESC_DIRECTX    ${LANG_ENGLISH} "Only uncheck this if you are quite certain your Direct3D runtimes are up to date."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CORE}        $(DESC_CORE)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_STARTMENU}   $(DESC_STARTMENU)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DESKTOP}     $(DESC_DESKTOP)

!if ${INC_CRT_2008} > 0
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CRT2008}     $(DESC_CRT2008)
!endif

!if ${INC_CRT_2010} > 0
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_CRT2010}     $(DESC_CRT2010)
!endif

  !insertmacro MUI_DESCRIPTION_TEXT ${SEC_DIRECTX}     $(DESC_DIRECTX)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

