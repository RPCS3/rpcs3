
; PCSX2 NSIS installer script
; Copyright 2009-2010  PCSX2 Dev Team

; Application version, changed for each release to match the version

; Uncomment this to create a package that includes binaries and binary dependencies only.
!define INC_PLUGINS

!system 'SubWCRev.exe ..\pcsx2 templates\svnrev_pcsx2.nsh svnrev_pcsx2.nsh'
!include "svnrev_pcsx2.nsh"

;LoadLanguageFile "${NSISDIR}\Contrib\Language files\english.nlf"

; =======================================================================
;                          Global Names and Such
; =======================================================================

!define APP_VERSION      "0.9.7"
!define APP_NAME         "PCSX2 ${APP_VERSION} (r${SVNREV})"
!define APP_FILENAME     "pcsx2-r${SVNREV}"

!define UNINSTALL_LOG    "Uninst-${APP_FILENAME}"

!define INSTDIR_REG_ROOT "HKLM"


; The name of the installer
Name "${APP_NAME}"

OutFile "${APP_FILENAME}-setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\PCSX2 ${APP_VERSION}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey ${INSTDIR_REG_ROOT} "Software\PCSX2" "Install_Dir"

XPStyle on

; LZMA is the best, by far, so let's make sure it's always in use:
;  (dictionaries larger than 24MB don't seem to help)
SetCompressor /SOLID lzma
SetCompressorDictSize 24

; These defines are dependent on NSIS vars assigned above.

!define APP_EXE          "$INSTDIR\${APP_FILENAME}.exe"
!define INSTDIR_REG_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_FILENAME}"

Var DirectXSetupError

; =======================================================================
;                             Plugin Includes
; =======================================================================
; Note that zzOgl is disabled for now because it requires CG dependencies to be installed.

!ifdef INC_PLUGINS
  !system 'SubWCRev.exe ..\plugins\gsdx       templates\svnrev_gsdx.nsh     svnrev_gsdx.nsh'
  !system 'SubWCRev.exe ..\plugins\spu2-x     templates\svnrev_spu2x.nsh    svnrev_spu2x.nsh'
  !system 'SubWCRev.exe ..\plugins\cdvdiso    templates\svnrev_cdvdiso.nsh  svnrev_cdvdiso.nsh'
  !system 'SubWCRev.exe ..\plugins\lilypad    templates\svnrev_lilypad.nsh  svnrev_lilypad.nsh'
  !system 'SubWCRev.exe ..\plugins\zerogs\dx  templates\svnrev_zerogs.nsh   svnrev_zerogs.nsh'
  ;!system 'SubWCRev.exe ..\plugins\zzogl-pg   templates\svnrev_zzogl.nsh    svnrev_zzogl.nsh'
  !system 'SubWCRev.exe ..\plugins\zerospu2   templates\svnrev_zerospu2.nsh svnrev_zerospu2.nsh'

  !include "svnrev_gsdx.nsh"
  !include "svnrev_spu2x.nsh"
  !include "svnrev_cdvdiso.nsh"
  !include "svnrev_lilypad.nsh"
  !include "svnrev_zerogs.nsh"
  ;!include "svnrev_zzogl.nsh"
  !include "svnrev_zerospu2.nsh"
!endif


!include "MUI2.nsh"
!include "AdvUninstLog.nsh"

; =======================================================================
;                          Vista/Win7 UAC Stuff
; =======================================================================

!include "IsUserAdmin.nsi"

; Allow admin-rights PCSX2 users to be hardcore!
AllowRootDirInstall true

; FIXME !!
; Request application privileges for Windows Vista/7; I'd love for this to be sensible about which
; execution level it requests, but UAC is breaking my mind.  I included some code for User type
; detection in function IsUserAdmin, but not really using it constructively yet.  (see also our
; uses of SetShellVarContext in the installer sections) 
RequestExecutionLevel admin

; This defines the Advanced Uninstaller mode of operation...
!insertmacro UNATTENDED_UNINSTALL

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "banner.bmp"
!define MUI_COMPONENTSPAGE_NODESC

!insertmacro MUI_PAGE_COMPONENTS 
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
  
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_COMPONENTS
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; =======================================================================
;                            Setup.exe Properties
; =======================================================================
; (for the professionalism!!)

VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName"     "${APP_NAME}"
;VIAddVersionKey /LANG=${LANG_ENGLISH} "Comments"        "A test comment"
VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright"  "© 2010 PCSX2 Dev Team"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Installs PCSX2, a Playstation 2 Emulator for the PC"
VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion"     "${APP_VERSION}.${SVNREV}"

VIProductVersion "${APP_VERSION}.${SVNREV}"

; =======================================================================
;                            Installer Sections
; =======================================================================

