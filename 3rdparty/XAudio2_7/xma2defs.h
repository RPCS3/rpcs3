/***************************************************************************
 *
 * Copyright (c) Microsoft Corporation.  All rights reserved.
 *
 * File:     xma2defs.h
 * Content:  Constants, data types and functions for XMA2 compressed audio.
 *
 ***************************************************************************/

#ifndef __XMA2DEFS_INCLUDED__
#define __XMA2DEFS_INCLUDED__

#include <sal.h>        // Markers for documenting API semantics
#include <winerror.h>   // For S_OK, E_FAIL
#include <audiodefs.h>  // Basic data types and constants for audio work


/***************************************************************************
 *  Overview
 ***************************************************************************/

// A typical XMA2 file contains these RIFF chunks:
//
// 'fmt' or 'XMA2' chunk (or both): A description of the XMA data's structure
// and characteristics (length, channels, sample rate, loops, block size, etc).
//
// 'seek' chunk: A seek table to help navigate the XMA data.
//
// 'data' chunk: The encoded XMA2 data.
//
// The encoded XMA2 data is structured as a set of BLOCKS, which contain PACKETS,
// which contain FRAMES, which contain SUBFRAMES (roughly speaking).  The frames
// in a file may also be divided into several subsets, called STREAMS.
//
// FRAME: A variable-sized segment of XMA data that decodes to exactly 512 mono
//      or stereo PCM samples.  This is the smallest unit of XMA data that can
//      be decoded in isolation.  Frames are an arbitrary number of bits in
//      length, and need not be byte-aligned.  See "XMA frame structure" below.
//
// SUBFRAME: A region of bits in an XMA frame that decodes to 128 mono or stereo
//      samples.  The XMA decoder cannot decode a subframe in isolation; it needs
//      a whole frame to work with.  However, it can begin emitting the frame's
//      decoded samples at any one of the four subframe boundaries.  Subframes
//      can be addressed for seeking and looping purposes.
//
// PACKET: A 2Kb region containing a 32-bit header and some XMA frames.  Frames
//      can (and usually do) span packets.  A packet's header includes the offset
//      in bits of the first frame that begins within that packet.  All of the
//      frames that begin in a given packet belong to the same "stream" (see the
//      Multichannel Audio section below).
//
// STREAM: A set of packets within an XMA file that all contain data for the
//      same mono or stereo component of a PCM file with more than two channels.
//      The packets comprising a given stream may be interleaved with each other
//      more or less arbitrarily; see Multichannel Audio.
//
// BLOCK: An array of XMA packets; or, to break it down differently, a series of
//      consecutive XMA frames, padded at the end with reserved data.  A block
//      must contain at least one 2Kb packet per stream, and it can hold up to
//      4095 packets (8190Kb), but its size is typically in the 32Kb-128Kb range.
//      (The size chosen involves a trade-off between memory use and efficiency
//      of reading from permanent storage.)
//
//      XMA frames do not span blocks, so a block is guaranteed to begin with a
//      set of complete frames, one per stream.  Also, a block in a multi-stream
//      XMA2 file always contains the same number of samples for each stream;
//      see Multichannel Audio.
//
// The 'data' chunk in an XMA2 file is an array of XMA2WAVEFORMAT.BlockCount XMA
// blocks, all the same size (as specified in XMA2WAVEFORMAT.BlockSizeInBytes)
// except for the last one, which may be shorter.


// MULTICHANNEL AUDIO: the XMA decoder can only decode raw XMA data into either
// mono or stereo PCM data.  In order to encode a 6-channel file (say), the file
// must be deinterleaved into 3 stereo streams that are encoded independently,
// producing 3 encoded XMA data streams.  Then the packets in these 3 streams
// are interleaved to produce a single XMA2 file, and some information is added
// to the file so that the original 6-channel audio can be reconstructed at
// decode time.  This works using the concept of an XMA stream (see above).
//
// The frames for all the streams in an XMA file are interleaved in an arbitrary
// order.  To locate a frame that belongs to a given stream in a given XMA block,
// you must examine the first few packets in the block.  Here (and only here) the
// packets are guaranteed to be presented in stream order, so that all frames
// beginning in packet 0 belong to stream 0 (the first stereo pair), etc.
//
// (This means that when decoding multi-stream XMA files, only entire XMA blocks
// should be submitted to the decoder; otherwise it cannot know which frames
// belong to which stream.)
//
// Once you have one frame that belongs to a given stream, you can find the next
// one by looking at the frame's 'NextFrameOffsetBits' value (which is stored in
// its first 15 bits; see XMAFRAME below).  The GetXmaFrameBitPosition function
// uses this technique.


