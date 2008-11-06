//------------------------------------------------------------------------------
// File: WXDebug.cpp
//
// Desc: DirectShow base classes - implements ActiveX system debugging
//       facilities.    
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#define _WINDLL

#include <streams.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef DEBUG
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif // _UNICODE
#endif // UNICODE
#endif // DEBUG

#ifdef DEBUG

// The Win32 wsprintf() function writes a maximum of 1024 characters to it's output buffer.
// See the documentation for wsprintf()'s lpOut parameter for more information.
const INT iDEBUGINFO = 1024;                 // Used to format strings

/* For every module and executable we store a debugging level for each of
   the five categories (eg LOG_ERROR and LOG_TIMING). This makes it easy
   to isolate and debug individual modules without seeing everybody elses
   spurious debug output. The keys are stored in the registry under the
   HKEY_LOCAL_MACHINE\SOFTWARE\Debug\<Module Name>\<KeyName> key values
   NOTE these must be in the same order as their enumeration definition */

TCHAR *pKeyNames[] = {
    TEXT("TIMING"),      // Timing and performance measurements
    TEXT("TRACE"),       // General step point call tracing
    TEXT("MEMORY"),      // Memory and object allocation/destruction
    TEXT("LOCKING"),     // Locking/unlocking of critical sections
    TEXT("ERROR"),       // Debug error notification
    TEXT("CUSTOM1"),
    TEXT("CUSTOM2"),
    TEXT("CUSTOM3"),
    TEXT("CUSTOM4"),
    TEXT("CUSTOM5")
    };

const TCHAR CAutoTrace::_szEntering[] = TEXT("->: %s");
const TCHAR CAutoTrace::_szLeaving[]  = TEXT("<-: %s");

const INT iMAXLEVELS = NUMELMS(pKeyNames);  // Maximum debug categories

HINSTANCE m_hInst;                          // Module instance handle
TCHAR m_ModuleName[iDEBUGINFO];             // Cut down module name
DWORD m_Levels[iMAXLEVELS];                 // Debug level per category
CRITICAL_SECTION m_CSDebug;                 // Controls access to list
DWORD m_dwNextCookie;                       // Next active object ID
ObjectDesc *pListHead = NULL;               // First active object
DWORD m_dwObjectCount;                      // Active object count
BOOL m_bInit = FALSE;                       // Have we been initialised
HANDLE m_hOutput = INVALID_HANDLE_VALUE;    // Optional output written here
DWORD dwWaitTimeout = INFINITE;             // Default timeout value
DWORD dwTimeOffset;			    // Time of first DbgLog call
bool g_fUseKASSERT = false;                 // don't create messagebox
bool g_fDbgInDllEntryPoint = false;
bool g_fAutoRefreshLevels = false;

const TCHAR *pBaseKey = TEXT("SOFTWARE\\Debug");
const TCHAR *pGlobalKey = TEXT("GLOBAL");
static CHAR *pUnknownName = "UNKNOWN";

TCHAR *TimeoutName = TEXT("TIMEOUT");

/* This sets the instance handle that the debug library uses to find
   the module's file name from the Win32 GetModuleFileName function */

void WINAPI DbgInitialise(HINSTANCE hInst)
{
    InitializeCriticalSection(&m_CSDebug);
    m_bInit = TRUE;

    m_hInst = hInst;
    DbgInitModuleName();
    if (GetProfileInt(m_ModuleName, TEXT("BreakOnLoad"), 0))
       DebugBreak();
    DbgInitModuleSettings(false);
    DbgInitGlobalSettings(true);
    dwTimeOffset = timeGetTime();
}


/* This is called to clear up any resources the debug library uses - at the
   moment we delete our critical section and the object list. The values we
   retrieve from the registry are all done during initialisation but we don't
   go looking for update notifications while we are running, if the values
   are changed then the application has to be restarted to pick them up */

void WINAPI DbgTerminate()
{
    if (m_hOutput != INVALID_HANDLE_VALUE) {
       EXECUTE_ASSERT(CloseHandle(m_hOutput));
       m_hOutput = INVALID_HANDLE_VALUE;
    }
    DeleteCriticalSection(&m_CSDebug);
    m_bInit = FALSE;
}


/* This is called by DbgInitLogLevels to read the debug settings
   for each logging category for this module from the registry */

void WINAPI DbgInitKeyLevels(HKEY hKey, bool fTakeMax)
{
    LONG lReturn;               // Create key return value
    LONG lKeyPos;               // Current key category
    DWORD dwKeySize;            // Size of the key value
    DWORD dwKeyType;            // Receives it's type
    DWORD dwKeyValue;           // This fields value

    /* Try and read a value for each key position in turn */
    for (lKeyPos = 0;lKeyPos < iMAXLEVELS;lKeyPos++) {

        dwKeySize = sizeof(DWORD);
        lReturn = RegQueryValueEx(
            hKey,                       // Handle to an open key
            pKeyNames[lKeyPos],         // Subkey name derivation
            NULL,                       // Reserved field
            &dwKeyType,                 // Returns the field type
            (LPBYTE) &dwKeyValue,       // Returns the field's value
            &dwKeySize );               // Number of bytes transferred

        /* If either the key was not available or it was not a DWORD value
           then we ensure only the high priority debug logging is output
           but we try and update the field to a zero filled DWORD value */

        if (lReturn != ERROR_SUCCESS || dwKeyType != REG_DWORD)  {

            dwKeyValue = 0;
            lReturn = RegSetValueEx(
                hKey,                   // Handle of an open key
                pKeyNames[lKeyPos],     // Address of subkey name
                (DWORD) 0,              // Reserved field
                REG_DWORD,              // Type of the key field
                (PBYTE) &dwKeyValue,    // Value for the field
                sizeof(DWORD));         // Size of the field buffer

            if (lReturn != ERROR_SUCCESS) {
                DbgLog((LOG_ERROR,0,TEXT("Could not create subkey %s"),pKeyNames[lKeyPos]));
                dwKeyValue = 0;
            }
        }
        if(fTakeMax)
        {
            m_Levels[lKeyPos] = max(dwKeyValue,m_Levels[lKeyPos]);
        }
        else
        {
            if((m_Levels[lKeyPos] & LOG_FORCIBLY_SET) == 0) {
                m_Levels[lKeyPos] = dwKeyValue;
            }
        }
    }

    /*  Read the timeout value for catching hangs */
    dwKeySize = sizeof(DWORD);
    lReturn = RegQueryValueEx(
        hKey,                       // Handle to an open key
        TimeoutName,                // Subkey name derivation
        NULL,                       // Reserved field
        &dwKeyType,                 // Returns the field type
        (LPBYTE) &dwWaitTimeout,    // Returns the field's value
        &dwKeySize );               // Number of bytes transferred

    /* If either the key was not available or it was not a DWORD value
       then we ensure only the high priority debug logging is output
       but we try and update the field to a zero filled DWORD value */

    if (lReturn != ERROR_SUCCESS || dwKeyType != REG_DWORD)  {

        dwWaitTimeout = INFINITE;
        lReturn = RegSetValueEx(
            hKey,                   // Handle of an open key
            TimeoutName,            // Address of subkey name
            (DWORD) 0,              // Reserved field
            REG_DWORD,              // Type of the key field
            (PBYTE) &dwWaitTimeout, // Value for the field
            sizeof(DWORD));         // Size of the field buffer

        if (lReturn != ERROR_SUCCESS) {
            DbgLog((LOG_ERROR,0,TEXT("Could not create subkey %s"),TimeoutName));
            dwWaitTimeout = INFINITE;
        }
    }
}