; -----------------------------------------------------------------------
; Basic section (emulation proper)
Section "${APP_NAME} (required)"

  SectionIn RO

  ; --- UAC NIGHTMARES ---
  ; Ideally this would default to 'current' for user-level installs and 'all' for admin-level installs.
  ; There are problems to be aware of, however!
  ;
  ; * If the user is an admin, Windows Vista/7 will DEFAULT to an "all" shell context (installing shortcuts
  ;   for all users), even if we don't want it to (which causes the uninstaller to fail!)
  ; * If the user is not an admin, setting Shell Context to all will cause the installer to fail because the
  ;   user won't have permission enough to install it at all (sigh).
  ;
  ; For now we just require Admin rights to install PCSX2.  An ideal solution would be to use our IsUserAdmin
  ; function to auto-detect and modify nsis installer behavior accordingly.
  ;
  ; (note!  the SetShellVarContext use in the uninstaller section must match this one!)

  SetShellVarContext all
  ;SetShellVarContext current
  
  ; Note that v3 pthreads is compatible with v4 pthreads, so we just copy v4 oover both
  ; filenames.  This allows many older plugin versions to continue to work.  (note that
  ; v3 will be removed for 0.9.8).
  
  SetOutPath "$INSTDIR"
  !insertmacro UNINSTALL.LOG_OPEN_INSTALL
  File           /oname=${APP_EXE}                ..\bin\pcsx2.exe
  ;File /nonfatal /oname=pcsx2-dev-r${SVNREV}.exe  ..\bin\pcsx2-dev.exe
  File                                            ..\bin\w32pthreads.v4.dll
  File           /oname=w32pthreads.v3.dll        ..\bin\w32pthreads.v4.dll
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL

  ; ------------------------------------------
  ;        -- Game Database and Languages --
  ; ------------------------------------------

	!insertmacro UNINSTALL.LOG_OPEN_INSTALL
  File                                             ..\bin\DataBase.dbf
  ; HostFS is not enabled by default, so no need to bundle it yet
  ;File                                             ..\bin\pcsx2hostfs_ldr.elf
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
  
  ; In 0.9.7 there is only English, so including the other mo files (for now) is pointless.
  ; This code will be re-enabled when the new GUI is translated.
  
  !ifdef INC_LANGS
    SetOutPath $INSTDIR\Langs
    !insertmacro UNINSTALL.LOG_OPEN_INSTALL
    File /nonfatal /r ..\bin\Langs\*.mo
    !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
!endif

  ; ------------------------------------------
  ;          -- Plugins Section --
  ; ------------------------------------------
  SetOutPath $INSTDIR\Plugins

  ; NULL plugins are required, and really there should be more but we don't have working
  ; SPU2 null plugins right now.
 
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
  File ..\bin\Plugins\GSnull.dll
  ;File ..\bin\Plugins\SPU2null.dll            
  File ..\bin\Plugins\USBnull.dll
  File ..\bin\Plugins\DEV9null.dll
  File ..\bin\Plugins\FWnull.dll
  File ..\bin\Plugins\CDVDnull.dll
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL

  ; -- Other plugins --

!ifdef INC_PLUGINS

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

!endif

  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL

  ; ------------------------------------------
  ;         -- Registry Section --
  ; ------------------------------------------

  ; Write the installation path into the registry
  WriteRegStr HKLM Software\PCSX2 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr   HKLM "${INSTDIR_REG_KEY}"  "DisplayName"      "PCSX2 - Playstation 2 Emulator"
  WriteRegStr   HKLM "${INSTDIR_REG_KEY}"  "UninstallString"  "${UNINST_EXE}"
  WriteRegDWORD HKLM "${INSTDIR_REG_KEY}"  "NoModify" 1
  WriteRegDWORD HKLM "${INSTDIR_REG_KEY}"  "NoRepair" 1
  WriteUninstaller "${UNINST_EXE}"

SectionEnd

; -----------------------------------------------------------------------
; Start Menu - Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

	; CreateShortCut gets the working directory from OutPath
  SetOutPath "$INSTDIR"
  
  CreateDirectory "$SMPROGRAMS\PCSX2"
  CreateShortCut "$SMPROGRAMS\PCSX2\Uninstall ${APP_NAME}.lnk"  "${UNINST_EXE}"      ""    "${UNINST_EXE}"    0
  CreateShortCut "$SMPROGRAMS\PCSX2\${APP_NAME}.lnk"            "${APP_EXE}"         ""    "${APP_EXE}"       0

  ;IfFileExists ..\bin\pcsx2-dev.exe 0 +2
  ;  CreateShortCut "PCSX2\pcsx2-dev-r${SVNREV}.lnk"  "$INSTDIR\pcsx2-dev-r${SVNREV}.exe"  "" "$INSTDIR\pcsx2-dev-r${SVNREV}.exe" 0 "" "" \
  ;    "PCSX2 Devel (has additional logging support)"

SectionEnd

