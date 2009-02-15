/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

// [TODO] : The layout of this code file is now a complete hackish mess after
// numerous timestretch-related additions.  The whole thing should really be
// rethought and redone at this point.

#include "spu2.h"
#include "SoundTouch/SoundTouch.h"
#include "SoundTouch/WavFile.h"

#include <new>

static int ts_stats_stretchblocks = 0;
static int ts_stats_normalblocks = 0;
static int ts_stats_logcounter = 0;

class NullOutModule: public SndOutModule
{
public:
	s32  Init(SndBuffer *)  { return 0; }
	void Close() { }
	s32  Test() const { return 0; }
	void Configure(HWND parent)  { }
	bool Is51Out() const { return false; }
	int GetEmptySampleCount() const { return 0; }
	
	const wchar_t* GetIdent() const
	{
		return _T("nullout");
	}

	const wchar_t* GetLongName() const
	{
		return _T("No Sound (Emulate SPU2 only)");
	}

} NullOut;

SndOutModule* mods[]=
{
	&NullOut,
	XAudio2Out,
	DSoundOut,
	WaveOut,
	//ASIOOut,
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


__forceinline s16 SndScaleVol( s32 inval )
{
	return inval >> SndOutVolumeShift;
}


// records last buffer status (fill %, range -100 to 100, with 0 being 50% full)
float lastPct;
float lastEmergencyAdj;

float cTempo=1;
float eTempo = 1;
int freezeTempo = 0;

soundtouch::SoundTouch* pSoundTouch=NULL;


//usefull when timestretch isn't available 

class SndBufferImpl: public SndBuffer
{
private:
	s32 *buffer;
	s32 size;
	s32 rpos;
	s32 wpos;
	s32 data;

	// data prediction amount, used to "commit" data that hasn't
	// finished timestretch processing.
	s32 predictData;

	bool pw;
	bool underrun_freeze;

protected:
	int GetAlignedBufferSize( int comp )
	{
		return (comp + SndOutPacketSize-1) & ~(SndOutPacketSize-1);
	}

public:
	SndBufferImpl( float latencyMS )
	{
		rpos=0;
		wpos=0;
		data=0;
		size=GetAlignedBufferSize( (int)(latencyMS * SampleRate / 500.0f ) );
		buffer = new s32[size];
		pw=false;
		underrun_freeze = false;
		predictData = 0;
	}

	virtual ~SndBufferImpl()
	{
		delete buffer;
	}

	virtual void WriteSamples(s32 *bData, int nSamples)
	{
		int free = size-data;
		predictData = 0;

		jASSUME( data <= size );

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
		
		if( free < nSamples )
		{
			// Buffer overrun!
			// Dump samples from the read portion of the buffer instead of dropping
			// the newly written stuff.

			s32 comp;

			if( !timeStretchDisabled )
			{
				// If we overran it means the timestretcher failed.  We need to speed
				// up audio playback.
				cTempo += cTempo * 0.12f;
				eTempo += eTempo * 0.40f;
				if( eTempo > 7.5f ) eTempo = 7.5f;
				pSoundTouch->setTempo( eTempo );

				// Throw out just a little bit (two packets worth) to help
				// give the TS some room to work:

				comp = SndOutPacketSize*2;
			}
			else
			{
				// Toss half the buffer plus whatever's being written anew:
				comp = GetAlignedBufferSize( (size + nSamples ) / 2 );
				if( comp > (size-SndOutPacketSize) ) comp = size-SndOutPacketSize;
			}

			data -= comp;
			rpos = (rpos+comp)%size;
			if( MsgOverruns() )
				ConLog(" * SPU2 > Overrun Compensation (%d packets tossed)\n", comp / SndOutPacketSize );
			lastPct = 0.0;		// normalize the timestretcher
		}

		// copy in two phases, since there's a chance the packet
		// wraps around the buffer (it'd be nice to deal in packets only, but
		// the timestretcher and DSP options require flexibility).

		const int endPos = wpos + nSamples;
		const int secondCopyLen = endPos - size;
		s32* wposbuffer = &buffer[wpos];

		data += nSamples;
		if( secondCopyLen > 0 )
		{
			nSamples -= secondCopyLen;
			memcpy( buffer, &bData[nSamples], secondCopyLen * sizeof( *bData ) );
			wpos = secondCopyLen;
		}
		else
			wpos += nSamples;

		memcpy( wposbuffer, bData, nSamples * sizeof( *bData ) );
	}

	protected:
	// Returns TRUE if there is data to be output, or false if no data
	// is available to be copied.
	bool CheckUnderrunStatus( int& nSamples, int& quietSampleCount )
	{
		quietSampleCount = 0;
		if( underrun_freeze )
		{			
			int toFill = (int)(size * ( timeStretchDisabled ? 0.50f : 0.1f ) );
			toFill = GetAlignedBufferSize( toFill );

			// toFill is now aligned to a SndOutPacket

			if( data < toFill )
			{
				quietSampleCount = nSamples;
				return false;
			}

			underrun_freeze = false;
			if( MsgOverruns() )
				ConLog(" * SPU2 > Underrun compensation (%d packets buffered)\n", toFill / SndOutPacketSize );
			lastPct = 0.0;		// normalize timestretcher
		}
		else if( data < nSamples )
		{
			nSamples = data;
			quietSampleCount = SndOutPacketSize - data;
			underrun_freeze = true;

			if( !timeStretchDisabled )
			{
				// timeStretcher failed it's job.  We need to slow down the audio some.

				cTempo -= (cTempo * 0.12f);
				eTempo -= (eTempo * 0.30f);
				if( eTempo < 0.1f ) eTempo = 0.1f;
				pSoundTouch->setTempo( eTempo );
			}

			return nSamples != 0;
		}

		return true;
	}

public:
	void ReadSamples( s16* bData )
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

			// [Air] [TODO]: This loop is probably a candidiate for SSE2 optimization.

			const int endPos = rpos + nSamples;
			const int secondCopyLen = endPos - size;
			const s32* rposbuffer = &buffer[rpos];

			data -= nSamples;

			if( secondCopyLen > 0 )
			{
				nSamples -= secondCopyLen;
				for( int i=0; i<secondCopyLen; i++ )
					bData[nSamples+i] = SndScaleVol( buffer[i] );
				rpos = secondCopyLen;
			}
			else
				rpos += nSamples;

			for( int i=0; i<nSamples; i++ )
				bData[i] = SndScaleVol( rposbuffer[i] );
		}

		// If quietSamples != 0 it means we have an underrun...
		// Let's just dull out some silence, because that's usually the least
		// painful way of dealing with underruns:
		memset( bData, 0, quietSamples * sizeof(*bData) );
	}

	void ReadSamples( s32* bData )
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
			// nSamples is garaunteed non-zero if CheckUnderrunStatus
			// returned true.

			const int endPos = rpos + nSamples;
			const int secondCopyLen = endPos - size;
			const int oldrpos = rpos;

			data -= nSamples;

			if( secondCopyLen > 0 )
			{
				nSamples -= secondCopyLen;
				memcpy( &bData[nSamples], buffer, secondCopyLen * sizeof( *bData ) );
				rpos = secondCopyLen;
			}
			else
				rpos += nSamples;

			memcpy( bData, &buffer[oldrpos], nSamples * sizeof( *bData ) );
		}

		// If quietSamples != 0 it means we have an underrun...
		// Let's just dull out some silence, because that's usually the least
		// painful way of dealing with underruns:
		memset( bData, 0, quietSamples * sizeof(*bData) );
	}

	void PredictDataWrite( int samples )
	{
		predictData += samples;
	}

	virtual void PauseOnWrite(bool doPause) { pw = doPause; }

	// Calculate the buffer status percentage.
	// Returns range from -1.0 to 1.0
	//    1.0 = buffer overflow!
	//    0.0 = buffer nominal (50% full)
	//   -1.0 = buffer underflow!
	float GetStatusPct()
	{
		// Get the buffer status of the output driver too, so that we can
		// obtain a more accurate overall buffer status.

		int drvempty = mods[OutputModule]->GetEmptySampleCount(); // / 2;

		//ConLog( "Data %d >>> driver: %d   predict: %d\n", data, drvempty, predictData );

		float result = (float)(data + predictData - drvempty) - (size/2);
		result /= (size/2);
		return result;
	}

};

