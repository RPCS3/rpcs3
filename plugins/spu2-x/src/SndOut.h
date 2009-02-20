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
#ifndef SNDOUT_H_INCLUDE
#define SNDOUT_H_INCLUDE

#include "BaseTypes.h"
#include "Lowpass.h"

// Number of stereo samples per SndOut block.
// All drivers must work in units of this size when communicating with
// SndOut.
static const int SndOutPacketSize = 512;

// Overall master volume shift.
// Converts the mixer's 32 bit value into a 16 bit value.
static const int SndOutVolumeShift = 13;

// Samplerate of the SPU2. For accurate playback we need to match this
// exactly.  Trying to scale samplerates and maintain SPU2's Ts timing accuracy
// is too problematic. :)
static const int SampleRate = 48000;

int FindOutputModuleById( const wchar_t* omodid );

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

struct StereoQuadOut16
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

struct Stereo21Out32
{
	s32 Left;
	s32 Right;
	s32 LFE;
};

struct Stereo41Out32
{
	s32 Left;
	s32 Right;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;
};

struct Stereo51Out32
{
	s32 Left;
	s32 Right;
	s32 Center;
	s32 LFE;
	s32 LeftBack;
	s32 RightBack;
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
	static int m_dsp_writepos;

	static int m_timestretch_progress;
	static int m_timestretch_writepos;

	static StereoOut32 *m_buffer;
	static s32 m_size;
	static s32 m_rpos;
	static s32 m_wpos;
	static s32 m_data;

	static float lastEmergencyAdj;
	static float cTempo;
	static float eTempo;
	static int freezeTempo;


	static void _InitFail();
	static void _WriteSamples(StereoOut32* bData, int nSamples);
	static bool CheckUnderrunStatus( int& nSamples, int& quietSampleCount );

	static void soundtouchInit();
	static void soundtouchCleanup();
	static void timeStretchWrite();
	static void timeStretchUnderrun();
	static s32 timeStretchOverrun();
	
	static void PredictDataWrite( int samples );
	static float GetStatusPct();
	static void UpdateTempoChange();
	
public:
	static void Init();
	static void Cleanup();
	static void Write( const StereoOut32& Sample );
	static s32 Test();

#ifdef _MSC_VER
	static void Configure(HWND parent, u32 module );
#else
	static void Configure(uptr parent, u32 module );
#endif
	
	// Note: When using with 32 bit output buffers, the user of this function is responsible
	// for shifting the values to where they need to be manually.  The fixed point depth of
	// the sample output is determined by the SndOutVolumeShift, which is the number of bits
	// to shift right to get a 16 bit result.
	template< typename T >
	static void ReadSamples( T* bData )
	{
		int nSamples = SndOutPacketSize;

		// Problem:
		//  If the SPU2 gets even the least bit out of sync with the SndOut device,
		//  the readpos of the circular buffer will overtake the writepos,
		//  leading to a prolonged period of hopscotching read/write accesses (ie,
		//  lots of staticy crap sound for several seconds).
		//
		// Fix:
		//  If the read position overtakes the write position, abort the
		//  transfer immediately and force the SndOut driver to wait until
		//  the read buffer has filled up again before proceeding.
		//  This will cause one brief hiccup that can never exceed the user's
		//  set buffer length in duration.

		int quietSamples;
		if( CheckUnderrunStatus( nSamples, quietSamples ) )
		{
			jASSUME( nSamples <= SndOutPacketSize );

			// [Air] [TODO]: This loop is probably a candidate for SSE2 optimization.

			const int endPos = m_rpos + nSamples;
			const int secondCopyLen = endPos - m_size;
			const StereoOut32* rposbuffer = &m_buffer[m_rpos];

			m_data -= nSamples;

			if( secondCopyLen > 0 )
			{
				nSamples -= secondCopyLen;
				for( int i=0; i<secondCopyLen; i++ )
					bData[nSamples+i].ResampleFrom( m_buffer[i] );
				m_rpos = secondCopyLen;
			}
			else
				m_rpos += nSamples;

			for( int i=0; i<nSamples; i++ )
				bData[i].ResampleFrom( rposbuffer[i] );
		}

		// If quietSamples != 0 it means we have an underrun...
		// Let's just dull out some silence, because that's usually the least
		// painful way of dealing with underruns:
		memset( bData, 0, quietSamples * sizeof(T) );
	}
};

class SndOutModule
{
public:
	// Virtual destructor, because it helps fight C+++ funny-business.
	virtual ~SndOutModule(){};

	// Returns a unique identification string for this driver.
	// (usually just matches the driver's cpp filename)
	virtual const wchar_t* GetIdent() const=0;

	// Returns the long name / description for this driver.
	// (for use in configuration screen)
	virtual const wchar_t* GetLongName() const=0;

	virtual s32  Init()=0;
	virtual void Close()=0;
	virtual s32  Test() const=0;
#ifdef _MSC_VER
	virtual void Configure(HWND parent)=0;
#else
	virtual void Configure(uptr parent)=0;
#endif
	virtual bool Is51Out() const=0;

	// Returns the number of empty samples in the output buffer.
	// (which is effectively the amount of data played since the last update)
	virtual int GetEmptySampleCount() const=0;
};


//internal
extern SndOutModule* WaveOut;
extern SndOutModule* DSoundOut;
extern SndOutModule* XAudio2Out;

extern SndOutModule* mods[];

#endif // SNDOUT_H_INCLUDE
