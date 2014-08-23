#pragma once

// Error Codes
enum
{
	CELL_SAIL_ERROR_INVALID_ARG        = 0x80610701,
	CELL_SAIL_ERROR_INVALID_STATE      = 0x80610702,
	CELL_SAIL_ERROR_UNSUPPORTED_STREAM = 0x80610703,
	CELL_SAIL_ERROR_INDEX_OUT_OF_RANGE = 0x80610704,
	CELL_SAIL_ERROR_EMPTY              = 0x80610705,
	CELL_SAIL_ERROR_FULLED             = 0x80610706,
	CELL_SAIL_ERROR_USING              = 0x80610707,
	CELL_SAIL_ERROR_NOT_AVAILABLE      = 0x80610708,
	CELL_SAIL_ERROR_CANCEL             = 0x80610709,
	CELL_SAIL_ERROR_MEMORY             = 0x806107F0,
	CELL_SAIL_ERROR_INVALID_FD         = 0x806107F1,
	CELL_SAIL_ERROR_FATAL              = 0x806107FF,
};

struct CellSailAudioFormat
{
	s8 coding;
	s8 chNum;
	be_t<s16> sampleNum;
	be_t<s32> fs;
	be_t<s32> chLayout;
	be_t<s32> reserved0; // Specify both -1
	be_t<s64> reserved1;
};

struct CellSailAudioFrameInfo
{
	be_t<u32> pPcm;
	be_t<s32> status;
	be_t<u64> pts;
	be_t<u64> reserved; // Specify 0
};

struct CellSailVideoFormat
{
	s8 coding;
	s8 scan;
	s8 bitsPerColor;
	s8 frameRate;
	be_t<s16> width;
	be_t<s16> height;
	be_t<s32> pitch;
	be_t<s32> alpha;
	s8 colorMatrix;
	s8 aspectRatio;
	s8 colorRange;
	s8 reserved1; // Specify all three -1
	be_t<s32> reserved2;
	be_t<s64> reserved3;
};

struct CellSailVideoFrameInfo
{
	be_t<u32> pPic;
	be_t<s32> status;
	be_t<u64> pts;
	be_t<u64> reserved; // Specify both 0
	be_t<u16> interval;
	u8 structure;
	s8 repeatNum;
	u8 reserved2[4];
};

struct CellSailSourceBufferItem
{
	u8 pBuf;
	be_t<u32> size;
	be_t<u32> sessionId;
	be_t<u32> reserved; // Specify 0
};

struct CellSailSourceStartCommand
{
	be_t<u64> startFlags;
	be_t<s64> startArg;
	be_t<s64> lengthArg;
	be_t<u64> optionalArg0;
	be_t<u64> optionalArg1;
};

struct CellSailSourceStreamingProfile
{
	be_t<u32> reserved0; // Specify 0
	be_t<u32> numItems;
	be_t<u32> maxBitrate;
	be_t<u32> reserved1; // Specify 0
	be_t<u64> duration;
	be_t<u64> streamSize;
};

union CellSailEvent
{
	struct u32x2 {
		be_t<u32> major;
		be_t<u32> minor;
	};

	struct ui64 {
		be_t<u64> value;
	};
};

typedef void(*CellSailMemAllocatorFuncAlloc)(u32 pArg, u32 boundary, u32 size);
typedef void(*CellSailMemAllocatorFuncFree)(u32 pArg, u32 boundary, u32 pMemory);

typedef int(*CellSailSoundAdapterFuncMakeup)(u32 pArg);
typedef int(*CellSailSoundAdapterFuncCleanup)(u32 pArg);
typedef void(*CellSailSoundAdapterFuncFormatChanged)(u32 pArg, mem_ptr_t<CellSailAudioFormat> pFormat, u32 sessionId);

typedef int(*CellSailGraphicsAdapterFuncMakeup)(u32 pArg);
typedef int(*CellSailGraphicsAdapterFuncCleanup)(u32 pArg);
typedef void(*CellSailGraphicsAdapterFuncFormatChanged)(u32 pArg, mem_ptr_t<CellSailVideoFormat> pFormat, u32 sessionId);
typedef int(*CellSailGraphicsAdapterFuncAllocFrame)(u32 pArg, u32 size, s32 num, u8 ppFrame);
typedef int(*CellSailGraphicsAdapterFuncFreeFrame)(u32 pArg, s32 num, u8 ppFrame);

