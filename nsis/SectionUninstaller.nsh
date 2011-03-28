; =======================================================================
;                           Un.Installer Sections
; =======================================================================
; (currently web and main installers share the same uninstaller behavior.  This
;  may change in the future, though I doubt it.)

; -----------------------------------------------------------------------
Section "Un.Exes and Plugins ${APP_NAME}"

  SetShellVarContext all

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"

  ; nsis todo: Some legacy key? Check
  ; Remove registry keys (but only the ones related to the installer -- user options remain)
  DeleteRegKey HKLM "${INSTDIR_REG_KEY}"

  Call un.removeShorties

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Langs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Plugins"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Docs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Cheats"

SectionEnd

Section "Un.Complete Settings / Registry Cleanup"

  ; nsis todo: Some legacy key? Check
  ; Kill the entire PCSX2 registry key!
  DeleteRegKey ${INSTDIR_REG_ROOT} Software\PCSX2

  ; Kill user options PCSX2 reg key
  DeleteRegKey HKCU Software\PCSX2
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\inis\"

  ; nsis todo: is this still used?
  ; Kill AppData/PCSX2 entry!
  StrCpy $0 $LOCALAPPDATA\PCSX2
  Call un.DeleteDirIfEmpty
  StrCpy $0 $APPDATA\PCSX2
  Call un.DeleteDirIfEmpty

SectionEnd
