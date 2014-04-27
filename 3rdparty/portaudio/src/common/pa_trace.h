#ifndef PA_TRACE_H
#define PA_TRACE_H
/*
 * $Id: pa_trace.h 1812 2012-02-14 09:32:57Z robiwan $
 * Portable Audio I/O Library Trace Facility
 * Store trace information in real-time for later printing.
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2000 Phil Burk
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

 @brief Real-time safe event trace logging facility for debugging.

 Allows data to be logged to a fixed size trace buffer in a real-time
 execution context (such as at interrupt time). Each log entry consists 
 of a message comprising a string pointer and an int.  The trace buffer 
 may be dumped to stdout later.

 This facility is only active if PA_TRACE_REALTIME_EVENTS is set to 1,
 otherwise the trace functions expand to no-ops.

 @fn PaUtil_ResetTraceMessages
 @brief Clear the trace buffer.

 @fn PaUtil_AddTraceMessage
 @brief Add a message to the trace buffer. A message consists of string and an int.
 @param msg The string pointer must remain valid until PaUtil_DumpTraceMessages 
    is called. As a result, usually only string literals should be passed as 
    the msg parameter.

 @fn PaUtil_DumpTraceMessages
 @brief Print all messages in the trace buffer to stdout and clear the trace buffer.
*/

#ifndef PA_TRACE_REALTIME_EVENTS
#define PA_TRACE_REALTIME_EVENTS     (0)   /**< Set to 1 to enable logging using the trace functions defined below */
#endif

#ifndef PA_MAX_TRACE_RECORDS
#define PA_MAX_TRACE_RECORDS      (2048)   /**< Maximum number of records stored in trace buffer */   
#endif

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#if PA_TRACE_REALTIME_EVENTS

void PaUtil_ResetTraceMessages();
void PaUtil_AddTraceMessage( const char *msg, int data );
void PaUtil_DumpTraceMessages();

/* Alternative interface */

typedef void* LogHandle;

int PaUtil_InitializeHighSpeedLog(LogHandle* phLog, unsigned maxSizeInBytes);
void PaUtil_ResetHighSpeedLogTimeRef(LogHandle hLog);
int PaUtil_AddHighSpeedLogMessage(LogHandle hLog, const char* fmt, ...);
void PaUtil_DumpHighSpeedLog(LogHandle hLog, const char* fileName);
void PaUtil_DiscardHighSpeedLog(LogHandle hLog);

#else

#define PaUtil_ResetTraceMessages() /* noop */
#define PaUtil_AddTraceMessage(msg,data) /* noop */
#define PaUtil_DumpTraceMessages() /* noop */

#define PaUtil_InitializeHighSpeedLog(phLog, maxSizeInBytes)  (0)
#define PaUtil_ResetHighSpeedLogTimeRef(hLog)
#define PaUtil_AddHighSpeedLogMessage(...)   (0)
#define PaUtil_DumpHighSpeedLog(hLog, fileName)
#define PaUtil_DiscardHighSpeedLog(hLog)

#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_TRACE_H */
