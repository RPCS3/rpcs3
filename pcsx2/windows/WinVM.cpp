/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include "win32.h"


#ifdef PCSX2_VIRTUAL_MEM

// virtual memory/privileges
#include "ntsecapi.h"

static wchar_t s_szUserName[255];

LRESULT WINAPI UserNameProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			SetWindowPos(hDlg, HWND_TOPMOST, 200, 100, 0, 0, SWP_NOSIZE);
			return TRUE;

		case WM_COMMAND:
			switch(wParam) {
				case IDOK:
				{
					wchar_t str[255];
					GetWindowTextW(GetDlgItem(hDlg, IDC_USER_NAME), str, 255);
					swprintf(s_szUserName, 255, L"%hs", &str);
					EndDialog(hDlg, TRUE );
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(hDlg, FALSE );
					return TRUE;
			}
			break;
	}
	return FALSE;
}

BOOL InitLsaString(
  PLSA_UNICODE_STRING pLsaString,
  LPCWSTR pwszString
)
{
  DWORD dwLen = 0;

  if (NULL == pLsaString)
      return FALSE;

  if (NULL != pwszString) 
  {
      dwLen = wcslen(pwszString);
      if (dwLen > 0x7ffe)   // String is too large
          return FALSE;
  }

  // Store the string.
  pLsaString->Buffer = (WCHAR *)pwszString;
  pLsaString->Length =  (USHORT)dwLen * sizeof(WCHAR);
  pLsaString->MaximumLength= (USHORT)(dwLen+1) * sizeof(WCHAR);

  return TRUE;
}

PLSA_TRANSLATED_SID2 GetSIDInformation (LPWSTR AccountName,LSA_HANDLE PolicyHandle)
{
  LSA_UNICODE_STRING lucName;
  PLSA_TRANSLATED_SID2 ltsTranslatedSID;
  PLSA_REFERENCED_DOMAIN_LIST lrdlDomainList;
  //LSA_TRUST_INFORMATION myDomain;
  NTSTATUS ntsResult;
  PWCHAR DomainString = NULL;

  // Initialize an LSA_UNICODE_STRING with the name.
  if (!InitLsaString(&lucName, AccountName))
  {
         wprintf(L"Failed InitLsaString\n");
         return NULL;
  }

  ntsResult = LsaLookupNames2(
     PolicyHandle,     // handle to a Policy object
	 0,
     1,                // number of names to look up
     &lucName,         // pointer to an array of names
     &lrdlDomainList,  // receives domain information
     &ltsTranslatedSID // receives relative SIDs
  );
  if (0 != ntsResult) 
  {
    wprintf(L"Failed LsaLookupNames - %lu \n",
      LsaNtStatusToWinError(ntsResult));
    return NULL;
  }

  // Get the domain the account resides in.
//  myDomain = lrdlDomainList->Domains[ltsTranslatedSID->DomainIndex];
//  DomainString = (PWCHAR) LocalAlloc(LPTR, myDomain.Name.Length + 1);
//  wcsncpy(DomainString, myDomain.Name.Buffer, myDomain.Name.Length);

  // Display the relative Id. 
//  wprintf(L"Relative Id is %lu in domain %ws.\n",
//    ltsTranslatedSID->RelativeId,
//    DomainString);

  LsaFreeMemory(lrdlDomainList);

  return ltsTranslatedSID;
}