SndBufferImpl *sndBuffer=NULL;

s32* sndTempBuffer=NULL;
s32 sndTempProgress=NULL;
s16* sndTempBuffer16=NULL;

void UpdateTempoChange()
{
	if( --freezeTempo > 0 )
	{
		return;
	}

	float statusPct = sndBuffer->GetStatusPct();
	float pctChange = statusPct - lastPct;

	float tempoChange;
	float emergencyAdj = 0;
	float newcee = cTempo;		// workspace var. for cTempo

	// IMPORTANT!
	// If you plan to tweak these values, make sure you're using a release build
	// OUTSIDE THE DEBUGGER to test it!  The Visual Studio debugger can really cause
	// erratic behavior in the audio buffers, and makes the timestretcher seem a
	// lot more inconsistent than it really is.

	// We have two factors.
	//   * Distance from nominal buffer status (50% full)
	//   * The change from previous update to this update.

	// Prediction based on the buffer change:
	// (linear seems to work better here)

	tempoChange = pctChange * 0.75f;

	if( statusPct * tempoChange < 0.0f )
	{
		// only apply tempo change if it is in synch with the buffer status.
		// In other words, if the buffer is high (over 0%), and is decreasing,
		// ignore it.  It'll just muck things up.

		tempoChange = 0;
	}

	// Sudden spikes in framerate can cause the nominal buffer status
	// to go critical, in which case we have to enact an emergency
	// stretch. The following cubic formulas do that.  Values near
	// the extremeites give much larger results than those near 0.
	// And the value is added only this time, and does not accumulate.
	// (otherwise a large value like this would cause problems down the road)

	// Constants:
	// Weight - weights the statusPct's "emergency" consideration.
	//   higher values here will make the buffer perform more drastic
	//   compensations at the outer edges of the buffer (at -75 or +75%
	//   or beyond, for example).

	// Range - scales the adjustment to the given range (more or less).
	//   The actual range is dependent on the weight used, so if you increase
	//   Weight you'll usually want to decrease Range somewhat to compensate.

	// Prediction based on the buffer fill status:

	const float statusWeight = 2.99f;
	const float statusRange = 0.068f;

	// "non-emergency" deadzone:  In this area stretching will be strongly discouraged.
	// Note: due tot he nature of timestretch latency, it's always a wee bit harder to
	// cope with low fps (underruns) tha it is high fps (overruns).  So to help out a
	// little, the low-end portions of this check are less forgiving than the high-sides.

	if( cTempo < 0.965f || cTempo > 1.060f ||
		pctChange < -0.38f || pctChange > 0.54f ||
		statusPct < -0.32f || statusPct > 0.39f ||
		eTempo < 0.89f || eTempo > 1.19f )
	{
		emergencyAdj = ( pow( statusPct*statusWeight, 3.0f ) * statusRange);
	}

	// Smooth things out by factoring our previous adjustment into this one.
	// It helps make the system 'feel' a little smarter by  giving it at least
	// one packet worth of history to help work off of:

	emergencyAdj = (emergencyAdj * 0.75f) + (lastEmergencyAdj * 0.25f );

	lastEmergencyAdj = emergencyAdj;
	lastPct = statusPct;

	// Accumulate a fraction of the tempo change into the tempo itself.
	// This helps the system run "smarter" to games that run consistently
	// fast or slow by altering the base tempo to something closer to the
	// game's active speed.  In tests most games normalize within 2 seconds
	// at 100ms latency, which is pretty good (larger buffers normalize even
	// quicker).

	newcee += newcee * (tempoChange+emergencyAdj) * 0.03f;

	// Apply tempoChange as a scale of cTempo.  That way the effect is proportional
	// to the current tempo.  (otherwise tempos rate of change at the extremes would
	// be too drastic)

	float newTempo = newcee + ( emergencyAdj * cTempo );

	// ... and as a final optimization, only stretch if the new tempo is outside
	// a nominal threshold.  Keep this threshold check small, because it could
	// cause some serious side effects otherwise. (enlarging the cTempo check above
	// is usually better/safer)
	if( newTempo < 0.970f || newTempo > 1.045f )
	{
		cTempo = (float)newcee;

		if( newTempo < 0.10f ) newTempo = 0.10f;
		else if( newTempo > 10.0f ) newTempo = 10.0f;

		if( cTempo < 0.15f ) cTempo = 0.15f;
		else if( cTempo > 7.5f ) cTempo = 7.5f;

		pSoundTouch->setTempo( eTempo = (float)newTempo );
		ts_stats_stretchblocks++;

		/*ConLog(" * SPU2: [Nominal %d%%] [Emergency: %d%%] (baseTempo: %d%% ) (newTempo: %d%%) (buffer: %d%%)\n",
			//(relation < 0.0) ? "Normalize" : "",
			(int)(tempoChange * 100.0 * 0.03),
			(int)(emergencyAdj * 100.0),
			(int)(cTempo * 100.0),
			(int)(newTempo * 100.0),
			(int)(statusPct * 100.0)
		);*/
	}
	else
	{
		// Nominal operation -- turn off stretching.
		// note: eTempo 'slides' toward 1.0 for smoother audio and better
		// protection against spikes.
		if( cTempo != 1.0f )
		{
			cTempo = 1.0f;
			eTempo = ( 1.0f + eTempo ) * 0.5f;
			pSoundTouch->setTempo( eTempo );
		}
		else
		{
			if( eTempo != cTempo )
				pSoundTouch->setTempo( eTempo=cTempo );
			ts_stats_normalblocks++;
		}
	}
}

