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

#ifdef SPU2X_DEVBUILD
static const int LATENCY_MAX = 3000;
#else
static const int LATENCY_MAX = 750;
#endif

static const int LATENCY_MIN = 40;

int AutoDMAPlayRate[2] = {0,0};

// MIXING
int Interpolation = 1;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
*/

bool EffectsDisabled = false;

// OUTPUT
int SndOutLatencyMS = 160;
bool timeStretchDisabled = false;

u32 OutputModule = 0;

CONFIG_DSOUNDOUT Config_DSoundOut;
CONFIG_WAVEOUT Config_WaveOut;
CONFIG_XAUDIO2 Config_XAudio2;

// DSP
bool dspPluginEnabled = false;
int  dspPluginModule = 0;
wchar_t dspPlugin[256];

bool StereoExpansionDisabled = true;

/*****************************************************************************/

void ReadSettings()
{
	AutoDMAPlayRate[0] = CfgReadInt(_T("MIXING"),_T("AutoDMA_Play_Rate_0"),0);
	AutoDMAPlayRate[1] = CfgReadInt(_T("MIXING"),_T("AutoDMA_Play_Rate_1"),0);

	Interpolation = CfgReadInt(_T("MIXING"),_T("Interpolation"),1);

	timeStretchDisabled = CfgReadBool( _T("OUTPUT"), _T("Disable_Timestretch"), false );
	EffectsDisabled = CfgReadBool( _T("MIXING"), _T("Disable_Effects"), false );

	StereoExpansionDisabled = CfgReadBool( _T("OUTPUT"), _T("Disable_StereoExpansion"), false );
	SndOutLatencyMS = CfgReadInt(_T("OUTPUT"),_T("Latency"), 160);

	wchar_t omodid[128];
	CfgReadStr( _T("OUTPUT"), _T("Output_Module"), omodid, 127, XAudio2Out->GetIdent() );

	// find the driver index of this module:
	OutputModule = FindOutputModuleById( omodid );

	CfgReadStr( _T("DSP PLUGIN"),_T("Filename"),dspPlugin,255,_T(""));
	dspPluginModule = CfgReadInt(_T("DSP PLUGIN"),_T("ModuleNum"),0);
	dspPluginEnabled= CfgReadBool(_T("DSP PLUGIN"),_T("Enabled"),false);

	// Read DSOUNDOUT and WAVEOUT configs:
	CfgReadStr( _T("DSOUNDOUT"), _T("Device"), Config_DSoundOut.Device, 254, _T("default") );
	CfgReadStr( _T("WAVEOUT"), _T("Device"), Config_WaveOut.Device, 254, _T("default") );
	Config_DSoundOut.NumBuffers = CfgReadInt( _T("DSOUNDOUT"), _T("Buffer_Count"), 5 );
	Config_WaveOut.NumBuffers = CfgReadInt( _T("WAVEOUT"), _T("Buffer_Count"), 4 );

	SoundtouchCfg::ReadSettings();
	DebugConfig::ReadSettings();

	// Sanity Checks
	// -------------

	Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );
	
	Clampify( Config_DSoundOut.NumBuffers, (s8)2, (s8)8 );
	Clampify( Config_DSoundOut.NumBuffers, (s8)3, (s8)8 );

	if( mods[OutputModule] == NULL )
	{
		// Unsupported or legacy module.
		fprintf( stderr, " * SPU2: Unknown output module '%s' specified in configuration file.\n", omodid );
		fprintf( stderr, " * SPU2: Defaulting to DirectSound (%S).\n", DSoundOut->GetIdent() );
		OutputModule = FindOutputModuleById( DSoundOut->GetIdent() );
	}
}

/*****************************************************************************/