typedef int(*CellSailSourceFuncMakeup)(u32 pArg, s8 pProtocolNames);
typedef int(*CellSailSourceFuncCleanup)(u32 pArg);
typedef void(*CellSailSourceFuncOpen)(u32 pArg, s32 streamType, u32 pMediaInfo, s8 pUri, mem_ptr_t<CellSailSourceStreamingProfile> pProfile);
typedef void(*CellSailSourceFuncClose)(u32 pArg);
typedef void(*CellSailSourceFuncStart)(u32 pArg, mem_ptr_t<CellSailSourceStartCommand> pCommand, u32 sessionId);
typedef void(*CellSailSourceFuncStop)(u32 pArg);
typedef void(*CellSailSourceFuncCancel)(u32 pArg);
typedef int(*CellSailSourceFuncCheckout)(u32 pArg, mem_ptr_t<CellSailSourceBufferItem> ppItem);
typedef int(*CellSailSourceFuncCheckin)(u32 pArg, mem_ptr_t<CellSailSourceBufferItem> pItem);
typedef int(*CellSailSourceFuncClear)(u32 pArg);
typedef int(*CellSailSourceFuncRead)(u32 pArg, s32 streamType, u32 pMediaInfo, s8 pUri, u64 offset, u8 pBuf, u32 size, u64 pTotalSize);
typedef int(*CellSailSourceFuncReadSync)(u32 pArg, s32 streamType, u32 pMediaInfo, s8 pUri, u64 offset, u8 pBuf, u32 size, u64 pTotalSize);
typedef int(*CellSailSourceFuncGetCapabilities)(u32 pArg, s32 streamType, u32 pMediaInfo, s8 pUri, u64 pCapabilities);
typedef int(*CellSailSourceFuncInquireCapability)(u32 pArg, s32 streamType, u32 pMediaInfo, s8 pUri, mem_ptr_t<CellSailSourceStartCommand> pCommand);
typedef void(*CellSailSourceCheckFuncError)(u32 pArg, s8 pMsg, s32 line);

typedef int(*CellSailFsFuncOpen)(s8 pPath, s32 flag, s32 pFd, u32 pArg, u64 size);
typedef int(*CellSailFsFuncOpenSecond)(s8 pPath, s32 flag, s32 fd, u32 pArg, u64 size);
typedef int(*CellSailFsFuncClose)(s32 fd);
typedef int(*CellSailFsFuncFstat)(s32 fd, u32 pStat_addr);
typedef int(*CellSailFsFuncRead)(s32 fd, u32 pBuf, u64 numBytes, u64 pNumRead);
typedef int(*CellSailFsFuncLseek)(s32 fd, s64 offset, s32 whence, u64 pPosition);
typedef int(*CellSailFsFuncCancel)(s32 fd);

typedef int(*CellSailRendererAudioFuncMakeup)(u32 pArg);
typedef int(*CellSailRendererAudioFuncCleanup)(u32 pArg);
typedef void(*CellSailRendererAudioFuncOpen)(u32 pArg, mem_ptr_t<CellSailAudioFormat> pInfo, u32 frameNum);
typedef void(*CellSailRendererAudioFuncClose)(u32 pArg);
typedef void(*CellSailRendererAudioFuncStart)(u32 pArg, bool buffering);
typedef void(*CellSailRendererAudioFuncStop)(u32 pArg, bool flush);
typedef void(*CellSailRendererAudioFuncCancel)(u32 pArg);
typedef int(*CellSailRendererAudioFuncCheckout)(u32 pArg, mem_ptr_t<CellSailAudioFrameInfo> ppInfo);
typedef int(*CellSailRendererAudioFuncCheckin)(u32 pArg, mem_ptr_t<CellSailAudioFrameInfo> pInfo);

