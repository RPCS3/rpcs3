/*
 * Microsoft COM initialization routines
 * Copyright (c) 1999-2011 Ross Bencina, Phil Burk
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

 @brief Microsoft COM initialization routines.
*/

#ifndef PA_WIN_COINITIALIZE_H
#define PA_WIN_COINITIALIZE_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/**
 @brief Data type used to hold the result of an attempt to initialize COM
    using PaWinUtil_CoInitialize. Must be retained between a call to 
    PaWinUtil_CoInitialize and a matching call to PaWinUtil_CoUninitialize.
*/
typedef struct PaWinUtilComInitializationResult{
    int state;
    int initializingThreadId;
} PaWinUtilComInitializationResult;


/**
 @brief Initialize Microsoft COM subsystem on the current thread.

 @param hostApiType the host API type id of the caller. Used for error reporting.

 @param comInitializationResult An output parameter. The value pointed to by 
        this parameter stores information required by PaWinUtil_CoUninitialize 
        to correctly uninitialize COM. The value should be retained and later 
        passed to PaWinUtil_CoUninitialize.

 If PaWinUtil_CoInitialize returns paNoError, the caller must later call
 PaWinUtil_CoUninitialize once.
*/
PaError PaWinUtil_CoInitialize( PaHostApiTypeId hostApiType, PaWinUtilComInitializationResult *comInitializationResult );


/**
 @brief Uninitialize the Microsoft COM subsystem on the current thread using 
 the result of a previous call to PaWinUtil_CoInitialize. Must be called on the same
 thread as PaWinUtil_CoInitialize.

 @param hostApiType the host API type id of the caller. Used for error reporting.

 @param comInitializationResult An input parameter. A pointer to a value previously
 initialized by a call to PaWinUtil_CoInitialize.
*/
void PaWinUtil_CoUninitialize( PaHostApiTypeId hostApiType, PaWinUtilComInitializationResult *comInitializationResult );


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_WIN_COINITIALIZE_H */
