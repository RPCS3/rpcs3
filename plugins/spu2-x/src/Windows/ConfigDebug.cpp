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
#include "Utilities\Path.h"


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

bool _visual_debug_enabled=false;

// this is set true if PCSX2 invokes the SetLogDir callback, which tells SPU2-X to use that over
// the configured crap in the ini file.
static bool LogLocationSetByPcsx2 = false;

static wxString CfgLogsFolder;
static wxString CfgDumpsFolder;

static wxDirName LogsFolder;
static wxDirName DumpsFolder;

wxString AccessLogFileName;
wxString DMA4LogFileName;
wxString DMA7LogFileName;

wxString CoresDumpFileName;
wxString MemDumpFileName;
wxString RegDumpFileName;

void CfgSetLogDir( const char* dir )
{
	LogsFolder	= (dir==NULL) ? wxString(L"logs") : wxString(dir, wxConvFile);
	DumpsFolder	= (dir==NULL) ? wxString(L"logs") : wxString(dir, wxConvFile);
	LogLocationSetByPcsx2 = (dir!=NULL);
}

FILE* OpenBinaryLog( const wxString& logfile )
{
	return wxFopen( Path::Combine(LogsFolder, logfile), L"wb" );
}

FILE* OpenLog( const wxString& logfile )
{
	return wxFopen( Path::Combine(LogsFolder, logfile), L"w" );
}

FILE* OpenDump( const wxString& logfile )
{
	return wxFopen( Path::Combine(DumpsFolder, logfile), L"w" );
}

