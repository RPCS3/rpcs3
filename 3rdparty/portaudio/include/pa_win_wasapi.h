#ifndef PA_WIN_WASAPI_H
#define PA_WIN_WASAPI_H
/*
 * $Id:  $
 * PortAudio Portable Real-Time Audio Library
 * DirectSound specific extensions
 *
 * Copyright (c) 1999-2007 Ross Bencina and Phil Burk
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
 @ingroup public_header
 @brief WASAPI-specific PortAudio API extension header file.
*/

#include "portaudio.h"
#include "pa_win_waveformat.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/* Setup flags */
typedef enum PaWasapiFlags
{
    /* puts WASAPI into exclusive mode */
    paWinWasapiExclusive                = (1 << 0),

    /* allows to skip internal PA processing completely */
    paWinWasapiRedirectHostProcessor    = (1 << 1),

    /* assigns custom channel mask */
    paWinWasapiUseChannelMask           = (1 << 2),

    /* selects non-Event driven method of data read/write
       Note: WASAPI Event driven core is capable of 2ms latency!!!, but Polling
             method can only provide 15-20ms latency. */
    paWinWasapiPolling                  = (1 << 3),

    /* forces custom thread priority setting. must be used if PaWasapiStreamInfo::threadPriority 
       is set to custom value. */
    paWinWasapiThreadPriority           = (1 << 4)
}
PaWasapiFlags;
#define paWinWasapiExclusive             (paWinWasapiExclusive)
#define paWinWasapiRedirectHostProcessor (paWinWasapiRedirectHostProcessor)
#define paWinWasapiUseChannelMask        (paWinWasapiUseChannelMask)
#define paWinWasapiPolling               (paWinWasapiPolling)
#define paWinWasapiThreadPriority        (paWinWasapiThreadPriority)


/* Host processor. Allows to skip internal PA processing completely. 
   You must set paWinWasapiRedirectHostProcessor flag to PaWasapiStreamInfo::flags member
   in order to have host processor redirected to your callback.
   Use with caution! inputFrames and outputFrames depend solely on final device setup.
   To query maximal values of inputFrames/outputFrames use PaWasapi_GetFramesPerHostBuffer.
*/
typedef void (*PaWasapiHostProcessorCallback) (void *inputBuffer,  long inputFrames,
                                               void *outputBuffer, long outputFrames,
                                               void *userData);

/* Device role */
typedef enum PaWasapiDeviceRole
{
    eRoleRemoteNetworkDevice = 0,
    eRoleSpeakers,
    eRoleLineLevel,
    eRoleHeadphones,
    eRoleMicrophone,
    eRoleHeadset,
    eRoleHandset,
    eRoleUnknownDigitalPassthrough,
    eRoleSPDIF,
    eRoleHDMI,
    eRoleUnknownFormFactor
}
PaWasapiDeviceRole;


/* Jack connection type */
typedef enum PaWasapiJackConnectionType
{
    eJackConnTypeUnknown,
    eJackConnType3Point5mm,
    eJackConnTypeQuarter,
    eJackConnTypeAtapiInternal,
    eJackConnTypeRCA,
    eJackConnTypeOptical,
    eJackConnTypeOtherDigital,
    eJackConnTypeOtherAnalog,
    eJackConnTypeMultichannelAnalogDIN,
    eJackConnTypeXlrProfessional,
    eJackConnTypeRJ11Modem,
    eJackConnTypeCombination
} 
PaWasapiJackConnectionType;


/* Jack geometric location */
typedef enum PaWasapiJackGeoLocation
{
	eJackGeoLocUnk = 0,
    eJackGeoLocRear = 0x1, /* matches EPcxGeoLocation::eGeoLocRear */
    eJackGeoLocFront,
    eJackGeoLocLeft,
    eJackGeoLocRight,
    eJackGeoLocTop,
    eJackGeoLocBottom,
    eJackGeoLocRearPanel,
    eJackGeoLocRiser,
    eJackGeoLocInsideMobileLid,
    eJackGeoLocDrivebay,
    eJackGeoLocHDMI,
    eJackGeoLocOutsideMobileLid,
    eJackGeoLocATAPI,
    eJackGeoLocReserved5,
    eJackGeoLocReserved6,
} 
PaWasapiJackGeoLocation;


/* Jack general location */
typedef enum PaWasapiJackGenLocation
{
    eJackGenLocPrimaryBox = 0,
    eJackGenLocInternal,
    eJackGenLocSeparate,
    eJackGenLocOther
} 
PaWasapiJackGenLocation;


