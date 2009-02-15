////////////////////////////////////////////////////////////////////////////////
/// 
/// Sampled sound tempo changer/time stretch algorithm. Changes the sound tempo 
/// while maintaining the original pitch by using a time domain WSOLA-like 
/// method with several performance-increasing tweaks.
///
/// Note : MMX optimized functions reside in a separate, platform-specific 
/// file, e.g. 'mmx_win.cpp' or 'mmx_gcc.cpp'
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.24 $
//
// $Id: TDStretch.cpp,v 1.24 2006/02/05 16:44:06 Olli Exp $
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

#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "STTypes.h"
#include "cpu_detect.h"
#include "TDStretch.h"

using namespace soundtouch;

#ifndef min
#define min(a,b) ((a > b) ? b : a)
#define max(a,b) ((a < b) ? b : a)
#endif



/*****************************************************************************
 *
 * Constant definitions
 *
 *****************************************************************************/


// Table for the hierarchical mixing position seeking algorithm
int scanOffsets[4][24]={
    { 124,  186,  248,  310,  372,  434,  496,  558,  620,  682,  744, 806, 
      868,  930,  992, 1054, 1116, 1178, 1240, 1302, 1364, 1426, 1488,   0}, 
    {-100,  -75,  -50,  -25,   25,   50,   75,  100,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    { -20,  -15,  -10,   -5,    5,   10,   15,   20,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0},
    {  -4,   -3,   -2,   -1,    1,    2,    3,    4,    0,    0,    0,   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0}};

/*****************************************************************************
 *
 * Implementation of the class 'TDStretch'
 *
 *****************************************************************************/


TDStretch::TDStretch() : FIFOProcessor(&outputBuffer)
{
    bQuickseek = FALSE;
    channels = 2;
    bMidBufferDirty = FALSE;

    pMidBuffer = NULL;
    pRefMidBufferUnaligned = NULL;
    overlapLength = 0;

    setParameters(48000, DEFAULT_SEQUENCE_MS, DEFAULT_SEEKWINDOW_MS, DEFAULT_OVERLAP_MS);

    setTempo(1.0f);
}




TDStretch::~TDStretch()
{
    delete[] pMidBuffer;
    delete[] pRefMidBufferUnaligned;
}


    
// Calculates the x having the closest 2^x value for the given value
static int _getClosest2Power(double value)
{
    return (int)(log(value) / log(2.0) + 0.5);
}



// Sets routine control parameters. These control are certain time constants
// defining how the sound is stretched to the desired duration.
//
// 'sampleRate' = sample rate of the sound
// 'sequenceMS' = one processing sequence length in milliseconds (default = 82 ms)
// 'seekwindowMS' = seeking window length for scanning the best overlapping 
//      position (default = 28 ms)
// 'overlapMS' = overlapping length (default = 12 ms)

void TDStretch::setParameters(uint aSampleRate, uint aSequenceMS, 
                              uint aSeekWindowMS, uint aOverlapMS)
{
    this->sampleRate = aSampleRate;
    this->sequenceMs = aSequenceMS;
    this->seekWindowMs = aSeekWindowMS;
    this->overlapMs = aOverlapMS;

    seekLength = (sampleRate * seekWindowMs) / 1000;
    seekWindowLength = (sampleRate * sequenceMs) / 1000;

    maxOffset = seekLength;

    calculateOverlapLength(overlapMs);

    // set tempo to recalculate 'sampleReq'
    setTempo(tempo);

}



/// Get routine control parameters, see setParameters() function.
/// Any of the parameters to this function can be NULL, in such case corresponding parameter
/// value isn't returned.
void TDStretch::getParameters(uint *pSampleRate, uint *pSequenceMs, uint *pSeekWindowMs, uint *pOverlapMs)
{
    if (pSampleRate)
    {
        *pSampleRate = sampleRate;
    }

    if (pSequenceMs)
    {
        *pSequenceMs = sequenceMs;
    }

    if (pSeekWindowMs)
    {
        *pSeekWindowMs = seekWindowMs;
    }

    if (pOverlapMs)
    {
        *pOverlapMs = overlapMs;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'input'
void TDStretch::overlapMono(SAMPLETYPE *output, const SAMPLETYPE *input) const
{
    int i, itemp;

    for (i = 0; i < (int)overlapLength ; i ++) 
    {
        itemp = overlapLength - i;
        output[i] = (input[i] * i + pMidBuffer[i] * itemp ) / overlapLength;    // >> overlapDividerBits;
    }
}



void TDStretch::clearMidBuffer()
{
    if (bMidBufferDirty) 
    {
        memset(pMidBuffer, 0, 2 * sizeof(SAMPLETYPE) * overlapLength);
        bMidBufferDirty = FALSE;
    }
}


void TDStretch::clearInput()
{
    inputBuffer.clear();
    clearMidBuffer();
}


// Clears the sample buffers
void TDStretch::clear()
{
    outputBuffer.clear();
    inputBuffer.clear();
    clearMidBuffer();
}



// Enables/disables the quick position seeking algorithm. Zero to disable, nonzero
// to enable
void TDStretch::enableQuickSeek(BOOL enable)
{
    bQuickseek = enable;
}


// Returns nonzero if the quick seeking algorithm is enabled.
BOOL TDStretch::isQuickSeekEnabled() const
{
    return bQuickseek;
}


// Seeks for the optimal overlap-mixing position.
uint TDStretch::seekBestOverlapPosition(const SAMPLETYPE *refPos)
{
    if (channels == 2) 
    {
        // stereo sound
        if (bQuickseek) 
        {
            return seekBestOverlapPositionStereoQuick(refPos);
        } 
        else 
        {
            return seekBestOverlapPositionStereo(refPos);
        }
    } 
    else 
    {
        // mono sound
        if (bQuickseek) 
        {
            return seekBestOverlapPositionMonoQuick(refPos);
        } 
        else 
        {
            return seekBestOverlapPositionMono(refPos);
        }
    }
}




// Overlaps samples in 'midBuffer' with the samples in 'inputBuffer' at position
// of 'ovlPos'.
inline void TDStretch::overlap(SAMPLETYPE *output, const SAMPLETYPE *input, uint ovlPos) const
{
    if (channels == 2) 
    {
        // stereo sound
        overlapStereo(output, input + 2 * ovlPos);
    } else {
        // mono sound.
        overlapMono(output, input + ovlPos);
    }
}




// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
uint TDStretch::seekBestOverlapPositionStereo(const SAMPLETYPE *refPos) 
{
    uint bestOffs;
    LONG_SAMPLETYPE bestCorr, corr;
    uint i;

    // Slopes the amplitudes of the 'midBuffer' samples
    precalcCorrReferenceStereo();

    bestCorr = INT_MIN;
    bestOffs = 0;

    // Scans for the best correlation value by testing each possible position
    // over the permitted range.
    for (i = 0; i < seekLength; i ++) 
    {
        // Calculates correlation value for the mixing position corresponding
        // to 'i'
        corr = calcCrossCorrStereo(refPos + 2 * i, pRefMidBuffer);

        // Checks for the highest correlation value
        if (corr > bestCorr) 
        {
            bestCorr = corr;
            bestOffs = i;
        }
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

    return bestOffs;
}


// Seeks for the optimal overlap-mixing position. The 'stereo' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
uint TDStretch::seekBestOverlapPositionStereoQuick(const SAMPLETYPE *refPos) 
{
    uint j;
    uint bestOffs;
    LONG_SAMPLETYPE bestCorr, corr;
    uint scanCount, corrOffset, tempOffset;

    // Slopes the amplitude of the 'midBuffer' samples
    precalcCorrReferenceStereo();

    bestCorr = INT_MIN;
    bestOffs = 0;
    corrOffset = 0;
    tempOffset = 0;

    // Scans for the best correlation value using four-pass hierarchical search.
    //
    // The look-up table 'scans' has hierarchical position adjusting steps.
    // In first pass the routine searhes for the highest correlation with 
    // relatively coarse steps, then rescans the neighbourhood of the highest
    // correlation with better resolution and so on.
    for (scanCount = 0;scanCount < 4; scanCount ++) 
    {
        j = 0;
        while (scanOffsets[scanCount][j]) 
        {
            tempOffset = corrOffset + scanOffsets[scanCount][j];
            if (tempOffset >= seekLength) break;

            // Calculates correlation value for the mixing position corresponding
            // to 'tempOffset'
            corr = calcCrossCorrStereo(refPos + 2 * tempOffset, pRefMidBuffer);

            // Checks for the highest correlation value
            if (corr > bestCorr) 
            {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
            j ++;
        }
        corrOffset = bestOffs;
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

    return bestOffs;
}



// Seeks for the optimal overlap-mixing position. The 'mono' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
uint TDStretch::seekBestOverlapPositionMono(const SAMPLETYPE *refPos) 
{
    uint bestOffs;
    LONG_SAMPLETYPE bestCorr, corr;
    uint tempOffset;
    const SAMPLETYPE *compare;

    // Slopes the amplitude of the 'midBuffer' samples
    precalcCorrReferenceMono();

    bestCorr = INT_MIN;
    bestOffs = 0;

    // Scans for the best correlation value by testing each possible position
    // over the permitted range.
    for (tempOffset = 0; tempOffset < seekLength; tempOffset ++) 
    {
        compare = refPos + tempOffset;

        // Calculates correlation value for the mixing position corresponding
        // to 'tempOffset'
        corr = calcCrossCorrMono(pRefMidBuffer, compare);

        // Checks for the highest correlation value
        if (corr > bestCorr) 
        {
            bestCorr = corr;
            bestOffs = tempOffset;
        }
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

    return bestOffs;
}


// Seeks for the optimal overlap-mixing position. The 'mono' version of the
// routine
//
// The best position is determined as the position where the two overlapped
// sample sequences are 'most alike', in terms of the highest cross-correlation
// value over the overlapping period
uint TDStretch::seekBestOverlapPositionMonoQuick(const SAMPLETYPE *refPos) 
{
    uint j;
    uint bestOffs;
    LONG_SAMPLETYPE bestCorr, corr;
    uint scanCount, corrOffset, tempOffset;

    // Slopes the amplitude of the 'midBuffer' samples
    precalcCorrReferenceMono();

    bestCorr = INT_MIN;
    bestOffs = 0;
    corrOffset = 0;
    tempOffset = 0;

    // Scans for the best correlation value using four-pass hierarchical search.
    //
    // The look-up table 'scans' has hierarchical position adjusting steps.
    // In first pass the routine searhes for the highest correlation with 
    // relatively coarse steps, then rescans the neighbourhood of the highest
    // correlation with better resolution and so on.
    for (scanCount = 0;scanCount < 4; scanCount ++) 
    {
        j = 0;
        while (scanOffsets[scanCount][j]) 
        {
            tempOffset = corrOffset + scanOffsets[scanCount][j];
            if (tempOffset >= seekLength) break;

            // Calculates correlation value for the mixing position corresponding
            // to 'tempOffset'
            corr = calcCrossCorrMono(refPos + tempOffset, pRefMidBuffer);

            // Checks for the highest correlation value
            if (corr > bestCorr) 
            {
                bestCorr = corr;
                bestOffs = tempOffset;
            }
            j ++;
        }
        corrOffset = bestOffs;
    }
    // clear cross correlation routine state if necessary (is so e.g. in MMX routines).
    clearCrossCorrState();

    return bestOffs;
}


/// clear cross correlation routine state if necessary 
void TDStretch::clearCrossCorrState()
{
    // default implementation is empty.
}


// Sets new target tempo. Normal tempo = 'SCALE', smaller values represent slower 
// tempo, larger faster tempo.
void TDStretch::setTempo(float newTempo)
{
    uint intskip;

    tempo = newTempo;

    // Calculate ideal skip length (according to tempo value) 
    nominalSkip = tempo * (seekWindowLength - overlapLength);
    skipFract = 0;
    intskip = (int)(nominalSkip + 0.5f);

    // Calculate how many samples are needed in the 'inputBuffer' to 
    // process another batch of samples
    sampleReq = max(intskip + overlapLength, seekWindowLength) + maxOffset;
}



// Sets the number of channels, 1 = mono, 2 = stereo
void TDStretch::setChannels(uint numChannels)
{
    if (channels == numChannels) return;
    assert(numChannels == 1 || numChannels == 2);

    channels = numChannels;
    inputBuffer.setChannels(channels);
    outputBuffer.setChannels(channels);
}


// nominal tempo, no need for processing, just pass the samples through
// to outputBuffer
void TDStretch::processNominalTempo()
{
    assert(tempo == 1.0f);

    if (bMidBufferDirty) 
    {
        // If there are samples in pMidBuffer waiting for overlapping,
        // do a single sliding overlapping with them in order to prevent a 
        // clicking distortion in the output sound
        if (inputBuffer.numSamples() < overlapLength) 
        {
            // wait until we've got overlapLength input samples
            return;
        }
        // Mix the samples in the beginning of 'inputBuffer' with the 
        // samples in 'midBuffer' using sliding overlapping 
        overlap(outputBuffer.ptrEnd(overlapLength), inputBuffer.ptrBegin(), 0);
        outputBuffer.putSamples(overlapLength);
        inputBuffer.receiveSamples(overlapLength);
        clearMidBuffer();
        // now we've caught the nominal sample flow and may switch to
        // bypass mode
    }

    // Simply bypass samples from input to output
    outputBuffer.moveSamples(inputBuffer);
}


// Processes as many processing frames of the samples 'inputBuffer', store
// the result into 'outputBuffer'
void TDStretch::processSamples()
{
    uint ovlSkip, offset;
    int temp;

    /* Removed this small optimization - can introduce a click to sound when tempo setting
       crosses the nominal value
    if (tempo == 1.0f) 
    {
        // tempo not changed from the original, so bypass the processing
        processNominalTempo();
        return;
    }
    */

    if (bMidBufferDirty == FALSE) 
    {
        // if midBuffer is empty, move the first samples of the input stream 
        // into it
        if (inputBuffer.numSamples() < overlapLength) 
        {
            // wait until we've got overlapLength samples
            return;
        }
        memcpy(pMidBuffer, inputBuffer.ptrBegin(), channels * overlapLength * sizeof(SAMPLETYPE));
        inputBuffer.receiveSamples(overlapLength);
        bMidBufferDirty = TRUE;
    }

    // Process samples as long as there are enough samples in 'inputBuffer'
    // to form a processing frame.
    while (inputBuffer.numSamples() >= sampleReq) 
    {
        // If tempo differs from the normal ('SCALE'), scan for the best overlapping
        // position
        offset = seekBestOverlapPosition(inputBuffer.ptrBegin());

        // Mix the samples in the 'inputBuffer' at position of 'offset' with the 
        // samples in 'midBuffer' using sliding overlapping
        // ... first partially overlap with the end of the previous sequence
        // (that's in 'midBuffer')
        overlap(outputBuffer.ptrEnd(overlapLength), inputBuffer.ptrBegin(), offset);
        outputBuffer.putSamples(overlapLength);

        // ... then copy sequence samples from 'inputBuffer' to output
        temp = (seekWindowLength - 2 * overlapLength);// & 0xfffffffe;
        if (temp > 0)
        {
            outputBuffer.putSamples(inputBuffer.ptrBegin() + channels * (offset + overlapLength), temp);
        }

        // Copies the end of the current sequence from 'inputBuffer' to 
        // 'midBuffer' for being mixed with the beginning of the next 
        // processing sequence and so on
        assert(offset + seekWindowLength <= inputBuffer.numSamples());
        memcpy(pMidBuffer, inputBuffer.ptrBegin() + channels * (offset + seekWindowLength - overlapLength), 
            channels * sizeof(SAMPLETYPE) * overlapLength);
        bMidBufferDirty = TRUE;

        // Remove the processed samples from the input buffer. Update
        // the difference between integer & nominal skip step to 'skipFract'
        // in order to prevent the error from accumulating over time.
        skipFract += nominalSkip;   // real skip size
        ovlSkip = (int)skipFract;   // rounded to integer skip
        skipFract -= ovlSkip;       // maintain the fraction part, i.e. real vs. integer skip
        inputBuffer.receiveSamples(ovlSkip);
    }
}


// Adds 'numsamples' pcs of samples from the 'samples' memory position into
// the input of the object.
void TDStretch::putSamples(const SAMPLETYPE *samples, uint numSamples)
{
    // Add the samples into the input buffer
    inputBuffer.putSamples(samples, numSamples);
    // Process the samples in input buffer
    processSamples();
}



/// Set new overlap length parameter & reallocate RefMidBuffer if necessary.
void TDStretch::acceptNewOverlapLength(uint newOverlapLength)
{
    uint prevOvl;

    prevOvl = overlapLength;
    overlapLength = newOverlapLength;

    if (overlapLength > prevOvl)
    {
        delete[] pMidBuffer;
        delete[] pRefMidBufferUnaligned;

        pMidBuffer = new SAMPLETYPE[overlapLength * 2];
        bMidBufferDirty = TRUE;
        clearMidBuffer();

        pRefMidBufferUnaligned = new SAMPLETYPE[2 * overlapLength + 16 / sizeof(SAMPLETYPE)];
        // ensure that 'pRefMidBuffer' is aligned to 16 byte boundary for efficiency
        pRefMidBuffer = (SAMPLETYPE *)((((ulongptr)pRefMidBufferUnaligned) + 15) & -16);
    }
}


// Operator 'new' is overloaded so that it automatically creates a suitable instance 
// depending on if we've a MMX/SSE/etc-capable CPU available or not.
void * TDStretch::operator new(size_t s)
{
    // Notice! don't use "new TDStretch" directly, use "newInstance" to create a new instance instead!
    assert(FALSE);  
    return NULL;
}


TDStretch * TDStretch::newInstance()
{
    uint uExtensions = 0;

#if !defined(_MSC_VER) || !defined(__x86_64__)
    uExtensions = detectCPUextensions();
#endif

    // Check if MMX/SSE/3DNow! instruction set extensions supported by CPU

#ifdef ALLOW_MMX
    // MMX routines available only with integer sample types
    if (uExtensions & SUPPORT_MMX)
    {
        return ::new TDStretchMMX;
    }
    else
#endif // ALLOW_MMX


#ifdef ALLOW_SSE
    if (uExtensions & SUPPORT_SSE)
    {
        // SSE support
        return ::new TDStretchSSE;
    }
    else
#endif // ALLOW_SSE


#ifdef ALLOW_3DNOW
    if (uExtensions & SUPPORT_3DNOW)
    {
        // 3DNow! support
        return ::new TDStretch3DNow;
    }
    else
#endif // ALLOW_3DNOW

    {
        // ISA optimizations not supported, use plain C version
        return ::new TDStretch;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Integer arithmetics specific algorithm implementations.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef INTEGER_SAMPLES

// Slopes the amplitude of the 'midBuffer' samples so that cross correlation
// is faster to calculate
void TDStretch::precalcCorrReferenceStereo()
{
    int i, cnt2;
    int temp, temp2;

    for (i=0 ; i < (int)overlapLength ;i ++) 
    {
        temp = i * (overlapLength - i);
        cnt2 = i * 2;

        temp2 = (pMidBuffer[cnt2] * temp) / slopingDivider;
        pRefMidBuffer[cnt2] = (short)(temp2);
        temp2 = (pMidBuffer[cnt2 + 1] * temp) / slopingDivider;
        pRefMidBuffer[cnt2 + 1] = (short)(temp2);
    }
}


// Slopes the amplitude of the 'midBuffer' samples so that cross correlation
// is faster to calculate
void TDStretch::precalcCorrReferenceMono()
{
    int i;
    long temp;
    long temp2;

    for (i=0 ; i < (int)overlapLength ;i ++) 
    {
        temp = i * (overlapLength - i);
        temp2 = (pMidBuffer[i] * temp) / slopingDivider;
        pRefMidBuffer[i] = (short)temp2;
    }
}


// Overlaps samples in 'midBuffer' with the samples in 'input'. The 'Stereo' 
// version of the routine.
void TDStretch::overlapStereo(short *output, const short *input) const
{
    int i;
    short temp;
    uint cnt2;

    for (i = 0; i < (int)overlapLength ; i ++) 
    {
        temp = (short)(overlapLength - i);
        cnt2 = 2 * i;
        output[cnt2] = (input[cnt2] * i + pMidBuffer[cnt2] * temp )  / overlapLength;
        output[cnt2 + 1] = (input[cnt2 + 1] * i + pMidBuffer[cnt2 + 1] * temp ) / overlapLength;
    }
}


/// Calculates overlap period length in samples.
/// Integer version rounds overlap length to closest power of 2
/// for a divide scaling operation.
void TDStretch::calculateOverlapLength(uint overlapMs)
{
    uint newOvl;

    overlapDividerBits = _getClosest2Power((sampleRate * overlapMs) / 1000.0);
    if (overlapDividerBits > 9) overlapDividerBits = 9;
    if (overlapDividerBits < 4) overlapDividerBits = 4;
    newOvl = 1<<overlapDividerBits;

    acceptNewOverlapLength(newOvl);

    // calculate sloping divider so that crosscorrelation operation won't 
    // overflow 32-bit register. Max. sum of the crosscorrelation sum without 
    // divider would be 2^30*(N^3-N)/3, where N = overlap length
    slopingDivider = (newOvl * newOvl - 1) / 3;
}


long TDStretch::calcCrossCorrMono(const short *mixingPos, const short *compare) const
{
    long corr;
    uint i;

    corr = 0;
    for (i = 1; i < overlapLength; i ++) 
    {
        corr += (mixingPos[i] * compare[i]) >> overlapDividerBits;
    }

    return corr;
}


long TDStretch::calcCrossCorrStereo(const short *mixingPos, const short *compare) const
{
    long corr;
    uint i;

    corr = 0;
    for (i = 2; i < 2 * overlapLength; i += 2) 
    {
        corr += (mixingPos[i] * compare[i] +
                 mixingPos[i + 1] * compare[i + 1]) >> overlapDividerBits;
    }

    return corr;
}

#endif // INTEGER_SAMPLES

//////////////////////////////////////////////////////////////////////////////
//
// Floating point arithmetics specific algorithm implementations.
//

#ifdef FLOAT_SAMPLES


// Slopes the amplitude of the 'midBuffer' samples so that cross correlation
// is faster to calculate
void TDStretch::precalcCorrReferenceStereo()
{
    int i, cnt2;
    float temp;

    for (i=0 ; i < (int)overlapLength ;i ++) 
    {
        temp = (float)i * (float)(overlapLength - i);
        cnt2 = i * 2;
        pRefMidBuffer[cnt2] = (float)(pMidBuffer[cnt2] * temp);
        pRefMidBuffer[cnt2 + 1] = (float)(pMidBuffer[cnt2 + 1] * temp);
    }
}


// Slopes the amplitude of the 'midBuffer' samples so that cross correlation
// is faster to calculate
void TDStretch::precalcCorrReferenceMono()
{
    int i;
    float temp;

    for (i=0 ; i < (int)overlapLength ;i ++) 
    {
        temp = (float)i * (float)(overlapLength - i);
        pRefMidBuffer[i] = (float)(pMidBuffer[i] * temp);
    }
}


// SSE-optimized version of the function overlapStereo
void TDStretch::overlapStereo(float *output, const float *input) const
{
    int i;
    uint cnt2;
    float fTemp;
    float fScale;
    float fi;

    fScale = 1.0f / (float)overlapLength;

    for (i = 0; i < (int)overlapLength ; i ++) 
    {
        fTemp = (float)(overlapLength - i) * fScale;
        fi = (float)i * fScale;
        cnt2 = 2 * i;
        output[cnt2 + 0] = input[cnt2 + 0] * fi + pMidBuffer[cnt2 + 0] * fTemp;
        output[cnt2 + 1] = input[cnt2 + 1] * fi + pMidBuffer[cnt2 + 1] * fTemp;
    }
}


/// Calculates overlap period length in samples.
void TDStretch::calculateOverlapLength(uint overlapMs)
{
    uint newOvl;

    newOvl = (sampleRate * overlapMs) / 1000;
    if (newOvl < 16) newOvl = 16;

    // must be divisible by 8
    newOvl -= newOvl % 8;

    acceptNewOverlapLength(newOvl);
}



double TDStretch::calcCrossCorrMono(const float *mixingPos, const float *compare) const
{
    double corr;
    uint i;

    corr = 0;
    for (i = 1; i < overlapLength; i ++) 
    {
        corr += mixingPos[i] * compare[i];
    }

    return corr;
}


double TDStretch::calcCrossCorrStereo(const float *mixingPos, const float *compare) const
{
    double corr;
    uint i;

    corr = 0;
    for (i = 2; i < 2 * overlapLength; i += 2) 
    {
        corr += mixingPos[i] * compare[i] +
                mixingPos[i + 1] * compare[i + 1];
    }

    return corr;
}

#endif // FLOAT_SAMPLES