typedef int(*CellSailRendererVideoFuncMakeup)(u32 pArg);
typedef int(*CellSailRendererVideoFuncCleanup)(u32 pArg);
typedef void(*CellSailRendererVideoFuncOpen)(u32 pArg, mem_ptr_t<CellSailVideoFormat> pInfo, u32 frameNum, u32 minFrameNum);
typedef void(*CellSailRendererVideoFuncClose)(u32 pArg);
typedef void(*CellSailRendererVideoFuncStart)(u32 pArg, bool buffering);
typedef void(*CellSailRendererVideoFuncStop)(u32 pArg, bool flush, bool keepRendering);
typedef void(*CellSailRendererVideoFuncCancel)(u32 pArg);
typedef int(*CellSailRendererVideoFuncCheckout)(u32 pArg, mem_ptr_t<CellSailVideoFrameInfo> ppInfo);
typedef int(*CellSailRendererVideoFuncCheckin)(u32 pArg, mem_ptr_t<CellSailVideoFrameInfo> pInfo);

typedef void(*CellSailPlayerFuncNotified)(u32 pArg, mem_ptr_t<CellSailEvent> event, u64 arg0, u64 arg1);

struct CellSailMemAllocatorFuncs
{
	CellSailMemAllocatorFuncAlloc pAlloc;
	CellSailMemAllocatorFuncFree pFree;
};

struct CellSailMemAllocator
{
	CellSailMemAllocatorFuncs callbacks;
	be_t<u32> pArg;
};

struct CellSailFuture
{
	u32 mutex_id;
	u32 cond_id;
	volatile be_t<u32> flags;
	be_t<s32> result;
	be_t<u64> userParam;
};

struct CellSailSoundAdapterFuncs
{
	CellSailSoundAdapterFuncMakeup pMakeup;
	CellSailSoundAdapterFuncCleanup pCleanup;
	CellSailSoundAdapterFuncFormatChanged pFormatChanged;
};

struct CellSailSoundFrameInfo
{
	be_t<u32> pBuffer;
	be_t<u32> sessionId;
	be_t<u32> tag;
	be_t<s32> status;
	be_t<u64> pts;
};

struct CellSailSoundAdapter
{
	be_t<u64> internalData[32];
};

struct CellSailGraphicsAdapterFuncs
{
	CellSailGraphicsAdapterFuncMakeup pMakeup;
	CellSailGraphicsAdapterFuncCleanup pCleanup;
	CellSailGraphicsAdapterFuncFormatChanged pFormatChanged;
	CellSailGraphicsAdapterFuncAllocFrame pAlloc;
	CellSailGraphicsAdapterFuncFreeFrame pFree;
};

struct CellSailGraphicsFrameInfo
{
	be_t<u32> pBuffer;
	be_t<u32> sessionId;
	be_t<u32> tag;
	be_t<s32> status;
	be_t<u64> pts;
};

struct CellSailGraphicsAdapter
{
	be_t<u64> internalData[32];
};

struct CellSailAuInfo
{
	be_t<u32> pAu;
	be_t<u32> size;
	be_t<s32> status;
	be_t<u32> sessionId;
	be_t<u64> pts;
	be_t<u64> dts;
	be_t<u64> reserved; // Specify 0
};

struct CellSailAuReceiver
{
	be_t<u64> internalData[64];
};

struct CellSailRendererAudioFuncs
{
	CellSailRendererAudioFuncMakeup pMakeup;
	CellSailRendererAudioFuncCleanup pCleanup;
	CellSailRendererAudioFuncOpen pOpen;
	CellSailRendererAudioFuncClose pClose;
	CellSailRendererAudioFuncStart pStart;
	CellSailRendererAudioFuncStop pStop;
	CellSailRendererAudioFuncCancel pCancel;
	CellSailRendererAudioFuncCheckout pCheckout;
	CellSailRendererAudioFuncCheckin pCheckin;
};

struct CellSailRendererAudioAttribute
{
	be_t<u32> thisSize;
	CellSailAudioFormat pPreferredFormat;
};

struct CellSailRendererAudio
{
	be_t<u64> internalData[32];
};

