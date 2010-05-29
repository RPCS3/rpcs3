
; =======================================================================
;                        Core Includes and Defines
; =======================================================================

; Notes on Uninstall Log Location (UNINSTALL_LOG)
;   The name of the uninstall log determines whether or not future installers
;   fall under the same single uninstall entry, or if they use multiple (separate)
;   uninstall folders.  

!system 'SubWCRev.exe ..\pcsx2 templates\svnrev_pcsx2.nsh svnrev_pcsx2.nsh'
!include "svnrev_pcsx2.nsh"

!ifndef APP_VERSION
  !define APP_VERSION      "0.9.7"
!endif

!define APP_NAME         "PCSX2 ${APP_VERSION} (r${SVNREV})"
!define APP_FILENAME     "pcsx2-r${SVNREV}"
!define UNINSTALL_LOG    "Uninst-${APP_FILENAME}"

!define INSTDIR_REG_ROOT "HKLM"

XPStyle on

; LZMA is the best, by far, so let's make sure it's always in use:
;  (dictionaries larger than 24MB don't seem to help)
SetCompressor /SOLID lzma
SetCompressorDictSize 24


; The name of the installer
Name "${APP_NAME}"

OutFile "${APP_FILENAME}-${OUTFILE_POSTFIX}.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\PCSX2 ${APP_VERSION}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey ${INSTDIR_REG_ROOT} "Software\PCSX2" "Install_Dir_${APP_VERSION}"

; These defines are dependent on NSIS vars assigned above.

!define APP_EXE          "$INSTDIR\${APP_FILENAME}.exe"
!define INSTDIR_REG_KEY  "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_FILENAME}"

!define PCSX2_README     "pcsx2 readme ${APP_VERSION}.doc"
!define PCSX2_FAQ        "pcsx2 FAQ ${APP_VERSION}.pdf"

Var DirectXSetupError
