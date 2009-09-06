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

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include <string>

extern bool DebugEnabled;

extern bool _MsgToConsole;
extern bool _MsgKeyOnOff;
extern bool _MsgVoiceOff;
extern bool _MsgDMA;
extern bool _MsgAutoDMA;
extern bool _MsgOverruns;
extern bool _MsgCache;

extern bool _AccessLog;
extern bool _DMALog;
extern bool _WaveLog;

extern bool _CoresDump;
extern bool _MemDump;
extern bool _RegDump;

static __forceinline bool MsgToConsole() { return _MsgToConsole & DebugEnabled; }

static __forceinline bool MsgKeyOnOff() { return _MsgKeyOnOff & MsgToConsole(); }
static __forceinline bool MsgVoiceOff() { return _MsgVoiceOff & MsgToConsole(); }
static __forceinline bool MsgDMA() { return _MsgDMA & MsgToConsole(); }
static __forceinline bool MsgAutoDMA() { return _MsgAutoDMA & MsgDMA(); }
static __forceinline bool MsgOverruns() { return _MsgOverruns & MsgToConsole(); }
static __forceinline bool MsgCache() { return _MsgCache & MsgToConsole(); }

static __forceinline bool AccessLog() { return _AccessLog & DebugEnabled; }
static __forceinline bool DMALog() { return _DMALog & DebugEnabled; }
static __forceinline bool WaveLog() { return _WaveLog & DebugEnabled; }

static __forceinline bool CoresDump() { return _CoresDump & DebugEnabled; }
static __forceinline bool MemDump() { return _MemDump & DebugEnabled; }
static __forceinline bool RegDump() { return _RegDump & DebugEnabled; }


extern wchar_t AccessLogFileName[255];
extern wchar_t WaveLogFileName[255];
extern wchar_t DMA4LogFileName[255];
extern wchar_t DMA7LogFileName[255];
extern wchar_t CoresDumpFileName[255];
extern wchar_t MemDumpFileName[255];
extern wchar_t RegDumpFileName[255];

extern int Interpolation;

extern bool EffectsDisabled;

extern int AutoDMAPlayRate[2];

extern u32 OutputModule;
extern int SndOutLatencyMS;

extern wchar_t dspPlugin[];
extern int  dspPluginModule;

extern bool	dspPluginEnabled;
extern bool timeStretchDisabled;
extern bool StereoExpansionEnabled;

class SoundtouchCfg
{
	// Timestretch Slider Bounds, Min/Max
	static const int SequenceLen_Min = 30;
	static const int SequenceLen_Max = 90;

	static const int SeekWindow_Min = 10;
	static const int SeekWindow_Max = 32;

	static const int Overlap_Min = 3;
	static const int Overlap_Max = 15;

public:
	static int SequenceLenMS;
	static int SeekWindowMS;
	static int OverlapMS;

	static void ReadSettings();
	static void WriteSettings();
	static void OpenDialog( uptr hWnd );
	
protected:
	static void ClampValues();
	//static bool CALLBACK DialogProc(uptr hWnd,u32 uMsg,WPARAM wParam,LPARAM lParam);

};


// *** BEGIN DRIVER-SPECIFIC CONFIGURATION ***
// -------------------------------------------

// DSOUND
struct CONFIG_XAUDIO2
{
	std::wstring Device;
	s8 NumBuffers;

	CONFIG_XAUDIO2() :
		Device(),
		NumBuffers( 2 )
	{
	}
};

// DSOUND
struct CONFIG_DSOUNDOUT
{
	std::wstring Device;
	s8 NumBuffers;

	CONFIG_DSOUNDOUT() :
		Device(),
		NumBuffers( 3 )
	{
	}
};

// WAVEOUT
struct CONFIG_WAVEOUT
{
	std::wstring Device;
	s8 NumBuffers;

	CONFIG_WAVEOUT() :
		Device(),
		NumBuffers( 4 )
	{
	}
};

/*extern CONFIG_DSOUNDOUT Config_DSoundOut;
extern CONFIG_WAVEOUT Config_WaveOut;
extern CONFIG_XAUDIO2 Config_XAudio2;*/

//////

void ReadSettings();
void WriteSettings();
void configure();
void AboutBox();

#endif // CONFIG_H_INCLUDED
