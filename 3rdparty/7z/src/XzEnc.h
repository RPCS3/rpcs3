/* ZstdDec.h -- Zstd Decoder interfaces
2024-01-21 : Igor Pavlov : Public domain */

#ifndef ZIP7_INC_ZSTD_DEC_H
#define ZIP7_INC_ZSTD_DEC_H

EXTERN_C_BEGIN

typedef struct CZstdDec CZstdDec;
typedef CZstdDec * CZstdDecHandle;

CZstdDecHandle ZstdDec_Create(ISzAllocPtr alloc_Small, ISzAllocPtr alloc_Big);
void ZstdDec_Destroy(CZstdDecHandle p);

typedef enum
{
  ZSTD_STATUS_NOT_SPECIFIED,    /* use main error code instead */
  ZSTD_STATUS_FINISHED_FRAME,   /* data frame or skip frame was finished */
  ZSTD_STATUS_NOT_FINISHED,     /* just finished non-empty block or unfinished RAW/RLE block */
  ZSTD_STATUS_NEEDS_MORE_INPUT, /* the callee needs more input bytes. It has more priority over ZSTD_STATUS_NOT_FINISHED */
  ZSTD_STATUS_OUT_REACHED       /* is not finihed frame and ((outProcessed > outSize) || (outProcessed == outSize && unfinished RAW/RLE block) */
} enum_ZstdStatus_Dummy;

#define ZstdDecState_DOES_NEED_MORE_INPUT_OR_FINISHED_FRAME(p) \
    ((p)->status & ZSTD_STATUS_FINISHED_FRAME)
/*
    ((p)->status == ZSTD_STATUS_NEEDS_MORE_INPUT || \
     (p)->status == ZSTD_STATUS_FINISHED_FRAME)
*/

typedef Byte enum_ZstdStatus;


void ZstdDec_Init(CZstdDecHandle p);

typedef struct
{
  UInt64 num_Blocks;
  Byte descriptor_OR;
  Byte descriptor_NOT_OR;
  Byte are_ContentSize_Unknown;
  Byte windowDescriptor_MAX;

  // Byte are_ContentSize_Known;
  // Byte are_SingleSegments;
  // Byte are_WindowDescriptors;
  Byte checksum_Defined;
  // Byte are_Checksums;
  // Byte are_Non_Checksums;
  
  // Byte are_DictionaryId;
  Byte are_DictionaryId_Different;
  
  // Byte reserved[3];

  UInt32 checksum;        // checksum of last data frame
  /// UInt32 dictionaryId_Cur;
  UInt32 dictionaryId; // if there are non-zero dictionary IDs, then it's first dictionaryId
  
  UInt64 num_DataFrames;
  UInt64 num_SkipFrames;
  UInt64 skipFrames_Size;
  UInt64 contentSize_Total;
  UInt64 contentSize_MAX;
  // UInt64 num_Checksums;
  // UInt64 num_Non_Checksums; // frames without checksum
  // UInt64 num_WindowDescriptors;
  // UInt64 num_SingleSegments;
  // UInt64 num_Frames_with_ContentSize;
  // UInt64 num_Frames_without_ContentSize;
  UInt64 windowSize_MAX;
  UInt64 windowSize_Allocate_MAX;
  // UInt64 num_DictionaryIds;
  // UInt64 num_Blocks_forType[4];
  // UInt64 num_BlockBytes_forType[4];
  // UInt64 num_SingleSegments;
  // UInt64 singleSegment_ContentSize_MAX;
} CZstdDecInfo;

#define ZstdDecInfo_CLEAR(p)  { memset(p, 0, sizeof(*(p))); }

#define ZstdDecInfo_GET_NUM_FRAMES(p)  ((p)->num_DataFrames + (p)->num_SkipFrames)


typedef struct CZstdDecState
{
  enum_ZstdStatus status; // out
  Byte disableHash;
  // Byte mustBeFinished;
  Byte outSize_Defined;
  // Byte isAfterSizeMode;
  // UInt64 inProcessed;
  // SRes codeRes;
  // Byte needWrite_IsStrong;

  const Byte *inBuf;
  size_t inPos;           // in/out
  size_t inLim;

  const Byte *win;        // out
  size_t winPos;          // out
  size_t wrPos;           // in/out
  // size_t cycSize;      // out : if (!outBuf_fromCaller)
  size_t needWrite_Size;  // out

  Byte *outBuf_fromCaller;
  size_t outBufSize_fromCaller;
  /* (outBufSize_fromCaller >= full_uncompressed_size_of_all_frames) is required
     for success decoding.
     If outBufSize_fromCaller < full_uncompressed_size_of_all_frames),
     decoding can give error message, because we decode per block basis.
  */

  // size_t outStep;
  UInt64 outSize;         // total in all frames
  UInt64 outProcessed;    // out decoded in all frames (it can be >= outSize)

  CZstdDecInfo info;
} CZstdDecState;

void ZstdDecState_Clear(CZstdDecState *p);

/*
ZstdDec_Decode()
return:
  SZ_OK                 - no error
  SZ_ERROR_DATA         - Data Error
  SZ_ERROR_MEM          - Memory allocation error
  SZ_ERROR_UNSUPPORTED  - Unsupported method or method properties
  SZ_ERROR_CRC          - XXH hash Error
  // SZ_ERROR_ARCHIVE   - Headers error (not used now)
*/
SRes ZstdDec_Decode(CZstdDecHandle dec, CZstdDecState *p);

/*
ZstdDec_ReadUnusedFromInBuf():
returns: the number of bytes that were read from InBuf
(*afterDecoding_tempPos) must be set to zero before first call of ZstdDec_ReadUnusedFromInBuf()
*/
size_t ZstdDec_ReadUnusedFromInBuf(
    CZstdDecHandle dec,
    size_t afterDecoding_tempPos, // in/out
    void *data, size_t size);

typedef struct
{
  SRes decode_SRes;   // error code of data decoding
  Byte is_NonFinishedFrame;  // there is unfinished decoding for data frame or skip frame
  Byte extraSize;
} CZstdDecResInfo;

/*
#define ZstdDecResInfo_CLEAR(p) \
{ (p)->decode_SRes = 0; \
  (p)->is_NonFinishedFrame; \
  (p)->extraSize = 0; \
}
// memset(p, 0, sizeof(*p));
*/

/*
additional error codes for CZstdDecResInfo::decode_SRes:
  SZ_ERROR_NO_ARCHIVE - is not zstd stream (no frames)
  SZ_ERROR_INPUT_EOF  - need more data in input stream
*/
void ZstdDec_GetResInfo(const CZstdDec *dec,
    const CZstdDecState *p,
    SRes res, // it's result from ZstdDec_Decode()
    CZstdDecResInfo *info);

EXTERN_C_END

#endif