void WINAPI DbgOutString(LPCTSTR psz)
{
    if (m_hOutput != INVALID_HANDLE_VALUE) {
        UINT  cb = lstrlen(psz);
        DWORD dw;
#ifdef UNICODE
        CHAR szDest[2048];
        WideCharToMultiByte(CP_ACP, 0, psz, -1, szDest, NUMELMS(szDest), 0, 0);
        WriteFile (m_hOutput, szDest, cb, &dw, NULL);
#else
        WriteFile (m_hOutput, psz, cb, &dw, NULL);
#endif
    } else {
        OutputDebugString (psz);
    }
}

/* Called by DbgInitGlobalSettings to setup alternate logging destinations
 */

void WINAPI DbgInitLogTo (
    HKEY hKey)
{
    LONG  lReturn;
    DWORD dwKeyType;
    DWORD dwKeySize;
    TCHAR szFile[MAX_PATH] = {0};
    static const TCHAR cszKey[] = TEXT("LogToFile");

    dwKeySize = MAX_PATH;
    lReturn = RegQueryValueEx(
        hKey,                       // Handle to an open key
        cszKey,                     // Subkey name derivation
        NULL,                       // Reserved field
        &dwKeyType,                 // Returns the field type
        (LPBYTE) szFile,            // Returns the field's value
        &dwKeySize);                // Number of bytes transferred

    // create an empty key if it does not already exist
    //
    if (lReturn != ERROR_SUCCESS || dwKeyType != REG_SZ)
       {
       dwKeySize = sizeof(TCHAR);
       lReturn = RegSetValueEx(
            hKey,                   // Handle of an open key
            cszKey,                 // Address of subkey name
            (DWORD) 0,              // Reserved field
            REG_SZ,                 // Type of the key field
            (PBYTE)szFile,          // Value for the field
            dwKeySize);            // Size of the field buffer
       }

    // if an output-to was specified.  try to open it.
    //
    if (m_hOutput != INVALID_HANDLE_VALUE) {
       EXECUTE_ASSERT(CloseHandle (m_hOutput));
       m_hOutput = INVALID_HANDLE_VALUE;
    }
    if (szFile[0] != 0)
       {
       if (!lstrcmpi(szFile, TEXT("Console"))) {
          m_hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
          if (m_hOutput == INVALID_HANDLE_VALUE) {
             AllocConsole ();
             m_hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
          }
          SetConsoleTitle (TEXT("ActiveX Debug Output"));
       } else if (szFile[0] &&
                lstrcmpi(szFile, TEXT("Debug")) &&
                lstrcmpi(szFile, TEXT("Debugger")) &&
                lstrcmpi(szFile, TEXT("Deb")))
          {
            m_hOutput = CreateFile(szFile, GENERIC_WRITE,
                                 FILE_SHARE_READ,
                                 NULL, OPEN_ALWAYS,
                                 FILE_ATTRIBUTE_NORMAL,
                                 NULL);
          if (INVALID_HANDLE_VALUE != m_hOutput)
              {
              static const TCHAR cszBar[] = TEXT("\r\n\r\n=====DbgInitialize()=====\r\n\r\n");
              SetFilePointer (m_hOutput, 0, NULL, FILE_END);
              DbgOutString (cszBar);
              }
          }
       }
}



/* This is called by DbgInitLogLevels to read the global debug settings for
   each logging category for this module from the registry. Normally each
   module has it's own values set for it's different debug categories but
   setting the global SOFTWARE\Debug\Global applies them to ALL modules */

void WINAPI DbgInitGlobalSettings(bool fTakeMax)
{
    LONG lReturn;               // Create key return value
    TCHAR szInfo[iDEBUGINFO];   // Constructs key names
    HKEY hGlobalKey;            // Global override key

    /* Construct the global base key name */
    (void)StringCchPrintf(szInfo,NUMELMS(szInfo),TEXT("%s\\%s"),pBaseKey,pGlobalKey);

    /* Create or open the key for this module */
    lReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,   // Handle of an open key
                             szInfo,               // Address of subkey name
                             (DWORD) 0,            // Reserved value
                             NULL,                 // Address of class name
                             (DWORD) 0,            // Special options flags
                             KEY_ALL_ACCESS,       // Desired security access
                             NULL,                 // Key security descriptor
                             &hGlobalKey,          // Opened handle buffer
                             NULL);                // What really happened

    if (lReturn != ERROR_SUCCESS) {
        DbgLog((LOG_ERROR,0,TEXT("Could not access GLOBAL module key")));
        return;
    }

    DbgInitKeyLevels(hGlobalKey, fTakeMax);
    RegCloseKey(hGlobalKey);
}


/* This sets the debugging log levels for the different categories. We start
   by opening (or creating if not already available) the SOFTWARE\Debug key
   that all these settings live under. We then look at the global values
   set under SOFTWARE\Debug\Global which apply on top of the individual
   module settings. We then load the individual module registry settings */