/* Jack's type of port */
typedef enum PaWasapiJackPortConnection
{
    eJackPortConnJack = 0,
    eJackPortConnIntegratedDevice,
    eJackPortConnBothIntegratedAndJack,
    eJackPortConnUnknown
} 
PaWasapiJackPortConnection;


/* Thread priority */
typedef enum PaWasapiThreadPriority
{
    eThreadPriorityNone = 0,
    eThreadPriorityAudio,            //!< Default for Shared mode.
    eThreadPriorityCapture,
    eThreadPriorityDistribution,
    eThreadPriorityGames,
    eThreadPriorityPlayback,
    eThreadPriorityProAudio,        //!< Default for Exclusive mode.
    eThreadPriorityWindowManager
}
PaWasapiThreadPriority;


/* Stream descriptor. */
typedef struct PaWasapiJackDescription 
{
    unsigned long              channelMapping;
    unsigned long              color; /* derived from macro: #define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16))) */
    PaWasapiJackConnectionType connectionType;
    PaWasapiJackGeoLocation    geoLocation;
    PaWasapiJackGenLocation    genLocation;
    PaWasapiJackPortConnection portConnection;
    unsigned int               isConnected;
}
PaWasapiJackDescription;


/* Stream descriptor. */
typedef struct PaWasapiStreamInfo 
{
    unsigned long size;             /**< sizeof(PaWasapiStreamInfo) */
    PaHostApiTypeId hostApiType;    /**< paWASAPI */
    unsigned long version;          /**< 1 */

    unsigned long flags;            /**< collection of PaWasapiFlags */

    /* Support for WAVEFORMATEXTENSIBLE channel masks. If flags contains
       paWinWasapiUseChannelMask this allows you to specify which speakers 
       to address in a multichannel stream. Constants for channelMask
       are specified in pa_win_waveformat.h. Will be used only if 
       paWinWasapiUseChannelMask flag is specified.
    */
    PaWinWaveFormatChannelMask channelMask;

    /* Delivers raw data to callback obtained from GetBuffer() methods skipping 
       internal PortAudio processing inventory completely. userData parameter will 
       be the same that was passed to Pa_OpenStream method. Will be used only if 
       paWinWasapiRedirectHostProcessor flag is specified.
    */
    PaWasapiHostProcessorCallback hostProcessorOutput;
    PaWasapiHostProcessorCallback hostProcessorInput;

    /* Specifies thread priority explicitly. Will be used only if paWinWasapiThreadPriority flag
       is specified.

       Please note, if Input/Output streams are opened simultaniously (Full-Duplex mode)
       you shall specify same value for threadPriority or othervise one of the values will be used
       to setup thread priority.
    */
    PaWasapiThreadPriority threadPriority;
} 
PaWasapiStreamInfo;


/** Returns default sound format for device. Format is represented by PaWinWaveFormat or 
    WAVEFORMATEXTENSIBLE structure.

 @param pFormat Pointer to PaWinWaveFormat or WAVEFORMATEXTENSIBLE structure.
 @param nFormatSize Size of PaWinWaveFormat or WAVEFORMATEXTENSIBLE structure in bytes.
 @param nDevice Device index.

 @return Non-negative value indicating the number of bytes copied into format decriptor
         or, a PaErrorCode (which are always negative) if PortAudio is not initialized
         or an error is encountered.
*/
int PaWasapi_GetDeviceDefaultFormat( void *pFormat, unsigned int nFormatSize, PaDeviceIndex nDevice );


/** Returns device role (PaWasapiDeviceRole enum).

 @param nDevice device index.

 @return Non-negative value indicating device role or, a PaErrorCode (which are always negative)
         if PortAudio is not initialized or an error is encountered.
*/
int/*PaWasapiDeviceRole*/ PaWasapi_GetDeviceRole( PaDeviceIndex nDevice );


/** Boost thread priority of calling thread (MMCSS). Use it for Blocking Interface only for thread
    which makes calls to Pa_WriteStream/Pa_ReadStream.

 @param hTask Handle to pointer to priority task. Must be used with PaWasapi_RevertThreadPriority
              method to revert thread priority to initial state.

 @param nPriorityClass Id of thread priority of PaWasapiThreadPriority type. Specifying 
                       eThreadPriorityNone does nothing.

 @return Error code indicating success or failure.
 @see    PaWasapi_RevertThreadPriority
*/
PaError PaWasapi_ThreadPriorityBoost( void **hTask, PaWasapiThreadPriority nPriorityClass );


