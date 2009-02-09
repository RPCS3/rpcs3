///GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "spu2.h"
#include "dialogs.h"
// Config Vars


static bool VolumeBoostEnabled = false;
static int VolumeShiftModifier = 0;

#ifndef PUBLIC
static const int LATENCY_MAX = 2000;
#else
static const int LATENCY_MAX = 500;
#endif

static const int LATENCY_MIN = 40;


// DEBUG

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



char AccessLogFileName[255];
char WaveLogFileName[255];

char DMA4LogFileName[255];
char DMA7LogFileName[255];

char CoresDumpFileName[255];
char MemDumpFileName[255];
char RegDumpFileName[255];

int WaveDumpFormat;

int AutoDMAPlayRate[2]={0,0};

// MIXING
int Interpolation=1;
/* values:
		0: no interpolation (use nearest)
		1. linear interpolation
		2. cubic interpolation
*/

bool EffectsEnabled=false;

// OUTPUT
int SampleRate=48000;
int SndOutLatencyMS=160;
//int SndOutLatency=1024;
//int MaxBufferCount=8;
//int CurBufferCount=MaxBufferCount;
bool timeStretchEnabled=true;

u32 OutputModule=0; //OUTPUT_DSOUND;

//int VolumeMultiplier=1;
//int VolumeDivisor=1;

int LimitMode=0;
/* values:
	0. No limiter
	1. Soft limiter -- less cpu-intensive, but can cause problems
	2. Hard limiter -- more cpu-intensive while limiting, but should give better (constant) speeds
*/


CONFIG_DSOUNDOUT Config_DSoundOut;
CONFIG_DSOUND51 Config_DSound51;
CONFIG_WAVEOUT Config_WaveOut;

// MISC
bool LimiterToggleEnabled=false;
int  LimiterToggle=VK_SUBTRACT;

// DSP
bool dspPluginEnabled=false;
char dspPlugin[256];
int  dspPluginModule=0;

// OUTPUT MODULES
char AsioDriver[129]="";


//////

const char NewCfgFile[]="inis\\SPU2Ghz-v2.ini";
const char LegacyCfgFile[]="inis\\SPU2Ghz.ini";
const char* CfgFile=NewCfgFile;


 /*| Config File Format: |¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
+--+---------------------+------------------------+
|												  |
| Option=Value									  |
|												  |
|												  |
| Boolean Values: TRUE,YES,1,T,Y mean 'true',	  |
|                 everything else means 'false'.  |
|												  |
| All Values are limited to 255 chars.			  |
|												  |
+-------------------------------------------------+
 \*_____________________________________________*/