void WINAPI DbgInitModuleSettings(bool fTakeMax)
{
    LONG lReturn;               // Create key return value
    TCHAR szInfo[iDEBUGINFO];   // Constructs key names
    HKEY hModuleKey;            // Module key handle

    /* Construct the base key name */
    (void)StringCchPrintf(szInfo,NUMELMS(szInfo), TEXT("%s\\%s"),pBaseKey,m_ModuleName);

    /* Create or open the key for this module */
    lReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,   // Handle of an open key
                             szInfo,               // Address of subkey name
                             (DWORD) 0,            // Reserved value
                             NULL,                 // Address of class name
                             (DWORD) 0,            // Special options flags
                             KEY_ALL_ACCESS,       // Desired security access
                             NULL,                 // Key security descriptor
                             &hModuleKey,          // Opened handle buffer
                             NULL);                // What really happened

    if (lReturn != ERROR_SUCCESS) {
        DbgLog((LOG_ERROR,0,TEXT("Could not access module key")));
        return;
    }

    DbgInitLogTo(hModuleKey);
    DbgInitKeyLevels(hModuleKey, fTakeMax);
    RegCloseKey(hModuleKey);
}


/* Initialise the module file name */

void WINAPI DbgInitModuleName()
{
    TCHAR FullName[iDEBUGINFO];     // Load the full path and module name
    TCHAR *pName;                   // Searches from the end for a backslash

    GetModuleFileName(m_hInst,FullName,iDEBUGINFO);
    pName = _tcsrchr(FullName,'\\');
    if (pName == NULL) {
        pName = FullName;
    } else {
        pName++;
    }
    (void)StringCchCopy(m_ModuleName,NUMELMS(m_ModuleName), pName);
}

struct MsgBoxMsg
{
    HWND hwnd;
    TCHAR *szTitle;
    TCHAR *szMessage;
    DWORD dwFlags;
    INT iResult;
};

//
// create a thread to call MessageBox(). calling MessageBox() on
// random threads at bad times can confuse the host (eg IE).
//
DWORD WINAPI MsgBoxThread(
  LPVOID lpParameter   // thread data
  )
{
    MsgBoxMsg *pmsg = (MsgBoxMsg *)lpParameter;
    pmsg->iResult = MessageBox(
        pmsg->hwnd,
        pmsg->szTitle,
        pmsg->szMessage,
        pmsg->dwFlags);
    
    return 0;
}

INT MessageBoxOtherThread(
    HWND hwnd,
    TCHAR *szTitle,
    TCHAR *szMessage,
    DWORD dwFlags)
{
    if(g_fDbgInDllEntryPoint)
    {
        // can't wait on another thread because we have the loader
        // lock held in the dll entry point.
        return MessageBox(hwnd, szTitle, szMessage, dwFlags);
    }
    else
    {
        MsgBoxMsg msg = {hwnd, szTitle, szMessage, dwFlags, 0};
        DWORD dwid;
        HANDLE hThread = CreateThread(
            0,                      // security
            0,                      // stack size
            MsgBoxThread,
            (void *)&msg,           // arg
            0,                      // flags
            &dwid);
        if(hThread)
        {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
            return msg.iResult;
        }

        // break into debugger on failure.
        return IDCANCEL;
    }
}

/* Displays a message box if the condition evaluated to FALSE */

void WINAPI DbgAssert(const TCHAR *pCondition,const TCHAR *pFileName,INT iLine)
{
    if(g_fUseKASSERT)
    {
        DbgKernelAssert(pCondition, pFileName, iLine);
    }
    else
    {

        TCHAR szInfo[iDEBUGINFO];

        (void)StringCchPrintf(szInfo, NUMELMS(szInfo), TEXT("%s \nAt line %d of %s\nContinue? (Cancel to debug)"),
                 pCondition, iLine, pFileName);

        INT MsgId = MessageBoxOtherThread(NULL,szInfo,TEXT("ASSERT Failed"),
                                          MB_SYSTEMMODAL |
                                          MB_ICONHAND |
                                          MB_YESNOCANCEL |
                                          MB_SETFOREGROUND);
        switch (MsgId)
        {
          case IDNO:              /* Kill the application */

              FatalAppExit(FALSE, TEXT("Application terminated"));
              break;

          case IDCANCEL:          /* Break into the debugger */

              DebugBreak();
              break;

          case IDYES:             /* Ignore assertion continue execution */
              break;
        }
    }
}

/* Displays a message box at a break point */

void WINAPI DbgBreakPoint(const TCHAR *pCondition,const TCHAR *pFileName,INT iLine)
{
    if(g_fUseKASSERT)
    {
        DbgKernelAssert(pCondition, pFileName, iLine);
    }
    else
    {
        TCHAR szInfo[iDEBUGINFO];

        (void)StringCchPrintf(szInfo, NUMELMS(szInfo), TEXT("%s \nAt line %d of %s\nContinue? (Cancel to debug)"),
                 pCondition, iLine, pFileName);

        INT MsgId = MessageBoxOtherThread(NULL,szInfo,TEXT("Hard coded break point"),
                                          MB_SYSTEMMODAL |
                                          MB_ICONHAND |
                                          MB_YESNOCANCEL |
                                          MB_SETFOREGROUND);
        switch (MsgId)
        {
          case IDNO:              /* Kill the application */

              FatalAppExit(FALSE, TEXT("Application terminated"));
              break;

          case IDCANCEL:          /* Break into the debugger */

              DebugBreak();
              break;

          case IDYES:             /* Ignore break point continue execution */
              break;
        }
    }
}

