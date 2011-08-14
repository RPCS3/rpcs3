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


StereoOut32 StereoOut32::Empty( 0, 0 );

StereoOut32::StereoOut32( const StereoOut16& src ) :
	Left( src.Left ),
	Right( src.Right )
{
}

StereoOut32::StereoOut32( const StereoOutFloat& src ) :
	Left( (s32)(src.Left * 2147483647.0f) ),
	Right( (s32)(src.Right * 2147483647.0f) )
{
}

StereoOut16 StereoOut32::DownSample() const
{
	return StereoOut16(
		Left >> SndOutVolumeShift,
		Right >> SndOutVolumeShift
	);
}

StereoOut32 StereoOut16::UpSample() const
{
	return StereoOut32(
		Left << SndOutVolumeShift,
		Right << SndOutVolumeShift
	);

}


class NullOutModule: public SndOutModule
{
public:
	s32  Init()  { return 0; }
	void Close() { }
	s32  Test() const { return 0; }
	void Configure(uptr parent)  { }
	int GetEmptySampleCount()  { return 0; }

	const wchar_t* GetIdent() const
	{
		return L"nullout";
	}

	const wchar_t* GetLongName() const
	{
		return L"No Sound (Emulate SPU2 only)";
	}

	void ReadSettings()
	{
	}

	void SetApiSettings(wxString api)
	{
	}

	void WriteSettings() const
	{
	}

} NullOut;

SndOutModule* mods[]=
{
	&NullOut,
#ifdef _MSC_VER
	XAudio2Out,
	DSoundOut,
	WaveOut,
#endif
	PortaudioOut,
#ifdef __LINUX__
	AlsaOut,
#endif
	NULL		// signals the end of our list
};

int FindOutputModuleById( const wchar_t* omodid )
{
	int modcnt = 0;
	while( mods[modcnt] != NULL )
	{
		if( wcscmp( mods[modcnt]->GetIdent(), omodid ) == 0 )
			break;
		++modcnt;
	}
	return modcnt;
}

StereoOut32 *SndBuffer::m_buffer;
s32 SndBuffer::m_size;
__aligned(4) volatile s32 SndBuffer::m_rpos;
__aligned(4) volatile s32 SndBuffer::m_wpos;

bool SndBuffer::m_underrun_freeze;
StereoOut32* SndBuffer::sndTempBuffer = NULL;
StereoOut16* SndBuffer::sndTempBuffer16 = NULL;
int SndBuffer::sndTempProgress = 0;

int GetAlignedBufferSize( int comp )
{
	return (comp + SndOutPacketSize-1) & ~(SndOutPacketSize-1);
}

// Returns TRUE if there is data to be output, or false if no data
// is available to be copied.
bool SndBuffer::CheckUnderrunStatus( int& nSamples, int& quietSampleCount )
{
	quietSampleCount = 0;

	int data = _GetApproximateDataInBuffer();
	if( m_underrun_freeze )
	{
		int toFill = m_size / ( (SynchMode == 2) ? 32 : 400); // TimeStretch and Async off?
		toFill = GetAlignedBufferSize( toFill );

		// toFill is now aligned to a SndOutPacket

		if( data < toFill )
		{
			quietSampleCount = nSamples;
			return false;
		}

		m_underrun_freeze = false;
		if( MsgOverruns() )
			ConLog(" * SPU2 > Underrun compensation (%d packets buffered)\n", toFill / SndOutPacketSize );
		lastPct = 0.0;		// normalize timestretcher
	}
	else if( data < nSamples )
	{
		nSamples = data;
		quietSampleCount = SndOutPacketSize - data;
		m_underrun_freeze = true;

		if( SynchMode == 0 ) // TimeStrech on
			timeStretchUnderrun();

		return nSamples != 0;
	}

	return true;
}

void SndBuffer::_InitFail()
{
	// If a failure occurs, just initialize the NoSound driver.  This'll allow
	// the game to emulate properly (hopefully), albeit without sound.
	OutputModule = FindOutputModuleById( NullOut.GetIdent() );
	mods[OutputModule]->Init();
}

int SndBuffer::_GetApproximateDataInBuffer()
{
	// WARNING: not necessarily 100% up to date by the time it's used, but it will have to do.
	return (m_wpos + m_size - m_rpos) % m_size;
}

void SndBuffer::_WriteSamples_Internal(StereoOut32 *bData, int nSamples)
{
	// WARNING: This assumes the write will NOT wrap around,
	// and also assumes there's enough free space in the buffer.

	memcpy(m_buffer + m_wpos, bData, nSamples * sizeof(StereoOut32));
	m_wpos = (m_wpos + nSamples) % m_size;
}

void SndBuffer::_DropSamples_Internal(int nSamples)
{
	m_rpos = (m_rpos + nSamples) % m_size;
}