; -----------------------------------------------------------------------
; Desktop Icon - Optional section (can be disabled by the user)
Section "Desktop Shortcut"

  ; CreateShortCut gets the working directory from OutPath
  SetOutPath "$INSTDIR"
  
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk"            "${APP_EXE}"      "" "${APP_EXE}"     0 "" "" "A Playstation 2 Emulator"
    
SectionEnd

; -----------------------------------------------------------------------
; MSVC Redistributable - required if the user does not already have it
; Note: if your NSIS generates an error here it means you need to download the latest
; visual studio redist package from microsoft.  Any redist 2008/SP1 or newer will do.
Section "Microsoft Visual C++ 2008 SP1 Redist (required)"

  SectionIn RO

  SetOutPath "$TEMP"
  File "vcredist_x86.exe"
  DetailPrint "Running Visual C++ 2008 SP1 Redistributable Setup..."
  ExecWait '"$TEMP\vcredist_x86.exe" /qb'
  DetailPrint "Finished Visual C++ 2008 SP1 Redistributable Setup"
  
  Delete "$TEMP\vcredist_x86.exe"

SectionEnd

; -----------------------------------------------------------------------
; This section needs to be last, so that in case it fails, the rest of the program will
; be installed cleanly.
; 
; This section could be optional, but why not?  It's pretty painless to double-check that
; all the libraries are up-to-date.
;
Section "DirectX Web Setup (required)" SEC_DIRECTX
                                                                              
 SectionIn RO

 SetOutPath "$TEMP"
 File "dxwebsetup.exe"
 DetailPrint "Running DirectX Web Setup..."
 ExecWait '"$TEMP\dxwebsetup.exe" /Q' $DirectXSetupError
 DetailPrint "Finished DirectX Web Setup"                                     
                                                                              
 Delete "$TEMP\dxwebsetup.exe"

SectionEnd

;--------------------------------
Function .onInit

  ;prepare Advanced Uninstall log always within .onInit function
  !insertmacro UNINSTALL.LOG_PREPARE_INSTALL

  ; MORE UAC HELL ---------- >
  call IsUserAdmin

FunctionEnd


Function .onInstSuccess

  ;create/update log always within .onInstSuccess function
  !insertmacro UNINSTALL.LOG_UPDATE_INSTALL

FunctionEnd


; =======================================================================
;                           Un.Installer Sections
; =======================================================================

; Safe directory deletion code. :)
; 
Function un.DeleteDirIfEmpty

  ; Use $TEMP as the out dir when removing directories, since NSIS won't let us remove the
  ; "current" directory.
  SetOutPath "$TEMP"

  FindFirst $R0 $R1 "$0\*.*"
  strcmp $R1 "." 0 NoDelete
   FindNext $R0 $R1
   strcmp $R1 ".." 0 NoDelete
    ClearErrors
    FindNext $R0 $R1
    IfErrors 0 NoDelete
     FindClose $R0
     Sleep 1000
     RMDir "$0"
  NoDelete:
   FindClose $R0
FunctionEnd

Function un.removeShorties

  ; Remove shortcuts, if any

  Delete "$DESKTOP\${APP_NAME}.lnk"

  Delete "$SMPROGRAMS\PCSX2\Uninstall ${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\PCSX2\${APP_NAME}.lnk"
  ;Delete "$SMPROGRAMS\PCSX2\pcsx2-dev-r${SVNREV}.lnk"

  StrCpy $0 "$SMPROGRAMS\PCSX2"
  Call un.DeleteDirIfEmpty

FunctionEnd

; -----------------------------------------------------------------------
Section "Un.Core Executables ${APP_NAME}"

  SetShellVarContext all

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"

  ; Remove registry keys (but only the ones related to the installer -- user options remain)
  DeleteRegKey HKLM "${INSTDIR_REG_KEY}"

  Call un.removeShorties

SectionEnd

; -----------------------------------------------------------------------
Section "Un.Shared Components (DLLs, Languages, etc)"

  MessageBox MB_YESNO "WARNING!  If you have multiple versions of PCSX2 installed, removing all shared files will probably break them.  Are you sure you want to proceed?" \
    IDYES true IDNO false

  true:
    !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Langs"
    !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Plugins"

    ; Kill the entire PCSX2 registry key.
    DeleteRegKey ${INSTDIR_REG_ROOT} Software\PCSX2

  false:
    ; User cancelled -- do nothing!!

SectionEnd

; begin uninstall, could be added on top of uninstall section instead
Function un.onInit
  !insertmacro UNINSTALL.LOG_BEGIN_UNINSTALL
FunctionEnd


Function un.onUninstSuccess
  !insertmacro UNINSTALL.LOG_END_UNINSTALL

  ; And remove the various install dir(s) but only if they're clean of user content:
  
  StrCpy $0 "$INSTDIR\langs"
  Call un.DeleteDirIfEmpty

  StrCpy $0 "$INSTDIR\plugins"
  Call un.DeleteDirIfEmpty

  StrCpy $0 "$INSTDIR"
  Call un.DeleteDirIfEmpty
FunctionEnd

