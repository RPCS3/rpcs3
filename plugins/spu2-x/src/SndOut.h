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

// Overall master volume shift.
// Converts the mixer's 32 bit value into a 16 bit value.
static const int SndOutVolumeShift = 13;

// Samplerate of the SPU2. For accurate playback we need to match this
// exactly.  Trying to scale samplerates and maintain SPU2's Ts timing accuracy
// is too problematic. :)
static const int SampleRate = 48000;

extern int FindOutputModuleById( const wchar_t* omodid );

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
		static const u8 sLogTable[256] = {
			0x00,0x3C,0x60,0x78,0x8C,0x9C,0xA8,0xB4,0xBE,0xC8,0xD0,0xD8,0xDE,0xE4,0xEA,0xF0,
			0xF6,0xFA,0xFE,0x04,0x08,0x0C,0x10,0x14,0x16,0x1A,0x1E,0x20,0x24,0x26,0x2A,0x2C,
			0x2E,0x32,0x34,0x36,0x38,0x3A,0x3E,0x40,0x42,0x44,0x46,0x48,0x4A,0x4C,0x4E,0x50,
			0x50,0x52,0x54,0x56,0x58,0x5A,0x5A,0x5C,0x5E,0x60,0x60,0x62,0x64,0x66,0x66,0x68,
			0x6A,0x6A,0x6C,0x6E,0x6E,0x70,0x70,0x72,0x74,0x74,0x76,0x76,0x78,0x7A,0x7A,0x7C,
			0x7C,0x7E,0x7E,0x80,0x80,0x82,0x82,0x84,0x84,0x86,0x86,0x88,0x88,0x8A,0x8A,0x8C,
			0x8C,0x8C,0x8E,0x8E,0x90,0x90,0x92,0x92,0x92,0x94,0x94,0x96,0x96,0x96,0x98,0x98,
			0x9A,0x9A,0x9A,0x9C,0x9C,0x9C,0x9E,0x9E,0xA0,0xA0,0xA0,0xA2,0xA2,0xA2,0xA4,0xA4,
			0xA4,0xA6,0xA6,0xA6,0xA8,0xA8,0xA8,0xAA,0xAA,0xAA,0xAC,0xAC,0xAC,0xAC,0xAE,0xAE,
			0xAE,0xB0,0xB0,0xB0,0xB2,0xB2,0xB2,0xB2,0xB4,0xB4,0xB4,0xB6,0xB6,0xB6,0xB6,0xB8,
			0xB8,0xB8,0xB8,0xBA,0xBA,0xBA,0xBC,0xBC,0xBC,0xBC,0xBE,0xBE,0xBE,0xBE,0xC0,0xC0,
			0xC0,0xC0,0xC2,0xC2,0xC2,0xC2,0xC2,0xC4,0xC4,0xC4,0xC4,0xC6,0xC6,0xC6,0xC6,0xC8,
			0xC8,0xC8,0xC8,0xC8,0xCA,0xCA,0xCA,0xCA,0xCC,0xCC,0xCC,0xCC,0xCC,0xCE,0xCE,0xCE,
			0xCE,0xCE,0xD0,0xD0,0xD0,0xD0,0xD0,0xD2,0xD2,0xD2,0xD2,0xD2,0xD4,0xD4,0xD4,0xD4,
			0xD4,0xD6,0xD6,0xD6,0xD6,0xD6,0xD8,0xD8,0xD8,0xD8,0xD8,0xD8,0xDA,0xDA,0xDA,0xDA,
			0xDA,0xDC,0xDC,0xDC,0xDC,0xDC,0xDC,0xDE,0xDE,0xDE,0xDE,0xDE,0xDE,0xE0,0xE0,0xE0,
		};

		static s32 Gfl=0,Gfr=0;
		static s32 LMax=0,RMax=0;

		static s32 LAccum;
		static s32 RAccum;
		static s32 ANum;

		s32 ValL = src.Left >> (SndOutVolumeShift-8);
		s32 ValR = src.Right >> (SndOutVolumeShift-8);

		s32 XL = abs(ValL>>8);
		s32 XR = abs(ValR>>8);

		if(XL>LMax) LMax = XL;
		if(XR>RMax) RMax = XR;

		ANum++;
		if(ANum>=128)
		{
			ANum=0;
			LAccum = 1+((LAccum * 224 + LMax * 31)>>8);
			RAccum = 1+((RAccum * 224 + RMax * 31)>>8);

			LMax = 0;
			RMax = 0;

			s32 Tfl=(RAccum)*255/(LAccum);
			s32 Tfr=(LAccum)*255/(RAccum);

			int gMax = std::max(Tfl,Tfr);
			Tfl = Tfl*255/gMax;
			Tfr = Tfr*255/gMax;

			if(Tfl>255) Tfl=255;
			if(Tfr>255) Tfr=255;
			if(Tfl<1) Tfl=1;
			if(Tfr<1) Tfr=1;

			Gfl = (Gfl * 200 + Tfl * 56)>>8;
			Gfr = (Gfr * 200 + Tfr * 56)>>8;

		}

		s32 L,R,C,SUB,SL,SR;

		C=(ValL+ValR)>>1; //16.8

		ValL-=C;//16.8
		ValR-=C;//16.8

		L=ValL>>8; //16.0
		R=ValR>>8; //16.0
		C=C>>8;    //16.0
		SUB = C;

		{
			s32 Cfl = 1+sLogTable[Gfl];
			s32 Cfr = 1+sLogTable[Gfr];

			s32 VL=(ValL>>4) * Cfl; //16.12
			s32 VR=(ValR>>4) * Cfr;

			//s32 SC = (VL-VR)>>15;

			SL = (((VR/148 - VL/209)>>4)*Cfr)>>8;
			SR = (((VR/209 - VL/148)>>4)*Cfl)>>8;

		}

		// Random-ish values to get it to compile
		int GainL = 200;
		int GainR = 200;
		int GainC = 180;
		int GainSL = 230;
		int GainSR = 230;
		int GainLFE = 200;
		int AddCLR = 55;

		int AddCX  = (C * AddCLR)>>8;

		Left	= (((L   * GainL  ))>>8) + AddCX;
		Right	= (((R   * GainR  ))>>8) + AddCX;
		Center	= (((C   * GainC  ))>>8);
		LFE		= (((SUB * GainLFE))>>8);
		LeftBack	= (((SL  * GainSL ))>>8);
		RightBack	= (((SR  * GainSR ))>>8);
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
	static int ssFreeze;

	static void _InitFail();
	static void _WriteSamples(StereoOut32* bData, int nSamples);
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

	// Saves settings to the INI file for this driver
	virtual void WriteSettings() const=0;

	virtual bool Is51Out() const=0;

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
