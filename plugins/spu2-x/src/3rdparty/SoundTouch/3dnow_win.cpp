////////////////////////////////////////////////////////////////////////////////
///
/// Win32 version of the AMD 3DNow! optimized routines for AMD K6-2/Athlon 
/// processors. All 3DNow! optimized functions have been gathered into this
/// single source code file, regardless to their class or original source code 
/// file, in order to ease porting the library to other compiler and processor 
/// platforms.
///
/// By the way; the performance gain depends heavily on the CPU generation: On 
/// K6-2 these routines provided speed-up of even 2.4 times, while on Athlon the 
/// difference to the original routines stayed at unremarkable 8%! Such a small 
/// improvement on Athlon is due to 3DNow can perform only two operations in 
/// parallel, and obviously also the Athlon FPU is doing a very good job with
/// the standard C floating point routines! Here these routines are anyway, 
/// although it might not be worth the effort to convert these to GCC platform, 
/// for Athlon CPU at least. The situation is different regarding the SSE 
/// optimizations though, thanks to the four parallel operations of SSE that 
/// already make a difference.
/// 
/// This file is to be compiled in Windows platform with Microsoft Visual C++ 
/// Compiler. Please see '3dnow_gcc.cpp' for the gcc compiler version for all
/// GNU platforms (if file supplied).
///
/// NOTICE: If using Visual Studio 6.0, you'll need to install the "Visual C++ 
/// 6.0 processor pack" update to support 3DNow! instruction set. The update is 
/// available for download at Microsoft Developers Network, see here:
/// http://msdn.microsoft.com/vstudio/downloads/tools/ppack/default.aspx
///
/// If the above URL is expired or removed, go to "http://msdn.microsoft.com" and 
/// perform a search with keywords "processor pack".
///
/// Author        : Copyright (c) Olli Parviainen
/// Author e-mail : oparviai 'at' iki.fi
/// SoundTouch WWW: http://www.surina.net/soundtouch
///
////////////////////////////////////////////////////////////////////////////////
//
// Last changed  : $Date: 2006/02/05 16:44:06 $
// File revision : $Revision: 1.10 $
//
// $Id: 3dnow_win.cpp,v 1.10 2006/02/05 16:44:06 Olli Exp $
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

#include "cpu_detect.h"
#include "STTypes.h"

#ifndef _WIN32
#error "wrong platform - this source code file is exclusively for Win32 platform"
#endif

using namespace soundtouch;

#ifdef ALLOW_3DNOW
// 3DNow! routines available only with float sample type    

//////////////////////////////////////////////////////////////////////////////
//
// implementation of 3DNow! optimized functions of class 'TDStretch3DNow'
//
//////////////////////////////////////////////////////////////////////////////

#include "TDStretch.h"
#include <limits.h>

// these are declared in 'TDStretch.cpp'
extern int scanOffsets[4][24];


// Calculates cross correlation of two buffers
double TDStretch3DNow::calcCrossCorrStereo(const float *pV1, const float *pV2) const
{
    uint overlapLengthLocal = overlapLength;
    float corr;

    // Calculates the cross-correlation value between 'pV1' and 'pV2' vectors
    /*
    c-pseudocode:

        corr = 0;
        for (i = 0; i < overlapLength / 4; i ++)
        {
            corr += pV1[0] * pV2[0];
                    pV1[1] * pV2[1];
                    pV1[2] * pV2[2];
                    pV1[3] * pV2[3];
                    pV1[4] * pV2[4];
                    pV1[5] * pV2[5];
                    pV1[6] * pV2[6];
                    pV1[7] * pV2[7];

            pV1 += 8;
            pV2 += 8;
        }
    */

    _asm 
    {
        // give prefetch hints to CPU of what data are to be needed soonish.
        // give more aggressive hints on pV1 as that changes more between different calls 
        // while pV2 stays the same.
        prefetch [pV1]
        prefetch [pV2]
        prefetch [pV1 + 32]

        mov     eax, dword ptr pV2
        mov     ebx, dword ptr pV1

        pxor    mm0, mm0

        mov     ecx, overlapLengthLocal
        shr     ecx, 2  // div by four

    loop1:
        movq    mm1, [eax]
        prefetch [eax + 32]     // give a prefetch hint to CPU what data are to be needed soonish
        pfmul   mm1, [ebx]
        prefetch [ebx + 64]     // give a prefetch hint to CPU what data are to be needed soonish

        movq    mm2, [eax + 8]
        pfadd   mm0, mm1
        pfmul   mm2, [ebx + 8]

        movq    mm3, [eax + 16]
        pfadd   mm0, mm2
        pfmul   mm3, [ebx + 16]

        movq    mm4, [eax + 24]
        pfadd   mm0, mm3
        pfmul   mm4, [ebx + 24]

        add     eax, 32
        pfadd   mm0, mm4
        add     ebx, 32

        dec     ecx
        jnz     loop1

        // add halfs of mm0 together and return the result. 
        // note: mm1 is used as a dummy parameter only, we actually don't care about it's value
        pfacc   mm0, mm1
        movd    corr, mm0
        femms
    }

    return corr;
}




//////////////////////////////////////////////////////////////////////////////
//
// implementation of 3DNow! optimized functions of class 'FIRFilter'
//
//////////////////////////////////////////////////////////////////////////////

#include "FIRFilter.h"

FIRFilter3DNow::FIRFilter3DNow() : FIRFilter()
{
    filterCoeffsUnalign = NULL;
}


