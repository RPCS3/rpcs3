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

#include "Win32.h"

#include <math.h>
#include "libintlmsc.h"

#include "System.h"
#include "McdsDlg.h"

#include "Sio.h"

HWND mcdDlg;

#define SET_CHECK(idc,value) SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,((value)==0)?BST_UNCHECKED:BST_CHECKED,0)
#define ENABLE_CONTROL(idc,value) EnableWindow(GetDlgItem(hWnd,idc),value)

#define HANDLE_CHECK(idc,hvar)		case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0); break
#define HANDLE_CHECKNB(idc,hvar)	case idc: (hvar) = !(hvar); SendMessage(GetDlgItem(hWnd,idc),BM_SETCHECK,(hvar)?BST_CHECKED:BST_UNCHECKED,0)

void DlgItem_GetText( HWND hwnd, int dlgId, string& dest )
{
	HWND dlg = GetDlgItem( hwnd, dlgId );
	int length = GetWindowTextLength( dlg );

	if( length <= 0 )
	{
		dest.clear();
		return;
	}

	char* outstr = new char[length+2];
	if( outstr != NULL )
	{
		GetWindowText( dlg, outstr, length+1 );
		dest = outstr;
		safe_delete_array( outstr );
	}
}

// strips path information so that absolute paths are reduced to relative paths
// where appropriate.
static const char* _stripPathInfo( const char* src )
{
	const char* retval = src;
	const char* workingfold = g_WorkingFolder;

	while( (*retval != 0) && (*workingfold != 0) && (tolower(*retval) == tolower(*workingfold)) )
	{
		retval++;
		workingfold++;
	}

	// If a difference is found before we reach the end of our pcsx2 working folder, it
	// means we need to use the fully absolute path form the user.
	if( *workingfold != 0 ) return src;

	while( (*retval != 0) && (*retval == '\\') ) retval++;

	return retval;
}

void MemcardConfig::Open_Mcd_Proc(HWND hW, int mcd)
{
	OPENFILENAME ofn;
	char szFileName[256];
	char szFileTitle[256];
	char szFilter[1024];
	char *str;

	memzero_obj( szFileName );
	memzero_obj( szFileTitle );
	memzero_obj( szFilter );

	strcpy(szFilter, _("Ps2 Memory Card (*.ps2)"));
	str = szFilter + strlen(szFilter) + 1; 
	strcpy(str, "*.ps2");

	str += strlen(str) + 1;
	strcpy(str, _("All Files"));
	str += strlen(str) + 1;
	strcpy(str, "*.*");

    ofn.lStructSize			= sizeof(OPENFILENAME);
    ofn.hwndOwner			= hW;
    ofn.lpstrFilter			= szFilter;
	ofn.lpstrCustomFilter	= NULL;
    ofn.nMaxCustFilter		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFileName;
    ofn.nMaxFile			= 256;
    ofn.lpstrInitialDir		= MEMCARDS_DIR;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= 256;
    ofn.lpstrTitle			= NULL;
    ofn.lpstrDefExt			= "PS2";
    ofn.Flags				= OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_EXPLORER;

	if (GetOpenFileName ((LPOPENFILENAME)&ofn))
	{
		Edit_SetText(GetDlgItem(hW,mcd == 1 ? IDC_MCD_FILE1 : IDC_MCD_FILE2),
			_stripPathInfo( Path::Combine( g_WorkingFolder, szFileName ).c_str() ) );
	}
}

static string m_Default_MemcardsDir[2] = 
{
	Path::Combine( MEMCARDS_DIR, DEFAULT_MEMCARD1 ),
	Path::Combine( MEMCARDS_DIR, DEFAULT_MEMCARD2 )
};

