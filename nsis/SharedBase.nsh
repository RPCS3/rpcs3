
; =======================================================================
;                        Core Includes and Defines
; =======================================================================

!ifndef INC_CORE
  !define INC_CORE      1
!endif

!ifndef INC_PLUGINS
  ; Set to 0 to include the core binaries only (no plugins)
  !define INC_PLUGINS   1
!endif

!ifndef INC_LANGS
  ; Set to 1 to enable inclusion of Languages folders
  !define INC_LANGS     1
!endif

!ifndef USE_PACKAGE_REV
  ; When enabled, all exe and plugins use a single revision based ont he trunk/HEAD svn revision.
  ; When disabled, each plugin and the main exe get their own revision number based on the actual
  ; revision the component was last updated.
  !define USE_PACKAGE_REV 1
!endif

!if ${INC_CORE} > 0
  ; FIXME: Technically we'd want to exclude plugin revisions here, but it isn't easy to do. 
  !system 'SubWCRev.exe ..\           templates\svnrev_package.nsh    svnrev_package.nsh'
!else
  ; Revision information for all plugins; used to moniker the output file when building
  ; plugin-only packages.
  !system 'SubWCRev.exe ..\plugins    templates\svnrev_package.nsh    svnrev_package.nsh'
!endif

!system 'SubWCRev.exe ..\pcsx2 templates\svnrev_pcsx2.nsh   svnrev_pcsx2.nsh'

!include "svnrev_package.nsh"
!include "svnrev_pcsx2.nsh"

; Notes on Uninstall Log Location (UNINSTALL_LOG)
;   The name of the uninstall log determines whether or not future installers
;   fall under the same single uninstall entry, or if they use multiple (separate)
;   uninstall folders.  

!ifndef APP_VERSION
  !define APP_VERSION      "0.9.8"
!endif

!define APP_NAME         "PCSX2 ${APP_VERSION} (r${SVNREV_PACKAGE})"
!define APP_FILENAME     "pcsx2-r${SVNREV_PCSX2}"
!define UNINSTALL_LOG    "Uninst-pcsx2-r${SVNREV_PACKAGE}"

!define INSTDIR_REG_ROOT "HKLM"

XPStyle on

; LZMA is the best, by far, so let's make sure it's always in use:
;  (dictionaries larger than 24MB don't seem to help)
SetCompressor /SOLID lzma
SetCompressorDictSize 24


; The name of the installer
Name "${APP_NAME}"

OutFile "output\pcsx2-${APP_VERSION}-r${SVNREV_PACKAGE}-${OUTFILE_POSTFIX}.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\PCSX2 ${APP_VERSION}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey ${INSTDIR_REG_ROOT} "Software\PCSX2\${APP_VERSION}" "Install_Dir"

; These defines are dependent on NSIS vars assigned above.

!define APP_EXE          "$INSTDIR\${APP_FILENAME}.exe"
!define INSTDIR_REG_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_FILENAME}"

!define PCSX2_README     "pcsx2 readme ${APP_VERSION}.doc"
!define PCSX2_FAQ        "pcsx2 FAQ ${APP_VERSION}.pdf"


Var DirectXSetupError

; =======================================================================
;                          Vista/Win7 UAC Stuff
; =======================================================================
; FIXME !!
; Request application privileges for Windows Vista/7; I'd love for this to be sensible about which
; execution level it requests, but UAC is breaking my mind.  I included some code for User type
; detection in function IsUserAdmin, but not really using it constructively yet.  (see also our
; uses of SetShellVarContext in the installer sections) 

;!include "IsUserAdmin.nsi"

; Allow admin-rights PCSX2 users to be hardcore!
AllowRootDirInstall true

; Just require admin for now, until we figure out a nice way to allow for casual user installs.
RequestExecutionLevel admin

; =======================================================================
;                  MUI2 and Advanced Uninstaller Basics
; =======================================================================
!include "MUI2.nsh"
!include "AdvUninstLog.nsh"

; This defines the Advanced Uninstaller mode of operation...
!insertmacro UNATTENDED_UNINSTALL

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "banner.bmp"
!define MUI_COMPONENTSPAGE_SMALLDESC

