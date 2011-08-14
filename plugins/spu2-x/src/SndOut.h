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

// Number of stereo samples per SndOut block.
// All drivers must work in units of this size when communicating with
// SndOut.
static const int SndOutPacketSize = 64;

// Overall master volume shift; this is meant to be a precision value and does not affect
// actual output volumes.  It converts SPU2 16 bit volumes to 32-bit volumes, and likewise
// downsamples 32 bit samples to 16 bit sound driver output (this way timestretching and
// DSP effects get better precision results)
static const int SndOutVolumeShift = 12;
static const int SndOutVolumeShift32 = 16-SndOutVolumeShift; // shift up, not down

// Samplerate of the SPU2. For accurate playback we need to match this
// exactly.  Trying to scale samplerates and maintain SPU2's Ts timing accuracy
// is too problematic. :)
static const int SampleRate = 48000;

extern int FindOutputModuleById( const wchar_t* omodid );

struct Stereo51Out16DplII;
struct Stereo51Out32DplII;

struct Stereo51Out16Dpl; // similar to DplII but without rear balancing
struct Stereo51Out32Dpl;

extern void ResetDplIIDecoder();
extern void ProcessDplIISample16( const StereoOut32& src, Stereo51Out16DplII * s);
extern void ProcessDplIISample32( const StereoOut32& src, Stereo51Out32DplII * s);
extern void ProcessDplSample16( const StereoOut32& src, Stereo51Out16Dpl * s);
extern void ProcessDplSample32( const StereoOut32& src, Stereo51Out32Dpl * s);

struct StereoOut16
{
	s16 Left;
	s16 Right;

	StereoOut16() :
		Left( 0 ),
		Right( 0 )
	{
	}

	StereoOut16( const StereoOut32& src ) :
		Left( (s16)src.Left ),
		Right( (s16)src.Right )
	{
	}

	StereoOut16( s16 left, s16 right ) :
		Left( left ),
		Right( right )
	{
	}

	StereoOut32 UpSample() const;

	void ResampleFrom( const StereoOut32& src )
	{
		// Use StereoOut32's built in conversion
		*this = src.DownSample();
	}
};

struct StereoOutFloat
{
	float Left;
	float Right;

	StereoOutFloat() :
		Left( 0 ),
		Right( 0 )
	{
	}

	explicit StereoOutFloat( const StereoOut32& src ) :
		Left( src.Left / 2147483647.0f ),
		Right( src.Right / 2147483647.0f )
	{
	}

	explicit StereoOutFloat( s32 left, s32 right ) :
		Left( left / 2147483647.0f ),
		Right( right / 2147483647.0f )
	{
	}

	StereoOutFloat( float left, float right ) :
		Left( left ),
		Right( right )
	{
	}
};

struct Stereo21Out16
{
	s16 Left;
	s16 Right;
	s16 LFE;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left >> SndOutVolumeShift;
		Right = src.Right >> SndOutVolumeShift;
		LFE = (src.Left + src.Right) >> (SndOutVolumeShift + 1);
	}
};

struct Stereo40Out16
{
	s16 Left;
	s16 Right;
	s16 LeftBack;
	s16 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left >> SndOutVolumeShift;
		Right = src.Right >> SndOutVolumeShift;
		LeftBack = src.Left >> SndOutVolumeShift;
		RightBack = src.Right >> SndOutVolumeShift;
	}
};

struct Stereo40Out32
{
	s32 Left;
	s32 Right;
	s32 LeftBack;
	s32 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
		LeftBack = src.Left << SndOutVolumeShift32;
		RightBack = src.Right << SndOutVolumeShift32;
	}
};

struct Stereo41Out16
{
	s16 Left;
	s16 Right;
	s16 LFE;
	s16 LeftBack;
	s16 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left >> SndOutVolumeShift;
		Right = src.Right >> SndOutVolumeShift;
		LFE = (src.Left + src.Right) >> (SndOutVolumeShift + 1);
		LeftBack = src.Left >> SndOutVolumeShift;
		RightBack = src.Right >> SndOutVolumeShift;
	}
};