void SndBuffer::_ReadSamples_Internal(StereoOut32 *bData, int nSamples)
{
	// WARNING: This assumes the read will NOT wrap around,
	// and also assumes there's enough data in the buffer.
	memcpy(bData, m_buffer + m_rpos, nSamples * sizeof(StereoOut32));
	_DropSamples_Internal(nSamples);
}

void SndBuffer::_WriteSamples_Safe(StereoOut32 *bData, int nSamples)
{
	// WARNING: This code assumes there's only ONE writing process.
	if( (m_size - m_wpos) < nSamples)
	{
		int b1 = m_size - m_wpos;
		int b2 = nSamples - b1;

		_WriteSamples_Internal(bData, b1);
		_WriteSamples_Internal(bData+b1, b2);
	}
	else
	{
		_WriteSamples_Internal(bData, nSamples);
	}
}

void SndBuffer::_ReadSamples_Safe(StereoOut32* bData, int nSamples)
{
	// WARNING: This code assumes there's only ONE reading process.
	if( (m_size - m_rpos) < nSamples)
	{
		int b1 = m_size - m_rpos;
		int b2 = nSamples - b1;

		_ReadSamples_Internal(bData, b1);
		_ReadSamples_Internal(bData+b1, b2);
	}
	else
	{
		_ReadSamples_Internal(bData, nSamples);
	}
}

// Note: When using with 32 bit output buffers, the user of this function is responsible
// for shifting the values to where they need to be manually.  The fixed point depth of
// the sample output is determined by the SndOutVolumeShift, which is the number of bits
// to shift right to get a 16 bit result.
template<typename T> void SndBuffer::ReadSamples(T* bData)
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
		
		// WARNING: This code assumes there's only ONE reading process.
		int b1 = m_size - m_rpos;

		if(b1 > nSamples)
			b1 = nSamples;

		// First part
		for( int i=0; i<b1; i++ )
			bData[i].ResampleFrom( m_buffer[i + m_rpos] );

		// Second part
		int b2 = nSamples - b1;
		for( int i=0; i<b2; i++ )
			bData[i+b1].ResampleFrom( m_buffer[i] );

		_DropSamples_Internal(nSamples);
	}

	// If quietSamples != 0 it means we have an underrun...
	// Let's just dull out some silence, because that's usually the least
	// painful way of dealing with underruns:
	memset( bData, 0, quietSamples * sizeof(T) );
}

template void SndBuffer::ReadSamples(StereoOut16*);
template void SndBuffer::ReadSamples(StereoOut32*);

//template void SndBuffer::ReadSamples(StereoOutFloat*);
template void SndBuffer::ReadSamples(Stereo21Out16*);
template void SndBuffer::ReadSamples(Stereo40Out16*);
template void SndBuffer::ReadSamples(Stereo41Out16*);
template void SndBuffer::ReadSamples(Stereo51Out16*);
template void SndBuffer::ReadSamples(Stereo51Out16Dpl*);
template void SndBuffer::ReadSamples(Stereo51Out16DplII*);
template void SndBuffer::ReadSamples(Stereo71Out16*);

template void SndBuffer::ReadSamples(Stereo20Out32*);
template void SndBuffer::ReadSamples(Stereo21Out32*);
template void SndBuffer::ReadSamples(Stereo40Out32*);
template void SndBuffer::ReadSamples(Stereo41Out32*);
template void SndBuffer::ReadSamples(Stereo51Out32*);
template void SndBuffer::ReadSamples(Stereo51Out32Dpl*);
template void SndBuffer::ReadSamples(Stereo51Out32DplII*);
template void SndBuffer::ReadSamples(Stereo71Out32*);

void SndBuffer::_WriteSamples(StereoOut32 *bData, int nSamples)
{
	m_predictData = 0;

	// Problem:
	//  If the SPU2 gets out of sync with the SndOut device, the writepos of the
	//  circular buffer will overtake the readpos, leading to a prolonged period
	//  of hopscotching read/write accesses (ie, lots of staticy crap sound for
	//  several seconds).
	//
	// Compromise:
	//  When an overrun occurs, we adapt by discarding a portion of the buffer.
	//  The older portion of the buffer is discarded rather than incoming data,
	//  so that the overall audio synchronization is better.

	int free = m_size - _GetApproximateDataInBuffer(); // -1, but the <= handles that
	if( free <= nSamples )
	{
		// Disabled since the lock-free queue can't handle changing the read end from the write thread
#if 0
		// Buffer overrun!
		// Dump samples from the read portion of the buffer instead of dropping
		// the newly written stuff.

		s32 comp;

		if( SynchMode == 0 ) // TimeStrech on
		{
			comp = timeStretchOverrun();
		}
		else
		{
			// Toss half the buffer plus whatever's being written anew:
			comp = GetAlignedBufferSize( (m_size + nSamples ) / 16 );
			if( comp > (m_size-SndOutPacketSize) ) comp = m_size-SndOutPacketSize;
		}

		_DropSamples_Internal(comp);

		if( MsgOverruns() )
			ConLog(" * SPU2 > Overrun Compensation (%d packets tossed)\n", comp / SndOutPacketSize );
		lastPct = 0.0;		// normalize the timestretcher
#else
		if( MsgOverruns() )
			ConLog(" * SPU2 > Overrun! 1 packet tossed)\n");
		lastPct = 0.0;		// normalize the timestretcher
		return;
#endif
	}

	_WriteSamples_Safe(bData, nSamples);
}