struct CellSailRendererVideoFuncs
{
	CellSailRendererVideoFuncMakeup pMakeup;
	CellSailRendererVideoFuncCleanup pCleanup;
	CellSailRendererVideoFuncOpen pOpen;
	CellSailRendererVideoFuncClose pClose;
	CellSailRendererVideoFuncStart pStart;
	CellSailRendererVideoFuncStop pStop;
	CellSailRendererVideoFuncCancel pCancel;
	CellSailRendererVideoFuncCheckout pCheckout;
	CellSailRendererVideoFuncCheckin pCheckin;
};

struct CellSailRendererVideoAttribute
{
	be_t<u32> thisSize;
	CellSailVideoFormat *pPreferredFormat;
};

struct CellSailRendererVideo
{
	be_t<u64> internalData[32];
};

struct CellSailSourceFuncs
{
	CellSailSourceFuncMakeup pMakeup;
	CellSailSourceFuncCleanup pCleanup;
	CellSailSourceFuncOpen pOpen;
	CellSailSourceFuncClose pClose;
	CellSailSourceFuncStart pStart;
	CellSailSourceFuncStop pStop;
	CellSailSourceFuncCancel pCancel;
	CellSailSourceFuncCheckout pCheckout;
	CellSailSourceFuncCheckin pCheckin;
	CellSailSourceFuncClear pClear;
	CellSailSourceFuncRead pRead;
	CellSailSourceFuncReadSync pReadSync;
	CellSailSourceFuncGetCapabilities pGetCapabilities;
	CellSailSourceFuncInquireCapability pInquireCapability;
};

struct CellSailSource
{
	be_t<u64> internalData[20];
};

struct CellSailSourceCheckStream
{
	be_t<s32> streamType;
	be_t<u32> pMediaInfo;
	s8 pUri;
};

struct CellSailSourceCheckResource
{
	CellSailSourceCheckStream ok;
	CellSailSourceCheckStream readError;
	CellSailSourceCheckStream openError;
	CellSailSourceCheckStream startError;
	CellSailSourceCheckStream runningError;
};

struct CellSailMp4DateTime
{
	be_t<u16> second;
	be_t<u16> minute;
	be_t<u16> hour;
	be_t<u16> day;
	be_t<u16> month;
	be_t<u16> year;
	//be_t<u16> reserved0;
	//be_t<u16> reserved1;
};

struct CellSailMp4Movie
{
	be_t<u64> internalData[16];
};

struct CellSailMp4MovieInfo
{
	CellSailMp4DateTime creationDateTime;
	CellSailMp4DateTime modificationDateTime;
	be_t<u32> trackCount;
	be_t<u32> movieTimeScale;
	be_t<u32> movieDuration;
	//be_t<u32> reserved[16];
};

struct CellSailMp4Track
{
	be_t<u64> internalData[6];
};

struct CellSailMp4TrackInfo
{
	bool isTrackEnabled;
	u8 reserved0[3];
	be_t<u32> trackId;
	be_t<u64> trackDuration;
	be_t<s16> layer;
	be_t<s16> alternateGroup;
	be_t<u16> reserved1[2];
	be_t<u32> trackWidth;
	be_t<u32> trackHeight;
	be_t<u16> language;
	be_t<u16> reserved2;
	be_t<u16> mediaType;
	//be_t<u32> reserved3[3];
};

struct CellSailAviMovie
{
	be_t<u64> internalData[16];
};

struct CellSailAviMovieInfo
{
	be_t<u32> maxBytesPerSec;
	be_t<u32> flags;
	be_t<u32> reserved0;
	be_t<u32> streams;
	be_t<u32> suggestedBufferSize;
	be_t<u32> width;
	be_t<u32> height;
	be_t<u32> scale;
	be_t<u32> rate;
	be_t<u32> length;
	//be_t<u32> reserved1;
	//be_t<u32> reserved2;
};

struct CellSailAviMainHeader
{
	be_t<u32> microSecPerFrame;
	be_t<u32> maxBytesPerSec;
	be_t<u32> paddingGranularity;
	be_t<u32> flags;
	be_t<u32> totalFrames;
	be_t<u32> initialFrames;
	be_t<u32> streams;
	be_t<u32> suggestedBufferSize;
	be_t<u32> width;
	be_t<u32> height;
	//be_t<u32> reserved[4];
};

struct CellSailAviExtendedHeader
{
	be_t<u32> totalFrames;
};