struct Stereo51Out16
{
	s16 Left;
	s16 Right;
	s16 Center;
	s16 LFE;
	s16 LeftBack;
	s16 RightBack;

	// Implementation Note: Center and Subwoofer/LFE -->
	// This method is simple and sounds nice.  It relies on the speaker/soundcard
	// systems do to their own low pass / crossover.  Manual lowpass is wasted effort
	// and can't match solid state results anyway.

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left >> SndOutVolumeShift;
		Right = src.Right >> SndOutVolumeShift;
		Center = (src.Left + src.Right) >> (SndOutVolumeShift + 1);
		LFE = Center;
		LeftBack = src.Left >> SndOutVolumeShift;
		RightBack = src.Right >> SndOutVolumeShift;
	}
};

struct Stereo51Out16DplII
{
	s16 Left;
	s16 Right;
	s16 Center;
	s16 LFE;
	s16 LeftBack;
	s16 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		ProcessDplIISample16(src, this);
	}
};

struct Stereo51Out32DplII
{
	s32 Left;
	s32 Right;
	s32 Center;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		ProcessDplIISample32(src, this);
	}
};


struct Stereo51Out16Dpl
{
	s16 Left;
	s16 Right;
	s16 Center;
	s16 LFE;
	s16 LeftBack;
	s16 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		ProcessDplSample16(src, this);
	}
};

struct Stereo51Out32Dpl
{
	s32 Left;
	s32 Right;
	s32 Center;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		ProcessDplSample32(src, this);
	}
};

struct Stereo71Out16
{
	s16 Left;
	s16 Right;
	s16 Center;
	s16 LFE;
	s16 LeftBack;
	s16 RightBack;
	s16 LeftSide;
	s16 RightSide;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left >> SndOutVolumeShift;
		Right = src.Right >> SndOutVolumeShift;
		Center = (src.Left + src.Right) >> (SndOutVolumeShift + 1);
		LFE = Center;
		LeftBack = src.Left >> SndOutVolumeShift;
		RightBack = src.Right >> SndOutVolumeShift;

		LeftSide = src.Left >> (SndOutVolumeShift+1);
		RightSide = src.Right >> (SndOutVolumeShift+1);
	}
};

struct Stereo71Out32
{
	s32 Left;
	s32 Right;
	s32 Center;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;
	s32 LeftSide;
	s32 RightSide;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
		Center = (src.Left + src.Right) << (SndOutVolumeShift32 - 1);
		LFE = Center;
		LeftBack = src.Left << SndOutVolumeShift32;
		RightBack = src.Right << SndOutVolumeShift32;

		LeftSide = src.Left << (SndOutVolumeShift32 - 1);
		RightSide = src.Right << (SndOutVolumeShift32 - 1);
	}
};

struct Stereo20Out32
{
	s32 Left;
	s32 Right;
	
	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
	}
};

struct Stereo21Out32
{
	s32 Left;
	s32 Right;
	s32 LFE;
	
	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
		LFE = (src.Left + src.Right) << (SndOutVolumeShift32 - 1);
	}
};

struct Stereo41Out32
{
	s32 Left;
	s32 Right;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;
	
	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
		LFE = (src.Left + src.Right) << (SndOutVolumeShift32 - 1);

		LeftBack = src.Left << SndOutVolumeShift32;
		RightBack = src.Right << SndOutVolumeShift32;
	}
};

struct Stereo51Out32
{
	s32 Left;
	s32 Right;
	s32 Center;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;

	void ResampleFrom( const StereoOut32& src )
	{
		Left = src.Left << SndOutVolumeShift32;
		Right = src.Right << SndOutVolumeShift32;
		Center = (src.Left + src.Right) << (SndOutVolumeShift32 - 1);
		LFE = Center;
		LeftBack = src.Left << SndOutVolumeShift32;
		RightBack = src.Right << SndOutVolumeShift32;
	}
};