void WINAPI DbgBreakPoint(const TCHAR *pFileName,INT iLine,const TCHAR* szFormatString,...)
{
    // A debug break point message can have at most 2000 characters if 
    // ANSI or UNICODE characters are being used.  A debug break point message
    // can have between 1000 and 2000 double byte characters in it.  If a 
    // particular message needs more characters, then the value of this constant
    // should be increased.
    const DWORD MAX_BREAK_POINT_MESSAGE_SIZE = 2000;

    TCHAR szBreakPointMessage[MAX_BREAK_POINT_MESSAGE_SIZE];
    
    const DWORD MAX_CHARS_IN_BREAK_POINT_MESSAGE = sizeof(szBreakPointMessage) / sizeof(TCHAR);

    va_list va;
    va_start( va, szFormatString );

    HRESULT hr = StringCchVPrintf( szBreakPointMessage, MAX_CHARS_IN_BREAK_POINT_MESSAGE, szFormatString, va );

    va_end(va);
    
    if( S_OK != hr ) {
        DbgBreak( "ERROR in DbgBreakPoint().  The variable length debug message could not be displayed because _vsnprintf() failed." );
        return;
    }

    ::DbgBreakPoint( szBreakPointMessage, pFileName, iLine );
}


/* When we initialised the library we stored in the m_Levels array the current
   debug output level for this module for each of the five categories. When
   some debug logging is sent to us it can be sent with a combination of the
   categories (if it is applicable to many for example) in which case we map
   the type's categories into their current debug levels and see if any of
   them can be accepted. The function looks at each bit position in turn from
   the input type field and then compares it's debug level with the modules.

   A level of 0 means that output is always sent to the debugger.  This is
   due to producing output if the input level is <= m_Levels.
*/


BOOL WINAPI DbgCheckModuleLevel(DWORD Type,DWORD Level)
{
    if(g_fAutoRefreshLevels)
    {
        // re-read the registry every second. We cannot use RegNotify() to
        // notice registry changes because it's not available on win9x.
        static DWORD g_dwLastRefresh = 0;
        DWORD dwTime = timeGetTime();
        if(dwTime - g_dwLastRefresh > 1000) {
            g_dwLastRefresh = dwTime;

            // there's a race condition: multiple threads could update the
            // values. plus read and write not synchronized. no harm
            // though.
            DbgInitModuleSettings(false);
        }
    }


    DWORD Mask = 0x01;

    // If no valid bits are set return FALSE
    if ((Type & ((1<<iMAXLEVELS)-1))) {

	// speed up unconditional output.
	if (0==Level)
	    return(TRUE);
	
        for (LONG lKeyPos = 0;lKeyPos < iMAXLEVELS;lKeyPos++) {
            if (Type & Mask) {
                if (Level <= (m_Levels[lKeyPos] & ~LOG_FORCIBLY_SET)) {
                    return TRUE;
                }
            }
            Mask <<= 1;
        }
    }
    return FALSE;
}


/* Set debug levels to a given value */

void WINAPI DbgSetModuleLevel(DWORD Type, DWORD Level)
{
    DWORD Mask = 0x01;

    for (LONG lKeyPos = 0;lKeyPos < iMAXLEVELS;lKeyPos++) {
        if (Type & Mask) {
            m_Levels[lKeyPos] = Level | LOG_FORCIBLY_SET;
        }
        Mask <<= 1;
    }
}

/* whether to check registry values periodically. this isn't turned
   automatically because of the potential performance hit. */
void WINAPI DbgSetAutoRefreshLevels(bool fAuto)
{
    g_fAutoRefreshLevels = fAuto;
}

#ifdef UNICODE
// 
// warning -- this function is implemented twice for ansi applications
// linking to the unicode library
// 
void WINAPI DbgLogInfo(DWORD Type,DWORD Level,const CHAR *pFormat,...)
{
    /* Check the current level for this type combination */

    BOOL bAccept = DbgCheckModuleLevel(Type,Level);
    if (bAccept == FALSE) {
        return;
    }

    TCHAR szInfo[2000];

    /* Format the variable length parameter list */

    va_list va;
    va_start(va, pFormat);

    (void)StringCchCopy(szInfo,NUMELMS(szInfo),m_ModuleName);
    size_t len = lstrlen(szInfo);
    (void)StringCchPrintf(szInfo + len,
             NUMELMS(szInfo) - len,
             TEXT("(tid %x) %8d : "),
             GetCurrentThreadId(), timeGetTime() - dwTimeOffset);

    CHAR szInfoA[2000];
    WideCharToMultiByte(CP_ACP, 0, szInfo, -1, szInfoA, NUMELMS(szInfoA), 0, 0);

    len = lstrlenA(szInfoA);
    (void)StringCchVPrintfA(szInfoA + len, NUMELMS(szInfoA) - len, pFormat, va);
    len = lstrlenA(szInfoA);
    (void)StringCchCatA(szInfoA, NUMELMS(szInfoA) - len,  "\r\n");

    WCHAR wszOutString[2000];
    MultiByteToWideChar(CP_ACP, 0, szInfoA, -1, wszOutString, NUMELMS(wszOutString));
    DbgOutString(wszOutString);

    va_end(va);
}


void WINAPI DbgAssert(const CHAR *pCondition,const CHAR *pFileName,INT iLine)
{
    if(g_fUseKASSERT)
    {
        DbgKernelAssert(pCondition, pFileName, iLine);
    }
    else
    {

        TCHAR szInfo[iDEBUGINFO];

        (void)StringCchPrintf(szInfo, NUMELMS(szInfo),TEXT("%S \nAt line %d of %S\nContinue? (Cancel to debug)"),
                 pCondition, iLine, pFileName);

        INT MsgId = MessageBoxOtherThread(NULL,szInfo,TEXT("ASSERT Failed"),
                                          MB_SYSTEMMODAL |
                                          MB_ICONHAND |
                                          MB_YESNOCANCEL |
                                          MB_SETFOREGROUND);
        switch (MsgId)
        {
          case IDNO:              /* Kill the application */

              FatalAppExit(FALSE, TEXT("Application terminated"));
              break;

          case IDCANCEL:          /* Break into the debugger */

              DebugBreak();
              break;

          case IDYES:             /* Ignore assertion continue execution */
              break;
        }
    }
}

/* Displays a message box at a break point */