struct CellSailAviStream
{
	be_t<u64> internalData[2];
};

struct CellSailAviMediaType
{
	be_t<u32> fccType;
	be_t<u32> fccHandler;
	union u {
		struct audio {
			be_t<u16> formatTag;
			be_t<u16> reserved; // Specify 0 
			union u {
				struct mpeg {
					be_t<u16> headLayer; // Specify 0
					be_t<u16> reserved; // Specify 0
				};
			};
		};
		struct video {
			be_t<u32> compression;
			be_t<u32> reserved;  // Specify 0
		};
	};
};

struct CellSailAviStreamHeader
{
	be_t<u32> fccType;
	be_t<u32> fccHandler;
	be_t<u32> flags;
	be_t<u16> priority;
	be_t<u32> initialFrames;
	be_t<u32> scale;
	be_t<u32> rate;
	be_t<u32> start;
	be_t<u32> length;
	be_t<u32> suggestedBufferSize;
	be_t<u32> quality;
	be_t<u32> sampleSize;
	struct frame {
		be_t<u16> left;
		be_t<u16> top;
		be_t<u16> right;
		be_t<u16> bottom;
	};
};

struct CellSailBitmapInfoHeader
{
	be_t<u32> size;
	be_t<s32> width;
	be_t<s32> height;
	be_t<u16> planes;
	be_t<u16> bitCount;
	be_t<u32> compression;
	be_t<u32> sizeImage;
	be_t<s32> xPelsPerMeter;
	be_t<s32> yPelsPerMeter;
	be_t<u32> clrUsed;
	be_t<u32> clrImportant;
};

struct CellSailWaveFormatEx
{
	be_t<u16> formatTag;
	be_t<u16> channels;
	be_t<u32> samplesPerSec;
	be_t<u32> avgBytesPerSec;
	be_t<u16> blockAlign;
	be_t<u16> bitsPerSample;
	be_t<u16> cbSize;
};

struct CellSailMpeg1WaveFormat
{
	CellSailWaveFormatEx wfx;
	be_t<u16> headLayer;
	be_t<u32> headBitrate;
	be_t<u16> headMode;
	be_t<u16> headModeExt;
	be_t<u16> headEmphasis;
	be_t<u16> headFlags;
	be_t<u32> PTSLow;
	be_t<u32> PTSHigh;
};

struct CellSailMpegLayer3WaveFormat
{
	CellSailWaveFormatEx wfx;
	be_t<u16> ID;
	be_t<u32> flags;
	be_t<u16> blockSize;
	be_t<u16> framesPerBlock;
	be_t<u16> codecDelay;
};

struct CellSailDescriptor
{
	be_t<u64> internalData[32];
};

struct CellSailStartCommand
{
	be_t<u32> startType;
	be_t<u32> seekType;
	be_t<u32> terminusType;
	be_t<u32> flags;
	be_t<u32> startArg;
	be_t<u32> reserved;
	be_t<u64> seekArg;
	be_t<u64> terminusArg;
};

struct CellSailFsReadFuncs
{
	CellSailFsFuncOpen pOpen;
	CellSailFsFuncOpenSecond pOpenSecond;
	CellSailFsFuncClose pClose;
	CellSailFsFuncFstat pFstat;
	CellSailFsFuncRead pRead;
	CellSailFsFuncLseek pLseek;
	CellSailFsFuncCancel pCancel;
	be_t<u32> reserved[2]; // Specify 0
};

struct CellSailFsRead
{
	be_t<u32> capability;
	CellSailFsReadFuncs funcs;
};

struct CellSailPlayerAttribute
{
	be_t<s32> preset;
	be_t<u32> maxAudioStreamNum;
	be_t<u32> maxVideoStreamNum;
	be_t<u32> maxUserStreamNum;
	be_t<u32> queueDepth;
	be_t<u32> reserved0; // All three specify 0
	be_t<u32> reserved1;
	be_t<u32> reserved2;
};

struct CellSailPlayerResource
{
	be_t<u32> pSpurs;
	be_t<u32> reserved0; // All three specify 0
	be_t<u32> reserved1;
	be_t<u32> reserved2;
};

struct CellSailPlayer
{
	be_t<u64> internalData[128];
};