; =======================================================================
;                           Un.Installer Sections
; =======================================================================
; (currently web and main installers share the same uninstaller behavior.  This
;  may change in the future, though I doubt it.)

; -----------------------------------------------------------------------
Section "Un.Program and Plugins ${APP_NAME}"

  SetShellVarContext all
  ; First thing, remove the registry entry in case uninstall doesn't complete successfully
  ;   otherwise, pcsx2 will be "confused" if it's re-installed later.
  DeleteRegKey HKCU Software\PCSX2

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

SectionEnd

; /o for optional and unticked by default
Section /o "Un.Configuration files (Program and Plugins)"
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\inis\"

SectionEnd

; /o for optional and unticked by default
Section /o "Un.User files (Memory Cards, Savestates, etc)"
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\Cheats_ws\"
  RMDir /r "$DOCUMENTS\PCSX2\inis\"
  RMDir /r "$DOCUMENTS\PCSX2\logs\"
  RMDir /r "$DOCUMENTS\PCSX2\memcards\"
  RMDir /r "$DOCUMENTS\PCSX2\snaps\"
  RMDir /r "$DOCUMENTS\PCSX2\sstates\"

SectionEnd

; /o for optional and unticked by default
Section /o "Un.BIOS files"
  
  SetShellVarContext current
  RMDir /r "$DOCUMENTS\PCSX2\bios\"

SectionEnd