BOOL AddPrivileges(PSID AccountSID, LSA_HANDLE PolicyHandle, BOOL bAdd)
{
  LSA_UNICODE_STRING lucPrivilege;
  NTSTATUS ntsResult;

  // Create an LSA_UNICODE_STRING for the privilege name(s).
  if (!InitLsaString(&lucPrivilege, L"SeLockMemoryPrivilege"))
  {
         wprintf(L"Failed InitLsaString\n");
         return FALSE;
  }

  if( bAdd ) {
    ntsResult = LsaAddAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID.
        &lucPrivilege, // The privilege(s).
        1              // Number of privileges.
    );
  }
  else {
      ntsResult = LsaRemoveAccountRights(
        PolicyHandle,  // An open policy handle.
        AccountSID,    // The target SID
        FALSE,
        &lucPrivilege, // The privilege(s).
        1              // Number of privileges.
    );
  }
      
  if (ntsResult == 0) 
  {
    wprintf(L"Privilege added.\n");
  }
  else
  {
	  int err = LsaNtStatusToWinError(ntsResult);
	  char str[255];
		_snprintf(str, 255, "Privilege was not added - %lu \n", LsaNtStatusToWinError(ntsResult));
		MessageBox(NULL, str, "Privilege error", MB_OK);
	return FALSE;
  }

  return TRUE;
} 

#define TARGET_SYSTEM_NAME L"mysystem"
LSA_HANDLE GetPolicyHandle()
{
  LSA_OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR SystemName[] = TARGET_SYSTEM_NAME;
  USHORT SystemNameLength;
  LSA_UNICODE_STRING lusSystemName;
  NTSTATUS ntsResult;
  LSA_HANDLE lsahPolicyHandle;

  // Object attributes are reserved, so initialize to zeroes.
  ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

  //Initialize an LSA_UNICODE_STRING to the server name.
  SystemNameLength = wcslen(SystemName);
  lusSystemName.Buffer = SystemName;
  lusSystemName.Length = SystemNameLength * sizeof(WCHAR);
  lusSystemName.MaximumLength = (SystemNameLength+1) * sizeof(WCHAR);

  // Get a handle to the Policy object.
  ntsResult = LsaOpenPolicy(
        NULL,    //Name of the target system.
        &ObjectAttributes, //Object attributes.
        POLICY_ALL_ACCESS, //Desired access permissions.
        &lsahPolicyHandle  //Receives the policy handle.
    );

  if (ntsResult != 0)
  {
    // An error occurred. Display it as a win32 error code.
    wprintf(L"OpenPolicy returned %lu\n",
      LsaNtStatusToWinError(ntsResult));
    return NULL;
  } 
  return lsahPolicyHandle;
}