FIRFilter3DNow::~FIRFilter3DNow()
{
    delete[] filterCoeffsUnalign;
}


// (overloaded) Calculates filter coefficients for 3DNow! routine
void FIRFilter3DNow::setCoefficients(const float *coeffs, uint newLength, uint uResultDivFactor)
{
    uint i;
    float fDivider;

    FIRFilter::setCoefficients(coeffs, newLength, uResultDivFactor);

    // Scale the filter coefficients so that it won't be necessary to scale the filtering result
    // also rearrange coefficients suitably for 3DNow!
    // Ensure that filter coeffs array is aligned to 16-byte boundary
    delete[] filterCoeffsUnalign;
    filterCoeffsUnalign = new float[2 * newLength + 4];
    filterCoeffsAlign = (float *)(((uint)filterCoeffsUnalign + 15) & -16);

    fDivider = (float)resultDivider;

    // rearrange the filter coefficients for mmx routines 
    for (i = 0; i < newLength; i ++)
    {
        filterCoeffsAlign[2 * i + 0] =
        filterCoeffsAlign[2 * i + 1] = coeffs[i + 0] / fDivider;
    }
}


// 3DNow!-optimized version of the filter routine for stereo sound
uint FIRFilter3DNow::evaluateFilterStereo(float *dest, const float *src, const uint numSamples) const
{
    float *filterCoeffsLocal = filterCoeffsAlign;
    uint count = (numSamples - length) & -2;
    uint lengthLocal = length / 4;

    assert(length != 0);
    assert(count % 2 == 0);

    /* original code:

    double suml1, suml2;
    double sumr1, sumr2;
    uint i, j;

    for (j = 0; j < count; j += 2)
    {
        const float *ptr;

        suml1 = sumr1 = 0.0;
        suml2 = sumr2 = 0.0;
        ptr = src;
        filterCoeffsLocal = filterCoeffs;
        for (i = 0; i < lengthLocal; i ++) 
        {
            // unroll loop for efficiency.

            suml1 += ptr[0] * filterCoeffsLocal[0] + 
                     ptr[2] * filterCoeffsLocal[2] +
                     ptr[4] * filterCoeffsLocal[4] +
                     ptr[6] * filterCoeffsLocal[6];

            sumr1 += ptr[1] * filterCoeffsLocal[1] + 
                     ptr[3] * filterCoeffsLocal[3] +
                     ptr[5] * filterCoeffsLocal[5] +
                     ptr[7] * filterCoeffsLocal[7];

            suml2 += ptr[8] * filterCoeffsLocal[0] + 
                     ptr[10] * filterCoeffsLocal[2] +
                     ptr[12] * filterCoeffsLocal[4] +
                     ptr[14] * filterCoeffsLocal[6];

            sumr2 += ptr[9] * filterCoeffsLocal[1] + 
                     ptr[11] * filterCoeffsLocal[3] +
                     ptr[13] * filterCoeffsLocal[5] +
                     ptr[15] * filterCoeffsLocal[7];

            ptr += 16;
            filterCoeffsLocal += 8;
        }
        dest[0] = (float)suml1;
        dest[1] = (float)sumr1;
        dest[2] = (float)suml2;
        dest[3] = (float)sumr2;

        src += 4;
        dest += 4;
    }

    */
    _asm
    {
        mov     eax, dword ptr dest
        mov     ebx, dword ptr src
        mov     edx, count
        shr     edx, 1

    loop1:
        // "outer loop" : during each round 2*2 output samples are calculated
        prefetch  [ebx]                 // give a prefetch hint to CPU what data are to be needed soonish
        prefetch  [filterCoeffsLocal]   // give a prefetch hint to CPU what data are to be needed soonish

        mov     esi, ebx
        mov     edi, filterCoeffsLocal
        pxor    mm0, mm0
        pxor    mm1, mm1
        mov     ecx, lengthLocal

    loop2:
        // "inner loop" : during each round four FIR filter taps are evaluated for 2*2 output samples
        movq    mm2, [edi]
        movq    mm3, mm2
        prefetch  [edi + 32]     // give a prefetch hint to CPU what data are to be needed soonish
        pfmul   mm2, [esi]
        prefetch  [esi + 32]     // give a prefetch hint to CPU what data are to be needed soonish
        pfmul   mm3, [esi + 8]

        movq    mm4, [edi + 8]
        movq    mm5, mm4
        pfadd   mm0, mm2
        pfmul   mm4, [esi + 8]
        pfadd   mm1, mm3
        pfmul   mm5, [esi + 16]

        movq    mm2, [edi + 16]
        movq    mm6, mm2
        pfadd   mm0, mm4
        pfmul   mm2, [esi + 16]
        pfadd   mm1, mm5
        pfmul   mm6, [esi + 24]

        movq    mm3, [edi + 24]
        movq    mm7, mm3
        pfadd   mm0, mm2
        pfmul   mm3, [esi + 24]
        pfadd   mm1, mm6
        pfmul   mm7, [esi + 32]
        add     esi, 32
        pfadd   mm0, mm3
        add     edi, 32
        pfadd   mm1, mm7

        dec     ecx
        jnz     loop2

        movq    [eax], mm0
        add     ebx, 16
        movq    [eax + 8], mm1
        add     eax, 16

        dec     edx
        jnz     loop1

        femms
    }

    return count;
}


#endif  // ALLOW_3DNOW
