/*
 * $Id: pa_win_util.c 1584 2011-02-02 18:58:17Z rossb $
 * Portable Audio I/O Library
 * Win32 platform-specific support functions
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2008 Ross Bencina
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/** @file
 @ingroup win_src

 @brief Win32 implementation of platform-specific PaUtil support functions.
*/
 
#include <windows.h>
#include <mmsystem.h> /* for timeGetTime() */

#include "pa_util.h"

#if (defined(WIN32) && (defined(_MSC_VER) && (_MSC_VER >= 1200))) && !defined(_WIN32_WCE) /* MSC version 6 and above */
#pragma comment( lib, "winmm.lib" )
#endif


/*
   Track memory allocations to avoid leaks.
 */

#if PA_TRACK_MEMORY
static int numAllocations_ = 0;
#endif


void *PaUtil_AllocateMemory( long size )
{
    void *result = GlobalAlloc( GPTR, size );

#if PA_TRACK_MEMORY
    if( result != NULL ) numAllocations_ += 1;
#endif
    return result;
}


void PaUtil_FreeMemory( void *block )
{
    if( block != NULL )
    {
        GlobalFree( block );
#if PA_TRACK_MEMORY
        numAllocations_ -= 1;
#endif

    }
}


int PaUtil_CountCurrentlyAllocatedBlocks( void )
{
#if PA_TRACK_MEMORY
    return numAllocations_;
#else
    return 0;
#endif
}


void Pa_Sleep( long msec )
{
    Sleep( msec );
}

static int usePerformanceCounter_;
static double secondsPerTick_;

void PaUtil_InitializeClock( void )
{
    LARGE_INTEGER ticksPerSecond;

    if( QueryPerformanceFrequency( &ticksPerSecond ) != 0 )
    {
        usePerformanceCounter_ = 1;
        secondsPerTick_ = 1.0 / (double)ticksPerSecond.QuadPart;
    }
    else
    {
        usePerformanceCounter_ = 0;
    }
}


double PaUtil_GetTime( void )
{
    LARGE_INTEGER time;

    if( usePerformanceCounter_ )
    {
        /*
            Note: QueryPerformanceCounter has a known issue where it can skip forward
            by a few seconds (!) due to a hardware bug on some PCI-ISA bridge hardware.
            This is documented here:
            http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q274323&

            The work-arounds are not very paletable and involve querying GetTickCount 
            at every time step.

            Using rdtsc is not a good option on multi-core systems.

            For now we just use QueryPerformanceCounter(). It's good, most of the time.
        */
        QueryPerformanceCounter( &time );
        return time.QuadPart * secondsPerTick_;
    }
    else
    {
#ifndef UNDER_CE    	
        return timeGetTime() * .001;
#else
        return GetTickCount() * .001;
#endif                
    }
}