// SEEKING IN XMA2 FILES: Here is some pseudocode to find the byte position and
// subframe in an XMA2 file which will contain sample S when decoded.
//
// 1. Traverse the seek table to find the XMA2 block containing sample S. The
//    seek table is an array of big-endian DWORDs, one per block in the file.
//    The Nth DWORD is the total number of PCM samples that would be obtained
//    by decoding the entire XMA file up to the end of block N.  Hence, the
//    block we want is the first one whose seek table entry is greater than S.
//    (See the GetXmaBlockContainingSample helper function.)
//
// 2. Calculate which frame F within the block found above contains sample S.
//    Since each frame decodes to 512 samples, this is straightforward.  The
//    first frame in the block produces samples X to X + 512, where X is the
//    seek table entry for the prior block.  So F is (S - X) / 512.
//
// 3. Find the bit offset within the block where frame F starts.  Since frames
//    are variable-sized, this can only be done by traversing all the frames in
//    the block until we reach frame F.  (See GetXmaFrameBitPosition.)
//
// 4. Frame F has four 128-sample subframes.  To find the subframe containing S,
//    we can use the formula (S % 512) / 128.
//
// In the case of multi-stream XMA files, sample S is a multichannel sample with
// parts coming from several frames, one per stream.  To find all these frames,
// steps 2-4 need to be repeated for each stream N, using the knowledge that the
// first packets in a block are presented in stream order.  The frame traversal
// in step 3 must be started at the first frame in the Nth packet of the block,
// which will be the first frame for stream N.  (And the packet header will tell
// you the first frame's start position within the packet.)
//
// Step 1 can be performed using the GetXmaBlockContainingSample function below,
// and steps 2-4 by calling GetXmaDecodePositionForSample once for each stream.



/***************************************************************************
 *  XMA constants
 ***************************************************************************/

// Size of the PCM samples produced by the XMA decoder
#define XMA_OUTPUT_SAMPLE_BYTES         2u
#define XMA_OUTPUT_SAMPLE_BITS          (XMA_OUTPUT_SAMPLE_BYTES * 8u)

// Size of an XMA packet
#define XMA_BYTES_PER_PACKET            2048u
#define XMA_BITS_PER_PACKET             (XMA_BYTES_PER_PACKET * 8u)

// Size of an XMA packet header
#define XMA_PACKET_HEADER_BYTES         4u
#define XMA_PACKET_HEADER_BITS          (XMA_PACKET_HEADER_BYTES * 8u)

// Sample blocks in a decoded XMA frame
#define XMA_SAMPLES_PER_FRAME           512u

// Sample blocks in a decoded XMA subframe
#define XMA_SAMPLES_PER_SUBFRAME        128u

// Maximum encoded data that can be submitted to the XMA decoder at a time
#define XMA_READBUFFER_MAX_PACKETS      4095u
#define XMA_READBUFFER_MAX_BYTES        (XMA_READBUFFER_MAX_PACKETS * XMA_BYTES_PER_PACKET)

// Maximum size allowed for the XMA decoder's output buffers
#define XMA_WRITEBUFFER_MAX_BYTES       (31u * 256u)

// Required byte alignment of the XMA decoder's output buffers
#define XMA_WRITEBUFFER_BYTE_ALIGNMENT  256u

// Decode chunk sizes for the XMA_PLAYBACK_INIT.subframesToDecode field
#define XMA_MIN_SUBFRAMES_TO_DECODE     1u
#define XMA_MAX_SUBFRAMES_TO_DECODE     8u
#define XMA_OPTIMAL_SUBFRAMES_TO_DECODE 4u

// LoopCount<255 means finite repetitions; LoopCount=255 means infinite looping
#define XMA_MAX_LOOPCOUNT               254u
#define XMA_INFINITE_LOOP               255u



/***************************************************************************
 *  XMA format structures
 ***************************************************************************/

// The currently recommended way to express format information for XMA2 files
// is the XMA2WAVEFORMATEX structure.  This structure is fully compliant with
// the WAVEFORMATEX standard and contains all the information needed to parse
// and manage XMA2 files in a compact way.

