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
#include "Config.h"

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


#ifdef PCSX2_DEVBUILD
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
int ReverbBoost = 0;
bool EffectsDisabled = false;

// OUTPUT
int SndOutLatencyMS = 160;
bool timeStretchDisabled = false;

u32 OutputModule = FindOutputModuleById( PortaudioOut->GetIdent() );

CONFIG_DSOUNDOUT Config_DSoundOut;
CONFIG_WAVEOUT Config_WaveOut;
CONFIG_XAUDIO2 Config_XAudio2;

// DSP
bool dspPluginEnabled = false;
int  dspPluginModule = 0;
wchar_t dspPlugin[256];

bool StereoExpansionEnabled = false;

/*****************************************************************************/

void ReadSettings()
{
	Interpolation = CfgReadInt( L"MIXING",L"Interpolation", 1 );
	ReverbBoost = CfgReadInt( L"MIXING",L"Reverb_Boost", 0 );

	timeStretchDisabled = CfgReadBool( L"OUTPUT", L"Disable_Timestretch", false );
	EffectsDisabled = CfgReadBool( L"MIXING", L"Disable_Effects", false );

	StereoExpansionEnabled = CfgReadBool( L"OUTPUT", L"Enable_StereoExpansion", false );
	SndOutLatencyMS = CfgReadInt(L"OUTPUT",L"Latency", 150);

	wchar_t omodid[128];
	//CfgReadStr( L"OUTPUT", L"Output_Module", omodid, 127, PortaudioOut->GetIdent() );

	// find the driver index of this module:
	//OutputModule = FindOutputModuleById( omodid );

	// Read DSOUNDOUT and WAVEOUT configs:
	CfgReadStr( L"WAVEOUT", L"Device", Config_WaveOut.Device, 254, L"default" );
	Config_WaveOut.NumBuffers = CfgReadInt( L"WAVEOUT", L"Buffer_Count", 4 );

	PortaudioOut->ReadSettings();

	SoundtouchCfg::ReadSettings();
	//DebugConfig::ReadSettings();

	// Sanity Checks
	// -------------

	Clampify( SndOutLatencyMS, LATENCY_MIN, LATENCY_MAX );
	
	WriteSettings();
}

/*****************************************************************************/

void WriteSettings()
{
	CfgWriteInt(L"MIXING",L"Interpolation",Interpolation);
	CfgWriteInt(L"MIXING",L"Reverb_Boost",ReverbBoost);

	CfgWriteBool(L"MIXING",L"Disable_Effects",EffectsDisabled);

	CfgWriteStr(L"OUTPUT",L"Output_Module", mods[OutputModule]->GetIdent() );
	CfgWriteInt(L"OUTPUT",L"Latency", SndOutLatencyMS);
	CfgWriteBool(L"OUTPUT",L"Disable_Timestretch", timeStretchDisabled);
	CfgWriteBool(L"OUTPUT",L"Enable_StereoExpansion", StereoExpansionEnabled);

	if( Config_WaveOut.Device.empty() ) Config_WaveOut.Device = L"default";
	CfgWriteStr(L"WAVEOUT",L"Device",Config_WaveOut.Device);
	CfgWriteInt(L"WAVEOUT",L"Buffer_Count",Config_WaveOut.NumBuffers);

	PortaudioOut->WriteSettings();	
	SoundtouchCfg::WriteSettings();
	//DebugConfig::WriteSettings();
}


void configure()
{
	ReadSettings();
	WriteSettings();
}

void MessageBox(char const*, ...)
{
}