/** Boost thread priority of calling thread (MMCSS). Use it for Blocking Interface only for thread
    which makes calls to Pa_WriteStream/Pa_ReadStream.

 @param  hTask Task handle obtained by PaWasapi_BoostThreadPriority method.
 @return Error code indicating success or failure.
 @see    PaWasapi_BoostThreadPriority
*/
PaError PaWasapi_ThreadPriorityRevert( void *hTask );


/** Get number of frames per host buffer. This is maximal value of frames of WASAPI buffer which 
    can be locked for operations. Use this method as helper to findout maximal values of 
    inputFrames/outputFrames of PaWasapiHostProcessorCallback.

 @param  pStream Pointer to PaStream to query.
 @param  nInput  Pointer to variable to receive number of input frames. Can be NULL.
 @param  nOutput Pointer to variable to receive number of output frames. Can be NULL.
 @return Error code indicating success or failure.
 @see    PaWasapiHostProcessorCallback
*/
PaError PaWasapi_GetFramesPerHostBuffer( PaStream *pStream, unsigned int *nInput, unsigned int *nOutput );


/** Get number of jacks associated with a WASAPI device.  Use this method to determine if
    there are any jacks associated with the provided WASAPI device.  Not all audio devices
	will support this capability.  This is valid for both input and output devices.
 @param  nDevice  device index.
 @param  jcount   Number of jacks is returned in this variable
 @return Error code indicating success or failure
 @see PaWasapi_GetJackDescription
 */
PaError PaWasapi_GetJackCount(PaDeviceIndex nDevice, int *jcount);


/** Get the jack description associated with a WASAPI device and jack number
    Before this function is called, use PaWasapi_GetJackCount to determine the
	number of jacks associated with device.  If jcount is greater than zero, then
	each jack from 0 to jcount can be queried with this function to get the jack
	description.
 @param  nDevice  device index.
 @param  jindex   Which jack to return information
 @param  KSJACK_DESCRIPTION This structure filled in on success.
 @return Error code indicating success or failure
 @see PaWasapi_GetJackCount
 */
PaError PaWasapi_GetJackDescription(PaDeviceIndex nDevice, int jindex, PaWasapiJackDescription *pJackDescription);


/*
    IMPORTANT:

    WASAPI is implemented for Callback and Blocking interfaces. It supports Shared and Exclusive
    share modes. 
    
    Exclusive Mode:

        Exclusive mode allows to deliver audio data directly to hardware bypassing
        software mixing.
        Exclusive mode is specified by 'paWinWasapiExclusive' flag.

    Callback Interface:

        Provides best audio quality with low latency. Callback interface is implemented in 
        two versions:

        1) Event-Driven:
        This is the most powerful WASAPI implementation which provides glitch-free
        audio at around 3ms latency in Exclusive mode. Lowest possible latency for this mode is 
        3 ms for HD Audio class audio chips. For the Shared mode latency can not be 
		lower than 20 ms.

        2) Poll-Driven:
        Polling is another 2-nd method to operate with WASAPI. It is less efficient than Event-Driven
        and provides latency at around 10-13ms. Polling must be used to overcome a system bug
        under Windows Vista x64 when application is WOW64(32-bit) and Event-Driven method simply 
        times out (event handle is never signalled on buffer completion). Please note, such WOW64 bug 
        does not exist in Vista x86 or Windows 7.
        Polling can be setup by speciying 'paWinWasapiPolling' flag. Our WASAPI implementation detects
        WOW64 bug and sets 'paWinWasapiPolling' automatically.

    Thread priority:

        Normally thread priority is set automatically and does not require modification. Although
        if user wants some tweaking thread priority can be modified by setting 'paWinWasapiThreadPriority'
        flag and specifying 'PaWasapiStreamInfo::threadPriority' with value from PaWasapiThreadPriority 
        enum.

    Blocking Interface:

        Blocking interface is implemented but due to above described Poll-Driven method can not
        deliver lowest possible latency. Specifying too low latency in Shared mode will result in 
        distorted audio although Exclusive mode adds stability.

    Pa_IsFormatSupported:

        To check format with correct Share Mode (Exclusive/Shared) you must supply
        PaWasapiStreamInfo with flags paWinWasapiExclusive set through member of 
        PaStreamParameters::hostApiSpecificStreamInfo structure.

    Pa_OpenStream:

        To set desired Share Mode (Exclusive/Shared) you must supply
        PaWasapiStreamInfo with flags paWinWasapiExclusive set through member of 
        PaStreamParameters::hostApiSpecificStreamInfo structure.
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_WASAPI_H */                                  
