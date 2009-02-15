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


bool DebugEnabled=false;
bool _MsgToConsole=false;
bool _MsgKeyOnOff=false;
bool _MsgVoiceOff=false;
bool _MsgDMA=false;
bool _MsgAutoDMA=false;
bool _MsgOverruns=false;
bool _MsgCache=false;

bool _AccessLog=false;
bool _DMALog=false;
bool _WaveLog=false;

bool _CoresDump=false;
bool _MemDump=false;
bool _RegDump=false;



wchar_t AccessLogFileName[255];
wchar_t WaveLogFileName[255];

wchar_t DMA4LogFileName[255];
wchar_t DMA7LogFileName[255];

wchar_t CoresDumpFileName[255];
wchar_t MemDumpFileName[255];
wchar_t RegDumpFileName[255];

namespace DebugConfig {

static const TCHAR* Section = _T("DEBUG");

void ReadSettings()
{
	DebugEnabled = CfgReadBool(Section, _T("Global_Enable"),0);
	_MsgToConsole= CfgReadBool(Section, _T("Show_Messages"),0);
	_MsgKeyOnOff = CfgReadBool(Section, _T("Show_Messages_Key_On_Off"),0);
	_MsgVoiceOff = CfgReadBool(Section, _T("Show_Messages_Voice_Off"),0);
	_MsgDMA      = CfgReadBool(Section, _T("Show_Messages_DMA_Transfer"),0);
	_MsgAutoDMA  = CfgReadBool(Section, _T("Show_Messages_AutoDMA"),0);
	_MsgOverruns = CfgReadBool(Section, _T("Show_Messages_Overruns"),0);
	_MsgCache    = CfgReadBool(Section, _T("Show_Messages_CacheStats"),0);

	_AccessLog   = CfgReadBool(Section, _T("Log_Register_Access"),0);
	_DMALog      = CfgReadBool(Section, _T("Log_DMA_Transfers"),0);
	_WaveLog     = CfgReadBool(Section, _T("Log_WAVE_Output"),0);

	_CoresDump   = CfgReadBool(Section, _T("Dump_Info"),0);
	_MemDump     = CfgReadBool(Section, _T("Dump_Memory"),0);
	_RegDump     = CfgReadBool(Section, _T("Dump_Regs"),0);

	CfgReadStr(Section,_T("Access_Log_Filename"),AccessLogFileName,255,_T("logs\\SPU2Log.txt"));
	CfgReadStr(Section,_T("WaveLog_Filename"),   WaveLogFileName,  255,_T("logs\\SPU2log.wav"));
	CfgReadStr(Section,_T("DMA4Log_Filename"),   DMA4LogFileName,  255,_T("logs\\SPU2dma4.dat"));
	CfgReadStr(Section,_T("DMA7Log_Filename"),   DMA7LogFileName,  255,_T("logs\\SPU2dma7.dat"));

	CfgReadStr(Section,_T("Info_Dump_Filename"),CoresDumpFileName,255,_T("logs\\SPU2Cores.txt"));
	CfgReadStr(Section,_T("Mem_Dump_Filename"), MemDumpFileName,  255,_T("logs\\SPU2mem.dat"));
	CfgReadStr(Section,_T("Reg_Dump_Filename"), RegDumpFileName,  255,_T("logs\\SPU2regs.dat"));
}


void WriteSettings()
{
	CfgWriteBool(Section,_T("Global_Enable"),DebugEnabled);

	CfgWriteBool(Section,_T("Show_Messages"),             _MsgToConsole);
	CfgWriteBool(Section,_T("Show_Messages_Key_On_Off"),  _MsgKeyOnOff);
	CfgWriteBool(Section,_T("Show_Messages_Voice_Off"),   _MsgVoiceOff);
	CfgWriteBool(Section,_T("Show_Messages_DMA_Transfer"),_MsgDMA);
	CfgWriteBool(Section,_T("Show_Messages_AutoDMA"),     _MsgAutoDMA);
	CfgWriteBool(Section,_T("Show_Messages_Overruns"),    _MsgOverruns);
	CfgWriteBool(Section,_T("Show_Messages_CacheStats"),  _MsgCache);

	CfgWriteBool(Section,_T("Log_Register_Access"),_AccessLog);
	CfgWriteBool(Section,_T("Log_DMA_Transfers"),  _DMALog);
	CfgWriteBool(Section,_T("Log_WAVE_Output"),    _WaveLog);

	CfgWriteBool(Section,_T("Dump_Info"),  _CoresDump);
	CfgWriteBool(Section,_T("Dump_Memory"),_MemDump);
	CfgWriteBool(Section,_T("Dump_Regs"),  _RegDump);

	CfgWriteStr(Section,_T("Access_Log_Filename"),AccessLogFileName);
	CfgWriteStr(Section,_T("WaveLog_Filename"),   WaveLogFileName);
	CfgWriteStr(Section,_T("DMA4Log_Filename"),   DMA4LogFileName);
	CfgWriteStr(Section,_T("DMA7Log_Filename"),   DMA7LogFileName);

	CfgWriteStr(Section,_T("Info_Dump_Filename"),CoresDumpFileName);
	CfgWriteStr(Section,_T("Mem_Dump_Filename"), MemDumpFileName);
	CfgWriteStr(Section,_T("Reg_Dump_Filename"), RegDumpFileName);

}

static void EnableMessages( HWND hWnd )
{
	ENABLE_CONTROL(IDC_MSGSHOW, DebugEnabled);
	ENABLE_CONTROL(IDC_MSGKEY,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGVOICE,MsgToConsole());
	ENABLE_CONTROL(IDC_MSGDMA,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGADMA, MsgDMA());
	ENABLE_CONTROL(IDC_DBG_OVERRUNS, MsgToConsole());
	ENABLE_CONTROL(IDC_DBG_CACHE,    MsgToConsole());
}

void EnableControls( HWND hWnd )
{
	EnableMessages( hWnd );
	ENABLE_CONTROL(IDC_LOGDMA,  DebugEnabled);
#ifdef PUBLIC
	ENABLE_CONTROL(IDC_LOGREGS, false);
	ENABLE_CONTROL(IDC_LOGWAVE, false);
#else
	ENABLE_CONTROL(IDC_LOGREGS, DebugEnabled);
	ENABLE_CONTROL(IDC_LOGWAVE, DebugEnabled);
#endif
	ENABLE_CONTROL(IDC_DUMPCORE,DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPMEM, DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPREGS,DebugEnabled);
}

static BOOL CALLBACK DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	wchar_t temp[384]={0};

