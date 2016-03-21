/***************************************************************************
 *
 *  Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 *  File:     audiodefs.h
 *  Content:  Basic constants and data types for audio work.
 *
 *  Remarks:  This header file defines all of the audio format constants and
 *            structures required for XAudio2 and XACT work.  Providing these
 *            in a single location avoids certain dependency problems in the
 *            legacy audio headers (mmreg.h, mmsystem.h, ksmedia.h).
 *
 *            NOTE: Including the legacy headers after this one may cause a
 *            compilation error, because they define some of the same types
 *            defined here without preprocessor guards to avoid multiple
 *            definitions.  If a source file needs one of the old headers,
 *            it must include it before including audiodefs.h.
 *
 ***************************************************************************/

#ifndef __AUDIODEFS_INCLUDED__
#define __AUDIODEFS_INCLUDED__

#include <windef.h>  // For WORD, DWORD, etc.

#pragma pack(push, 1)  // Pack structures to 1-byte boundaries


/**************************************************************************
 *
 *  WAVEFORMATEX: Base structure for many audio formats.  Format-specific
 *  extensions can be defined for particular formats by using a non-zero
 *  cbSize value and adding extra fields to the end of this structure.
 *
 ***************************************************************************/

#ifndef _WAVEFORMATEX_

    #define _WAVEFORMATEX_
    typedef struct tWAVEFORMATEX
    {
        WORD wFormatTag;        // Integer identifier of the format
        WORD nChannels;         // Number of audio channels
        DWORD nSamplesPerSec;   // Audio sample rate
        DWORD nAvgBytesPerSec;  // Bytes per second (possibly approximate)
        WORD nBlockAlign;       // Size in bytes of a sample block (all channels)
        WORD wBitsPerSample;    // Size in bits of a single per-channel sample
        WORD cbSize;            // Bytes of extra data appended to this struct
    } WAVEFORMATEX;

#endif

// Defining pointer types outside of the #if block to make sure they are
// defined even if mmreg.h or mmsystem.h is #included before this file

typedef WAVEFORMATEX *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;
typedef const WAVEFORMATEX *PCWAVEFORMATEX, *LPCWAVEFORMATEX;


/**************************************************************************
 *
 *  WAVEFORMATEXTENSIBLE: Extended version of WAVEFORMATEX that should be
 *  used as a basis for all new audio formats.  The format tag is replaced
 *  with a GUID, allowing new formats to be defined without registering a
 *  format tag with Microsoft.  There are also new fields that can be used
 *  to specify the spatial positions for each channel and the bit packing
 *  used for wide samples (e.g. 24-bit PCM samples in 32-bit containers).
 *
 ***************************************************************************/

#ifndef _WAVEFORMATEXTENSIBLE_

    #define _WAVEFORMATEXTENSIBLE_
    typedef struct
    {
        WAVEFORMATEX Format;          // Base WAVEFORMATEX data
        union
        {
            WORD wValidBitsPerSample; // Valid bits in each sample container
            WORD wSamplesPerBlock;    // Samples per block of audio data; valid
                                      // if wBitsPerSample=0 (but rarely used).
            WORD wReserved;           // Zero if neither case above applies.
        } Samples;
        DWORD dwChannelMask;          // Positions of the audio channels
        GUID SubFormat;               // Format identifier GUID
    } WAVEFORMATEXTENSIBLE;

#endif

typedef WAVEFORMATEXTENSIBLE *PWAVEFORMATEXTENSIBLE, *LPWAVEFORMATEXTENSIBLE;
typedef const WAVEFORMATEXTENSIBLE *PCWAVEFORMATEXTENSIBLE, *LPCWAVEFORMATEXTENSIBLE;



/**************************************************************************
 *
 *  Define the most common wave format tags used in WAVEFORMATEX formats.
 *
 ***************************************************************************/

#ifndef WAVE_FORMAT_PCM  // Pulse Code Modulation

    // If WAVE_FORMAT_PCM is not defined, we need to define some legacy types
    // for compatibility with the Windows mmreg.h / mmsystem.h header files.

    // Old general format structure (information common to all formats)
    typedef struct waveformat_tag
    {
        WORD wFormatTag;
        WORD nChannels;
        DWORD nSamplesPerSec;
        DWORD nAvgBytesPerSec;
        WORD nBlockAlign;
    } WAVEFORMAT, *PWAVEFORMAT, NEAR *NPWAVEFORMAT, FAR *LPWAVEFORMAT;

    // Specific format structure for PCM data
    typedef struct pcmwaveformat_tag
    {
        WAVEFORMAT wf;
        WORD wBitsPerSample;
    } PCMWAVEFORMAT, *PPCMWAVEFORMAT, NEAR *NPPCMWAVEFORMAT, FAR *LPPCMWAVEFORMAT;

    #define WAVE_FORMAT_PCM 0x0001

#endif

#ifndef WAVE_FORMAT_ADPCM  // Microsoft Adaptive Differental PCM

    // Replicate the Microsoft ADPCM type definitions from mmreg.h.

    typedef struct adpcmcoef_tag
    {
        short iCoef1;
        short iCoef2;
    } ADPCMCOEFSET;

    #pragma warning(push)
    #pragma warning(disable:4200)  // Disable zero-sized array warnings

    typedef struct adpcmwaveformat_tag {
        WAVEFORMATEX wfx;
        WORD wSamplesPerBlock;
        WORD wNumCoef;
        ADPCMCOEFSET aCoef[];  // Always 7 coefficient pairs for MS ADPCM
    } ADPCMWAVEFORMAT;

    #pragma warning(pop)

    #define WAVE_FORMAT_ADPCM 0x0002

