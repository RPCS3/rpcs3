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
#include "soundtouch/SoundTouch.h"
#include <wx/datetime.h>

//Uncomment the next line to use the old time stretcher
//#define SPU2X_USE_OLD_STRETCHER

static soundtouch::SoundTouch* pSoundTouch = NULL;

// data prediction amount, used to "commit" data that hasn't
// finished timestretch processing.
s32 SndBuffer::m_predictData;

// records last buffer status (fill %, range -100 to 100, with 0 being 50% full)
float SndBuffer::lastPct;
float SndBuffer::lastEmergencyAdj;

float SndBuffer::cTempo = 1;
float SndBuffer::eTempo = 1;

void SndBuffer::PredictDataWrite( int samples )
{
	m_predictData += samples;
}

// Calculate the buffer status percentage.
// Returns range from -1.0 to 1.0
//    1.0 = buffer overflow!
//    0.0 = buffer nominal (50% full)
//   -1.0 = buffer underflow!
float SndBuffer::GetStatusPct()
{
	// Get the buffer status of the output driver too, so that we can
	// obtain a more accurate overall buffer status.

	int drvempty = mods[OutputModule]->GetEmptySampleCount(); // / 2;

	//ConLog( "Data %d >>> driver: %d   predict: %d\n", m_data, drvempty, m_predictData );

	float result = (float)(m_data + m_predictData - drvempty) - (m_size/16);
	result /= (m_size/16);
	return result;
}


//Alternative simple tempo adjustment. Based only on the soundtouch buffer state.
//Base algorithm: aim at specific average number of samples at the buffer (by GUI), and adjust tempo simply by current/target.
//An extra mechanism is added to keep adjustment at perfect 1:1 ratio (when emulation speed is stable around 100%)
//  to prevent constant stretching/shrinking of packets if possible.
//  This mechanism is triggered when the adjustment is close to 1:1 for long enough (defaults to 100 iterations within hys_ok_factor - defaults to 3%).
//  1:1 state is aborted when required adjustment goes beyond hys_bad_factor (defaults to 20%).
//
//To compensate for wide variation of the <num of samples> ratio due to relatively small size of the buffer,
//  The required tempo is a running average of STRETCH_AVERAGE_LEN (defaults to 50) last calculations.
//  This averaging slows down the respons time of the algorithm, but greatly stablize it towards steady stretching.
//
//Keeping the buffer at required latency:
//  This algorithm stabilises when the actual latency is <speed>*<required_latency>. While this is just fine at 100% speed,
//  it's problematic especially for slow speeds, as the number of actual samples at the buffer gets very small on that case,
//  which may lead to underruns (or just too much latency when running very fast fast).
//To compensate for that, the algorithm has a slowly moving compensation factor which will eventually bring the actual latency to the required one.
//compensationDivider defines how slow this compensation changes. By default it's set to 100,
//  which will finalize the compensation after about 200 iterations.
//
// Note, this algorithm is intentionally simplified by not taking extreme actions at extreme scenarios (mostly underruns when speed dtops sharply),
//  and let's the overrun/underrun protections do what they should (doesn't happen much though in practice, even at big FPS variations).
//
//  These params were tested to show good respond and stability, on all audio systems (dsound, wav, port audio, xaudio2),
//    even at extreme small latency of 50ms which can handle 50%-100% variations without audible glitches.

int targetIPS=750;

//Dynamic tuning changes the values of the base algorithm parameters (derived from targetIPS) to adapt, in real time, to
//  diferent number of invocations/sec (mostly affects number of iterations to average).
//  Dynamic tuning can have a slight negative effect on the behavior of the algorithm, so it's preferred to have it off.
//Currently it looks like it's around 750/sec on all systems when playing at 100% speed (50/60fps),
//  and proportional to that speed otherwise.
//If changes are made to SPU2X which affects this number (but it's still the same on all systems), then just change targetIPS.
//If we find out that some systems are very different, we can turn on dynamic tuning by uncommenting the next line.
//#define NEWSTRETCHER_USE_DYNAMIC_TUNING


//running average can be implemented in O(1) time. 
//For the sake of simplicity, this average is calculated in O(<buffer-size>). Possibly improve later.
#define MAX_STRETCH_AVERAGE_LEN 100
int STRETCH_AVERAGE_LEN=50.0 *targetIPS/750;
//adds a value to the running average buffer, and return new running average.
float addToAvg(float val){
	static float avg_fullness[MAX_STRETCH_AVERAGE_LEN]={0};
	static int nextAvgPos=0;

	avg_fullness[nextAvgPos]=val;
	nextAvgPos=(nextAvgPos+1)%STRETCH_AVERAGE_LEN;
	float sum=0;
	for(int c=0, i=(nextAvgPos-1)%STRETCH_AVERAGE_LEN; c<STRETCH_AVERAGE_LEN; c++, i=(i+STRETCH_AVERAGE_LEN-1)%STRETCH_AVERAGE_LEN)
		sum+=avg_fullness[i];

	sum= (float)sum/(float)STRETCH_AVERAGE_LEN;
	return sum?sum:1;
}