void CfgWriteBool(const char *Section, const char*Name, bool Value) {
	char *Data=Value?"TRUE":"FALSE";

	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

void CfgWriteInt(const char *Section, const char*Name, int Value) {
	char Data[255];
	_itoa(Value,Data,10);

	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

void CfgWriteStr(const char *Section, const char* Name, const char *Data) {
	WritePrivateProfileString(Section,Name,Data,CfgFile);
}

/*****************************************************************************/

bool CfgReadBool(const char *Section,const char *Name,bool Default) {
	char Data[255]="";
	GetPrivateProfileString(Section,Name,"",Data,255,CfgFile);
	Data[254]=0;
	if(strlen(Data)==0) {
		CfgWriteBool(Section,Name,Default);
		return Default;
	}

	if(strcmp(Data,"1")==0) return true;
	if(strcmp(Data,"Y")==0) return true;
	if(strcmp(Data,"T")==0) return true;
	if(strcmp(Data,"YES")==0) return true;
	if(strcmp(Data,"TRUE")==0) return true;
	return false;
}


int CfgReadInt(const char *Section, const char*Name,int Default) {
	char Data[255]="";
	GetPrivateProfileString(Section,Name,"",Data,255,CfgFile);
	Data[254]=0;

	if(strlen(Data)==0) {
		CfgWriteInt(Section,Name,Default);
		return Default;
	}
	
	return atoi(Data);
}


void CfgReadStr(const char *Section, const char* Name, char *Data, int DataSize, const char *Default) {
	GetPrivateProfileString(Section,Name,"",Data,DataSize,CfgFile);

	if(strlen(Data)==0) { 
		sprintf_s( Data, DataSize, "%s", Default );
		CfgWriteStr(Section,Name,Data);
	}
}

// Tries to read the requested value.
// Returns FALSE if the value isn't found.
bool CfgFindName( const char *Section, const char* Name)
{
	// Only load 24 characters.  No need to load more.
	char Data[24]="";
	GetPrivateProfileString(Section,Name,"",Data,24,CfgFile);
	Data[23]=0;

	if(strlen(Data)==0) return false;
	return true;
}
/*****************************************************************************/

void ReadSettings()
{
	DebugEnabled=CfgReadBool("DEBUG","Global_Debug_Enabled",0);
	_MsgToConsole=CfgReadBool("DEBUG","Show_Messages",0);
	_MsgKeyOnOff =CfgReadBool("DEBUG","Show_Messages_Key_On_Off",0);
	_MsgVoiceOff =CfgReadBool("DEBUG","Show_Messages_Voice_Off",0);
	_MsgDMA      =CfgReadBool("DEBUG","Show_Messages_DMA_Transfer",0);
	_MsgAutoDMA  =CfgReadBool("DEBUG","Show_Messages_AutoDMA",0);
	_MsgOverruns =CfgReadBool("DEBUG","Show_Messages_Overruns",0);
	_MsgCache    =CfgReadBool("DEBUG","Show_Messages_CacheStats",0);

	_AccessLog   =CfgReadBool("DEBUG","Log_Register_Access",0);
	_DMALog      =CfgReadBool("DEBUG","Log_DMA_Transfers",0);
	_WaveLog     =CfgReadBool("DEBUG","Log_WAVE_Output",0);

	_CoresDump   =CfgReadBool("DEBUG","Dump_Info",0);
	_MemDump     =CfgReadBool("DEBUG","Dump_Memory",0);
	_RegDump     =CfgReadBool("DEBUG","Dump_Regs",0);

	CfgReadStr("DEBUG","Access_Log_Filename",AccessLogFileName,255,"logs\\SPU2Log.txt");
	CfgReadStr("DEBUG","WaveLog_Filename",   WaveLogFileName,  255,"logs\\SPU2log.wav");
	CfgReadStr("DEBUG","DMA4Log_Filename",   DMA4LogFileName,  255,"logs\\SPU2dma4.dat");
	CfgReadStr("DEBUG","DMA7Log_Filename",   DMA7LogFileName,  255,"logs\\SPU2dma7.dat");

	CfgReadStr("DEBUG","Info_Dump_Filename",CoresDumpFileName,255,"logs\\SPU2Cores.txt");
	CfgReadStr("DEBUG","Mem_Dump_Filename", MemDumpFileName,  255,"logs\\SPU2mem.dat");
	CfgReadStr("DEBUG","Reg_Dump_Filename", RegDumpFileName,  255,"logs\\SPU2regs.dat");

	WaveDumpFormat=CfgReadInt("DEBUG","Wave_Log_Format",0);


	AutoDMAPlayRate[0]=CfgReadInt("MIXING","AutoDMA_Play_Rate_0",0);
	AutoDMAPlayRate[1]=CfgReadInt("MIXING","AutoDMA_Play_Rate_1",0);

	Interpolation=CfgReadInt("MIXING","Interpolation",1);

	// Moved Timestretch from DSP to Output
	timeStretchEnabled = CfgReadBool(
		CfgFindName( "OUTPUT", "Timestretch_Enable" ) ? "OUTPUT" : "DSP",
		"Timestretch_Enable", true
	);

	// Moved Effects_Enable from Effects to Mixing
	EffectsEnabled = CfgReadBool(
		CfgFindName( "MIXING", "Enable_Effects" ) ? "MIXING" : "EFFECTS",
		"Enable_Effects", false
	);
	EffectsEnabled = false;		// force disabled for now.

	SampleRate=CfgReadInt("OUTPUT","Sample_Rate",48000);
	SndOutLatencyMS=CfgReadInt("OUTPUT","Latency", 160);

	//OutputModule = CfgReadInt("OUTPUT","Output_Module", OUTPUT_DSOUND );
	char omodid[128];
	CfgReadStr( "OUTPUT", "Output_Module", omodid, 127, XAudio2Out->GetIdent() );

	// find the driver index of this module:
	OutputModule = FindOutputModuleById( omodid );

	VolumeShiftModifier = CfgReadInt( "OUTPUT","Volume_Shift", 0 );
	LimitMode=CfgReadInt("OUTPUT","Speed_Limit_Mode",0);

	CfgReadStr("DSP PLUGIN","Filename",dspPlugin,255,"");
	dspPluginModule = CfgReadInt("DSP PLUGIN","ModuleNum",0);
	dspPluginEnabled= CfgReadBool("DSP PLUGIN","Enabled",false);

	LimiterToggleEnabled = CfgReadBool("KEYS","Limiter_Toggle_Enabled",false);
	LimiterToggle        = CfgReadInt ("KEYS","Limiter_Toggle",VK_SUBTRACT);

	// Read DSOUNDOUT and WAVEOUT configs:
	CfgReadStr( "DSOUNDOUT", "Device", Config_DSoundOut.Device, 254, "default" );
	CfgReadStr( "WAVEOUT", "Device", Config_WaveOut.Device, 254, "default" );
	Config_DSoundOut.NumBuffers = CfgReadInt( "DSOUNDOUT", "Buffer_Count", 5 );
	Config_WaveOut.NumBuffers = CfgReadInt( "WAVEOUT", "Buffer_Count", 4 );

	// Read DSOUND51 config:
	CfgReadStr( "DSOUND51", "Device", Config_DSound51.Device, 254, "default" );
	Config_DSound51.NumBuffers = CfgReadInt( "DSOUND51", "Buffer_Count", 5 );
	Config_DSound51.GainL  =CfgReadInt("DSOUND51","Channel_Gain_L",  256);
	Config_DSound51.GainR  =CfgReadInt("DSOUND51","Channel_Gain_R",  256);
	Config_DSound51.GainC  =CfgReadInt("DSOUND51","Channel_Gain_C",  256);
	Config_DSound51.GainLFE=CfgReadInt("DSOUND51","Channel_Gain_LFE",256);
	Config_DSound51.GainSL =CfgReadInt("DSOUND51","Channel_Gain_SL", 200);
	Config_DSound51.GainSR =CfgReadInt("DSOUND51","Channel_Gain_SR", 200);
	Config_DSound51.AddCLR =CfgReadInt("DSOUND51","Channel_Center_In_LR", 56);
	Config_DSound51.LowpassLFE = CfgReadInt("DSOUND51","LFE_Lowpass_Frequency", 80);

	// Sanity Checks
	// -------------

	SampleRate = 48000;		// Yup nothing else is supported for now.
	VolumeShiftModifier = min( max( VolumeShiftModifier, -2 ), 2 );
	SndOutVolumeShift = SndOutVolumeShiftBase - VolumeShiftModifier;
	SndOutLatencyMS = min( max( SndOutLatencyMS, LATENCY_MIN ), LATENCY_MAX );
	
	Config_DSoundOut.NumBuffers = min( max( Config_DSoundOut.NumBuffers, 2 ), 8 );
	Config_WaveOut.NumBuffers = min( max( Config_DSoundOut.NumBuffers, 3 ), 8 );

	if( mods[OutputModule] == NULL )
	{
		// Unsupported or legacy module.
		fprintf( stderr, " * SPU2: Unknown output module '%s' specified in configuration file.\n", omodid );
		fprintf( stderr, " * SPU2: Defaulting to DirectSound (%s).\n", DSoundOut->GetIdent() );
		OutputModule = FindOutputModuleById( DSoundOut->GetIdent() );
	}


}

/*****************************************************************************/

void WriteSettings()
{
	CfgWriteBool("DEBUG","Global_Debug_Enabled",DebugEnabled);

	// [Air] : Commented out so that we retain debug settings even if disabled...
	//if(DebugEnabled)
	{
		CfgWriteBool("DEBUG","Show_Messages",             _MsgToConsole);
		CfgWriteBool("DEBUG","Show_Messages_Key_On_Off",  _MsgKeyOnOff);
		CfgWriteBool("DEBUG","Show_Messages_Voice_Off",   _MsgVoiceOff);
		CfgWriteBool("DEBUG","Show_Messages_DMA_Transfer",_MsgDMA);
		CfgWriteBool("DEBUG","Show_Messages_AutoDMA",     _MsgAutoDMA);
		CfgWriteBool("DEBUG","Show_Messages_Overruns",    _MsgOverruns);
		CfgWriteBool("DEBUG","Show_Messages_CacheStats",  _MsgCache);

		CfgWriteBool("DEBUG","Log_Register_Access",_AccessLog);
		CfgWriteBool("DEBUG","Log_DMA_Transfers",  _DMALog);
		CfgWriteBool("DEBUG","Log_WAVE_Output",    _WaveLog);

		CfgWriteBool("DEBUG","Dump_Info",  _CoresDump);
		CfgWriteBool("DEBUG","Dump_Memory",_MemDump);
		CfgWriteBool("DEBUG","Dump_Regs",  _RegDump);

		CfgWriteStr("DEBUG","Access_Log_Filename",AccessLogFileName);
		CfgWriteStr("DEBUG","WaveLog_Filename",   WaveLogFileName);
		CfgWriteStr("DEBUG","DMA4Log_Filename",   DMA4LogFileName);
		CfgWriteStr("DEBUG","DMA7Log_Filename",   DMA7LogFileName);

		CfgWriteStr("DEBUG","Info_Dump_Filename",CoresDumpFileName);
		CfgWriteStr("DEBUG","Mem_Dump_Filename", MemDumpFileName);
		CfgWriteStr("DEBUG","Reg_Dump_Filename", RegDumpFileName);

		CfgWriteInt("DEBUG","Wave_Log_Format",   WaveDumpFormat);
	}

	CfgWriteInt("MIXING","Interpolation",Interpolation);

	CfgWriteInt("MIXING","AutoDMA_Play_Rate_0",AutoDMAPlayRate[0]);
	CfgWriteInt("MIXING","AutoDMA_Play_Rate_1",AutoDMAPlayRate[1]);

	CfgWriteBool("MIXING","Enable_Effects",EffectsEnabled);

	CfgWriteStr("OUTPUT","Output_Module",mods[OutputModule]->GetIdent() );
	CfgWriteInt("OUTPUT","Sample_Rate",SampleRate);
	CfgWriteInt("OUTPUT","Latency",SndOutLatencyMS);
	CfgWriteBool("OUTPUT","Timestretch_Enable",timeStretchEnabled);
	CfgWriteInt("OUTPUT","Speed_Limit_Mode",LimitMode);

	CfgWriteInt("OUTPUT","Volume_Shift",SndOutVolumeShiftBase - SndOutVolumeShift);

	if( strlen( Config_DSoundOut.Device ) == 0 ) strcpy( Config_DSoundOut.Device, "default" );
	if( strlen( Config_DSound51.Device ) == 0 ) strcpy( Config_DSound51.Device, "default" );
	if( strlen( Config_WaveOut.Device ) == 0 ) strcpy( Config_WaveOut.Device, "default" );

	CfgWriteStr("DSOUNDOUT","Device",Config_DSoundOut.Device);
	CfgWriteInt("DSOUNDOUT","Buffer_Count",Config_DSoundOut.NumBuffers);

	CfgWriteStr("WAVEOUT","Device",Config_WaveOut.Device);
	CfgWriteInt("WAVEOUT","Buffer_Count",Config_WaveOut.NumBuffers);

	CfgWriteStr("DSOUND51","Device",Config_DSound51.Device);
	CfgWriteInt("DSOUND51","Buffer_Count",  Config_DSound51.NumBuffers);
	CfgWriteInt("DSOUND51","Channel_Gain_L",  Config_DSound51.GainL);
	CfgWriteInt("DSOUND51","Channel_Gain_R",  Config_DSound51.GainR);
	CfgWriteInt("DSOUND51","Channel_Gain_C",  Config_DSound51.GainC);
	CfgWriteInt("DSOUND51","Channel_Gain_LFE",Config_DSound51.GainLFE);
	CfgWriteInt("DSOUND51","Channel_Gain_SL", Config_DSound51.GainSL);
	CfgWriteInt("DSOUND51","Channel_Gain_SR", Config_DSound51.GainSR);
	CfgWriteInt("DSOUND51","Channel_Center_In_LR", Config_DSound51.AddCLR);
	CfgWriteInt("DSOUND51","LFE_Lowpass_Frequency", Config_DSound51.LowpassLFE);

	CfgWriteStr("DSP PLUGIN","Filename",dspPlugin);
	CfgWriteInt("DSP PLUGIN","ModuleNum",dspPluginModule);
	CfgWriteBool("DSP PLUGIN","Enabled",dspPluginEnabled);

	CfgWriteBool("KEYS","Limiter_Toggle_Enabled",LimiterToggleEnabled);
	CfgWriteInt ("KEYS","Limiter_Toggle",LimiterToggle);
}

static void EnableDebugMessages( HWND hWnd )
{
	ENABLE_CONTROL(IDC_MSGSHOW, DebugEnabled);
	ENABLE_CONTROL(IDC_MSGKEY,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGVOICE,MsgToConsole());
	ENABLE_CONTROL(IDC_MSGDMA,  MsgToConsole());
	ENABLE_CONTROL(IDC_MSGADMA, MsgDMA());
	ENABLE_CONTROL(IDC_DBG_OVERRUNS, MsgToConsole());
	ENABLE_CONTROL(IDC_DBG_CACHE,    MsgToConsole());
}

static void EnableDebugControls( HWND hWnd )
{
	EnableDebugMessages( hWnd );
	ENABLE_CONTROL(IDC_LOGREGS, DebugEnabled);
	ENABLE_CONTROL(IDC_LOGDMA,  DebugEnabled);
	ENABLE_CONTROL(IDC_LOGWAVE, DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPCORE,DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPMEM, DebugEnabled);
	ENABLE_CONTROL(IDC_DUMPREGS,DebugEnabled);
}

static int myWidth, myDebugWidth;
static int myHeight;
static bool debugShow = false;

BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	int wmId,wmEvent;
	char temp[384]={0};

	switch(uMsg)
	{

		case WM_PAINT:
			return FALSE;

		case WM_INITDIALOG:

			// If debugging is enabled, show the debug box by default:
			debugShow = DebugEnabled;

			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_RESETCONTENT,0,0); 
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"16000");
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"22050");
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"24000");
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"32000");
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"44100");
			SendMessage(GetDlgItem(hWnd,IDC_SRATE),CB_ADDSTRING,0,(LPARAM)"48000");

			sprintf_s(temp,48,"%d",SampleRate);
			SetDlgItemText(hWnd,IDC_SRATE,temp);

			SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_RESETCONTENT,0,0); 
			SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_ADDSTRING,0,(LPARAM)"0 - Nearest (none/fast)");
			SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_ADDSTRING,0,(LPARAM)"1 - Linear (recommended)");
			SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_ADDSTRING,0,(LPARAM)"2 - Cubic (better/slower)");
			SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_SETCURSEL,Interpolation,0); 

			SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_RESETCONTENT,0,0);
			
			{
			int modidx = 0;
			while( mods[modidx] != NULL )
			{
				sprintf_s( temp, 72, "%d - %s", modidx, mods[modidx]->GetLongName() );
				SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_ADDSTRING,0,(LPARAM)temp);
				++modidx;
			}
			SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_SETCURSEL,OutputModule,0); 
			}

			//INIT_SLIDER(IDC_BUFFER,512,16384,4096,2048,512);
			INIT_SLIDER( IDC_LATENCY_SLIDER, LATENCY_MIN, LATENCY_MAX, 100, 20, 5 );

			SendMessage(GetDlgItem(hWnd,IDC_LATENCY_SLIDER),TBM_SETPOS,TRUE,SndOutLatencyMS); 
			sprintf_s(temp,80,"%d ms (avg)",SndOutLatencyMS);
			SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);

			EnableDebugControls( hWnd );

			// Debugging / Logging Flags:
			SET_CHECK(IDC_EFFECTS, EffectsEnabled);
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

			SET_CHECK(IDC_SPEEDLIMIT,LimitMode);
			SET_CHECK(IDC_SPEEDLIMIT_RUNTIME_TOGGLE,LimiterToggleEnabled);
			SET_CHECK(IDC_DSP_ENABLE,dspPluginEnabled);
			SET_CHECK(IDC_TS_ENABLE,timeStretchEnabled);

			VolumeBoostEnabled = ( VolumeShiftModifier > 0 ) ? true : false;
			SET_CHECK(IDC_VOLBOOST, VolumeBoostEnabled );

			break;

		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDOK:
					GetDlgItemText(hWnd,IDC_SRATE,temp,20);
					temp[19]=0;
					SampleRate=atoi(temp);

					SndOutLatencyMS = (int)SendMessage( GetDlgItem( hWnd, IDC_LATENCY_SLIDER ), TBM_GETPOS, 0, 0 );

					if( SndOutLatencyMS > LATENCY_MAX ) SndOutLatencyMS = LATENCY_MAX;
					if( SndOutLatencyMS < LATENCY_MIN ) SndOutLatencyMS = LATENCY_MIN;

					Interpolation=(int)SendMessage(GetDlgItem(hWnd,IDC_INTERPOLATE),CB_GETCURSEL,0,0);
					OutputModule=(int)SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_GETCURSEL,0,0);

					WriteSettings();
					EndDialog(hWnd,0);
					break;
				case IDCANCEL:
					EndDialog(hWnd,0);
					break;

				case IDC_OUTCONF:
					SndConfigure( hWnd,
						(int)SendMessage(GetDlgItem(hWnd,IDC_OUTPUT),CB_GETCURSEL,0,0)
					);
					break;

				HANDLE_CHECKNB( IDC_VOLBOOST, VolumeBoostEnabled );
					VolumeShiftModifier = (VolumeBoostEnabled ? 1 : 0 );
					SndOutVolumeShift = SndOutVolumeShiftBase - VolumeShiftModifier;
					break;

				HANDLE_CHECK(IDC_EFFECTS,EffectsEnabled);
				HANDLE_CHECKNB(IDC_DEBUG,DebugEnabled);
					EnableDebugControls( hWnd );
					break;
				HANDLE_CHECKNB(IDC_MSGSHOW,_MsgToConsole);
					EnableDebugMessages( hWnd );
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
				HANDLE_CHECK(IDC_DSP_ENABLE,dspPluginEnabled);
				HANDLE_CHECK(IDC_TS_ENABLE,timeStretchEnabled);
				HANDLE_CHECK(IDC_SPEEDLIMIT,LimitMode);
				HANDLE_CHECK(IDC_SPEEDLIMIT_RUNTIME_TOGGLE,LimiterToggleEnabled);
				default:
					return FALSE;
			}
			break;
		case WM_HSCROLL:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			switch(wmId) {
				//case TB_ENDTRACK:
				//case TB_THUMBPOSITION:
				case TB_LINEUP:
				case TB_LINEDOWN:
				case TB_PAGEUP:
				case TB_PAGEDOWN:
					wmEvent=(int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
				case TB_THUMBTRACK:
					if(wmEvent<LATENCY_MIN) wmEvent=LATENCY_MIN;
					if(wmEvent>LATENCY_MAX) wmEvent=LATENCY_MAX;
					SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
					sprintf_s(temp,80,"%d ms (avg)",wmEvent);
					SetDlgItemText(hWnd,IDC_LATENCY_LABEL,temp);
					break;
				default:
					return FALSE;
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
	ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_CONFIG),GetActiveWindow(),(DLGPROC)ConfigProc,1);
	if(ret==-1)
	{
		MessageBoxEx(GetActiveWindow(),"Error Opening the config dialog.","OMG ERROR!",MB_OK,0);
		return;
	}
	ReadSettings();
}
