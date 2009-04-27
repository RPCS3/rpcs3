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

#ifndef _PCSX2_WIN32_H__
#define _PCSX2_WIN32_H__

#include "RedtapeWindows.h"		// our "safe" include of windows (sets version and undefs uselessness)
#include <commctrl.h>

#include "PrecompiledHeader.h"

#include <windowsx.h>
#include <tchar.h>

#include "System.h"
#include "HostGui.h"
#include "resource.h"
#include "WinDebugResource.h"

#define COMPILEDATE         __DATE__

//Exception handler for the VTLB-based recompilers.
int SysPageFaultExceptionFilter(EXCEPTION_POINTERS* eps);

#define PCSX2_MEM_PROTECT_BEGIN() __try {
#define PCSX2_MEM_PROTECT_END() } __except(SysPageFaultExceptionFilter(GetExceptionInformation())) {}


// --->>  Ini Configuration [ini.c]

extern char g_WorkingFolder[g_MaxPath];
extern const char* g_CustomConfigFile;

bool LoadConfig();
void SaveConfig();

// <<--- END Ini Configuration [ini.c]

// --->>  Patch Browser Stuff (in the event we ever use it

void ListPatches (HWND hW);
int ReadPatch (HWND hW, char fileName[1024]);
char * lTrim (char *s);
BOOL Save_Patch_Proc( char * filename );

// <<--- END Patch Browser

struct AppData
{
	HWND hWnd;           // Main window handle
	HINSTANCE hInstance; // Application instance
	HMENU hMenu;         // Main window menu
};

class IniFile
{
protected:
	wxString m_filename;
	wxString m_section;

public:
	virtual ~IniFile();
	IniFile();

	void SetCurrentSection( const wxString& newsection );

	virtual void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() )=0;
	virtual void Entry( const wxString& var, char (&value)[g_MaxPath], const wxString& defvalue=wxString() )=0;
	virtual void Entry( const wxString& var, int& value, const int defvalue=0 )=0;
	virtual void Entry( const wxString& var, uint& value, const uint defvalue=0 )=0;
	virtual void Entry( const wxString& var, bool& value, const bool defvalue=0 )=0;
	virtual void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 )=0;

	void DoConfig( PcsxConfig& Conf );
	void MemcardSettings( PcsxConfig& Conf );
};

class IniFileLoader : public IniFile
{
protected:
	SafeArray<char> m_workspace;

public:
	virtual ~IniFileLoader();
	IniFileLoader();

	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, char (&value)[g_MaxPath], const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );
	void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 );
};

class IniFileSaver : public IniFile
{
public:
	virtual ~IniFileSaver();
	IniFileSaver();
	
	void Entry( const wxString& var, const wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, char (&value)[g_MaxPath], const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );
	void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 );
};

extern LRESULT WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);
extern void CreateMainWindow();
extern void RunGui();
extern bool HostGuiInit();

extern BOOL Pcsx2Configure(HWND hWnd);
extern void InitLanguages();
extern char *GetLanguageNext();
extern void CloseLanguages();
extern void ChangeLanguage(char *lang);

extern void WinClose();
extern void ExecuteCpu();
extern void OnStates_LoadOther();
extern void OnStates_SaveOther();
extern int ParseCommandLine( int tokenCount, TCHAR *const *const tokens );

extern BOOL CALLBACK PatchBDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK CpuDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern BOOL CALLBACK AdvancedOptionsProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam);
//extern BOOL CALLBACK HacksProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

extern AppData gApp;
extern HWND hStatusWnd;
extern PcsxConfig winConfig;		// local storage of the configuration options.

extern bool nDisableSC; // screensaver
extern unsigned int langsMax;

extern SafeArray<u8>* g_RecoveryState;
extern SafeArray<u8>* g_gsRecoveryState;
extern const char* g_pRunGSState;
extern int g_SaveGSStream;

// Throws an exception based on the value returned from GetLastError.
// Performs an option return value success/fail check on hresult.
extern void StreamException_ThrowLastError( const wxString& streamname, HANDLE result=INVALID_HANDLE_VALUE );

// Throws an exception based on the given error code (usually taken from ANSI C's errno)
extern void StreamException_ThrowFromErrno( const wxString& streamname, errno_t errcode );

extern bool StreamException_LogFromErrno( const wxString& streamname, const wxChar* action, errno_t result );
extern bool StreamException_LogLastError( const wxString& streamname, const wxChar* action, HANDLE result=INVALID_HANDLE_VALUE );

// Sets the NTFS compression flag for a directory or file. This function does not operate
// recursively.  Set compressStatus to false to decompress compressed files (and do nothing
// to already decompressed files).
extern void NTFS_CompressFile( const wxString& file, bool compressStatus=true );

#endif