	switch(uMsg)
	{
		case WM_PAINT:
			return FALSE;

		case WM_INITDIALOG:
		{
			EnableControls( hWnd );

			// Debugging / Logging Flags:
			SET_CHECK(IDC_DEBUG,   DebugEnabled);
			SET_CHECK(IDC_MSGSHOW, _MsgToConsole);
			SET_CHECK(IDC_MSGKEY,  _MsgKeyOnOff);
			SET_CHECK(IDC_MSGVOICE,_MsgVoiceOff);
			SET_CHECK(IDC_MSGDMA,  _MsgDMA);
			SET_CHECK(IDC_MSGADMA, _MsgAutoDMA);
			SET_CHECK(IDC_DBG_OVERRUNS, _MsgOverruns );
			SET_CHECK(IDC_DBG_CACHE, _MsgCache );
			SET_CHECK(IDC_LOGREGS, _AccessLog);
			SET_CHECK(IDC_LOGDMA,  _DMALog);
			SET_CHECK(IDC_LOGWAVE, _WaveLog);
			SET_CHECK(IDC_DUMPCORE,_CoresDump);
			SET_CHECK(IDC_DUMPMEM, _MemDump);
			SET_CHECK(IDC_DUMPREGS,_RegDump);

			#ifdef PUBLIC
			ShowWindow( GetDlgItem( hWnd, IDC_MSG_PUBLIC_BUILD ), true );
			#else
			ShowWindow( GetDlgItem( hWnd, IDC_MSG_PUBLIC_BUILD ), false );
			#endif
		}
		break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
					WriteSettings();
					EndDialog(hWnd,0);
				break;

				case IDCANCEL:
					EndDialog(hWnd,0);
				break;

				HANDLE_CHECKNB(IDC_MSGSHOW,_MsgToConsole);
					EnableMessages( hWnd );
				break;

				HANDLE_CHECK(IDC_MSGKEY,_MsgKeyOnOff);
				HANDLE_CHECK(IDC_MSGVOICE,_MsgVoiceOff);
				HANDLE_CHECKNB(IDC_MSGDMA,_MsgDMA);
					ENABLE_CONTROL(IDC_MSGADMA, MsgDMA());
				break;

				HANDLE_CHECK(IDC_MSGADMA,_MsgAutoDMA);
				HANDLE_CHECK(IDC_DBG_OVERRUNS,_MsgOverruns);
				HANDLE_CHECK(IDC_DBG_CACHE,_MsgCache);
				HANDLE_CHECK(IDC_LOGREGS,_AccessLog);
				HANDLE_CHECK(IDC_LOGDMA, _DMALog);
				HANDLE_CHECK(IDC_LOGWAVE,_WaveLog);
				HANDLE_CHECK(IDC_DUMPCORE,_CoresDump);
				HANDLE_CHECK(IDC_DUMPMEM, _MemDump);
				HANDLE_CHECK(IDC_DUMPREGS,_RegDump);
				default:
					return FALSE;
			}
		break;

		default:
			return FALSE;
	}
	return TRUE;

}

void OpenDialog()
{
	INT_PTR ret;
	ret = DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CONFIG_DEBUG),GetActiveWindow(),(DLGPROC)DialogProc,1);
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),_T("Error Opening the debug configuration dialog."),_T("OMG ERROR!"),MB_OK,0);
		return;
	}
	ReadSettings();
}

}