/*****************************************************************
   LoggedSetLockPagesPrivilege: a function to obtain, if possible, or
   release the privilege of locking physical pages.

   Inputs:

       HANDLE hProcess: Handle for the process for which the
       privilege is needed

       BOOL bEnable: Enable (TRUE) or disable?

   Return value: TRUE indicates success, FALSE failure.

*****************************************************************/
BOOL SysLoggedSetLockPagesPrivilege ( HANDLE hProcess, BOOL bEnable)
{
	struct {
	u32 Count;
	LUID_AND_ATTRIBUTES Privilege [1];
	} Info;

	HANDLE Token;
	BOOL Result;

	// Open the token.

	Result = OpenProcessToken ( hProcess,
								TOKEN_ADJUST_PRIVILEGES,
								& Token);

	if( Result != TRUE ) {
		Console::Error( "VirtualMemory Error > Cannot open process token." );
		return FALSE;
	}

	// Enable or disable?

	Info.Count = 1;
	if( bEnable ) 
	{
		Info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	} 
	else 
	{
		Info.Privilege[0].Attributes = SE_PRIVILEGE_REMOVED;
	}

	// Get the LUID.
	Result = LookupPrivilegeValue ( NULL,
									SE_LOCK_MEMORY_NAME,
									&(Info.Privilege[0].Luid));

	if( Result != TRUE ) 
	{
		Console::Error( "VirtualMemory Error > Cannot get privilege value for %s.", params SE_LOCK_MEMORY_NAME );
		return FALSE;
	}

	// Adjust the privilege.

	Result = AdjustTokenPrivileges ( Token, FALSE,
									(PTOKEN_PRIVILEGES) &Info,
									0, NULL, NULL);

	// Check the result.
	if( Result != TRUE ) 
	{
		Console::Error( "VirtualMemory Error > Cannot adjust token privileges, error %u.", params GetLastError() );
		return FALSE;
	} 
	else 
	{
		if( GetLastError() != ERROR_SUCCESS ) 
		{

			BOOL bSuc = FALSE;
			LSA_HANDLE policy;
			PLSA_TRANSLATED_SID2 ltsTranslatedSID;

//			if( !DialogBox(gApp.hInstance, MAKEINTRESOURCE(IDD_USERNAME), gApp.hWnd, (DLGPROC)UserNameProc) )
//				return FALSE;
            DWORD len = sizeof(s_szUserName);
            GetUserNameW(s_szUserName, &len);

			policy = GetPolicyHandle();

			if( policy != NULL ) {

				ltsTranslatedSID = GetSIDInformation(s_szUserName, policy);

				if( ltsTranslatedSID != NULL ) {
					bSuc = AddPrivileges(ltsTranslatedSID->Sid, policy, bEnable);
					LsaFreeMemory(ltsTranslatedSID);
				}

				LsaClose(policy);
			}

			if( bSuc ) {
				// Get the LUID.
				LookupPrivilegeValue ( NULL, SE_LOCK_MEMORY_NAME, &(Info.Privilege[0].Luid));

				bSuc = AdjustTokenPrivileges ( Token, FALSE, (PTOKEN_PRIVILEGES) &Info, 0, NULL, NULL);
			}

			if( bSuc ) {
				if( MessageBox(NULL, "PCSX2 just changed your SE_LOCK_MEMORY privilege in order to gain access to physical memory.\n"
								"Log off/on and run pcsx2 again. Do you want to log off?\n",
								"Privilege changed query", MB_YESNO) == IDYES ) {
					ExitWindows(EWX_LOGOFF, 0);
				}
			}
			else {
				MessageBox(NULL, "Failed adding SE_LOCK_MEMORY privilege, please check the local policy.\n"
					"Go to security settings->Local Policies->User Rights. There should be a \"Lock pages in memory\".\n"
					"Add your user to that and log off/on. This enables pcsx2 to run at real-time by allocating physical memory.\n"
					"Also can try Control Panel->Local Security Policy->... (this does not work on Windows XP Home)\n"
					"(zerofrog)\n", "Virtual Memory Access Denied", MB_OK);
				return FALSE;
			}
		}
	}

	CloseHandle( Token );

	return TRUE;
}

static u32 s_dwPageSize = 0;
int SysPhysicalAlloc(u32 size, PSMEMORYBLOCK* pblock)
{
//#ifdef WIN32_FILE_MAPPING
//	assert(0);
//#endif
	ULONG_PTR NumberOfPagesInitial; // initial number of pages requested
	int PFNArraySize;               // memory to request for PFN array
	BOOL bResult;

	assert( pblock != NULL );
	memset(pblock, 0, sizeof(PSMEMORYBLOCK));

	if( s_dwPageSize == 0 ) {
		SYSTEM_INFO sSysInfo;           // useful system information
		GetSystemInfo(&sSysInfo);  // fill the system information structure
		s_dwPageSize = sSysInfo.dwPageSize;

		if( s_dwPageSize != 0x1000 ) {
			Msgbox::Alert("Error! OS page size must be 4Kb!\n"
				"If for some reason the OS cannot have 4Kb pages, then run the TLB build.");
			return -1;
		}
	}

	// Calculate the number of pages of memory to request.
	pblock->NumberPages = (size+s_dwPageSize-1)/s_dwPageSize;
	PFNArraySize = pblock->NumberPages * sizeof (ULONG_PTR);

	pblock->aPFNs = (uptr*)HeapAlloc (GetProcessHeap (), 0, PFNArraySize);

	if (pblock->aPFNs == NULL) {
		Console::Error("Failed to allocate on heap.");
		goto eCleanupAndExit;
	}

	// Allocate the physical memory.
	NumberOfPagesInitial = pblock->NumberPages;
	bResult = AllocateUserPhysicalPages( GetCurrentProcess(), (PULONG_PTR)&pblock->NumberPages, (PULONG_PTR)pblock->aPFNs );

	if( bResult != TRUE ) 
	{
		Console::Error("Virtual Memory Error %u > Cannot allocate physical pages.", params GetLastError() );
		goto eCleanupAndExit;
	}

	if( NumberOfPagesInitial != pblock->NumberPages ) 
	{
		Console::Error("Virtual Memory > Physical allocation failed!\n\tAllocated only %p of %p pages.", params pblock->NumberPages, NumberOfPagesInitial );
		goto eCleanupAndExit;
	}

	pblock->aVFNs = (uptr*)HeapAlloc(GetProcessHeap(), 0, PFNArraySize);

	return 0;

eCleanupAndExit:
	SysPhysicalFree(pblock);
	return -1;
}