void WINAPI DbgBreakPoint(const CHAR *pCondition,const CHAR *pFileName,INT iLine)
{
    if(g_fUseKASSERT)
    {
        DbgKernelAssert(pCondition, pFileName, iLine);
    }
    else
    {
        TCHAR szInfo[iDEBUGINFO];

        (void)StringCchPrintf(szInfo, NUMELMS(szInfo),TEXT("%S \nAt line %d of %S\nContinue? (Cancel to debug)"),
                 pCondition, iLine, pFileName);

        INT MsgId = MessageBoxOtherThread(NULL,szInfo,TEXT("Hard coded break point"),
                                          MB_SYSTEMMODAL |
                                          MB_ICONHAND |
                                          MB_YESNOCANCEL |
                                          MB_SETFOREGROUND);
        switch (MsgId)
        {
          case IDNO:              /* Kill the application */

              FatalAppExit(FALSE, TEXT("Application terminated"));
              break;

          case IDCANCEL:          /* Break into the debugger */

              DebugBreak();
              break;

          case IDYES:             /* Ignore break point continue execution */
              break;
        }
    }
}

void WINAPI DbgKernelAssert(const CHAR *pCondition,const CHAR *pFileName,INT iLine)
{
    DbgLog((LOG_ERROR,0,TEXT("Assertion FAILED (%hs) at line %d in file %hs"),
           pCondition, iLine, pFileName));
    DebugBreak();
}

#endif

/* Print a formatted string to the debugger prefixed with this module's name
   Because the COMBASE classes are linked statically every module loaded will
   have their own copy of this code. It therefore helps if the module name is
   included on the output so that the offending code can be easily found */

// 
// warning -- this function is implemented twice for ansi applications
// linking to the unicode library
// 
void WINAPI DbgLogInfo(DWORD Type,DWORD Level,const TCHAR *pFormat,...)
{
    
    /* Check the current level for this type combination */

    BOOL bAccept = DbgCheckModuleLevel(Type,Level);
    if (bAccept == FALSE) {
        return;
    }

    TCHAR szInfo[2000];

    /* Format the variable length parameter list */

    va_list va;
    va_start(va, pFormat);

    (void)StringCchCopy(szInfo, NUMELMS(szInfo), m_ModuleName);
    size_t len = lstrlen(szInfo);
    (void)StringCchPrintf(szInfo + len, NUMELMS(szInfo) - len,
             TEXT("(tid %x) %8d : "),
             GetCurrentThreadId(), timeGetTime() - dwTimeOffset);
    len = lstrlen(szInfo);

    (void)StringCchVPrintf(szInfo + len, NUMELMS(szInfo) - len, pFormat, va);

    (void)StringCchCat(szInfo, NUMELMS(szInfo), TEXT("\r\n"));
    DbgOutString(szInfo);

    va_end(va);
}


/* If we are executing as a pure kernel filter we cannot display message
   boxes to the user, this provides an alternative which puts the error
   condition on the debugger output with a suitable eye catching message */

void WINAPI DbgKernelAssert(const TCHAR *pCondition,const TCHAR *pFileName,INT iLine)
{
    DbgLog((LOG_ERROR,0,TEXT("Assertion FAILED (%s) at line %d in file %s"),
           pCondition, iLine, pFileName));
    DebugBreak();
}



/* Each time we create an object derived from CBaseObject the constructor will
   call us to register the creation of the new object. We are passed a string
   description which we store away. We return a cookie that the constructor
   uses to identify the object when it is destroyed later on. We update the
   total number of active objects in the DLL mainly for debugging purposes */

DWORD WINAPI DbgRegisterObjectCreation(const CHAR *szObjectName,
                                       const WCHAR *wszObjectName)
{
    /* If this fires you have a mixed DEBUG/RETAIL build */

    ASSERT(!!szObjectName ^ !!wszObjectName);

    /* Create a place holder for this object description */

    ObjectDesc *pObject = new ObjectDesc;
    ASSERT(pObject);

    /* It is valid to pass a NULL object name */
    if (pObject == NULL) {
        return FALSE;
    }

    /* Check we have been initialised - we may not be initialised when we are
       being pulled in from an executable which has globally defined objects
       as they are created by the C++ run time before WinMain is called */

    if (m_bInit == FALSE) {
        DbgInitialise(GetModuleHandle(NULL));
    }

    /* Grab the list critical section */
    EnterCriticalSection(&m_CSDebug);

    /* If no name then default to UNKNOWN */
    if (!szObjectName && !wszObjectName) {
        szObjectName = pUnknownName;
    }

    /* Put the new description at the head of the list */

    pObject->m_szName = szObjectName;
    pObject->m_wszName = wszObjectName;
    pObject->m_dwCookie = ++m_dwNextCookie;
    pObject->m_pNext = pListHead;

    pListHead = pObject;
    m_dwObjectCount++;

    DWORD ObjectCookie = pObject->m_dwCookie;
    ASSERT(ObjectCookie);

    if(wszObjectName) {
        DbgLog((LOG_MEMORY,2,TEXT("Object created   %d (%ls) %d Active"),
                pObject->m_dwCookie, wszObjectName, m_dwObjectCount));
    } else {
        DbgLog((LOG_MEMORY,2,TEXT("Object created   %d (%hs) %d Active"),
                pObject->m_dwCookie, szObjectName, m_dwObjectCount));
    }

    LeaveCriticalSection(&m_CSDebug);
    return ObjectCookie;
}


/* This is called by the CBaseObject destructor when an object is about to be
   destroyed, we are passed the cookie we returned during construction that
   identifies this object. We scan the object list for a matching cookie and
   remove the object if successful. We also update the active object count */

BOOL WINAPI DbgRegisterObjectDestruction(DWORD dwCookie)
{
    /* Grab the list critical section */
    EnterCriticalSection(&m_CSDebug);

    ObjectDesc *pObject = pListHead;
    ObjectDesc *pPrevious = NULL;

    /* Scan the object list looking for a cookie match */

    while (pObject) {
        if (pObject->m_dwCookie == dwCookie) {
            break;
        }
        pPrevious = pObject;
        pObject = pObject->m_pNext;
    }

    if (pObject == NULL) {
        DbgBreak("Apparently destroying a bogus object");
        LeaveCriticalSection(&m_CSDebug);
        return FALSE;
    }

    /* Is the object at the head of the list */

    if (pPrevious == NULL) {
        pListHead = pObject->m_pNext;
    } else {
        pPrevious->m_pNext = pObject->m_pNext;
    }

    /* Delete the object and update the housekeeping information */

    m_dwObjectCount--;

    if(pObject->m_wszName) {
        DbgLog((LOG_MEMORY,2,TEXT("Object destroyed %d (%ls) %d Active"),
                pObject->m_dwCookie, pObject->m_wszName, m_dwObjectCount));
    } else {
        DbgLog((LOG_MEMORY,2,TEXT("Object destroyed %d (%hs) %d Active"),
                pObject->m_dwCookie, pObject->m_szName, m_dwObjectCount));
    }

    delete pObject;
    LeaveCriticalSection(&m_CSDebug);
    return TRUE;
}