void soundtouchInit()
{
	pSoundTouch = new soundtouch::SoundTouch();
	pSoundTouch->setSampleRate(SampleRate);
    pSoundTouch->setChannels(2);

    pSoundTouch->setSetting( SETTING_USE_QUICKSEEK, 0 );
    pSoundTouch->setSetting( SETTING_USE_AA_FILTER, 0 );

	pSoundTouch->setSetting( SETTING_SEQUENCE_MS, SoundtouchCfg::SequenceLenMS );
	pSoundTouch->setSetting( SETTING_SEEKWINDOW_MS, SoundtouchCfg::SeekWindowMS );
	pSoundTouch->setSetting( SETTING_OVERLAP_MS, SoundtouchCfg::OverlapMS );

	pSoundTouch->setTempo(1);

	// some timestretch management vars:

	cTempo = 1.0;
	eTempo = 1.0;
	lastPct = 0;
	lastEmergencyAdj = 0;

	// just freeze tempo changes for a while at startup.
	// the driver buffers are bogus anyway.
	freezeTempo = 8;
}

static void _sndInitFail()
{
	// If a failure occurs, just initialize the NoSound driver.  This'll allow
	// the game to emulate properly (hopefully), albeit without sound.
	OutputModule = FindOutputModuleById( NullOut.GetIdent() );
	mods[OutputModule]->Init( sndBuffer );
}

