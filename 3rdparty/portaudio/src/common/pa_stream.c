/*
 * $Id: pa_stream.c 1339 2008-02-15 07:50:33Z rossb $
 * Portable Audio I/O Library
 * stream interface
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 2008 Ross Bencina
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
 @ingroup common_src

 @brief Stream interfaces, representation structures and helper functions
 used to interface between pa_front.c host API implementations.
*/


#include "pa_stream.h"


void PaUtil_InitializeStreamInterface( PaUtilStreamInterface *streamInterface,
                                       PaError (*Close)( PaStream* ),
                                       PaError (*Start)( PaStream* ),
                                       PaError (*Stop)( PaStream* ),
                                       PaError (*Abort)( PaStream* ),
                                       PaError (*IsStopped)( PaStream* ),
                                       PaError (*IsActive)( PaStream* ),
                                       PaTime (*GetTime)( PaStream* ),
                                       double (*GetCpuLoad)( PaStream* ),
                                       PaError (*Read)( PaStream*, void *, unsigned long ),
                                       PaError (*Write)( PaStream*, const void *, unsigned long ),
                                       signed long (*GetReadAvailable)( PaStream* ),
                                       signed long (*GetWriteAvailable)( PaStream* )  )
{
    streamInterface->Close = Close;
    streamInterface->Start = Start;
    streamInterface->Stop = Stop;
    streamInterface->Abort = Abort;
    streamInterface->IsStopped = IsStopped;
    streamInterface->IsActive = IsActive;
    streamInterface->GetTime = GetTime;
    streamInterface->GetCpuLoad = GetCpuLoad;
    streamInterface->Read = Read;
    streamInterface->Write = Write;
    streamInterface->GetReadAvailable = GetReadAvailable;
    streamInterface->GetWriteAvailable = GetWriteAvailable;
}


void PaUtil_InitializeStreamRepresentation( PaUtilStreamRepresentation *streamRepresentation,
        PaUtilStreamInterface *streamInterface,
        PaStreamCallback *streamCallback,
        void *userData )
{
    streamRepresentation->magic = PA_STREAM_MAGIC;
    streamRepresentation->nextOpenStream = 0;
    streamRepresentation->streamInterface = streamInterface;
    streamRepresentation->streamCallback = streamCallback;
    streamRepresentation->streamFinishedCallback = 0;

    streamRepresentation->userData = userData;

    streamRepresentation->streamInfo.inputLatency = 0.;
    streamRepresentation->streamInfo.outputLatency = 0.;
    streamRepresentation->streamInfo.sampleRate = 0.;
}


void PaUtil_TerminateStreamRepresentation( PaUtilStreamRepresentation *streamRepresentation )
{
    streamRepresentation->magic = 0;
}


PaError PaUtil_DummyRead( PaStream* stream,
                               void *buffer,
                               unsigned long frames )
{
    (void)stream; /* unused parameter */
    (void)buffer; /* unused parameter */
    (void)frames; /* unused parameter */

    return paCanNotReadFromACallbackStream;
}


PaError PaUtil_DummyWrite( PaStream* stream,
                               const void *buffer,
                               unsigned long frames )
{
    (void)stream; /* unused parameter */
    (void)buffer; /* unused parameter */
    (void)frames; /* unused parameter */

    return paCanNotWriteToACallbackStream;
}


signed long PaUtil_DummyGetReadAvailable( PaStream* stream )
{
    (void)stream; /* unused parameter */

    return paCanNotReadFromACallbackStream;
}


signed long PaUtil_DummyGetWriteAvailable( PaStream* stream )
{
    (void)stream; /* unused parameter */

    return paCanNotWriteToACallbackStream;
}


double PaUtil_DummyGetCpuLoad( PaStream* stream )
{
    (void)stream; /* unused parameter */

    return 0.0;
}
