//GiGaHeRz's SPU2 Driver
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

#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

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


extern char AccessLogFileName[255];
extern char WaveLogFileName[255];

extern char DMA4LogFileName[255];
extern char DMA7LogFileName[255];

extern char CoresDumpFileName[255];
extern char MemDumpFileName[255];
extern char RegDumpFileName[255];

extern int Interpolation;

extern int WaveDumpFormat;

extern int SampleRate;

extern bool EffectsEnabled;

extern int AutoDMAPlayRate[2];

extern u32 OutputModule;
extern int SndOutLatencyMS;

//extern int VolumeMultiplier;
//extern int VolumeDivisor;

extern int LimitMode;

extern char AsioDriver[129];

extern char dspPlugin[];
extern int  dspPluginModule;

extern bool	dspPluginEnabled;
extern bool timeStretchEnabled;

extern bool LimiterToggleEnabled;
extern int  LimiterToggle;

// *** BEGIN DRIVER-SPECIFIC CONFIGURATION ***
// -------------------------------------------

// DSOUND
struct CONFIG_DSOUNDOUT
{
	char Device[256];
	s8 NumBuffers;

	CONFIG_DSOUNDOUT() :
		NumBuffers( 3 )
	{
		memset( Device, 0, sizeof( Device ) );
	}
};

// WAVEOUT
struct CONFIG_WAVEOUT
{
	char Device[255];
	s8 NumBuffers;

	CONFIG_WAVEOUT() :
		NumBuffers( 4 )
	{
		memset( Device, 0, sizeof( Device ) );
	}
};

// DSOUND51
struct CONFIG_DSOUND51
{
	char Device[256];
	u32 NumBuffers;

	u32 GainL;
	u32 GainR;
	u32 GainSL;
	u32 GainSR;
	u32 GainC;
	u32 GainLFE;
	u32 AddCLR;
	u32 LowpassLFE;

	// C++ style struct/class initializer
	CONFIG_DSOUND51() :
		NumBuffers( 4 )
	,	GainL( 256 )
	,	GainR( 256 )
	,	GainSL( 200 )
	,	GainSR( 200 )
	,	GainC( 200 )
	,	GainLFE( 256 )
	,	AddCLR( 56 )
	,	LowpassLFE( 80 )
	{
		memset( Device, 0, sizeof( Device ) );
	}
};


extern CONFIG_DSOUNDOUT Config_DSoundOut;
extern CONFIG_DSOUND51 Config_DSound51;
extern CONFIG_WAVEOUT Config_WaveOut;


//////

void ReadSettings();
void WriteSettings();
void configure();

#endif // CONFIG_H_INCLUDED