s32 SndInit()
{
	if( mods[OutputModule] == NULL )
	{
		_sndInitFail();
		return 0;
	}

	// initialize sound buffer
	// Buffer actually attempts to run ~50%, so allocate near double what
	// the requested latency is:

	try
	{
		sndBuffer = new SndBufferImpl( SndOutLatencyMS * (timeStretchDisabled ? 1.5f : 2.0f ) );
		sndTempBuffer = new s32[SndOutPacketSize];
		sndTempBuffer16 = new s16[SndOutPacketSize];
	}
	catch( std::bad_alloc& )
	{
		// out of memory exception (most likely)

		SysMessage( "Out of memory error occured while initializing SPU2." );
		_sndInitFail();
		return 0;
	}

	// clear buffers!
	// Fixes loopy sounds on emu resets.
	memset( sndTempBuffer, 0, sizeof(s32) * SndOutPacketSize );
	memset( sndTempBuffer16, 0, sizeof(s16) * SndOutPacketSize );

	sndTempProgress = 0;

	soundtouchInit();		// initializes the timestretching

	// some crap
	spdif_set51(mods[OutputModule]->Is51Out());

	// initialize module
	if( mods[OutputModule]->Init(sndBuffer) == -1 )
	{
		_sndInitFail();
	}

	return 0;
}