#define WAVE_FORMAT_XMA2 0x166

typedef struct XMA2WAVEFORMATEX
{
    WAVEFORMATEX wfx;
    // Meaning of the WAVEFORMATEX fields here:
    //    wFormatTag;        // Audio format type; always WAVE_FORMAT_XMA2
    //    nChannels;         // Channel count of the decoded audio
    //    nSamplesPerSec;    // Sample rate of the decoded audio
    //    nAvgBytesPerSec;   // Used internally by the XMA encoder
    //    nBlockAlign;       // Decoded sample size; channels * wBitsPerSample / 8
    //    wBitsPerSample;    // Bits per decoded mono sample; always 16 for XMA
    //    cbSize;            // Size in bytes of the rest of this structure (34)

    WORD  NumStreams;        // Number of audio streams (1 or 2 channels each)
    DWORD ChannelMask;       // Spatial positions of the channels in this file,
                             // stored as SPEAKER_xxx values (see audiodefs.h)
    DWORD SamplesEncoded;    // Total number of PCM samples the file decodes to
    DWORD BytesPerBlock;     // XMA block size (but the last one may be shorter)
    DWORD PlayBegin;         // First valid sample in the decoded audio
    DWORD PlayLength;        // Length of the valid part of the decoded audio
    DWORD LoopBegin;         // Beginning of the loop region in decoded sample terms
    DWORD LoopLength;        // Length of the loop region in decoded sample terms
    BYTE  LoopCount;         // Number of loop repetitions; 255 = infinite
    BYTE  EncoderVersion;    // Version of XMA encoder that generated the file
    WORD  BlockCount;        // XMA blocks in file (and entries in its seek table)
} XMA2WAVEFORMATEX, *PXMA2WAVEFORMATEX;


// The legacy XMA format structures are described here for reference, but they
// should not be used in new content.  XMAWAVEFORMAT was the structure used in
// XMA version 1 files.  XMA2WAVEFORMAT was used in early XMA2 files; it is not
// placed in the usual 'fmt' RIFF chunk but in its own 'XMA2' chunk.

#ifndef WAVE_FORMAT_XMA
#define WAVE_FORMAT_XMA 0x0165

// Values used in the ChannelMask fields below.  Similar to the SPEAKER_xxx
// values defined in audiodefs.h, but modified to fit in a single byte.
#ifndef XMA_SPEAKER_LEFT
    #define XMA_SPEAKER_LEFT            0x01
    #define XMA_SPEAKER_RIGHT           0x02
    #define XMA_SPEAKER_CENTER          0x04
    #define XMA_SPEAKER_LFE             0x08
    #define XMA_SPEAKER_LEFT_SURROUND   0x10
    #define XMA_SPEAKER_RIGHT_SURROUND  0x20
    #define XMA_SPEAKER_LEFT_BACK       0x40
    #define XMA_SPEAKER_RIGHT_BACK      0x80
#endif


// Used in XMAWAVEFORMAT for per-stream data
typedef struct XMASTREAMFORMAT
{
    DWORD PsuedoBytesPerSec; // Used by the XMA encoder (typo preserved for legacy reasons)
    DWORD SampleRate;        // The stream's decoded sample rate (in XMA2 files,
                             // this is the same for all streams in the file).
    DWORD LoopStart;         // Bit offset of the frame containing the loop start
                             // point, relative to the beginning of the stream.
    DWORD LoopEnd;           // Bit offset of the frame containing the loop end.
    BYTE  SubframeData;      // Two 4-bit numbers specifying the exact location of
                             // the loop points within the frames that contain them.
                             //   SubframeEnd: Subframe of the loop end frame where
                             //                the loop ends.  Ranges from 0 to 3.
                             //   SubframeSkip: Subframes to skip in the start frame to
                             //                 reach the loop.  Ranges from 0 to 4.
    BYTE  Channels;          // Number of channels in the stream (1 or 2)
    WORD  ChannelMask;       // Spatial positions of the channels in the stream
} XMASTREAMFORMAT;

