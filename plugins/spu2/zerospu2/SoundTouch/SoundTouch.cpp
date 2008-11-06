//////////////////////////////////////////////////////////////////////////////
///
/// SoundTouch - main class for tempo/pitch/rate adjusting routines. 
///
/// Notes:
/// - Initialize the SoundTouch object instance by setting up the sound stream 
///   parameters with functions 'setSampleRate' and 'setChannels', then set 
///   desired tempo/pitch/rate settings with the corresponding functions.
///
/// - The SoundTouch class behaves like a first-in-first-out pipeline: The 
///   samples that are to be processed are fed into one of the pipe by calling
///   function 'putSamples', while the ready processed samples can be read 
///   from the other end of the pipeline with function 'receiveSamples'.
/// 
/// - The SoundTouch processing classes require certain sized 'batches' of 
///   samples in order to process the sound. For this reason the classes buffer 
///   incoming samples until there are enough of samples available for 
///   processing, then they carry out the processing step and consequently
///   make the processed samples available for outputting.
/// 
/// - For the above reason, the processing routines introduce a certain 
///   'latency' between the input and output, so that the samples input to
///   SoundTouch may not be immediately available in the output, and neither 
///   the amount of outputtable samples may not immediately be in direct 
///   relationship with the amount of previously input samples.
///
/// - The tempo/pitch/rate control parameters can be altered during processing.
///   Please notice though that they aren't currently protected by semaphores,
///   so in multi-thread application external semaphore protection may be
///   required.
///
/// - This class utilizes classes 'TDStretch' for tempo change (without modifying
///   pitch) and 'RateTransposer' for changing the playback rate (that is, both 
///   tempo and pitch in the same ratio) of the sound. The third available control 
///   'pitch' (change pitch but maintain tempo) is produced by a combination of
///   combining the two other controls.
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.13 $
//
// $Id: SoundTouch.cpp,v 1.13 2006/02/05 16:44:06 Olli Exp $
//
////////////////////////////////////////////////////////////////////////////////
//
// License :
//
//  SoundTouch audio processing library
//  Copyright (c) Olli Parviainen
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <stdexcept>
#include <stdio.h>

#include "SoundTouch.h"
#include "TDStretch.h"
#include "RateTransposer.h"
#include "cpu_detect.h"

using namespace soundtouch;

/// Print library version string
extern "C" void soundtouch_ac_test()
{
    printf("SoundTouch Version: %s\n",SOUNDTOUCH_VERSION);
} 


SoundTouch::SoundTouch()
{
    // Initialize rate transposer and tempo changer instances

    pRateTransposer = RateTransposer::newInstance();
    pTDStretch = TDStretch::newInstance();

    setOutPipe(pTDStretch);

    rate = tempo = 0;

    virtualPitch = 
    virtualRate = 
    virtualTempo = 1.0;

    calcEffectiveRateAndTempo();

    channels = 0;
    bSrateSet = FALSE;
}



SoundTouch::~SoundTouch()
{
    delete pRateTransposer;
    delete pTDStretch;
}



/// Get SoundTouch library version string
const char *SoundTouch::getVersionString()
{
    static const char *_version = SOUNDTOUCH_VERSION;

    return _version;
}


/// Get SoundTouch library version Id
uint SoundTouch::getVersionId()
{
    return SOUNDTOUCH_VERSION_ID;
}


// Sets the number of channels, 1 = mono, 2 = stereo
void SoundTouch::setChannels(uint numChannels)
{
    if (numChannels != 1 && numChannels != 2) 
    {
        throw std::runtime_error("Illegal number of channels");
    }
    channels = numChannels;
    pRateTransposer->setChannels(numChannels);
    pTDStretch->setChannels(numChannels);
}



// Sets new rate control value. Normal rate = 1.0, smaller values
// represent slower rate, larger faster rates.
void SoundTouch::setRate(float newRate)
{
    virtualRate = newRate;
    calcEffectiveRateAndTempo();
}



// Sets new rate control value as a difference in percents compared
// to the original rate (-50 .. +100 %)
void SoundTouch::setRateChange(float newRate)
{
    virtualRate = 1.0f + 0.01f * newRate;
    calcEffectiveRateAndTempo();
}