BOOL CALLBACK MemcardConfig::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			mcdDlg = hWnd;
			SetWindowText(hWnd, _("MemoryCard Config - Pcsx2"));

			Button_SetText(GetDlgItem(hWnd, IDOK),        _("OK"));
			Button_SetText(GetDlgItem(hWnd, IDCANCEL),    _("Cancel"));

			Button_SetText(GetDlgItem(hWnd, IDC_MCDSEL1), _("Browse..."));
			Button_SetText(GetDlgItem(hWnd, IDC_MCDSEL2), _("Browse..."));

			Static_SetText(GetDlgItem(hWnd, IDC_MCD_ENABLE1), _("Memory Card Slot 1"));
			Static_SetText(GetDlgItem(hWnd, IDC_MCD_ENABLE2), _("Memory Card Slot 2"));

			if( Config.Mcd[0].Filename[0] == 0 )
			{
				strcpy_s( Config.Mcd[0].Filename,
					Path::Combine( g_WorkingFolder, m_Default_MemcardsDir[0] ).c_str() );
			}

			if( Config.Mcd[1].Filename[1] == 0 )
			{
				strcpy_s( Config.Mcd[1].Filename,
					Path::Combine( g_WorkingFolder, m_Default_MemcardsDir[1] ).c_str() );
			}

			Edit_SetText( GetDlgItem(hWnd,IDC_MCD_FILE1), _stripPathInfo( Config.Mcd[0].Filename ) );
			Edit_SetText( GetDlgItem(hWnd,IDC_MCD_FILE2), _stripPathInfo( Config.Mcd[1].Filename ) );

			SET_CHECK( IDC_MCD_ENABLE1, Config.Mcd[0].Enabled );
			SET_CHECK( IDC_MCD_ENABLE2, Config.Mcd[1].Enabled );
			
			SET_CHECK( IDC_NTFS_ENABLE, Config.McdEnableNTFS );
			SET_CHECK( IDC_MCD_EJECT_ENABLE, Config.McdEnableEject );
		}
		break;

		case WM_COMMAND:
		{
			int wmId    = LOWORD(wParam); 
			int wmEvent = HIWORD(wParam); 

			switch (LOWORD(wmId))
			{
				case IDC_MCD_BROWSE1: 
					Open_Mcd_Proc(hWnd, 1); 
				return TRUE;
			
				case IDC_MCD_BROWSE2: 
					Open_Mcd_Proc(hWnd, 2); 
				return TRUE;
       		
       			case IDCANCEL:
					EndDialog(hWnd,FALSE);
				return TRUE;

       			case IDOK:
       			{
       				//DlgItem_GetText( hWnd, IDC_MCD_FILE1, Config.Mcd[0].Filename );
       				//DlgItem_GetText( hWnd, IDC_MCD_FILE2, Config.Mcd[1].Filename );

					string oldone( Config.Mcd[0].Filename ), oldtwo( Config.Mcd[1].Filename );

       				GetWindowText( GetDlgItem( hWnd, IDC_MCD_FILE1 ), Config.Mcd[0].Filename, g_MaxPath );
       				GetWindowText( GetDlgItem( hWnd, IDC_MCD_FILE2 ), Config.Mcd[1].Filename, g_MaxPath );

					// reassign text with the extra unnecessary path info stripped out.

					_tcscpy( Config.Mcd[0].Filename, _stripPathInfo( Path::Combine( g_WorkingFolder, Config.Mcd[0].Filename ).c_str() ) );
					_tcscpy( Config.Mcd[1].Filename, _stripPathInfo( Path::Combine( g_WorkingFolder, Config.Mcd[1].Filename ).c_str() ) );

					if( g_EmulationInProgress )
					{
						if( !Config.Mcd[0].Enabled || oldone != Config.Mcd[0].Filename )
							sioEjectCard( 0 );

						if( !Config.Mcd[1].Enabled || oldtwo != Config.Mcd[1].Filename )
							sioEjectCard( 1 );
					}

					IniFileSaver().MemcardSettings( Config );
					EndDialog( hWnd, TRUE );
				}
				return TRUE;
				
				HANDLE_CHECKNB( IDC_MCD_ENABLE1, Config.Mcd[0].Enabled );
					ENABLE_CONTROL( IDC_MCD_BROWSE1, Config.Mcd[0].Enabled );
					ENABLE_CONTROL( IDC_MCD_FILE1, Config.Mcd[0].Enabled );
					ENABLE_CONTROL( IDC_MCD_LABEL1, Config.Mcd[0].Enabled );
				break;

				HANDLE_CHECKNB( IDC_MCD_ENABLE2, Config.Mcd[1].Enabled );
					ENABLE_CONTROL( IDC_MCD_BROWSE2, Config.Mcd[1].Enabled );
					ENABLE_CONTROL( IDC_MCD_FILE2, Config.Mcd[1].Enabled );
					ENABLE_CONTROL( IDC_MCD_LABEL2, Config.Mcd[1].Enabled );
				break;
				
				HANDLE_CHECK( IDC_NTFS_ENABLE, Config.McdEnableNTFS );
				HANDLE_CHECK( IDC_MCD_EJECT_ENABLE, Config.McdEnableEject );
				
				default:
					return FALSE;
			}
		}
		break;
		
		default:
			return FALSE;
	}
	return TRUE;
}

// Both loads and saves as a single unified settings function! :D
void IniFile::MemcardSettings( PcsxConfig& conf )
{
	SetCurrentSection( "Memorycards" );
	
	Entry( "Slot1_Path", conf.Mcd[0].Filename,
		Path::Combine( g_WorkingFolder, m_Default_MemcardsDir[0] ) );

	Entry( "Slot2_Path", conf.Mcd[1].Filename,
		Path::Combine( g_WorkingFolder, m_Default_MemcardsDir[1] ) );

	Entry( "Slot1_Enabled", conf.Mcd[0].Enabled, true );
	Entry( "Slot2_Enabled", conf.Mcd[1].Enabled, true );
	
	Entry( "EnableNTFS", conf.McdEnableNTFS, true );
	Entry( "EnableEjection", conf.McdEnableEject, true );
}

// Creates the MemoryCard configuration dialog box
void MemcardConfig::OpenDialog()
{
	DialogBox( gApp.hInstance, MAKEINTRESOURCE(IDD_CONF_MEMCARD), gApp.hWnd, (DLGPROC)DialogProc );

	// Re-loads old config if the user canceled.
	IniFileLoader().MemcardSettings( Config );
}