namespace DebugConfig {

static const wxChar* Section = L"DEBUG";

void ReadSettings()
{
	DebugEnabled = CfgReadBool(Section, L"Global_Enable",0);
	_MsgToConsole= CfgReadBool(Section, L"Show_Messages",0);
	_MsgKeyOnOff = CfgReadBool(Section, L"Show_Messages_Key_On_Off",0);
	_MsgVoiceOff = CfgReadBool(Section, L"Show_Messages_Voice_Off",0);
	_MsgDMA      = CfgReadBool(Section, L"Show_Messages_DMA_Transfer",0);
	_MsgAutoDMA  = CfgReadBool(Section, L"Show_Messages_AutoDMA",0);
	_MsgOverruns = CfgReadBool(Section, L"Show_Messages_Overruns",0);
	_MsgCache    = CfgReadBool(Section, L"Show_Messages_CacheStats",0);

	_AccessLog   = CfgReadBool(Section, L"Log_Register_Access",0);
	_DMALog      = CfgReadBool(Section, L"Log_DMA_Transfers",0);
	_WaveLog     = CfgReadBool(Section, L"Log_WAVE_Output",0);

	_CoresDump   = CfgReadBool(Section, L"Dump_Info",0);
	_MemDump     = CfgReadBool(Section, L"Dump_Memory",0);
	_RegDump     = CfgReadBool(Section, L"Dump_Regs",0);

	_visual_debug_enabled = CfgReadBool(Section, L"Visual_Debug_Enabled",0);

	CfgReadStr(Section,L"Logs_Folder", CfgLogsFolder, L"logs");
	CfgReadStr(Section,L"Dumps_Folder",CfgDumpsFolder,L"logs");

	CfgReadStr(Section,L"Access_Log_Filename",AccessLogFileName, L"SPU2Log.txt");
	CfgReadStr(Section,L"DMA4Log_Filename",   DMA4LogFileName,   L"SPU2dma4.dat");
	CfgReadStr(Section,L"DMA7Log_Filename",   DMA7LogFileName,   L"SPU2dma7.dat");

	CfgReadStr(Section,L"Info_Dump_Filename", CoresDumpFileName, L"SPU2Cores.txt");
	CfgReadStr(Section,L"Mem_Dump_Filename",  MemDumpFileName,   L"SPU2mem.dat");
	CfgReadStr(Section,L"Reg_Dump_Filename",  RegDumpFileName,   L"SPU2regs.dat");

	if( !LogLocationSetByPcsx2 )
	{
		LogsFolder	= CfgLogsFolder;
		DumpsFolder = CfgLogsFolder;
	}
}


void WriteSettings()
{
	CfgWriteBool(Section,L"Global_Enable",DebugEnabled);

	CfgWriteBool(Section,L"Show_Messages",             _MsgToConsole);
	CfgWriteBool(Section,L"Show_Messages_Key_On_Off",  _MsgKeyOnOff);
	CfgWriteBool(Section,L"Show_Messages_Voice_Off",   _MsgVoiceOff);
	CfgWriteBool(Section,L"Show_Messages_DMA_Transfer",_MsgDMA);
	CfgWriteBool(Section,L"Show_Messages_AutoDMA",     _MsgAutoDMA);
	CfgWriteBool(Section,L"Show_Messages_Overruns",    _MsgOverruns);
	CfgWriteBool(Section,L"Show_Messages_CacheStats",  _MsgCache);

	CfgWriteBool(Section,L"Log_Register_Access",_AccessLog);
	CfgWriteBool(Section,L"Log_DMA_Transfers",  _DMALog);
	CfgWriteBool(Section,L"Log_WAVE_Output",    _WaveLog);

	CfgWriteBool(Section,L"Dump_Info",  _CoresDump);
	CfgWriteBool(Section,L"Dump_Memory",_MemDump);
	CfgWriteBool(Section,L"Dump_Regs",  _RegDump);

	CfgWriteBool(Section,L"Visual_Debug_Enabled", _visual_debug_enabled);

	// None of the logs strings are changable via GUI, so no point in bothering to
	// write them back out.
	CfgWriteStr(Section,L"Logs_Folder", CfgLogsFolder);
	CfgWriteStr(Section,L"Dumps_Folder",CfgDumpsFolder);

	CfgWriteStr(Section,L"Access_Log_Filename",AccessLogFileName);
	CfgWriteStr(Section,L"DMA4Log_Filename",   DMA4LogFileName);
	CfgWriteStr(Section,L"DMA7Log_Filename",   DMA7LogFileName);

	CfgWriteStr(Section,L"Info_Dump_Filename",CoresDumpFileName);
	CfgWriteStr(Section,L"Mem_Dump_Filename", MemDumpFileName);
	CfgWriteStr(Section,L"Reg_Dump_Filename", RegDumpFileName);
}

static void EnableMessages( HWND hWnd )
{
	ENABLE_CONTROL(IDC_MSGSHOW, DebugEnabled);
	ENABLE_CONTROL(IDC_MSGKEY,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGVOICE,MsgToConsole());
	ENABLE_CONTROL(IDC_MSGDMA,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGADMA, MsgToConsole());
	ENABLE_CONTROL(IDC_DBG_OVERRUNS, MsgToConsole());
	ENABLE_CONTROL(IDC_DBG_CACHE,    MsgToConsole());
}

void EnableControls( HWND hWnd )
{
	EnableMessages( hWnd );
	ENABLE_CONTROL(IDC_LOGDMA,  DebugEnabled);
	ENABLE_CONTROL(IDC_LOGREGS, IsDevBuild ? DebugEnabled : false);
	ENABLE_CONTROL(IDC_LOGWAVE, IsDevBuild ? DebugEnabled : false);
	ENABLE_CONTROL(IDC_DUMPCORE,DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPMEM, DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPREGS,DebugEnabled);
	ENABLE_CONTROL(IDC_DEBUG_VISUAL, IsDevBuild ? DebugEnabled : false);
}

static BOOL CALLBACK DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId;
	//wchar_t temp[384]={0};

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
			SET_CHECK(IDC_DEBUG_VISUAL,_visual_debug_enabled);

			ShowWindow( GetDlgItem( hWnd, IDC_MSG_PUBLIC_BUILD ), !IsDevBuild );
		}
		break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam);
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
				HANDLE_CHECK(IDC_MSGDMA,_MsgDMA);
				HANDLE_CHECK(IDC_MSGADMA,_MsgAutoDMA);
				break;

				HANDLE_CHECK(IDC_DBG_OVERRUNS,_MsgOverruns);
				HANDLE_CHECK(IDC_DBG_CACHE,_MsgCache);
				HANDLE_CHECK(IDC_LOGREGS,_AccessLog);
				HANDLE_CHECK(IDC_LOGDMA, _DMALog);
				HANDLE_CHECK(IDC_LOGWAVE,_WaveLog);
				HANDLE_CHECK(IDC_DUMPCORE,_CoresDump);
				HANDLE_CHECK(IDC_DUMPMEM, _MemDump);
				HANDLE_CHECK(IDC_DUMPREGS,_RegDump);
				HANDLE_CHECK(IDC_DEBUG_VISUAL,_visual_debug_enabled);
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
	INT_PTR ret = DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CONFIG_DEBUG),GetActiveWindow(),(DLGPROC)DialogProc,1);
	if(ret == -1)
	{
		MessageBox(GetActiveWindow(),L"Error Opening the debug configuration dialog.",L"OMG ERROR!",MB_OK | MB_SETFOREGROUND);
		return;
	}
	ReadSettings();
}

}