// Legacy XMA1 format structure
typedef struct XMAWAVEFORMAT
{
    WORD FormatTag;          // Audio format type (always WAVE_FORMAT_XMA)
    WORD BitsPerSample;      // Bit depth (currently required to be 16)
    WORD EncodeOptions;      // Options for XMA encoder/decoder
    WORD LargestSkip;        // Largest skip used in interleaving streams
    WORD NumStreams;         // Number of interleaved audio streams
    BYTE LoopCount;          // Number of loop repetitions; 255 = infinite
    BYTE Version;            // XMA encoder version that generated the file.
                             // Always 3 or higher for XMA2 files.
    XMASTREAMFORMAT XmaStreams[1]; // Per-stream format information; the actual
                                   // array length is in the NumStreams field.
} XMAWAVEFORMAT;


// Used in XMA2WAVEFORMAT for per-stream data
typedef struct XMA2STREAMFORMAT
{
    BYTE Channels;           // Number of channels in the stream (1 or 2)
    BYTE RESERVED;           // Reserved for future use
    WORD ChannelMask;        // Spatial positions of the channels in the stream
} XMA2STREAMFORMAT;

// Legacy XMA2 format structure (big-endian byte ordering)
typedef struct XMA2WAVEFORMAT
{
    BYTE  Version;           // XMA encoder version that generated the file.
                             // Always 3 or higher for XMA2 files.
    BYTE  NumStreams;        // Number of interleaved audio streams
    BYTE  RESERVED;          // Reserved for future use
    BYTE  LoopCount;         // Number of loop repetitions; 255 = infinite
    DWORD LoopBegin;         // Loop begin point, in samples
    DWORD LoopEnd;           // Loop end point, in samples
    DWORD SampleRate;        // The file's decoded sample rate
    DWORD EncodeOptions;     // Options for the XMA encoder/decoder
    DWORD PsuedoBytesPerSec; // Used internally by the XMA encoder
    DWORD BlockSizeInBytes;  // Size in bytes of this file's XMA blocks (except
                             // possibly the last one).  Always a multiple of
                             // 2Kb, since XMA blocks are arrays of 2Kb packets.
    DWORD SamplesEncoded;    // Total number of PCM samples encoded in this file
    DWORD SamplesInSource;   // Actual number of PCM samples in the source
                             // material used to generate this file
    DWORD BlockCount;        // Number of XMA blocks in this file (and hence
                             // also the number of entries in its seek table)
    XMA2STREAMFORMAT Streams[1]; // Per-stream format information; the actual
                                 // array length is in the NumStreams field.
} XMA2WAVEFORMAT;

#endif // #ifndef WAVE_FORMAT_XMA



/***************************************************************************
 *  XMA packet structure (in big-endian form)
 ***************************************************************************/

typedef struct XMA2PACKET
{
    int FrameCount        :  6;  // Number of XMA frames that begin in this packet
    int FrameOffsetInBits : 15;  // Bit of XmaData where the first complete frame begins
    int PacketMetaData    :  3;  // Metadata stored in the packet (always 1 for XMA2)
    int PacketSkipCount   :  8;  // How many packets belonging to other streams must be
                                 // skipped to find the next packet belonging to this one
    BYTE XmaData[XMA_BYTES_PER_PACKET - sizeof(DWORD)];  // XMA encoded data
} XMA2PACKET;

// E.g. if the first DWORD of a packet is 0x30107902:
//
// 001100 000001000001111 001 00000010
//    |          |         |      |____ Skip 2 packets to find the next one for this stream
//    |          |         |___________ XMA2 signature (always 001)
//    |          |_____________________ First frame starts 527 bits into packet
//    |________________________________ Packet contains 12 frames


// Helper functions to extract the fields above from an XMA packet.  (Note that
// the bitfields cannot be read directly on little-endian architectures such as
// the Intel x86, as they are laid out in big-endian form.)

__inline DWORD GetXmaPacketFrameCount(__in_bcount(1) const BYTE* pPacket)
{
    return (DWORD)(pPacket[0] >> 2);
}

__inline DWORD GetXmaPacketFirstFrameOffsetInBits(__in_bcount(3) const BYTE* pPacket)
{
    return ((DWORD)(pPacket[0] & 0x3) << 13) |
           ((DWORD)(pPacket[1]) << 5) |
           ((DWORD)(pPacket[2]) >> 3);
}

__inline DWORD GetXmaPacketMetadata(__in_bcount(3) const BYTE* pPacket)
{
    return (DWORD)(pPacket[2] & 0x7);
}

__inline DWORD GetXmaPacketSkipCount(__in_bcount(4) const BYTE* pPacket)
{
    return (DWORD)(pPacket[3]);
}



/***************************************************************************
 *  XMA frame structure
 ***************************************************************************/