// Sets new tempo control value. Normal tempo = 1.0, smaller values
// represent slower tempo, larger faster tempo.
void SoundTouch::setTempo(float newTempo)
{
    virtualTempo = newTempo;
    calcEffectiveRateAndTempo();
}



// Sets new tempo control value as a difference in percents compared
// to the original tempo (-50 .. +100 %)
void SoundTouch::setTempoChange(float newTempo)
{
    virtualTempo = 1.0f + 0.01f * newTempo;
    calcEffectiveRateAndTempo();
}



// Sets new pitch control value. Original pitch = 1.0, smaller values
// represent lower pitches, larger values higher pitch.
void SoundTouch::setPitch(float newPitch)
{
    virtualPitch = newPitch;
    calcEffectiveRateAndTempo();
}



// Sets pitch change in octaves compared to the original pitch
// (-1.00 .. +1.00)
void SoundTouch::setPitchOctaves(float newPitch)
{
    virtualPitch = (float)exp(0.69314718056f * newPitch);
    calcEffectiveRateAndTempo();
}



// Sets pitch change in semi-tones compared to the original pitch
// (-12 .. +12)
void SoundTouch::setPitchSemiTones(int newPitch)
{
    setPitchOctaves((float)newPitch / 12.0f);
}



void SoundTouch::setPitchSemiTones(float newPitch)
{
    setPitchOctaves(newPitch / 12.0f);
}


// Calculates 'effective' rate and tempo values from the
// nominal control values.
void SoundTouch::calcEffectiveRateAndTempo()
{
    float oldTempo = tempo;
    float oldRate = rate;

    tempo = virtualTempo / virtualPitch;
    rate = virtualPitch * virtualRate;

    if (rate != oldRate) pRateTransposer->setRate(rate);
    if (tempo != oldTempo) pTDStretch->setTempo(tempo);

    if (rate > 1.0f) 
    {
        if (output != pRateTransposer) 
        {
            FIFOSamplePipe *transOut;

            assert(output == pTDStretch);
            // move samples in the current output buffer to the output of pRateTransposer
            transOut = pRateTransposer->getOutput();
            transOut->moveSamples(*output);
            // move samples in tempo changer's input to pitch transposer's input
            pRateTransposer->moveSamples(*pTDStretch->getInput());

            output = pRateTransposer;
        }
    } 
    else 
    {
        if (output != pTDStretch) 
        {
            FIFOSamplePipe *tempoOut;

            assert(output == pRateTransposer);
            // move samples in the current output buffer to the output of pTDStretch
            tempoOut = pTDStretch->getOutput();
            tempoOut->moveSamples(*output);
            // move samples in pitch transposer's store buffer to tempo changer's input
            pTDStretch->moveSamples(*pRateTransposer->getStore());

            output = pTDStretch;

        }
    }
}


// Sets sample rate.
void SoundTouch::setSampleRate(uint srate)
{
    bSrateSet = TRUE;
    // set sample rate, leave other tempo changer parameters as they are.
    pTDStretch->setParameters(srate);
}


// Adds 'numSamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void SoundTouch::putSamples(const SAMPLETYPE *samples, uint numSamples)
{
    if (bSrateSet == FALSE) 
    {
        throw std::runtime_error("SoundTouch : Sample rate not defined");
    } 
    else if (channels == 0) 
    {
        throw std::runtime_error("SoundTouch : Number of channels not defined");
    }

    // Transpose the rate of the new samples if necessary
    /* Bypass the nominal setting - can introduce a click in sound when tempo/pitch control crosses the nominal value...
    if (rate == 1.0f) 
    {
        // The rate value is same as the original, simply evaluate the tempo changer. 
        assert(output == pTDStretch);
        if (pRateTransposer->isEmpty() == 0) 
        {
            // yet flush the last samples in the pitch transposer buffer
            // (may happen if 'rate' changes from a non-zero value to zero)
            pTDStretch->moveSamples(*pRateTransposer);
        }
        pTDStretch->putSamples(samples, numSamples);
    } 
    */
    else if (rate <= 1.0f) 
    {
        // transpose the rate down, output the transposed sound to tempo changer buffer
        assert(output == pTDStretch);
        pRateTransposer->putSamples(samples, numSamples);
        pTDStretch->moveSamples(*pRateTransposer);
    } 
    else 
    {
        assert(rate > 1.0f);
        // evaluate the tempo changer, then transpose the rate up, 
        assert(output == pRateTransposer);
        pTDStretch->putSamples(samples, numSamples);
        pRateTransposer->moveSamples(*pTDStretch);
    }
}