void SndClose()
{
	mods[OutputModule]->Close();

	SAFE_DELETE_OBJ( sndBuffer );
	SAFE_DELETE_ARRAY( sndTempBuffer );
	SAFE_DELETE_ARRAY( sndTempBuffer16 );
	SAFE_DELETE_OBJ( pSoundTouch );
}

s32 SndWrite(s32 ValL, s32 ValR)
{
	// Log final output to wavefile.
	WaveDump::WriteCore( 1, CoreSrc_External, SndScaleVol(ValL), SndScaleVol(ValR) );

	RecordWrite(SndScaleVol(ValL),SndScaleVol(ValR));
 
	if(mods[OutputModule] == &NullOut) // null output doesn't need buffering or stretching! :p
		return 0;
 
	sndTempBuffer[sndTempProgress++] = ValL;
	sndTempBuffer[sndTempProgress++] = ValR;
 
	// If we haven't accumulated a full packet yet, do nothing more:
	if(sndTempProgress < SndOutPacketSize) return 1;

	if(dspPluginEnabled)
	{
		for(int i=0;i<SndOutPacketSize;i++) { sndTempBuffer16[i] = SndScaleVol( sndTempBuffer[i] ); }

		// send to winamp DSP
		sndTempProgress = DspProcess(sndTempBuffer16,sndTempProgress>>1)<<1;

		for(int i=0;i<sndTempProgress;i++) { sndTempBuffer[i] = sndTempBuffer16[i]<<SndOutVolumeShift; }
	}

	static int equalized = 0;
	if( !timeStretchDisabled )
	{
		bool progress = false;

		// data prediction helps keep the tempo adjustments more accurate.
		// The timestretcher returns packets in belated "clump" form.
		// Meaning that most of the time we'll get nothing back, and then
		// suddenly we'll get several chunks back at once.  Thus we use
		// data prediction to make the timestretcher more responsive.

		sndBuffer->PredictDataWrite( (int)( sndTempProgress / eTempo ) );
		for(int i=0;i<sndTempProgress;i++) { ((float*)sndTempBuffer)[i] = sndTempBuffer[i]/2147483648.0f; }

		pSoundTouch->putSamples((float*)sndTempBuffer, sndTempProgress>>1);

		while( ( sndTempProgress = pSoundTouch->receiveSamples((float*)sndTempBuffer, sndTempProgress>>1)<<1 ) != 0 )
		{
			// [Air] [TODO] : Implement an SSE downsampler to int.
			for(int i=0;i<sndTempProgress;i++)
			{
				sndTempBuffer[i] = (s32)(((float*)sndTempBuffer)[i]*2147483648.0f);
			}
			sndBuffer->WriteSamples(sndTempBuffer, sndTempProgress);
			progress = true;
		}

		UpdateTempoChange();

		if( MsgOverruns() )
		{
			if( progress )
			{
				if( ++ts_stats_logcounter > 300 )
				{
					ts_stats_logcounter = 0;
					ConLog( " * SPU2 > Timestretch Stats > %d%% of packets stretched.\n",
						( ts_stats_stretchblocks * 100 ) / ( ts_stats_normalblocks + ts_stats_stretchblocks ) );
					ts_stats_normalblocks = 0;
					ts_stats_stretchblocks = 0;
				}
			}
		}
	}
	else
	{
		sndBuffer->WriteSamples(sndTempBuffer, sndTempProgress);
		sndTempProgress=0;
	}

	return 1;
}

s32 SndTest()
{
	if( mods[OutputModule] == NULL )
		return -1;

	return mods[OutputModule]->Test();
}

void SndConfigure(HWND parent, u32 module )
{
	if( mods[module] == NULL )
		return;

	mods[module]->Configure(parent);
}
