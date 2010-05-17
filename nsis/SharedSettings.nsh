
; =======================================================================
;                        Core Includes and Defines
; =======================================================================

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
