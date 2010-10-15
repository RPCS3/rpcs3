; =======================================================================
;                           Un.Installer Sections
; =======================================================================
; (currently web and main installers share the same uninstaller behavior.  This
;  may change in the future, though I doubt it.)

; -----------------------------------------------------------------------
Section "Un.Exes and Plugins ${APP_NAME}"

  SetShellVarContext all

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"

  ; Remove registry keys (but only the ones related to the installer -- user options remain)
  DeleteRegKey HKLM "${INSTDIR_REG_KEY}"

  Call un.removeShorties

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Langs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Plugins"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Docs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Cheats"

SectionEnd

Section "Un.Complete Registry Cleanup"

  ; Kill the entire PCSX2 registry key!
  DeleteRegKey ${INSTDIR_REG_ROOT} Software\PCSX2

  ; Kill AppData/PCSX2 entry!

  SetShellVarContext current
  StrCpy $0 $LOCALAPPDATA\PCSX2
  Call un.DeleteDirIfEmpty
  StrCpy $0 $APPDATA\PCSX2
  Call un.DeleteDirIfEmpty

SectionEnd