// Flushes the last samples from the processing pipeline to the output.
// Clears also the internal processing buffers.
//
// Note: This function is meant for extracting the last samples of a sound
// stream. This function may introduce additional blank samples in the end
// of the sound stream, and thus it's not recommended to call this function
// in the middle of a sound stream.
void SoundTouch::flush()
{
    int i;
    uint nOut;
    SAMPLETYPE buff[128];

    nOut = numSamples();

    memset(buff, 0, 128 * sizeof(SAMPLETYPE));
    // "Push" the last active samples out from the processing pipeline by
    // feeding blank samples into the processing pipeline until new, 
    // processed samples appear in the output (not however, more than 
    // 8ksamples in any case)
    for (i = 0; i < 128; i ++) 
    {
        putSamples(buff, 64);
        if (numSamples() != nOut) break;  // new samples have appeared in the output!
    }

    // Clear working buffers
    pRateTransposer->clear();
    pTDStretch->clearInput();
    // yet leave the 'tempoChanger' output intouched as that's where the
    // flushed samples are!
}


// Changes a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
BOOL SoundTouch::setSetting(uint settingId, uint value)
{
    uint sampleRate, sequenceMs, seekWindowMs, overlapMs;

    // read current tdstretch routine parameters
    pTDStretch->getParameters(&sampleRate, &sequenceMs, &seekWindowMs, &overlapMs);

    switch (settingId) 
    {
        case SETTING_USE_AA_FILTER :
            // enables / disabless anti-alias filter
            pRateTransposer->enableAAFilter((value != 0) ? TRUE : FALSE);
            return TRUE;

        case SETTING_AA_FILTER_LENGTH :
            // sets anti-alias filter length
            pRateTransposer->getAAFilter()->setLength(value);
            return TRUE;

        case SETTING_USE_QUICKSEEK :
            // enables / disables tempo routine quick seeking algorithm
            pTDStretch->enableQuickSeek((value != 0) ? TRUE : FALSE);
            return TRUE;

        case SETTING_SEQUENCE_MS:
            // change time-stretch sequence duration parameter
            pTDStretch->setParameters(sampleRate, value, seekWindowMs, overlapMs);
            return TRUE;

        case SETTING_SEEKWINDOW_MS:
            // change time-stretch seek window length parameter
            pTDStretch->setParameters(sampleRate, sequenceMs, value, overlapMs);
            return TRUE;

        case SETTING_OVERLAP_MS:
            // change time-stretch overlap length parameter
            pTDStretch->setParameters(sampleRate, sequenceMs, seekWindowMs, value);
            return TRUE;

        default :
            return FALSE;
    }
}


// Reads a setting controlling the processing system behaviour. See the
// 'SETTING_...' defines for available setting ID's.
//
// Returns the setting value.
uint SoundTouch::getSetting(uint settingId) const
{
    uint temp;

    switch (settingId) 
    {
        case SETTING_USE_AA_FILTER :
            return pRateTransposer->isAAFilterEnabled();

        case SETTING_AA_FILTER_LENGTH :
            return pRateTransposer->getAAFilter()->getLength();

        case SETTING_USE_QUICKSEEK :
            return pTDStretch->isQuickSeekEnabled();

        case SETTING_SEQUENCE_MS:
            pTDStretch->getParameters(NULL, &temp, NULL, NULL);
            return temp;

        case SETTING_SEEKWINDOW_MS:
            pTDStretch->getParameters(NULL, NULL, &temp, NULL);
            return temp;

        case SETTING_OVERLAP_MS:
            pTDStretch->getParameters(NULL, NULL, NULL, &temp);
            return temp;

        default :
            return 0;
    }
}


// Clears all the samples in the object's output and internal processing
// buffers.
void SoundTouch::clear()
{
    pRateTransposer->clear();
    pTDStretch->clear();
}



/// Returns number of samples currently unprocessed.
uint SoundTouch::numUnprocessedSamples() const
{
    FIFOSamplePipe * psp;
    if (pTDStretch)
    {
        psp = pTDStretch->getInput();
        if (psp)
        {
            return psp->numSamples();
        }
    }
    return 0;
}
