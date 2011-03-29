; =======================================================================
;                           Un.Installer Sections
; =======================================================================
; (currently web and main installers share the same uninstaller behavior.  This
;  may change in the future, though I doubt it.)

; -----------------------------------------------------------------------
Section "Un.Program and Plugins ${APP_NAME}"

  SetShellVarContext all

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"

  ; Remove uninstaller info reg key ( Wow6432Node on 64bit Windows! )
  DeleteRegKey HKLM "${INSTDIR_REG_KEY}"

  Call un.removeShorties

  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Langs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Plugins"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Docs"
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR\Cheats"
  
  ; Remove files and registry key that store PCSX2 paths configurations
  SetShellVarContext current
  Delete $DOCUMENTS\PCSX2\inis\PCSX2_ui.ini
  DeleteRegKey HKCU Software\PCSX2

SectionEnd

; /o for optional and unticked by default
Section /o "Un.Program and Plugin configuration files"
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\inis\"

SectionEnd

; /o for optional and unticked by default
Section /o "Un.User files (Memory Cards, Savestates, BIOS, etc)"
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\"

SectionEnd