// Developer Note: This is a static class only (all static members).
class SndBuffer
{
private:
	static bool m_underrun_freeze;
	static s32 m_predictData;
	static float lastPct;

	static StereoOut32* sndTempBuffer;
	static StereoOut16* sndTempBuffer16;
	
	static int sndTempProgress;
	static int m_dsp_progress;

	static int m_timestretch_progress;
	static int m_timestretch_writepos;

	static StereoOut32 *m_buffer;
	static s32 m_size;

	static __aligned(4) volatile s32 m_rpos;
	static __aligned(4) volatile s32 m_wpos;
	
	static float lastEmergencyAdj;
	static float cTempo;
	static float eTempo;
	static int ssFreeze;

	static void _InitFail();
	static bool CheckUnderrunStatus( int& nSamples, int& quietSampleCount );

	static void soundtouchInit();
	static void soundtouchClearContents();
	static void soundtouchCleanup();
	static void timeStretchWrite();
	static void timeStretchUnderrun();
	static s32 timeStretchOverrun();

	static void PredictDataWrite( int samples );
	static float GetStatusPct();
	static void UpdateTempoChangeSoundTouch();
	static void UpdateTempoChangeSoundTouch2();

	static void _WriteSamples(StereoOut32* bData, int nSamples);
		
	static void _WriteSamples_Safe(StereoOut32* bData, int nSamples);
	static void _ReadSamples_Safe(StereoOut32* bData, int nSamples);

	static void _WriteSamples_Internal(StereoOut32 *bData, int nSamples);
	static void _DropSamples_Internal(int nSamples);
	static void _ReadSamples_Internal(StereoOut32 *bData, int nSamples);

	static int _GetApproximateDataInBuffer(); 
	
public:
	static void UpdateTempoChangeAsyncMixing();
	static void Init();
	static void Cleanup();
	static void Write( const StereoOut32& Sample );
	static s32 Test();
	static void ClearContents();

	// Note: When using with 32 bit output buffers, the user of this function is responsible
	// for shifting the values to where they need to be manually.  The fixed point depth of
	// the sample output is determined by the SndOutVolumeShift, which is the number of bits
	// to shift right to get a 16 bit result.
	template< typename T >
	static void ReadSamples( T* bData );
};

class SndOutModule
{
public:
	// Virtual destructor, because it helps fight C+++ funny-business.
	virtual ~SndOutModule() {}

	// Returns a unique identification string for this driver.
	// (usually just matches the driver's cpp filename)
	virtual const wchar_t* GetIdent() const=0;

	// Returns the long name / description for this driver.
	// (for use in configuration screen)
	virtual const wchar_t* GetLongName() const=0;

	virtual s32  Init()=0;
	virtual void Close()=0;
	virtual s32  Test() const=0;

	// Gui function: Used to open the configuration box for this driver.
	virtual void Configure(uptr parent)=0;

	// Loads settings from the INI file for this driver
	virtual void ReadSettings()=0;

	// Set output API for this driver
	virtual void SetApiSettings(wxString api) =0;

	// Saves settings to the INI file for this driver
	virtual void WriteSettings() const=0;
	
	// Returns the number of empty samples in the output buffer.
	// (which is effectively the amount of data played since the last update)
	virtual int GetEmptySampleCount() =0;
};


#ifdef _MSC_VER
//internal
extern SndOutModule* WaveOut;
extern SndOutModule* DSoundOut;
extern SndOutModule* XAudio2Out;
#endif
extern SndOutModule* PortaudioOut;
#ifdef __LINUX__
extern SndOutModule* AlsaOut;
#endif

extern SndOutModule* mods[];

// =====================================================================================================

extern bool WavRecordEnabled;

extern void RecordStart();
extern void RecordStop();
extern void RecordWrite( const StereoOut16& sample );

extern s32  DspLoadLibrary(wchar_t *fileName, int modNum);
extern void DspCloseLibrary();
extern int  DspProcess(s16 *buffer, int samples);
extern void DspUpdate(); // to let the Dsp process window messages
