/*
 * Microsoft COM initialization routines
 * Copyright (c) 1999-2011 Ross Bencina, Dmitry Kostjuchenko
 *
 * Based on the Open Source API proposed by Ross Bencina
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

#include <windows.h>
#include <objbase.h>

#include "portaudio.h"
#include "pa_util.h"
#include "pa_debugprint.h"

#include "pa_win_coinitialize.h"


#if (defined(WIN32) && (defined(_MSC_VER) && (_MSC_VER >= 1200))) && !defined(_WIN32_WCE) /* MSC version 6 and above */
#pragma comment( lib, "ole32.lib" )
#endif


/* use some special bit patterns here to try to guard against uninitialized memory errors */
#define PAWINUTIL_COM_INITIALIZED       (0xb38f)
#define PAWINUTIL_COM_NOT_INITIALIZED   (0xf1cd)


PaError PaWinUtil_CoInitialize( PaHostApiTypeId hostApiType, PaWinUtilComInitializationResult *comInitializationResult )
{
    HRESULT hr;

    comInitializationResult->state = PAWINUTIL_COM_NOT_INITIALIZED;

    /*
        If COM is already initialized CoInitialize will either return
        FALSE, or RPC_E_CHANGED_MODE if it was initialised in a different
        threading mode. In either case we shouldn't consider it an error
        but we need to be careful to not call CoUninitialize() if 
        RPC_E_CHANGED_MODE was returned.
    */

    hr = CoInitialize(0); /* use legacy-safe equivalent to CoInitializeEx(NULL, COINIT_APARTMENTTHREADED) */
    if( FAILED(hr) && hr != RPC_E_CHANGED_MODE )
    {
        PA_DEBUG(("CoInitialize(0) failed. hr=%d\n", hr));

        if( hr == E_OUTOFMEMORY )
            return paInsufficientMemory;

        {
            char *lpMsgBuf;
            FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0,
                NULL
            );
            PaUtil_SetLastHostErrorInfo( hostApiType, hr, lpMsgBuf );
            LocalFree( lpMsgBuf );
        }

        return paUnanticipatedHostError;
    }

    if( hr != RPC_E_CHANGED_MODE )
    {
        comInitializationResult->state = PAWINUTIL_COM_INITIALIZED;

        /*
            Memorize calling thread id and report warning on Uninitialize if 
            calling thread is different as CoInitialize must match CoUninitialize 
            in the same thread.
        */
        comInitializationResult->initializingThreadId = GetCurrentThreadId();
    }

    return paNoError;
}


void PaWinUtil_CoUninitialize( PaHostApiTypeId hostApiType, PaWinUtilComInitializationResult *comInitializationResult )
{
    if( comInitializationResult->state != PAWINUTIL_COM_NOT_INITIALIZED
            && comInitializationResult->state != PAWINUTIL_COM_INITIALIZED ){
    
        PA_DEBUG(("ERROR: PaWinUtil_CoUninitialize called without calling PaWinUtil_CoInitialize\n"));
    }

    if( comInitializationResult->state == PAWINUTIL_COM_INITIALIZED )
    {
        DWORD currentThreadId = GetCurrentThreadId();
		if( comInitializationResult->initializingThreadId != currentThreadId )
		{
			PA_DEBUG(("ERROR: failed PaWinUtil_CoUninitialize calling thread[%d] does not match initializing thread[%d]\n",
				currentThreadId, comInitializationResult->initializingThreadId));
		}
		else
		{
			CoUninitialize();

            comInitializationResult->state = PAWINUTIL_COM_NOT_INITIALIZED;
		}
    }
}