// There is no way to represent the XMA frame as a C struct, since it is a
// variable-sized string of bits that need not be stored at a byte-aligned
// position in memory.  This is the layout:
//
// XMAFRAME
// {
//    LengthInBits: A 15-bit number representing the length of this frame.
//    XmaData: Encoded XMA data; its size in bits is (LengthInBits - 15).
// }

// Size in bits of the frame's initial LengthInBits field
#define XMA_BITS_IN_FRAME_LENGTH_FIELD 15

// Special LengthInBits value that marks an invalid final frame
#define XMA_FINAL_FRAME_MARKER 0x7FFF



/***************************************************************************
 *  XMA helper functions
 ***************************************************************************/

// We define a local ASSERT macro to equal the global one if it exists.
// You can define XMA2DEFS_ASSERT in advance to override this default.
#ifndef XMA2DEFS_ASSERT
    #ifdef ASSERT
        #define XMA2DEFS_ASSERT ASSERT
    #else
        #define XMA2DEFS_ASSERT(a) /* No-op by default */
    #endif
#endif


// GetXmaBlockContainingSample: Use a given seek table to find the XMA block
// containing a given decoded sample.  Note that the seek table entries in an
// XMA file are stored in big-endian form and may need to be converted prior
// to calling this function.

__inline HRESULT GetXmaBlockContainingSample
(
    DWORD nBlockCount,                      // Blocks in the file (= seek table entries)
    __in_ecount(nBlockCount) const DWORD* pSeekTable,  // Pointer to the seek table data
    DWORD nDesiredSample,                   // Decoded sample to locate
    __out DWORD* pnBlockContainingSample,   // Index of the block containing the sample
    __out DWORD* pnSampleOffsetWithinBlock  // Position of the sample in this block
)
{
    DWORD nPreviousTotalSamples = 0;
    DWORD nBlock;
    DWORD nTotalSamplesSoFar;

    XMA2DEFS_ASSERT(pSeekTable);
    XMA2DEFS_ASSERT(pnBlockContainingSample);
    XMA2DEFS_ASSERT(pnSampleOffsetWithinBlock);

    for (nBlock = 0; nBlock < nBlockCount; ++nBlock)
    {
        nTotalSamplesSoFar = pSeekTable[nBlock];
        if (nTotalSamplesSoFar > nDesiredSample)
        {
            *pnBlockContainingSample = nBlock;
            *pnSampleOffsetWithinBlock = nDesiredSample - nPreviousTotalSamples;
            return S_OK;
        }
        nPreviousTotalSamples = nTotalSamplesSoFar;
    }

    return E_FAIL;
}


// GetXmaFrameLengthInBits: Reads a given frame's LengthInBits field.

__inline DWORD GetXmaFrameLengthInBits
(
    __in_bcount(nBitPosition / 8 + 3)
    __in const BYTE* pPacket,  // Pointer to XMA packet[s] containing the frame
    DWORD nBitPosition         // Bit offset of the frame within this packet
)
{
    DWORD nRegion;
    DWORD nBytePosition = nBitPosition / 8;
    DWORD nBitOffset = nBitPosition % 8;

    if (nBitOffset < 2) // Only need to read 2 bytes (and might not be safe to read more)
    {
        nRegion = (DWORD)(pPacket[nBytePosition+0]) << 8 |
                  (DWORD)(pPacket[nBytePosition+1]);
        return (nRegion >> (1 - nBitOffset)) & 0x7FFF;  // Last 15 bits
    }
    else // Need to read 3 bytes
    {
        nRegion = (DWORD)(pPacket[nBytePosition+0]) << 16 |
                  (DWORD)(pPacket[nBytePosition+1]) << 8 |
                  (DWORD)(pPacket[nBytePosition+2]);
        return (nRegion >> (9 - nBitOffset)) & 0x7FFF;  // Last 15 bits
    }
}


// GetXmaFrameBitPosition: Calculates the bit offset of a given frame within
// an XMA block or set of blocks.  Returns 0 on failure.

