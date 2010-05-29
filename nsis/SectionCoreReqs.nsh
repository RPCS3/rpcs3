
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
  
  SetOutPath "$INSTDIR"
  !insertmacro UNINSTALL.LOG_OPEN_INSTALL
    File           /oname=${APP_EXE}                ..\bin\pcsx2.exe
    ;File /nonfatal /oname=pcsx2-dev-r${SVNREV}.exe  ..\bin\pcsx2-dev.exe

  ; ------------------------------------------
  ;       -- Shared Core Components --
  ; ------------------------------------------
  ; (Binaries, shared DLLs, null plugins, game database, languages, etc)

  ; Note that v3 pthreads is compatible with v4 pthreads, so we just copy v4 oover both
  ; filenames.  This allows many older plugin versions to continue to work.  (note that
  ; v3 will be removed for 0.9.8).

    File                                            ..\bin\w32pthreads.v4.dll
    File           /oname=w32pthreads.v3.dll        ..\bin\w32pthreads.v4.dll
    File                                            ..\bin\GameIndex.dbf

	!insertmacro UNINSTALL.LOG_CLOSE_INSTALL

    SetOutPath "$INSTDIR\Docs"
    !insertmacro UNINSTALL.LOG_OPEN_INSTALL
    File                                            ..\bin\docs\*
    !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
    
    SetOutPath "$INSTDIR\Plugins"
	!insertmacro UNINSTALL.LOG_OPEN_INSTALL
    ; NULL plugins are required, because the PCSX2 plugin selector needs a dummy plugin in every slot
    ; in order to run (including CDVD!) -- and really there should be more but we don't have working
    ; SPU2 null plugins right now.
 
    File ..\bin\Plugins\GSnull.dll
    ;File ..\bin\Plugins\SPU2null.dll            
    File ..\bin\Plugins\USBnull.dll
    File ..\bin\Plugins\DEV9null.dll
    File ..\bin\Plugins\FWnull.dll
    File ..\bin\Plugins\CDVDnull.dll
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
  
  ; In 0.9.7 there is only English, so including the other mo files (for now) is pointless.
  ; This code will be re-enabled when the new GUI is translated.
  
  !if ${INC_LANGS} > 0
    SetOutPath $INSTDIR\Langs
    !insertmacro UNINSTALL.LOG_OPEN_INSTALL
      File /nonfatal /r ..\bin\Langs\*.mo
    !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
  !endif


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