/* This runs through the active object list displaying their details */

void WINAPI DbgDumpObjectRegister()
{
    TCHAR szInfo[iDEBUGINFO];

    /* Grab the list critical section */

    EnterCriticalSection(&m_CSDebug);
    ObjectDesc *pObject = pListHead;

    /* Scan the object list displaying the name and cookie */

    DbgLog((LOG_MEMORY,2,TEXT("")));
    DbgLog((LOG_MEMORY,2,TEXT("   ID             Object Description")));
    DbgLog((LOG_MEMORY,2,TEXT("")));

    while (pObject) {
        if(pObject->m_wszName) {
            #ifdef UNICODE
            LPCTSTR FORMAT_STRING = TEXT("%5d (%8x) %30s");
            #else 
            LPCTSTR FORMAT_STRING = TEXT("%5d (%8x) %30S");
            #endif 

            (void)StringCchPrintf(szInfo,NUMELMS(szInfo), FORMAT_STRING, pObject->m_dwCookie, &pObject, pObject->m_wszName);

        } else {
            #ifdef UNICODE
            LPCTSTR FORMAT_STRING = TEXT("%5d (%8x) %30S");
            #else 
            LPCTSTR FORMAT_STRING = TEXT("%5d (%8x) %30s");
            #endif 

            (void)StringCchPrintf(szInfo,NUMELMS(szInfo),FORMAT_STRING,pObject->m_dwCookie, &pObject, pObject->m_szName);
        }
        DbgLog((LOG_MEMORY,2,szInfo));
        pObject = pObject->m_pNext;
    }

    (void)StringCchPrintf(szInfo,NUMELMS(szInfo),TEXT("Total object count %5d"),m_dwObjectCount);
    DbgLog((LOG_MEMORY,2,TEXT("")));
    DbgLog((LOG_MEMORY,1,szInfo));
    LeaveCriticalSection(&m_CSDebug);
}

/*  Debug infinite wait stuff */
DWORD WINAPI DbgWaitForSingleObject(HANDLE h)
{
    DWORD dwWaitResult;
    do {
        dwWaitResult = WaitForSingleObject(h, dwWaitTimeout);
        ASSERT(dwWaitResult == WAIT_OBJECT_0);
    } while (dwWaitResult == WAIT_TIMEOUT);
    return dwWaitResult;
}
DWORD WINAPI DbgWaitForMultipleObjects(DWORD nCount,
                                CONST HANDLE *lpHandles,
                                BOOL bWaitAll)
{
    DWORD dwWaitResult;
    do {
        dwWaitResult = WaitForMultipleObjects(nCount,
                                              lpHandles,
                                              bWaitAll,
                                              dwWaitTimeout);
        ASSERT((DWORD)(dwWaitResult - WAIT_OBJECT_0) < MAXIMUM_WAIT_OBJECTS);
    } while (dwWaitResult == WAIT_TIMEOUT);
    return dwWaitResult;
}

void WINAPI DbgSetWaitTimeout(DWORD dwTimeout)
{
    dwWaitTimeout = dwTimeout;
}

#endif /* DEBUG */

#ifdef _OBJBASE_H_

    /*  Stuff for printing out our GUID names */

    GUID_STRING_ENTRY g_GuidNames[] = {
    #define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    { #name, { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } },
        #include <uuids.h>
    };

    CGuidNameList GuidNames;
    int g_cGuidNames = sizeof(g_GuidNames) / sizeof(g_GuidNames[0]);

    char *CGuidNameList::operator [] (const GUID &guid)
    {
        for (int i = 0; i < g_cGuidNames; i++) {
            if (g_GuidNames[i].guid == guid) {
                return g_GuidNames[i].szName;
            }
        }
        if (guid == GUID_NULL) {
            return "GUID_NULL";
        }

	// !!! add something to print FOURCC guids?
	
	// shouldn't this print the hex CLSID?
        return "Unknown GUID Name";
    }

#endif /* _OBJBASE_H_ */

/*  CDisp class - display our data types */

// clashes with REFERENCE_TIME
CDisp::CDisp(LONGLONG ll, int Format)
{
    // note: this could be combined with CDisp(LONGLONG) by
    // introducing a default format of CDISP_REFTIME
    LARGE_INTEGER li;
    li.QuadPart = ll;
    switch (Format) {
	case CDISP_DEC:
	{
	    TCHAR  temp[20];
	    int pos=20;
	    temp[--pos] = 0;
	    int digit;
	    // always output at least one digit
	    do {
		// Get the rightmost digit - we only need the low word
	        digit = li.LowPart % 10;
		li.QuadPart /= 10;
		temp[--pos] = (TCHAR) digit+L'0';
	    } while (li.QuadPart);
	    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%s"), temp+pos);
	    break;
	}
	case CDISP_HEX:
	default:
	    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("0x%X%8.8X"), li.HighPart, li.LowPart);
    }
};

CDisp::CDisp(REFCLSID clsid)
{
    WCHAR strClass[CHARS_IN_GUID+1];
    StringFromGUID2(clsid, strClass, sizeof(strClass) / sizeof(strClass[0]));
    ASSERT(sizeof(m_String)/sizeof(m_String[0]) >= CHARS_IN_GUID+1);
    #ifdef UNICODE 
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%s"), strClass);
    #else
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%S"), strClass);
    #endif  
};

