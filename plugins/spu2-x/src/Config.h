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

#pragma once

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

static __forceinline bool MsgToConsole()	{ return _MsgToConsole & DebugEnabled; }

static __forceinline bool MsgKeyOnOff()		{ return _MsgKeyOnOff & MsgToConsole(); }
static __forceinline bool MsgVoiceOff()		{ return _MsgVoiceOff & MsgToConsole(); }
static __forceinline bool MsgDMA()			{ return _MsgDMA & MsgToConsole(); }
static __forceinline bool MsgAutoDMA()		{ return _MsgAutoDMA & MsgDMA(); }
static __forceinline bool MsgOverruns()		{ return _MsgOverruns & MsgToConsole(); }
static __forceinline bool MsgCache()		{ return _MsgCache & MsgToConsole(); }

static __forceinline bool AccessLog()		{ return _AccessLog & DebugEnabled; }
static __forceinline bool DMALog()			{ return _DMALog & DebugEnabled; }
static __forceinline bool WaveLog()			{ return _WaveLog & DebugEnabled; }

static __forceinline bool CoresDump()		{ return _CoresDump & DebugEnabled; }
static __forceinline bool MemDump()			{ return _MemDump & DebugEnabled; }
static __forceinline bool RegDump()			{ return _RegDump & DebugEnabled; }


extern wchar_t AccessLogFileName[255];
extern wchar_t WaveLogFileName[255];
extern wchar_t DMA4LogFileName[255];
extern wchar_t DMA7LogFileName[255];
extern wchar_t CoresDumpFileName[255];
extern wchar_t MemDumpFileName[255];
extern wchar_t RegDumpFileName[255];

extern int Interpolation;
extern int ReverbBoost;
extern bool EffectsDisabled;

extern u32 OutputModule;
extern int SndOutLatencyMS;
extern bool timeStretchEnabled;
extern bool asyncMixingEnabled;

#ifndef __LINUX__
extern wchar_t dspPlugin[];
extern int  dspPluginModule;

extern bool	dspPluginEnabled;
extern bool StereoExpansionEnabled;
#endif

namespace SoundtouchCfg
{
	extern void ApplySettings( soundtouch::SoundTouch& sndtouch );
}

//////

extern void ReadSettings();
extern void WriteSettings();
extern void configure();
extern void AboutBox();
