; Author: Lilla (lilla@earthlink.net) 2003-06-13
; function IsUserAdmin uses plugin \NSIS\PlusgIns\UserInfo.dll
; This function is based upon code in \NSIS\Contrib\UserInfo\UserInfo.nsi
; This function was tested under NSIS 2 beta 4 (latest CVS as of this writing).
;
; Usage:
;   Call IsUserAdmin
;   Pop $R0   ; at this point $R0 is "true" or "false"
;
Function IsUserAdmin
  Push $R0
  Push $R1
  Push $R2
 
  ClearErrors
  UserInfo::GetName
  IfErrors Win9x
  Pop $R1
  UserInfo::GetAccountType
  Pop $R2
 
  StrCmp $R2 "Admin" 0 Continue

  ; Observation: I get here when running Win98SE. (Lilla)
  ; The functions UserInfo.dll looks for are there on Win98 too, 
  ; but just don't work. So UserInfo.dll, knowing that admin isn't required
  ; on Win98, returns admin anyway. (per kichik)
  ; MessageBox MB_OK 'User "$R1" is in the Administrators group'
  StrCpy $R0 "true"
  Goto Done
 
  Continue:
    ; You should still check for an empty string because the functions
    ; UserInfo.dll looks for may not be present on Windows 95. (per kichik)
    StrCmp $R2 "" Win9x
    StrCpy $R0 "false"
    ;MessageBox MB_OK 'User "$R1" is in the "$R2" group'
  Goto Done
 
  Win9x:
    ; comment/message below is by UserInfo.nsi author:
    ; This one means you don't need to care about admin or
    ; not admin because Windows 9x doesn't either
    ;MessageBox MB_OK "Error! This DLL can't run under Windows 9x!"
    StrCpy $R0 "true"
 
  Done:
    ;MessageBox MB_OK 'User= "$R1"  AccountType= "$R2"  IsUserAdmin= "$R0"'
 
  Pop $R2
  Pop $R1
  Exch $R0
FunctionEnd