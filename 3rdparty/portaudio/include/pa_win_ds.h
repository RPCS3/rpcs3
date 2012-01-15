#ifndef PA_WIN_DS_H
#define PA_WIN_DS_H
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
 @brief DirectSound-specific PortAudio API extension header file.
*/

#include "portaudio.h"
#include "pa_win_waveformat.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


#define paWinDirectSoundUseLowLevelLatencyParameters            (0x01)
#define paWinDirectSoundUseChannelMask                          (0x04)


typedef struct PaWinDirectSoundStreamInfo{
    unsigned long size;             /**< sizeof(PaWinDirectSoundStreamInfo) */
    PaHostApiTypeId hostApiType;    /**< paDirectSound */
    unsigned long version;          /**< 2 */

    unsigned long flags;            /**< enable other features of this struct */

    /** 
       low-level latency setting support
       Sets the size of the DirectSound host buffer.
       When flags contains the paWinDirectSoundUseLowLevelLatencyParameters
       this size will be used instead of interpreting the generic latency 
       parameters to Pa_OpenStream(). If the flag is not set this value is ignored.

       If the stream is a full duplex stream the implementation requires that
       the values of framesPerBuffer for input and output match (if both are specified).
    */
    unsigned long framesPerBuffer;

    /**
        support for WAVEFORMATEXTENSIBLE channel masks. If flags contains
        paWinDirectSoundUseChannelMask this allows you to specify which speakers 
        to address in a multichannel stream. Constants for channelMask
        are specified in pa_win_waveformat.h

    */
    PaWinWaveFormatChannelMask channelMask;

}PaWinDirectSoundStreamInfo;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PA_WIN_DS_H */                                  
