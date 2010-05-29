

; =======================================================================
;                             Plugin Includes
; =======================================================================
; Note that zzOgl is disabled for now because it requires CG dependencies to be installed.

!if ${INC_PLUGINS} > 0
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

; =======================================================================
;                         Shared Install Functions
; =======================================================================

; ==================================================================================

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
;                         Shared Uninstall Functions
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


; ==================================================================================
Function un.removeShorties

  ; Remove shortcuts, if any

  Delete "$DESKTOP\${APP_NAME}.lnk"

  Delete "$SMPROGRAMS\PCSX2\Uninstall ${APP_NAME}.lnk"
  Delete "$SMPROGRAMS\PCSX2\${APP_NAME}.lnk"
  ;Delete "$SMPROGRAMS\PCSX2\pcsx2-dev-r${SVNREV}.lnk"

  Delete "$SMPROGRAMS\PCSX2\Readme ${APP_VERSION}.lnk"
  Delete "$SMPROGRAMS\PCSX2\Frequently Asked Questions ${APP_VERSION}.lnk"

  StrCpy $0 "$SMPROGRAMS\PCSX2"
  Call un.DeleteDirIfEmpty

FunctionEnd

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