void SysPhysicalFree(PSMEMORYBLOCK* pblock)
{
	assert( pblock != NULL );

	// Free the physical pages.
	FreeUserPhysicalPages( GetCurrentProcess(), (PULONG_PTR)&pblock->NumberPages, (PULONG_PTR)pblock->aPFNs );

	if( pblock->aPFNs != NULL ) HeapFree(GetProcessHeap(), 0, pblock->aPFNs);
	if( pblock->aVFNs != NULL ) HeapFree(GetProcessHeap(), 0, pblock->aVFNs);
	memset(pblock, 0, sizeof(PSMEMORYBLOCK));
}

int SysVirtualPhyAlloc(void* base, u32 size, PSMEMORYBLOCK* pblock)
{
	BOOL bResult;
	int i;

	LPVOID lpMemReserved = VirtualAlloc( base, size, MEM_RESERVE | MEM_PHYSICAL, PAGE_READWRITE );
	if( lpMemReserved == NULL || base != lpMemReserved )
	{
		Console::WriteLn("VirtualMemory Error %d > Cannot reserve memory at 0x%8.8x(%x).", params base, lpMemReserved, GetLastError());
		goto eCleanupAndExit;
	}

	// Map the physical memory into the window.  
	bResult = MapUserPhysicalPages( base, (ULONG_PTR)pblock->NumberPages, (PULONG_PTR)pblock->aPFNs );

	for(i = 0; i < pblock->NumberPages; ++i)
		pblock->aVFNs[i] = (uptr)base + 0x1000*i;

	if( bResult != TRUE ) 
	{
		Console::WriteLn("VirtualMemory Error %u > MapUserPhysicalPages failed to map.", params GetLastError() );
		goto eCleanupAndExit;
	}

	return 0;

eCleanupAndExit:
	SysVirtualFree(base, size);
	return -1;
}

void SysVirtualFree(void* lpMemReserved, u32 size)
{
	// unmap   
	if( MapUserPhysicalPages( lpMemReserved, (size+s_dwPageSize-1)/s_dwPageSize, NULL ) != TRUE ) 
	{
		Console::WriteLn("VirtualMemory Error %u > MapUserPhysicalPages failed to unmap", params GetLastError() );
		return;
	}

	// Free virtual memory.
	VirtualFree( lpMemReserved, 0, MEM_RELEASE );
}

int SysMapUserPhysicalPages(void* Addr, uptr NumPages, uptr* pfn, int pageoffset)
{
	BOOL bResult = MapUserPhysicalPages(Addr, NumPages, (PULONG_PTR)(pfn+pageoffset));

#ifdef _DEBUG
	//if( !bResult )
		//__Log("Failed to map user pages: 0x%x:0x%x, error = %d\n", Addr, NumPages, GetLastError());
#endif

	return bResult;
}

#else

#endif