#ifdef __STREAMS__
/*  Display stuff */
CDisp::CDisp(CRefTime llTime)
{
    LPTSTR lpsz = m_String;
    size_t len = NUMELMS(m_String);
    LONGLONG llDiv;
    if (llTime < 0) {
        llTime = -llTime;
        (void)StringCchPrintf(lpsz, len, TEXT("-"));
        size_t t = lstrlen(lpsz);
        lpsz += t;
        len -= t;
    }
    llDiv = (LONGLONG)24 * 3600 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(lpsz, len, TEXT("%d days "), (LONG)(llTime / llDiv));
        size_t t = lstrlen(lpsz);
        lpsz += t;
        len -= t;
        llTime = llTime % llDiv;
    }
    llDiv = (LONGLONG)3600 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(lpsz, len, TEXT("%d hrs "), (LONG)(llTime / llDiv));
        size_t t = lstrlen(lpsz);
        lpsz += t;
        len -= t;
        llTime = llTime % llDiv;
    }
    llDiv = (LONGLONG)60 * 10000000;
    if (llTime >= llDiv) {
        (void)StringCchPrintf(lpsz, len,  TEXT("%d mins "), (LONG)(llTime / llDiv));
        size_t t = lstrlen(lpsz);
        lpsz += t;
        len -= t;
        llTime = llTime % llDiv;
    }
    (void)StringCchPrintf(lpsz, len, TEXT("%d.%3.3d sec"),
             (LONG)llTime / 10000000,
             (LONG)((llTime % 10000000) / 10000));
};

#endif // __STREAMS__


/*  Display pin */
CDisp::CDisp(IPin *pPin)
{
    PIN_INFO pi;
    TCHAR str[MAX_PIN_NAME];
    CLSID clsid;

    if (pPin) {
       pPin->QueryPinInfo(&pi);
       pi.pFilter->GetClassID(&clsid);
       QueryPinInfoReleaseFilter(pi);
      #ifndef UNICODE
       WideCharToMultiByte(GetACP(), 0, pi.achName, lstrlenW(pi.achName) + 1,
                           str, MAX_PIN_NAME, NULL, NULL);
      #else
       (void)StringCchCopy(str, NUMELMS(str), pi.achName);
      #endif
    } else {
       (void)StringCchCopy(str, NUMELMS(str), TEXT("NULL IPin"));
    }

    size_t len = lstrlen(str)+64;
    m_pString = (PTCHAR) new TCHAR[len];
    if (!m_pString) {
	return;
    }

    #ifdef UNICODE
    LPCTSTR FORMAT_STRING = TEXT("%S(%s)");
    #else
    LPCTSTR FORMAT_STRING = TEXT("%s(%s)");
    #endif 

    (void)StringCchPrintf(m_pString, len, FORMAT_STRING, GuidNames[clsid], str);
}

/*  Display filter or pin */
CDisp::CDisp(IUnknown *pUnk)
{
    IBaseFilter *pf;
    HRESULT hr = pUnk->QueryInterface(IID_IBaseFilter, (void **)&pf);
    if(SUCCEEDED(hr))
    {
        FILTER_INFO fi;
        hr = pf->QueryFilterInfo(&fi);
        if(SUCCEEDED(hr))
        {
            QueryFilterInfoReleaseGraph(fi);

            size_t len = lstrlenW(fi.achName)  + 1;
            m_pString = new TCHAR[len];
            if(m_pString)
            {
                #ifdef UNICODE
                LPCTSTR FORMAT_STRING = TEXT("%s");
                #else
                LPCTSTR FORMAT_STRING = TEXT("%S");
                #endif 

                (void)StringCchPrintf(m_pString, len, FORMAT_STRING, fi.achName);
            }
        }

        pf->Release();

        return;
    }

    IPin *pp;
    hr = pUnk->QueryInterface(IID_IPin, (void **)&pp);
    if(SUCCEEDED(hr))
    {
        CDisp::CDisp(pp);
        pp->Release();
        return;
    }
}


CDisp::~CDisp()
{
}

CDispBasic::~CDispBasic()
{
    if (m_pString != m_String) {
	delete [] m_pString;
    }
}

CDisp::CDisp(double d)
{
#ifdef DEBUG
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%.16g"), d);
#else
    (void)StringCchPrintf(m_String, NUMELMS(m_String), TEXT("%d.%03d"), (int) d, (int) ((d - (int) d) * 1000));
#endif
}


/* If built for debug this will display the media type details. We convert the
   major and subtypes into strings and also ask the base classes for a string
   description of the subtype, so MEDIASUBTYPE_RGB565 becomes RGB 565 16 bit
   We also display the fields in the BITMAPINFOHEADER structure, this should
   succeed as we do not accept input types unless the format is big enough */