__inline DWORD GetXmaFrameBitPosition
(
    __in_bcount(nXmaDataBytes) const BYTE* pXmaData,  // Pointer to XMA block[s]
    DWORD nXmaDataBytes,                              // Size of pXmaData in bytes
    DWORD nStreamIndex,                               // Stream within which to seek
    DWORD nDesiredFrame                               // Frame sought
)
{
    const BYTE* pCurrentPacket;
    DWORD nPacketsExamined = 0;
    DWORD nFrameCountSoFar = 0;
    DWORD nFramesToSkip;
    DWORD nFrameBitOffset;

    XMA2DEFS_ASSERT(pXmaData);
    XMA2DEFS_ASSERT(nXmaDataBytes % XMA_BYTES_PER_PACKET == 0);

    // Get the first XMA packet belonging to the desired stream, relying on the
    // fact that the first packets for each stream are in consecutive order at
    // the beginning of an XMA block.

    pCurrentPacket = pXmaData + nStreamIndex * XMA_BYTES_PER_PACKET;
    for (;;)
    {
        // If we have exceeded the size of the XMA data, return failure
        if (pCurrentPacket + XMA_BYTES_PER_PACKET > pXmaData + nXmaDataBytes)
        {
            return 0;
        }

        // If the current packet contains the frame we are looking for...
        if (nFrameCountSoFar + GetXmaPacketFrameCount(pCurrentPacket) > nDesiredFrame)
        {
            // See how many frames in this packet we need to skip to get to it
            XMA2DEFS_ASSERT(nDesiredFrame >= nFrameCountSoFar);
            nFramesToSkip = nDesiredFrame - nFrameCountSoFar;

            // Get the bit offset of the first frame in this packet
            nFrameBitOffset = XMA_PACKET_HEADER_BITS + GetXmaPacketFirstFrameOffsetInBits(pCurrentPacket);

            // Advance nFrameBitOffset to the frame of interest
            while (nFramesToSkip--)
            {
                nFrameBitOffset += GetXmaFrameLengthInBits(pCurrentPacket, nFrameBitOffset);
            }

            // The bit offset to return is the number of bits from pXmaData to
            // pCurrentPacket plus the bit offset of the frame of interest
            return (DWORD)(pCurrentPacket - pXmaData) * 8 + nFrameBitOffset;
        }

        // If we haven't found the right packet yet, advance our counters
        ++nPacketsExamined;
        nFrameCountSoFar += GetXmaPacketFrameCount(pCurrentPacket);

        // And skip to the next packet belonging to the same stream
        pCurrentPacket += XMA_BYTES_PER_PACKET * (GetXmaPacketSkipCount(pCurrentPacket) + 1);
    }
}


// GetLastXmaFrameBitPosition: Calculates the bit offset of the last complete
// frame in an XMA block or set of blocks.

__inline DWORD GetLastXmaFrameBitPosition
(
    __in_bcount(nXmaDataBytes) const BYTE* pXmaData,  // Pointer to XMA block[s]
    DWORD nXmaDataBytes,                              // Size of pXmaData in bytes
    DWORD nStreamIndex                                // Stream within which to seek
)
{
    const BYTE* pLastPacket;
    DWORD nBytesToNextPacket;
    DWORD nFrameBitOffset;
    DWORD nFramesInLastPacket;

    XMA2DEFS_ASSERT(pXmaData);
    XMA2DEFS_ASSERT(nXmaDataBytes % XMA_BYTES_PER_PACKET == 0);
    XMA2DEFS_ASSERT(nXmaDataBytes >= XMA_BYTES_PER_PACKET * (nStreamIndex + 1));

    // Get the first XMA packet belonging to the desired stream, relying on the
    // fact that the first packets for each stream are in consecutive order at
    // the beginning of an XMA block.
    pLastPacket = pXmaData + nStreamIndex * XMA_BYTES_PER_PACKET;

    // Search for the last packet belonging to the desired stream
    for (;;)
    {
        nBytesToNextPacket = XMA_BYTES_PER_PACKET * (GetXmaPacketSkipCount(pLastPacket) + 1);
        XMA2DEFS_ASSERT(nBytesToNextPacket);
        if (pLastPacket + nBytesToNextPacket + XMA_BYTES_PER_PACKET > pXmaData + nXmaDataBytes)
        {
            break;  // The next packet would extend beyond the end of pXmaData
        }
        pLastPacket += nBytesToNextPacket;
    }

    // The last packet can sometimes have no seekable frames, in which case we
    // have to use the previous one
    if (GetXmaPacketFrameCount(pLastPacket) == 0)
    {
        pLastPacket -= nBytesToNextPacket;
    }

    // Found the last packet.  Get the bit offset of its first frame.
    nFrameBitOffset = XMA_PACKET_HEADER_BITS + GetXmaPacketFirstFrameOffsetInBits(pLastPacket);

    // Traverse frames until we reach the last one
    nFramesInLastPacket = GetXmaPacketFrameCount(pLastPacket);
    while (--nFramesInLastPacket)
    {
        nFrameBitOffset += GetXmaFrameLengthInBits(pLastPacket, nFrameBitOffset);
    }

    // The bit offset to return is the number of bits from pXmaData to
    // pLastPacket plus the offset of the last frame in this packet.
    return (DWORD)(pLastPacket - pXmaData) * 8 + nFrameBitOffset;
}