template <class T>
T clamp(T val, T min, T max){
	if( val > max ) return max;
	if( val < min ) return min;
	return val;
}

//actual stretch algorithm implementation
void SndBuffer::UpdateTempoChangeSoundTouch2()
{

	long targetSamplesReservoir=48*SndOutLatencyMS;//48000*SndOutLatencyMS/1000
	//base aim at buffer filled %
	float baseTargetFullness=(double)targetSamplesReservoir;///(double)m_size;//0.05;

	//state vars
	static bool inside_hysteresis=false;
	static int hys_ok_count=0;
	static float dynamicTargetFullness=baseTargetFullness;

	float bufferFullness=(float)m_data;///(float)m_size;

#ifdef NEWSTRETCHER_USE_DYNAMIC_TUNING
	{//test current iterations/sec every 0.5s, and change algo params accordingly if different than previous IPS more than 30%
		static long iters=0;
		static wxDateTime last=wxDateTime::UNow();
		wxDateTime unow=wxDateTime::UNow();
		wxTimeSpan delta = unow.Subtract(last);
		if( delta.GetMilliseconds()>500 ){
			int pot_targetIPS=1000.0/delta.GetMilliseconds().ToDouble()*iters;
			if(pot_targetIPS != clamp(pot_targetIPS, int((float)targetIPS/1.3f), int((float)targetIPS*1.3f)) ){
				if(MsgOverruns()) ConLog("Stretcher: setting iters/sec from %d to %d\n", targetIPS, pot_targetIPS);
				targetIPS=pot_targetIPS;
				STRETCH_AVERAGE_LEN=clamp((int)(50.0f *(float)targetIPS/750.0f), 3, MAX_STRETCH_AVERAGE_LEN);
			}
			last=unow;
			iters=0;
		}
		iters++;
	}
#endif

	//Algorithm params: (threshold params (hysteresis), etc) 
	const float hys_ok_factor=1.04;
	int hys_min_ok_count=clamp((int)(50.0 *(float)targetIPS/750.0), 2, 100); //consecutive iterations within hys_ok before going to 1:1 mode
	const float hys_bad_factor=1.2;
	int compensationDivider=clamp((int)(100.0 *(float)targetIPS/750), 15, 150);

	float tempoAdjust=bufferFullness/dynamicTargetFullness;
	float avgerage = addToAvg(tempoAdjust);
	tempoAdjust = avgerage;
	tempoAdjust = clamp( tempoAdjust, 0.05f, 10.0f);


	dynamicTargetFullness += (baseTargetFullness/tempoAdjust - dynamicTargetFullness)/(double)compensationDivider;
	if( 
		tempoAdjust == clamp(tempoAdjust, 0.9f, 1.1f) 
		&& dynamicTargetFullness == clamp( dynamicTargetFullness, baseTargetFullness*0.9f, baseTargetFullness*1.1f)
		)
		dynamicTargetFullness=baseTargetFullness;

	if( !inside_hysteresis )
	{
		if( tempoAdjust == clamp( tempoAdjust, 1.0f/hys_ok_factor, hys_ok_factor ) )
			hys_ok_count++;
		else
			hys_ok_count=0;

		if( hys_ok_count >= hys_min_ok_count ){
			inside_hysteresis=true;
			if(MsgOverruns()) ConLog("======> stretch: None (1:1)\n");
		}

	}
	else if( tempoAdjust != clamp( tempoAdjust, 1.0f/hys_bad_factor, hys_bad_factor ) ){
		if(MsgOverruns()) ConLog("~~~~~~> stretch: Dynamic\n");
		inside_hysteresis=false;
		hys_ok_count=0;
	}

	if(inside_hysteresis)
		tempoAdjust=1.0;

	if(MsgOverruns()){
		static int iters=0;
		static wxDateTime last=wxDateTime::UNow();
		wxDateTime unow=wxDateTime::UNow();
		wxTimeSpan delta = unow.Subtract(last);

		if(delta.GetMilliseconds()>1000){//report buffers state and tempo adjust every second
			ConLog("buffers: %4d ms (%3.0f%%), tempo: %f, comp: %2.3f, iters: %d, (N-IPS:%d -> avg:%d, minokc:%d, div:%d)\n", 
				(int)(m_data/48), (double)(100.0*bufferFullness/baseTargetFullness), (double)tempoAdjust, (double)(dynamicTargetFullness/baseTargetFullness), iters, (int)targetIPS
				, STRETCH_AVERAGE_LEN, hys_min_ok_count, compensationDivider
				);
			last=unow;
			iters=0;
		}
		iters++;
	}

	pSoundTouch->setTempo(tempoAdjust);

	return;
}


