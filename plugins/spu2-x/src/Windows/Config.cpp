/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 *
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"
#include "Dialogs.h"

#ifdef PCSX2_DEVBUILD
static const int LATENCY_MAX = 3000;
#else
static const int LATENCY_MAX = 750;
#endif

static const int LATENCY_MIN = 50;

// MIXING
int Interpolation = 1;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
		3. hermite interpolation
		4. catmull-rom interpolation
*/
int ReverbBoost = 0;
bool EffectsDisabled = false;

// OUTPUT
int SndOutLatencyMS = 150;
bool timeStretchEnabled = true;
bool asyncMixingEnabled = false;

u32 OutputModule = 0;

CONFIG_WAVEOUT Config_WaveOut;
CONFIG_XAUDIO2 Config_XAudio2;

// DSP
bool dspPluginEnabled = false;
int  dspPluginModule = 0;
wchar_t dspPlugin[256];

int numSpeakers = 0;

/*****************************************************************************/

void ReadSettings()
{
	Interpolation = CfgReadInt( L"MIXING",L"Interpolation", 1 );
	ReverbBoost = CfgReadInt( L"MIXING",L"Reverb_Boost", 0 );

	timeStretchEnabled = CfgReadBool( L"OUTPUT", L"Enable_Timestretch", true );
	asyncMixingEnabled = CfgReadBool( L"OUTPUT", L"Enable_AsyncMixing", false );
	EffectsDisabled = CfgReadBool( L"MIXING", L"Disable_Effects", false );

	numSpeakers = CfgReadInt( L"OUTPUT", L"XAudio2_SpeakerConfiguration", 0);
	SndOutLatencyMS = CfgReadInt(L"OUTPUT",L"Latency", 150);

	wchar_t omodid[128];
	CfgReadStr( L"OUTPUT", L"Output_Module", omodid, 127, XAudio2Out->GetIdent() );

	// find the driver index of this module:
	OutputModule = FindOutputModuleById( omodid );

	CfgReadStr( L"DSP PLUGIN",L"Filename",dspPlugin,255,L"");
	dspPluginModule = CfgReadInt(L"DSP PLUGIN",L"ModuleNum",0);
	dspPluginEnabled= CfgReadBool(L"DSP PLUGIN",L"Enabled",false);

	// Read DSOUNDOUT and WAVEOUT configs:
	CfgReadStr( L"WAVEOUT", L"Device", Config_WaveOut.Device, 254, L"default" );
	Config_WaveOut.NumBuffers = CfgReadInt( L"WAVEOUT", L"Buffer_Count", 4 );

	DSoundOut->ReadSettings();

	SoundtouchCfg::ReadSettings();
	DebugConfig::ReadSettings();

	// Sanity Checks
	// -------------

	Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );

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
	CfgWriteInt(L"MIXING",L"Interpolation",Interpolation);
	CfgWriteInt(L"MIXING",L"Reverb_Boost",ReverbBoost);

	CfgWriteBool(L"MIXING",L"Disable_Effects",EffectsDisabled);

	CfgWriteStr(L"OUTPUT",L"Output_Module", mods[OutputModule]->GetIdent() );
	CfgWriteInt(L"OUTPUT",L"Latency", SndOutLatencyMS);
	CfgWriteBool(L"OUTPUT",L"Enable_Timestretch", timeStretchEnabled);
	CfgWriteBool(L"OUTPUT",L"Enable_AsyncMixing", asyncMixingEnabled);
	CfgWriteInt(L"OUTPUT",L"XAudio2_SpeakerConfiguration", numSpeakers);

	if( Config_WaveOut.Device.empty() ) Config_WaveOut.Device = L"default";
	CfgWriteStr(L"WAVEOUT",L"Device",Config_WaveOut.Device);
	CfgWriteInt(L"WAVEOUT",L"Buffer_Count",Config_WaveOut.NumBuffers);

	CfgWriteStr(L"DSP PLUGIN",L"Filename",dspPlugin);
	CfgWriteInt(L"DSP PLUGIN",L"ModuleNum",dspPluginModule);
	CfgWriteBool(L"DSP PLUGIN",L"Enabled",dspPluginEnabled);

	DSoundOut->WriteSettings();
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
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) L"0 - Nearest (fastest/bad quality)" );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) L"1 - Linear (simple/nice)" );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) L"2 - Cubic (slower/good highs)" );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) L"3 - Hermite (slower/better highs)" );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_ADDSTRING,0,(LPARAM) L"4 - Catmull-Rom (slow/hq)" );
			SendDialogMsg( hWnd, IDC_INTERPOLATE, CB_SETCURSEL,Interpolation,0 );

			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_RESETCONTENT,0,0 );
			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_ADDSTRING,0,(LPARAM) L"1X - Normal Reverb Volume" );
			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_ADDSTRING,0,(LPARAM) L"2X - Reverb Volume * 2" );
			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_ADDSTRING,0,(LPARAM) L"4X - Reverb Volume * 4" );
			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_ADDSTRING,0,(LPARAM) L"8X - Reverb Volume * 8" );
			SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_SETCURSEL,ReverbBoost,0 );
			
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_RESETCONTENT,0,0 );
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_ADDSTRING,0,(LPARAM) L"Stereo (none, default)" );
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_ADDSTRING,0,(LPARAM) L"Quadrafonic" );
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_ADDSTRING,0,(LPARAM) L"Surround 5.1" );
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_ADDSTRING,0,(LPARAM) L"Surround 7.1" );			
			SendDialogMsg( hWnd, IDC_SPEAKERS, CB_SETCURSEL,numSpeakers,0 );

			SendDialogMsg( hWnd, IDC_OUTPUT, CB_RESETCONTENT,0,0 );

			int modidx = 0;
			while( mods[modidx] != NULL )
			{
				swprintf_s( temp, 72, L"%d - %s", modidx, mods[modidx]->GetLongName() );
				SendDialogMsg( hWnd, IDC_OUTPUT, CB_ADDSTRING,0,(LPARAM)temp );
				++modidx;
			}
			SendDialogMsg( hWnd, IDC_OUTPUT, CB_SETCURSEL, OutputModule, 0 );

			int minexp = (int)(pow( (double)LATENCY_MIN+1, 1.0/3.0 ) * 128.0);
			int maxexp = (int)(pow( (double)LATENCY_MAX+2, 1.0/3.0 ) * 128.0);
			INIT_SLIDER( IDC_LATENCY_SLIDER, minexp, maxexp, 200, 42, 1 );

			SendDialogMsg( hWnd, IDC_LATENCY_SLIDER, TBM_SETPOS, TRUE, (int)((pow( (double)SndOutLatencyMS, 1.0/3.0 ) * 128.0) + 1) );
			swprintf_s(temp,L"%d ms (avg)",SndOutLatencyMS);
			SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);

			EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_SOUNDTOUCH ), timeStretchEnabled );
			EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_DEBUG ), DebugEnabled );

			SET_CHECK(IDC_EFFECTS_DISABLE,	EffectsDisabled);
			SET_CHECK(IDC_TS_ENABLE,		timeStretchEnabled);
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
					ReverbBoost = (int)SendDialogMsg( hWnd, IDC_REVERB_BOOST, CB_GETCURSEL,0,0 );
					OutputModule = (int)SendDialogMsg( hWnd, IDC_OUTPUT, CB_GETCURSEL,0,0 );
					numSpeakers = (int)SendDialogMsg( hWnd, IDC_SPEAKERS, CB_GETCURSEL,0,0 );

					WriteSettings();
					EndDialog(hWnd,0);
				}
				break;

				case IDCANCEL:
					EndDialog(hWnd,0);
				break;

				case IDC_OUTCONF:
				{
					const int module = (int)SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_GETCURSEL,0,0);
					if( mods[module] == NULL ) break;
					mods[module]->Configure((uptr)hWnd);
				}
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
				HANDLE_CHECKNB(IDC_TS_ENABLE,timeStretchEnabled);
					EnableWindow( GetDlgItem( hWnd, IDC_OPEN_CONFIG_SOUNDTOUCH ), timeStretchEnabled );
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
						swprintf_s(temp,L"%d ms (avg)",curpos);
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
		MessageBox(GetActiveWindow(),L"Error Opening the config dialog.",L"OMG ERROR!",MB_OK | MB_SETFOREGROUND);
		return;
	}
	ReadSettings();
}