#endif

// Other frequently used format tags

#ifndef WAVE_FORMAT_UNKNOWN
    #define WAVE_FORMAT_UNKNOWN         0x0000 // Unknown or invalid format tag
#endif

#ifndef WAVE_FORMAT_IEEE_FLOAT
    #define WAVE_FORMAT_IEEE_FLOAT      0x0003 // 32-bit floating-point
#endif

#ifndef WAVE_FORMAT_MPEGLAYER3
    #define WAVE_FORMAT_MPEGLAYER3      0x0055 // ISO/MPEG Layer3
#endif

#ifndef WAVE_FORMAT_DOLBY_AC3_SPDIF
    #define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092 // Dolby Audio Codec 3 over S/PDIF
#endif

#ifndef WAVE_FORMAT_WMAUDIO2
    #define WAVE_FORMAT_WMAUDIO2        0x0161 // Windows Media Audio
#endif

#ifndef WAVE_FORMAT_WMAUDIO3
    #define WAVE_FORMAT_WMAUDIO3        0x0162 // Windows Media Audio Pro
#endif

#ifndef WAVE_FORMAT_WMASPDIF
    #define WAVE_FORMAT_WMASPDIF        0x0164 // Windows Media Audio over S/PDIF
#endif

#ifndef WAVE_FORMAT_EXTENSIBLE
    #define WAVE_FORMAT_EXTENSIBLE      0xFFFE // All WAVEFORMATEXTENSIBLE formats
#endif


/**************************************************************************
 *
 *  Define the most common wave format GUIDs used in WAVEFORMATEXTENSIBLE
 *  formats.  Note that including the Windows ksmedia.h header after this
 *  one will cause build problems; this cannot be avoided, since ksmedia.h
 *  defines these macros without preprocessor guards.
 *
 ***************************************************************************/

#ifdef __cplusplus // uuid() and __uuidof() are only available in C++

    #ifndef KSDATAFORMAT_SUBTYPE_PCM
        struct __declspec(uuid("00000001-0000-0010-8000-00aa00389b71")) KSDATAFORMAT_SUBTYPE_PCM_STRUCT;
        #define KSDATAFORMAT_SUBTYPE_PCM __uuidof(KSDATAFORMAT_SUBTYPE_PCM_STRUCT)
    #endif

    #ifndef KSDATAFORMAT_SUBTYPE_ADPCM
        struct __declspec(uuid("00000002-0000-0010-8000-00aa00389b71")) KSDATAFORMAT_SUBTYPE_ADPCM_STRUCT;
        #define KSDATAFORMAT_SUBTYPE_ADPCM __uuidof(KSDATAFORMAT_SUBTYPE_ADPCM_STRUCT)
    #endif

    #ifndef KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
        struct __declspec(uuid("00000003-0000-0010-8000-00aa00389b71")) KSDATAFORMAT_SUBTYPE_IEEE_FLOAT_STRUCT;
        #define KSDATAFORMAT_SUBTYPE_IEEE_FLOAT __uuidof(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT_STRUCT)
    #endif

#endif


/**************************************************************************
 *
 *  Speaker positions used in the WAVEFORMATEXTENSIBLE dwChannelMask field.
 *
 ***************************************************************************/

#ifndef SPEAKER_FRONT_LEFT
    #define SPEAKER_FRONT_LEFT            0x00000001
    #define SPEAKER_FRONT_RIGHT           0x00000002
    #define SPEAKER_FRONT_CENTER          0x00000004
    #define SPEAKER_LOW_FREQUENCY         0x00000008
    #define SPEAKER_BACK_LEFT             0x00000010
    #define SPEAKER_BACK_RIGHT            0x00000020
    #define SPEAKER_FRONT_LEFT_OF_CENTER  0x00000040
    #define SPEAKER_FRONT_RIGHT_OF_CENTER 0x00000080
    #define SPEAKER_BACK_CENTER           0x00000100
    #define SPEAKER_SIDE_LEFT             0x00000200
    #define SPEAKER_SIDE_RIGHT            0x00000400
    #define SPEAKER_TOP_CENTER            0x00000800
    #define SPEAKER_TOP_FRONT_LEFT        0x00001000
    #define SPEAKER_TOP_FRONT_CENTER      0x00002000
    #define SPEAKER_TOP_FRONT_RIGHT       0x00004000
    #define SPEAKER_TOP_BACK_LEFT         0x00008000
    #define SPEAKER_TOP_BACK_CENTER       0x00010000
    #define SPEAKER_TOP_BACK_RIGHT        0x00020000
    #define SPEAKER_RESERVED              0x7FFC0000
    #define SPEAKER_ALL                   0x80000000
    #define _SPEAKER_POSITIONS_
#endif

#ifndef SPEAKER_STEREO
    #define SPEAKER_MONO             (SPEAKER_FRONT_CENTER)
    #define SPEAKER_STEREO           (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
    #define SPEAKER_2POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY)
    #define SPEAKER_SURROUND         (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER)
    #define SPEAKER_QUAD             (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
    #define SPEAKER_4POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
    #define SPEAKER_5POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT)
    #define SPEAKER_7POINT1          (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_LEFT_OF_CENTER | SPEAKER_FRONT_RIGHT_OF_CENTER)
    #define SPEAKER_5POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)
    #define SPEAKER_7POINT1_SURROUND (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT  | SPEAKER_SIDE_RIGHT)
#endif


#pragma pack(pop)

#endif // #ifndef __AUDIODEFS_INCLUDED__