void SndBuffer::UpdateTempoChangeSoundTouch()
{
	float statusPct = GetStatusPct();
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
	// cope with low fps (underruns) than it is high fps (overruns).  So to help out a
	// little, the low-end portions of this check are less forgiving than the high-sides.

	if( cTempo < 0.965f || cTempo > 1.060f ||
		pctChange < -0.38f || pctChange > 0.54f ||
		statusPct < -0.42f || statusPct > 0.70f ||
		eTempo < 0.89f || eTempo > 1.19f )
	{
		//printf("Emergency stretch: cTempo = %f eTempo = %f pctChange = %f statusPct = %f\n",cTempo,eTempo,pctChange,statusPct);
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

		/*ConLog("* SPU2-X: [Nominal %d%%] [Emergency: %d%%] (baseTempo: %d%% ) (newTempo: %d%%) (buffer: %d%%)\n",
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
		}
	}
}

extern uint TickInterval;
void SndBuffer::UpdateTempoChangeAsyncMixing()
{
	float statusPct = GetStatusPct();

	lastPct = statusPct;
	if( statusPct < -0.1f )
	{
		TickInterval -= 4;
		if( statusPct < -0.3f ) TickInterval = 64;
		if( TickInterval < 64 ) TickInterval = 64;
		//printf("-- %d, %f\n",TickInterval,statusPct);
	}
	else if( statusPct > 0.2f )
	{
		TickInterval += 1;
		if( TickInterval >= 7000 ) TickInterval = 7000;
		//printf("++ %d, %f\n",TickInterval,statusPct);
	}
	else TickInterval = 768;
}

void SndBuffer::timeStretchUnderrun()
{
	// timeStretcher failed it's job.  We need to slow down the audio some.

	cTempo -= (cTempo * 0.12f);
	eTempo -= (eTempo * 0.30f);
	if( eTempo < 0.1f ) eTempo = 0.1f;
	pSoundTouch->setTempo( eTempo );
}

s32 SndBuffer::timeStretchOverrun()
{
	// If we overran it means the timestretcher failed.  We need to speed
	// up audio playback.
	cTempo += cTempo * 0.12f;
	eTempo += eTempo * 0.40f;
	if( eTempo > 7.5f ) eTempo = 7.5f;
	pSoundTouch->setTempo( eTempo );

	// Throw out just a little bit (two packets worth) to help
	// give the TS some room to work:

	return SndOutPacketSize*2;
}

static void CvtPacketToFloat( StereoOut32* srcdest )
{
	StereoOutFloat* dest = (StereoOutFloat*)srcdest;
	const StereoOut32* src = (StereoOut32*)srcdest;
	for( uint i=0; i<SndOutPacketSize; ++i, ++dest, ++src )
		*dest = (StereoOutFloat)*src;
}

// Parameter note: Size should always be a multiple of 128, thanks!
static void CvtPacketToInt( StereoOut32* srcdest, uint size )
{
	//jASSUME( (size & 127) == 0 );

	const StereoOutFloat* src = (StereoOutFloat*)srcdest;
	StereoOut32* dest = srcdest;

	for( uint i=0; i<size; ++i, ++dest, ++src )
		*dest = (StereoOut32)*src;
}

void SndBuffer::timeStretchWrite()
{
	bool progress = false;

	// data prediction helps keep the tempo adjustments more accurate.
	// The timestretcher returns packets in belated "clump" form.
	// Meaning that most of the time we'll get nothing back, and then
	// suddenly we'll get several chunks back at once.  Thus we use
	// data prediction to make the timestretcher more responsive.

	PredictDataWrite( (int)( SndOutPacketSize / eTempo ) );
	CvtPacketToFloat( sndTempBuffer );

	pSoundTouch->putSamples( (float*)sndTempBuffer, SndOutPacketSize );

	int tempProgress;
	while( tempProgress = pSoundTouch->receiveSamples( (float*)sndTempBuffer, SndOutPacketSize),
		tempProgress != 0 )
	{
		// Hint: It's assumed that pSoundTouch will return chunks of 128 bytes (it always does as
		// long as the SSE optimizations are enabled), which means we can do our own SSE opts here.

		CvtPacketToInt( sndTempBuffer, tempProgress );
		_WriteSamples( sndTempBuffer, tempProgress );
		progress = true;
	}

#ifdef SPU2X_USE_OLD_STRETCHER
	UpdateTempoChangeSoundTouch();
#else
	UpdateTempoChangeSoundTouch2();
#endif

}

void SndBuffer::soundtouchInit()
{
	pSoundTouch = new soundtouch::SoundTouch();
	pSoundTouch->setSampleRate(SampleRate);
	pSoundTouch->setChannels(2);

	pSoundTouch->setSetting( SETTING_USE_QUICKSEEK, 0 );
	pSoundTouch->setSetting( SETTING_USE_AA_FILTER, 0 );

	SoundtouchCfg::ApplySettings( *pSoundTouch );

	pSoundTouch->setTempo(1);

	// some timestretch management vars:

	cTempo = 1.0;
	eTempo = 1.0;
	lastPct = 0;
	lastEmergencyAdj = 0;

	m_predictData = 0;
}

// reset timestretch management vars, and delay updates a bit:
void SndBuffer::soundtouchClearContents()
{
	if( pSoundTouch == NULL ) return;

	pSoundTouch->clear();
	pSoundTouch->setTempo(1);

	cTempo = 1.0;
	eTempo = 1.0;
	lastPct = 0;
	lastEmergencyAdj = 0;

	m_predictData = 0;
}

void SndBuffer::soundtouchCleanup()
{
	safe_delete( pSoundTouch );
}