#ifdef DEBUG
void WINAPI DisplayType(LPTSTR label, const AM_MEDIA_TYPE *pmtIn)
{

    /* Dump the GUID types and a short description */

    DbgLog((LOG_TRACE,5,TEXT("")));
    DbgLog((LOG_TRACE,2,TEXT("%s  M type %hs  S type %hs"), label,
	    GuidNames[pmtIn->majortype],
	    GuidNames[pmtIn->subtype]));
    DbgLog((LOG_TRACE,5,TEXT("Subtype description %s"),GetSubtypeName(&pmtIn->subtype)));

    /* Dump the generic media types */

    if (pmtIn->bTemporalCompression) {
	DbgLog((LOG_TRACE,5,TEXT("Temporally compressed")));
    } else {
	DbgLog((LOG_TRACE,5,TEXT("Not temporally compressed")));
    }

    if (pmtIn->bFixedSizeSamples) {
	DbgLog((LOG_TRACE,5,TEXT("Sample size %d"),pmtIn->lSampleSize));
    } else {
	DbgLog((LOG_TRACE,5,TEXT("Variable size samples")));
    }

    if (pmtIn->formattype == FORMAT_VideoInfo) {
	/* Dump the contents of the BITMAPINFOHEADER structure */
	BITMAPINFOHEADER *pbmi = HEADER(pmtIn->pbFormat);
	VIDEOINFOHEADER *pVideoInfo = (VIDEOINFOHEADER *)pmtIn->pbFormat;

	DbgLog((LOG_TRACE,5,TEXT("Source rectangle (Left %d Top %d Right %d Bottom %d)"),
	       pVideoInfo->rcSource.left,
	       pVideoInfo->rcSource.top,
	       pVideoInfo->rcSource.right,
	       pVideoInfo->rcSource.bottom));

	DbgLog((LOG_TRACE,5,TEXT("Target rectangle (Left %d Top %d Right %d Bottom %d)"),
	       pVideoInfo->rcTarget.left,
	       pVideoInfo->rcTarget.top,
	       pVideoInfo->rcTarget.right,
	       pVideoInfo->rcTarget.bottom));

	DbgLog((LOG_TRACE,5,TEXT("Size of BITMAPINFO structure %d"),pbmi->biSize));
	if (pbmi->biCompression < 256) {
	    DbgLog((LOG_TRACE,2,TEXT("%dx%dx%d bit  (%d)"),
		    pbmi->biWidth, pbmi->biHeight,
		    pbmi->biBitCount, pbmi->biCompression));
	} else {
	    DbgLog((LOG_TRACE,2,TEXT("%dx%dx%d bit '%4.4hs'"),
		    pbmi->biWidth, pbmi->biHeight,
		    pbmi->biBitCount, &pbmi->biCompression));
	}

	DbgLog((LOG_TRACE,2,TEXT("Image size %d"),pbmi->biSizeImage));
	DbgLog((LOG_TRACE,5,TEXT("Planes %d"),pbmi->biPlanes));
	DbgLog((LOG_TRACE,5,TEXT("X Pels per metre %d"),pbmi->biXPelsPerMeter));
	DbgLog((LOG_TRACE,5,TEXT("Y Pels per metre %d"),pbmi->biYPelsPerMeter));
	DbgLog((LOG_TRACE,5,TEXT("Colours used %d"),pbmi->biClrUsed));

    } else if (pmtIn->majortype == MEDIATYPE_Audio) {
	DbgLog((LOG_TRACE,2,TEXT("     Format type %hs"),
	    GuidNames[pmtIn->formattype]));
	DbgLog((LOG_TRACE,2,TEXT("     Subtype %hs"),
	    GuidNames[pmtIn->subtype]));

	if ((pmtIn->subtype != MEDIASUBTYPE_MPEG1Packet)
	  && (pmtIn->cbFormat >= sizeof(PCMWAVEFORMAT)))
	{
	    /* Dump the contents of the WAVEFORMATEX type-specific format structure */

	    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pmtIn->pbFormat;
            DbgLog((LOG_TRACE,2,TEXT("wFormatTag %u"), pwfx->wFormatTag));
            DbgLog((LOG_TRACE,2,TEXT("nChannels %u"), pwfx->nChannels));
            DbgLog((LOG_TRACE,2,TEXT("nSamplesPerSec %lu"), pwfx->nSamplesPerSec));
            DbgLog((LOG_TRACE,2,TEXT("nAvgBytesPerSec %lu"), pwfx->nAvgBytesPerSec));
            DbgLog((LOG_TRACE,2,TEXT("nBlockAlign %u"), pwfx->nBlockAlign));
            DbgLog((LOG_TRACE,2,TEXT("wBitsPerSample %u"), pwfx->wBitsPerSample));

            /* PCM uses a WAVEFORMAT and does not have the extra size field */

            if (pmtIn->cbFormat >= sizeof(WAVEFORMATEX)) {
                DbgLog((LOG_TRACE,2,TEXT("cbSize %u"), pwfx->cbSize));
            }
	} else {
	}

    } else {
	DbgLog((LOG_TRACE,2,TEXT("     Format type %hs"),
	    GuidNames[pmtIn->formattype]));
	// !!!! should add code to dump wave format, others
    }
}


void WINAPI DumpGraph(IFilterGraph *pGraph, DWORD dwLevel)
{
    if( !pGraph )
    {
        return;
    }

    IEnumFilters *pFilters;

    DbgLog((LOG_TRACE,dwLevel,TEXT("DumpGraph [%x]"), pGraph));

    if (FAILED(pGraph->EnumFilters(&pFilters))) {
	DbgLog((LOG_TRACE,dwLevel,TEXT("EnumFilters failed!")));
    }

    IBaseFilter *pFilter;
    ULONG	n;
    while (pFilters->Next(1, &pFilter, &n) == S_OK) {
	FILTER_INFO	info;

	if (FAILED(pFilter->QueryFilterInfo(&info))) {
	    DbgLog((LOG_TRACE,dwLevel,TEXT("    Filter [%x]  -- failed QueryFilterInfo"), pFilter));
	} else {
	    QueryFilterInfoReleaseGraph(info);

	    // !!! should QueryVendorInfo here!
	
	    DbgLog((LOG_TRACE,dwLevel,TEXT("    Filter [%x]  '%ls'"), pFilter, info.achName));

	    IEnumPins *pins;

	    if (FAILED(pFilter->EnumPins(&pins))) {
		DbgLog((LOG_TRACE,dwLevel,TEXT("EnumPins failed!")));
	    } else {

		IPin *pPin;
		while (pins->Next(1, &pPin, &n) == S_OK) {
		    PIN_INFO	info;

		    if (FAILED(pPin->QueryPinInfo(&info))) {
			DbgLog((LOG_TRACE,dwLevel,TEXT("          Pin [%x]  -- failed QueryPinInfo"), pPin));
		    } else {
			QueryPinInfoReleaseFilter(info);

			IPin *pPinConnected = NULL;

			HRESULT hr = pPin->ConnectedTo(&pPinConnected);

			if (pPinConnected) {
			    DbgLog((LOG_TRACE,dwLevel,TEXT("          Pin [%x]  '%ls' [%sput]")
							   TEXT("  Connected to pin [%x]"),
				    pPin, info.achName,
				    info.dir == PINDIR_INPUT ? TEXT("In") : TEXT("Out"),
				    pPinConnected));

			    pPinConnected->Release();

			    // perhaps we should really dump the type both ways as a sanity
			    // check?
			    if (info.dir == PINDIR_OUTPUT) {
				AM_MEDIA_TYPE mt;

				hr = pPin->ConnectionMediaType(&mt);

				if (SUCCEEDED(hr)) {
				    DisplayType(TEXT("Connection type"), &mt);

				    FreeMediaType(mt);
				}
			    }
			} else {
			    DbgLog((LOG_TRACE,dwLevel,
				    TEXT("          Pin [%x]  '%ls' [%sput]"),
				    pPin, info.achName,
				    info.dir == PINDIR_INPUT ? TEXT("In") : TEXT("Out")));

			}
		    }

		    pPin->Release();

		}

		pins->Release();
	    }

	}
	
	pFilter->Release();
    }

    pFilters->Release();

}

#endif