// GetXmaDecodePositionForSample: Obtains the information needed to make the
// decoder generate audio starting at a given sample position relative to the
// beginning of the given XMA block: the bit offset of the appropriate frame,
// and the right subframe within that frame.  This data can be passed directly
// to the XMAPlaybackSetDecodePosition function.

__inline HRESULT GetXmaDecodePositionForSample
(
    __in_bcount(nXmaDataBytes) const BYTE* pXmaData,  // Pointer to XMA block[s]
    DWORD nXmaDataBytes,                              // Size of pXmaData in bytes
    DWORD nStreamIndex,                               // Stream within which to seek
    DWORD nDesiredSample,                             // Sample sought
    __out DWORD* pnBitOffset,                         // Returns the bit offset within pXmaData of
                                                      // the frame containing the sample sought
    __out DWORD* pnSubFrame                           // Returns the subframe containing the sample
)
{
    DWORD nDesiredFrame = nDesiredSample / XMA_SAMPLES_PER_FRAME;
    DWORD nSubFrame = (nDesiredSample % XMA_SAMPLES_PER_FRAME) / XMA_SAMPLES_PER_SUBFRAME;
    DWORD nBitOffset = GetXmaFrameBitPosition(pXmaData, nXmaDataBytes, nStreamIndex, nDesiredFrame);

    XMA2DEFS_ASSERT(pnBitOffset);
    XMA2DEFS_ASSERT(pnSubFrame);

    if (nBitOffset)
    {
        *pnBitOffset = nBitOffset;
        *pnSubFrame = nSubFrame;
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


// GetXmaSampleRate: Obtains the legal XMA sample rate (24, 32, 44.1 or 48Khz)
// corresponding to a generic sample rate.

__inline DWORD GetXmaSampleRate(DWORD dwGeneralRate)
{
    DWORD dwXmaRate = 48000; // Default XMA rate for all rates above 44100Hz

    if (dwGeneralRate <= 24000)      dwXmaRate = 24000;
    else if (dwGeneralRate <= 32000) dwXmaRate = 32000;
    else if (dwGeneralRate <= 44100) dwXmaRate = 44100;

    return dwXmaRate;
}


// Functions to convert between WAVEFORMATEXTENSIBLE channel masks (combinations
// of the SPEAKER_xxx flags defined in audiodefs.h) and XMA channel masks (which
// are limited to eight possible speaker positions: left, right, center, low
// frequency, side left, side right, back left and back right).

__inline DWORD GetStandardChannelMaskFromXmaMask(BYTE bXmaMask)
{
    DWORD dwStandardMask = 0;

    if (bXmaMask & XMA_SPEAKER_LEFT)           dwStandardMask |= SPEAKER_FRONT_LEFT;
    if (bXmaMask & XMA_SPEAKER_RIGHT)          dwStandardMask |= SPEAKER_FRONT_RIGHT;
    if (bXmaMask & XMA_SPEAKER_CENTER)         dwStandardMask |= SPEAKER_FRONT_CENTER;
    if (bXmaMask & XMA_SPEAKER_LFE)            dwStandardMask |= SPEAKER_LOW_FREQUENCY;
    if (bXmaMask & XMA_SPEAKER_LEFT_SURROUND)  dwStandardMask |= SPEAKER_SIDE_LEFT;
    if (bXmaMask & XMA_SPEAKER_RIGHT_SURROUND) dwStandardMask |= SPEAKER_SIDE_RIGHT;
    if (bXmaMask & XMA_SPEAKER_LEFT_BACK)      dwStandardMask |= SPEAKER_BACK_LEFT;
    if (bXmaMask & XMA_SPEAKER_RIGHT_BACK)     dwStandardMask |= SPEAKER_BACK_RIGHT;

    return dwStandardMask;
}

__inline BYTE GetXmaChannelMaskFromStandardMask(DWORD dwStandardMask)
{
    BYTE bXmaMask = 0;

    if (dwStandardMask & SPEAKER_FRONT_LEFT)    bXmaMask |= XMA_SPEAKER_LEFT;
    if (dwStandardMask & SPEAKER_FRONT_RIGHT)   bXmaMask |= XMA_SPEAKER_RIGHT;
    if (dwStandardMask & SPEAKER_FRONT_CENTER)  bXmaMask |= XMA_SPEAKER_CENTER;
    if (dwStandardMask & SPEAKER_LOW_FREQUENCY) bXmaMask |= XMA_SPEAKER_LFE;
    if (dwStandardMask & SPEAKER_SIDE_LEFT)     bXmaMask |= XMA_SPEAKER_LEFT_SURROUND;
    if (dwStandardMask & SPEAKER_SIDE_RIGHT)    bXmaMask |= XMA_SPEAKER_RIGHT_SURROUND;
    if (dwStandardMask & SPEAKER_BACK_LEFT)     bXmaMask |= XMA_SPEAKER_LEFT_BACK;
    if (dwStandardMask & SPEAKER_BACK_RIGHT)    bXmaMask |= XMA_SPEAKER_RIGHT_BACK;

    return bXmaMask;
}


// LocalizeXma2Format: Modifies a XMA2WAVEFORMATEX structure in place to comply
// with the current platform's byte-ordering rules (little- or big-endian).

__inline HRESULT LocalizeXma2Format(__inout XMA2WAVEFORMATEX* pXma2Format)
{
    #define XMASWAP2BYTES(n) ((WORD)(((n) >> 8) | (((n) & 0xff) << 8)))
    #define XMASWAP4BYTES(n) ((DWORD)((n) >> 24 | (n) << 24 | ((n) & 0xff00) << 8 | ((n) & 0xff0000) >> 8))

    if (pXma2Format->wfx.wFormatTag == WAVE_FORMAT_XMA2)
    {
        return S_OK;
    }
    else if (XMASWAP2BYTES(pXma2Format->wfx.wFormatTag) == WAVE_FORMAT_XMA2)
    {
        pXma2Format->wfx.wFormatTag      = XMASWAP2BYTES(pXma2Format->wfx.wFormatTag);
        pXma2Format->wfx.nChannels       = XMASWAP2BYTES(pXma2Format->wfx.nChannels);
        pXma2Format->wfx.nSamplesPerSec  = XMASWAP4BYTES(pXma2Format->wfx.nSamplesPerSec);
        pXma2Format->wfx.nAvgBytesPerSec = XMASWAP4BYTES(pXma2Format->wfx.nAvgBytesPerSec);
        pXma2Format->wfx.nBlockAlign     = XMASWAP2BYTES(pXma2Format->wfx.nBlockAlign);
        pXma2Format->wfx.wBitsPerSample  = XMASWAP2BYTES(pXma2Format->wfx.wBitsPerSample);
        pXma2Format->wfx.cbSize          = XMASWAP2BYTES(pXma2Format->wfx.cbSize);
        pXma2Format->NumStreams          = XMASWAP2BYTES(pXma2Format->NumStreams);
        pXma2Format->ChannelMask         = XMASWAP4BYTES(pXma2Format->ChannelMask);
        pXma2Format->SamplesEncoded      = XMASWAP4BYTES(pXma2Format->SamplesEncoded);
        pXma2Format->BytesPerBlock       = XMASWAP4BYTES(pXma2Format->BytesPerBlock);
        pXma2Format->PlayBegin           = XMASWAP4BYTES(pXma2Format->PlayBegin);
        pXma2Format->PlayLength          = XMASWAP4BYTES(pXma2Format->PlayLength);
        pXma2Format->LoopBegin           = XMASWAP4BYTES(pXma2Format->LoopBegin);
        pXma2Format->LoopLength          = XMASWAP4BYTES(pXma2Format->LoopLength);
        pXma2Format->BlockCount          = XMASWAP2BYTES(pXma2Format->BlockCount);
        return S_OK;
    }
    else
    {
        return E_FAIL; // Not a recognizable XMA2 format
    }

    #undef XMASWAP2BYTES
    #undef XMASWAP4BYTES
}


#endif // #ifndef __XMA2DEFS_INCLUDED__
