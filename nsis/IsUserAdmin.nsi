
Function IsUserAdmin

  ClearErrors
  UserInfo::GetName
  IfErrors Win9x

  Pop $0
  UserInfo::GetAccountType
  Pop $1
 
  # GetOriginalAccountType will check the tokens of the original user of the
  # current thread/process. If the user tokens were elevated or limited for
  # this process, GetOriginalAccountType will return the non-restricted
  # account type.
  # On Vista with UAC, for example, this is not the same value when running
  # with `RequestExecutionLevel user`. GetOriginalAccountType will return
  # "admin" while GetAccountType will return "user".
  UserInfo::GetOriginalAccountType
  Pop $2

  ; Windows9x can sometimes return empty strings... 
  StrCmp $1 "" 0 +2
    Goto Win9x

  StrCmp $1 "Admin" 0 +3
    DetailPrint '(UAC) User "$0" is in the Administrators group'
    Goto done
  
  StrCmp $1 "Power" 0 +3
    DetailPrint '(UAC) User "$0" is in the Power Users group'
    Goto done
  
  StrCmp $1 "User" 0 +3
    DetailPrint '(UAC) User "$0" is just a regular user'
    Goto done

  StrCmp $1 "Guest" 0 +3
    ; Guest account?  Probably doomed to failure, but might as well try, just in case some shit
    ; is being mis-reported.
    DetailPrint '(UAC) User "$0" is a guest -- this installer is probably going to fail.  Good luck.'
    Goto done

  ;MessageBox MB_OK "Unknown error while trying to detect "
  DetailPrint "(UAC) Unknown error while trying to detect account type; assuming USER mode."
  StrCpy $1 "User"
  Goto done

Win9x:
  # This one means you don't need to care about admin or
  # not admin because Windows 9x doesn't either
  MessageBox MB_OK "Error! PCSX2 requires Windows 2000 or newer to install and run!"
  Quit

done:

  ; How to return the admin modeas a variable?  NSIS confuses me -- air
  ;Exch $R0
 
FunctionEnd