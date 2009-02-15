/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#include "spu2.h"
#include "dialogs.h"

int SoundtouchCfg::SequenceLenMS = 63;
int SoundtouchCfg::SeekWindowMS = 16;
int SoundtouchCfg::OverlapMS = 7;

void SoundtouchCfg::ClampValues()
{
	Clampify( SequenceLenMS, SequenceLen_Min, SequenceLen_Max );
	Clampify( SeekWindowMS, SeekWindow_Min, SeekWindow_Max );
	Clampify( OverlapMS, Overlap_Min, Overlap_Max );
}

void SoundtouchCfg::ReadSettings()
{
	SequenceLenMS	= CfgReadInt( _T("SOUNDTOUCH"), _T("SequenceLengthMS"), 63 );
	SeekWindowMS	= CfgReadInt( _T("SOUNDTOUCH"), _T("SeekWindowMS"), 16 );
	OverlapMS		= CfgReadInt( _T("SOUNDTOUCH"), _T("OverlapMS"), 7 );

	ClampValues();		
}

void SoundtouchCfg::WriteSettings()
{
	CfgWriteInt( _T("SOUNDTOUCH"), _T("SequenceLengthMS"), SequenceLenMS );
	CfgWriteInt( _T("SOUNDTOUCH"), _T("SeekWindowMS"), SeekWindowMS );
	CfgWriteInt( _T("SOUNDTOUCH"), _T("OverlapMS"), OverlapMS );
}

BOOL CALLBACK SoundtouchCfg::DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	wchar_t temp[384]={0};

	switch(uMsg)
	{
		case WM_PAINT:
			return FALSE;

		case WM_INITDIALOG:
		{
			INIT_SLIDER( IDC_SEQLEN_SLIDER, SequenceLen_Min, SequenceLen_Max, 20, 5, 1 );
			INIT_SLIDER( IDC_SEEKWIN_SLIDER, SeekWindow_Min, SeekWindow_Max, 5, 2, 1 );
			INIT_SLIDER( IDC_OVERLAP_SLIDER, Overlap_Min, Overlap_Max, 3, 2, 1 );

			SendDialogMsg( hWnd, IDC_SEQLEN_SLIDER, TBM_SETPOS, TRUE, SequenceLenMS );
			SendDialogMsg( hWnd, IDC_SEEKWIN_SLIDER, TBM_SETPOS, TRUE, SeekWindowMS );
			SendDialogMsg( hWnd, IDC_OVERLAP_SLIDER, TBM_SETPOS, TRUE, OverlapMS );
		}
		
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			if( wmId == IDOK )
			{
				SequenceLenMS	= (int)SendDialogMsg( hWnd, IDC_SEQLEN_SLIDER, TBM_GETPOS, 0, 0 );
				SeekWindowMS	= (int)SendDialogMsg( hWnd, IDC_SEEKWIN_SLIDER, TBM_GETPOS, 0, 0 );
				OverlapMS		= (int)SendDialogMsg( hWnd, IDC_OVERLAP_SLIDER, TBM_GETPOS, 0, 0 );

				ClampValues();
				WriteSettings();
				EndDialog(hWnd,0);
			}
			else if( wmId == IDCANCEL )
			{
				EndDialog(hWnd,0);
			}
		break;
		
		case WM_HSCROLL:
			DoHandleScrollMessage( hWnd, wParam, lParam );
		break;
		
		default:
			return FALSE;
	}
	return TRUE;
}

void SoundtouchCfg::OpenDialog( HWND hWnd )
{
	INT_PTR ret;
	ret = DialogBox( hInstance, MAKEINTRESOURCE(IDD_CONFIG_SOUNDTOUCH), hWnd, (DLGPROC)DialogProc );
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),_T("Error Opening the Soundtouch advanced dialog."),_T("OMG ERROR!"),MB_OK,0);
		return;
	}
	ReadSettings();
}
