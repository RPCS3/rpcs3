

; -----------------------------------------------------------------------
; Start Menu - Optional section (can be disabled by the user)
Section "Start Menu Shortcuts" SEC_STARTMENU

  ; CreateShortCut gets the working directory from OutPath
  SetOutPath "$INSTDIR"

  CreateDirectory "$SMPROGRAMS\PCSX2"
  CreateShortCut "$SMPROGRAMS\PCSX2\Uninstall ${APP_NAME}.lnk"      "${UNINST_EXE}"      ""    "${UNINST_EXE}"    0
  CreateShortCut "$SMPROGRAMS\PCSX2\${APP_NAME}.lnk"                "${APP_EXE}"         ""    "${APP_EXE}"       0

  CreateShortCut "$SMPROGRAMS\PCSX2\Readme ${APP_VERSION}.lnk"                      "$INSTDIR\docs\${PCSX2_README}" \
    "" "" 0 "" "" "Typical usage overview and background information for PCSX2."
  CreateShortCut "$SMPROGRAMS\PCSX2\Frequently Asked Questions ${APP_VERSION}.lnk"  "$INSTDIR\docs\${PCSX2_FAQ}" \
    "" "" 0 "" "" "Common answers to common problems and inquiries."

  ;IfFileExists ..\bin\pcsx2-dev.exe 0 +2
  ;  CreateShortCut "PCSX2\pcsx2-dev-r${SVNREV}.lnk"  "$INSTDIR\pcsx2-dev-r${SVNREV}.exe"  "" "$INSTDIR\pcsx2-dev-r${SVNREV}.exe" 0 "" "" \
  ;    "PCSX2 Devel (has additional logging support)"

SectionEnd

; -----------------------------------------------------------------------
; Desktop Icon - Optional section (can be disabled by the user)
Section "Desktop Shortcut" SEC_DESKTOP

  ; CreateShortCut gets the working directory from OutPath
  SetOutPath "$INSTDIR"
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk"            "${APP_EXE}"      "" "${APP_EXE}"     0 "" "" "A Playstation 2 Emulator"
    
SectionEnd