void WriteSettings()
{
	CfgWriteInt(_T("MIXING"),_T("Interpolation"),Interpolation);

	CfgWriteInt(_T("MIXING"),_T("AutoDMA_Play_Rate_0"),AutoDMAPlayRate[0]);
	CfgWriteInt(_T("MIXING"),_T("AutoDMA_Play_Rate_1"),AutoDMAPlayRate[1]);

	CfgWriteBool(_T("MIXING"),_T("Disable_Effects"),EffectsDisabled);

	CfgWriteStr(_T("OUTPUT"),_T("Output_Module"), mods[OutputModule]->GetIdent() );
	CfgWriteInt(_T("OUTPUT"),_T("Latency"), SndOutLatencyMS);
	CfgWriteBool(_T("OUTPUT"),_T("Disable_Timestretch"), timeStretchDisabled);
	CfgWriteBool(_T("OUTPUT"),_T("Disable_StereoExpansion"), StereoExpansionDisabled);

	if( Config_DSoundOut.Device.empty() ) Config_DSoundOut.Device = _T("default");
	if( Config_WaveOut.Device.empty() ) Config_WaveOut.Device = _T("default");

	CfgWriteStr(_T("DSOUNDOUT"),_T("Device"),Config_DSoundOut.Device);
	CfgWriteInt(_T("DSOUNDOUT"),_T("Buffer_Count"),Config_DSoundOut.NumBuffers);

	CfgWriteStr(_T("WAVEOUT"),_T("Device"),Config_WaveOut.Device);
	CfgWriteInt(_T("WAVEOUT"),_T("Buffer_Count"),Config_WaveOut.NumBuffers);

	CfgWriteStr(_T("DSP PLUGIN"),_T("Filename"),dspPlugin);
	CfgWriteInt(_T("DSP PLUGIN"),_T("ModuleNum"),dspPluginModule);
	CfgWriteBool(_T("DSP PLUGIN"),_T("Enabled"),dspPluginEnabled);
	
	SoundtouchCfg::WriteSettings();
	DebugConfig::WriteSettings();

}

BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	wchar_t temp[384]={0};

	switch(uMsg)
	{
		case WM_PAINT:
			return FALSE;

		case WM_INITDIALOG:
		{
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_RESETCONTENT,0,0 ); 
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) _T("0 - Nearest (none/fast)") );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) _T("1 - Linear (recommended)") );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) _T("2 - Cubic (better/slower)") );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_SETCURSEL,Interpolation,0 ); 

			SendDialogMsg( hWnd, IDC_OUTPUT, CB_RESETCONTENT,0,0 );
			
			int modidx = 0;
			while( mods[modidx] != NULL )
			{
				swprintf_s( temp, 72, _T("%d - %s"), modidx, mods[modidx]->GetLongName() );
				SendDialogMsg( hWnd, IDC_OUTPUT, CB_ADDSTRING,0,(LPARAM)temp );
				++modidx;
			}
			SendDialogMsg( hWnd, IDC_OUTPUT, CB_SETCURSEL, OutputModule, 0 );

			int minexp = (int)(pow( (double)LATENCY_MIN+1, 1.0/3.0 ) * 128.0);
			int maxexp = (int)(pow( (double)LATENCY_MAX+2, 1.0/3.0 ) * 128.0);
			INIT_SLIDER( IDC_LATENCY_SLIDER, minexp, maxexp, 200, 42, 12 );

			SendDialogMsg( hWnd, IDC_LATENCY_SLIDER, TBM_SETPOS, TRUE, (int)((pow( (double)SndOutLatencyMS, 1.0/3.0 ) * 128.0) + 0.5) ); 
			swprintf_s(temp,_T("%d ms (avg)"),SndOutLatencyMS);
			SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
			
			EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_SOUNDTOUCH ), !timeStretchDisabled );
			EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_DEBUG ), DebugEnabled );
			
			SET_CHECK(IDC_EFFECTS_DISABLE,	EffectsDisabled);
			SET_CHECK(IDC_EXPANSION_DISABLE,StereoExpansionDisabled);
			SET_CHECK(IDC_TS_DISABLE,		timeStretchDisabled);
			SET_CHECK(IDC_DEBUG_ENABLE,		DebugEnabled);
			SET_CHECK(IDC_DSP_ENABLE,		dspPluginEnabled);
		}
		break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
				{
					double res = ((int)SendDialogMsg( hWnd, IDC_LATENCY_SLIDER, TBM_GETPOS, 0, 0 )) / 128.0;
					SndOutLatencyMS = (int)pow( res, 3.0 );
					Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );

					Interpolation = (int)SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_GETCURSEL,0,0 );
					OutputModule  = (int)SendDialogMsg( hWnd, IDC_OUTPUT, CB_GETCURSEL,0,0 );

					WriteSettings();
					EndDialog(hWnd,0);
				}
				break;

				case IDCANCEL:
					EndDialog(hWnd,0);
				break;

				case IDC_OUTCONF:
					SndBuffer::Configure( hWnd,
						(int)SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_GETCURSEL,0,0)
					);
				break;
				
				case IDC_OPEN_CONFIG_DEBUG:
				{
					// Quick Hack -- DebugEnabled is re-loaded with the DebugConfig's API,
					// so we need to override it here:

					bool dbgtmp = DebugEnabled;
					DebugConfig::OpenDialog();
					DebugEnabled = dbgtmp;
				}
				break;

				case IDC_OPEN_CONFIG_SOUNDTOUCH:
					SoundtouchCfg::OpenDialog( hWnd );
				break;

				HANDLE_CHECK(IDC_EFFECTS_DISABLE,EffectsDisabled);
				HANDLE_CHECK(IDC_DSP_ENABLE,dspPluginEnabled);
				HANDLE_CHECK(IDC_EXPANSION_DISABLE,StereoExpansionDisabled);
				HANDLE_CHECKNB(IDC_TS_DISABLE,timeStretchDisabled);
					EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_SOUNDTOUCH ), !timeStretchDisabled );
				break;
				
				HANDLE_CHECKNB(IDC_DEBUG_ENABLE,DebugEnabled);
					DebugConfig::EnableControls( hWnd );
					EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_DEBUG ), DebugEnabled );
				break;

				default:
					return FALSE;
			}
		break;
		
		case WM_HSCROLL:
		{
			wmEvent = LOWORD(wParam); 
			HWND hwndDlg = (HWND)lParam;
			
			// TB_THUMBTRACK passes the curpos in wParam.  Other messages
			// have to use TBM_GETPOS, so they will override this assignment (see below)

			int curpos = HIWORD( wParam );

			switch( wmEvent )
			{
				//case TB_ENDTRACK:
				//case TB_THUMBPOSITION:
				case TB_LINEUP:
				case TB_LINEDOWN:
				case TB_PAGEUP:
				case TB_PAGEDOWN:
					curpos = (int)SendMessage(hwndDlg,TBM_GETPOS,0,0);

				case TB_THUMBTRACK:
					Clampify( curpos,
						(int)SendMessage(hwndDlg,TBM_GETRANGEMIN,0,0),
						(int)SendMessage(hwndDlg,TBM_GETRANGEMAX,0,0)
					);

					SendMessage((HWND)lParam,TBM_SETPOS,TRUE,curpos);

					if( hwndDlg == GetDlgItem( hWnd, IDC_LATENCY_SLIDER ) )
					{
						double res = pow( curpos / 128.0, 3.0 );
						curpos = (int)res;
						swprintf_s(temp,_T("%d ms (avg)"),curpos);
						SetDlgItemText(hWnd,IDC_LATENCY_LABEL,temp);
					}
				break;

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
 
void configure()
{
	INT_PTR ret;
	ReadSettings();
	ret = DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CONFIG),GetActiveWindow(),(DLGPROC)ConfigProc,1);
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),_T("Error Opening the config dialog."),_T("OMG ERROR!"),MB_OK,0);
		return;
	}
	ReadSettings();
}
