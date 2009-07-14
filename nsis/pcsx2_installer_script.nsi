; Pcsx2 NSIS installer script
; loosely based on a collection of examples and on information from the wikipedia

!define APP_NAME "Pcsx2 - A Playstation 2 Emulator"
!define INSTDIR_REG_ROOT "HKLM"
!define INSTDIR_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

;--------------------------------
;Include Modern UI 2 and advanced log uninstaller. all credits to the respective authors

  !include "MUI2.nsh"
  !include "AdvUninstLog.nsh"

;--------------------------------

; The name of the installer
Name "Pcsx2"

; The file to write. To change each release
OutFile "Pcsx2-beta-1474-setup.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\Pcsx2"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Pcsx2" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

; Pages

  !insertmacro UNATTENDED_UNINSTALL
  !define MUI_COMPONENTSPAGE_NODESC ;no decription is really necessary at this stage...
  !insertmacro MUI_PAGE_COMPONENTS 
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES


;--------------------------------

; Basic section (emulation proper)
Section "Pcsx2 (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there. It's catched by the uninstaller script
  !insertmacro UNINSTALL.LOG_OPEN_INSTALL
  File /r "Pcsx2\"
  !insertmacro UNINSTALL.LOG_CLOSE_INSTALL
  
  ; Write the installation path into the registry
  WriteRegStr HKLM Software\Pcsx2 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pcsx2" "DisplayName" "Pcsx2 - Playstation 2 Emulator"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pcsx2" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pcsx2" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Pcsx2" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional sections (can be disabled by the user)
Section "Start Menu Shortcuts"

  ; Need to change name too, for each version
  CreateDirectory "$SMPROGRAMS\Pcsx2"
  CreateShortCut "$SMPROGRAMS\Pcsx2\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Pcsx2\Pcsx2.lnk" "$INSTDIR\pcsx2-beta-1474.exe" "" "$INSTDIR\pcsx2-beta-1474.exe" 0
  
SectionEnd 

; Optional, but required if you don't already have it
Section "Microsoft Visual C++ 2008 SP1 Redist(Required)"

	SetOutPath $TEMP 
	File "vcredist_x86.exe"
	ExecWait "$TEMP\vcredist_x86.exe"
 
SectionEnd

;--------------------------------

Function .onInit

        ;prepare log always within .onInit function
        !insertmacro UNINSTALL.LOG_PREPARE_INSTALL

FunctionEnd


Function .onInstSuccess

         ;create/update log always within .onInstSuccess function
         !insertmacro UNINSTALL.LOG_UPDATE_INSTALL

FunctionEnd

;--------------------------------



; Uninstaller

Section "Un.Standard uninstallation (Leaves user created files untouched)"
  
  ; Remove registry keys
  DeleteRegKey HKLM Software\Microsoft\Windows\CurrentVersion\Uninstall\Pcsx2
  DeleteRegKey HKLM Software\Pcsx2

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Pcsx2\*.*"
  RMDir "$SMPROGRAMS\Pcsx2"
  
  !insertmacro UNINSTALL.LOG_UNINSTALL "$INSTDIR"
  DeleteRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}"

SectionEnd

; This option lets you COMPLETELY uninstall by deleting the main folder. Caution! :)
Section /o "Un.Complete Uninstall. WARNING, this will delete every file in the program folder"

  MessageBox MB_YESNO "You have chosen to remove ALL files in the installation folder. This will remove all your saves, screenshots, patches and bios as well. Do you want to proceed?" IDYES true IDNO false
  true:
  Delete "$SMPROGRAMS\Pcsx2\*.*"
  RMDir "$SMPROGRAMS\Pcsx2"
  RMDir /r "$INSTDIR"
  DeleteRegKey ${INSTDIR_REG_ROOT} "${INSTDIR_REG_KEY}"
  false:
    
SectionEnd

Function UN.onInit

         ;begin uninstall, could be added on top of uninstall section instead
         !insertmacro UNINSTALL.LOG_BEGIN_UNINSTALL

FunctionEnd