void SndBuffer::Init()
{
	if( mods[OutputModule] == NULL )
	{
		_InitFail();
		return;
	}

	// initialize sound buffer
	// Buffer actually attempts to run ~50%, so allocate near double what
	// the requested latency is:
	
	m_rpos = 0;
	m_wpos = 0;

	try
	{
		const float latencyMS = SndOutLatencyMS * 16;
		m_size = GetAlignedBufferSize( (int)(latencyMS * SampleRate / 1000.0f ) );
		m_buffer = new StereoOut32[m_size];
		m_underrun_freeze = false;

		sndTempBuffer = new StereoOut32[SndOutPacketSize];
		sndTempBuffer16 = new StereoOut16[SndOutPacketSize * 2]; // in case of leftovers.
	}
	catch( std::bad_alloc& )
	{
		// out of memory exception (most likely)

		SysMessage( "Out of memory error occurred while initializing SPU2." );
		_InitFail();
		return;
	}

	// clear buffers!
	// Fixes loopy sounds on emu resets.
	memset( sndTempBuffer, 0, sizeof(StereoOut32) * SndOutPacketSize );
	memset( sndTempBuffer16, 0, sizeof(StereoOut16) * SndOutPacketSize );

	sndTempProgress = 0;

	soundtouchInit();		// initializes the timestretching

	// initialize module
	if( mods[OutputModule]->Init() == -1 ) _InitFail();
}

void SndBuffer::Cleanup()
{
	mods[OutputModule]->Close();

	soundtouchCleanup();

	safe_delete_array( m_buffer );
	safe_delete_array( sndTempBuffer );
	safe_delete_array( sndTempBuffer16 );
}

int SndBuffer::m_dsp_progress = 0;

int SndBuffer::m_timestretch_progress = 0;
int SndBuffer::ssFreeze = 0;

void SndBuffer::ClearContents()
{
	SndBuffer::soundtouchClearContents();
	SndBuffer::ssFreeze = 256; //Delays sound output for about 1 second.
}

void SndBuffer::Write( const StereoOut32& Sample )
{
	// Log final output to wavefile.
	WaveDump::WriteCore( 1, CoreSrc_External, Sample.DownSample() );

	if( WavRecordEnabled ) RecordWrite( Sample.DownSample() );

	if(mods[OutputModule] == &NullOut) // null output doesn't need buffering or stretching! :p
		return;

	sndTempBuffer[sndTempProgress++] = Sample;

	// If we haven't accumulated a full packet yet, do nothing more:
	if(sndTempProgress < SndOutPacketSize) return;
	sndTempProgress = 0;

	//Don't play anything directly after loading a savestate, avoids static killing your speakers.
	if ( ssFreeze > 0 )
	{
		ssFreeze--;
		memset( sndTempBuffer, 0, sizeof(StereoOut32) * SndOutPacketSize ); // Play silence
	}
#ifndef __LINUX__
	if( dspPluginEnabled )
	{
		// Convert in, send to winamp DSP, and convert out.

		int ei= m_dsp_progress;
		for( int i=0; i<SndOutPacketSize; ++i, ++ei ) { sndTempBuffer16[ei] = sndTempBuffer[i].DownSample(); }
		m_dsp_progress += DspProcess( (s16*)sndTempBuffer16 + m_dsp_progress, SndOutPacketSize );

		// Some ugly code to ensure full packet handling:
		ei = 0;
		while( m_dsp_progress >= SndOutPacketSize )
		{
			for( int i=0; i<SndOutPacketSize; ++i, ++ei ) { sndTempBuffer[i] = sndTempBuffer16[ei].UpSample(); }

			if( SynchMode == 0 ) // TimeStrech on
				timeStretchWrite();
			else
				_WriteSamples(sndTempBuffer, SndOutPacketSize);

			m_dsp_progress -= SndOutPacketSize;
		}

		// copy any leftovers to the front of the dsp buffer.
		if( m_dsp_progress > 0 )
		{
			memcpy( sndTempBuffer16, &sndTempBuffer16[ei],
				sizeof(sndTempBuffer16[0]) * m_dsp_progress
			);
		}
	}
#endif
	else
	{
		if( SynchMode == 0 ) // TimeStrech on
			timeStretchWrite();
		else
			_WriteSamples(sndTempBuffer, SndOutPacketSize);
	}
}

s32 SndBuffer::Test()
{
	if( mods[OutputModule] == NULL )
		return -1;

	return mods[OutputModule]->Test();
}
