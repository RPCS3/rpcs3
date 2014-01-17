#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellPamf_init();
Module cellPamf(0x0012, cellPamf_init);

// Error Codes
enum
{
	CELL_PAMF_ERROR_STREAM_NOT_FOUND	= 0x80610501,
	CELL_PAMF_ERROR_INVALID_PAMF		= 0x80610502,
	CELL_PAMF_ERROR_INVALID_ARG			= 0x80610503,
	CELL_PAMF_ERROR_UNKNOWN_TYPE		= 0x80610504,
	CELL_PAMF_ERROR_UNSUPPORTED_VERSION	= 0x80610505,
	CELL_PAMF_ERROR_UNKNOWN_STREAM		= 0x80610506,
	CELL_PAMF_ERROR_EP_NOT_FOUND		= 0x80610507,
};

// PamfReaderInitialize Attribute Flags
enum
{
	CELL_PAMF_ATTRIBUTE_VERIFY_ON = 1,
	CELL_PAMF_ATTRIBUTE_MINIMUM_HEADER = 2,
};

typedef enum {
	CELL_PAMF_STREAM_TYPE_AVC = 0,
	CELL_PAMF_STREAM_TYPE_M2V = 1,
	CELL_PAMF_STREAM_TYPE_ATRAC3PLUS = 2,
	CELL_PAMF_STREAM_TYPE_PAMF_LPCM = 3,
	CELL_PAMF_STREAM_TYPE_AC3 = 4,
	CELL_PAMF_STREAM_TYPE_USER_DATA = 5,
	CELL_PAMF_STREAM_TYPE_VIDEO = 20,
	CELL_PAMF_STREAM_TYPE_AUDIO = 21,
} CellPamfStreamType;

typedef enum {
	CELL_PAMF_FS_48kHz = 1,
};

typedef enum {
	CELL_PAMF_BIT_LENGTH_16 = 1,
	CELL_PAMF_BIT_LENGTH_24 = 3,
};

typedef enum {
	CELL_PAMF_AVC_FRC_24000DIV1001 = 0,
	CELL_PAMF_AVC_FRC_24 = 1,
	CELL_PAMF_AVC_FRC_25 = 2,
	CELL_PAMF_AVC_FRC_30000DIV1001 = 3,
	CELL_PAMF_AVC_FRC_30 = 4,
	CELL_PAMF_AVC_FRC_50 = 5,
	CELL_PAMF_AVC_FRC_60000DIV1001 = 6,
};

// Timestamp information (time in increments of 90 kHz)
struct CellCodecTimeStamp {
	be_t<u32> upper;
	be_t<u32> lower;
};

// Entry point information
struct CellPamfEp {
	be_t<u32> indexN;
	be_t<u32> nThRefPictureOffset;
	CellCodecTimeStamp pts;
	be_t<u64> rpnOffset;
};

// Entry point iterator
struct CellPamfEpIterator {
	be_t<bool> isPamf;
	be_t<u32> index;
	be_t<u32> num;
	be_t<u32> pCur_addr;
};

struct CellCodecEsFilterId {
	be_t<u32> filterIdMajor;
	be_t<u32> filterIdMinor;
	be_t<u32> supplementalInfo1;
	be_t<u32> supplementalInfo2;
};

// AVC (MPEG4 AVC Video) Specific Information
struct CellPamfAvcInfo {
	u8 profileIdc;
	u8 levelIdc;
	u8 frameMbsOnlyFlag;
	u8 videoSignalInfoFlag;
	u8 frameRateInfo;
	u8 aspectRatioIdc;
	be_t<u16> sarWidth; //reserved
	be_t<u16> sarHeight; //reserved
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	be_t<u16> frameCropLeftOffset; //reserved
	be_t<u16> frameCropRightOffset; //reserved
	be_t<u16> frameCropTopOffset; //reserved
	be_t<u16> frameCropBottomOffset; //!!!!!
	u8 videoFormat; //reserved
	u8 videoFullRangeFlag;
	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;
	u8 entropyCodingModeFlag; //reserved
	u8 deblockingFilterFlag;
	u8 minNumSlicePerPictureIdc; //reserved
	u8 nfwIdc; //reserved
	u8 maxMeanBitrate; //reserved
};

// M2V (MPEG2 Video) Specific Information
struct CellPamfM2vInfo {
	u8 profileAndLevelIndication;
	be_t<bool> progressiveSequence;
	u8 videoSignalInfoFlag;
	u8 frameRateInfo;
	u8 aspectRatioIdc;
	be_t<u16> sarWidth;
	be_t<u16> sarHeight;
	be_t<u16> horizontalSize;
	be_t<u16> verticalSize;
	be_t<u16> horizontalSizeValue;
	be_t<u16> verticalSizeValue;
	u8 videoFormat;
	u8 videoFullRangeFlag;
	u8 colourPrimaries;
	u8 transferCharacteristics;
	u8 matrixCoefficients;
};

// LPCM Audio Specific Information
struct CellPamfLpcmInfo {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
	be_t<u16> bitsPerSample;
};

// ATRAC3+ Audio Specific Information
struct CellPamfAtrac3plusInfo {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

// AC3 Audio Specific Information
struct CellPamfAc3Info {
	be_t<u32> samplingFrequency;
	u8 numberOfChannels;
};

#pragma pack(push, 1) //file data

struct PamfStreamHeader_AVC { //AVC specific information
	u8 profileIdc;
	u8 levelIdc;
	u8 unk0;
	u8 unk1; //1
	u32 unk2; //0
	be_t<u16> horizontalSize; //divided by 16
	be_t<u16> verticalSize; //divided by 16
	u32 unk3; //0
	u32 unk4; //0
	u8 unk5; //0xA0
	u8 unk6; //1
	u8 unk7; //1
	u8 unk8; //1
	u8 unk9; //0xB0
	u8 unk10;
	u16 unk11; //0
	u32 unk12; //0
};

struct PamfStreamHeader_M2V { //M2V specific information
	u8 unknown[32]; //no information yet
};

struct PamfStreamHeader_Audio { //Audio specific information
	u16 unknown; //== 0
	u8 channels; //number of channels (1, 2, 6 or 8)
	u8 freq; //== 1 (always 48000)
	u8 bps; //(LPCM only, 0x40 for 16 bit, ???? for 24)
	u8 reserved[27]; //probably nothing
};

struct PamfStreamHeader //48 bytes
{
	//TODO: look for correct beginning of stream header
	u8 type; //0x1B for video (AVC), 0xDC ATRAC3+, 0x80 LPCM, 0xDD userdata
	u8 unknown[3]; //0
	//TODO: examine stream_ch encoding
	u8 stream_id;
	u8 private_stream_id;
	u8 unknown1; //?????
	u8 unknown2; //?????
	//Entry Point Info
	be_t<u32> ep_offset; //offset of EP section in header
	be_t<u32> ep_num; //count of EPs
	//Specific Info
	u8 data[32];
};

struct PamfHeader
{
	u32 magic; //"PAMF"
	u32 version; //"0041" (is it const?)
	be_t<u32> data_offset; //== 2048 >> 11, PAMF headers seem to be always 2048 bytes in size
	be_t<u32> data_size; //== ((fileSize - 2048) >> 11)
	u64 reserved[8];
	be_t<u32> table_size; //== size of mapping-table
	u16 reserved1;
	be_t<u16> start_pts_high;
	be_t<u32> start_pts_low; //Presentation Time Stamp (start)
	be_t<u16> end_pts_high;
	be_t<u32> end_pts_low; //Presentation Time Stamp (end)
	be_t<u32> mux_rate_max; //== 0x01D470 (400 bps per unit, == 48000000 bps)
	be_t<u32> mux_rate_min; //== 0x0107AC (?????)
	u16 reserved2; // ?????
	u8 reserved3;
	u8 stream_count; //total stream count (reduced to 1 byte)
	be_t<u16> unk1; //== 1 (?????)
	be_t<u32> table_data_size; //== table_size - 0x20 == 0x14 + (0x30 * total_stream_num) (?????)
	//TODO: check relative offset of stream structs (could be from 0x0c to 0x14, currently 0x14)
	be_t<u16> start_pts_high2; //????? (probably same values)
	be_t<u32> start_pts_low2; //?????
	be_t<u16> end_pts_high2; //?????
	be_t<u32> end_pts_low2; //?????
	be_t<u32> unk2; //== 0x10000 (?????)
	be_t<u16> unk3; // ?????
	be_t<u16> unk4; // == stream_count
	//==========================
	PamfStreamHeader stream_headers[256];
};

struct PamfEpHeader { //12 bytes
	be_t<u16> value0; //mixed indexN (probably left 2 bits) and nThRefPictureOffset
	be_t<u16> pts_high;
	be_t<u32> pts_low;
	be_t<u32> rpnOffset;
};

#pragma pack(pop)

struct CellPamfReader
{
	//this struct can be used in any way, if it is not accessed directly by virtual CPU
	//be_t<u64> internalData[16];
	u32 pAddr;
	int stream;
	u64 fileSize;
	u32 internalData[28];
};

int pamfStreamTypeToEsFilterId(u8 type, u8 ch, mem_ptr_t<CellCodecEsFilterId> pEsFilterId)
{
	//TODO: convert type and ch to EsFilterId
	pEsFilterId->filterIdMajor = 0;
	pEsFilterId->filterIdMinor = 0;
	pEsFilterId->supplementalInfo1 = 0;
	pEsFilterId->supplementalInfo2 = 0;

	switch (type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
		switch (ch)
		{
		case 0:
			{
				pEsFilterId->filterIdMajor = 0xe0; //fake info
				pEsFilterId->filterIdMinor = 0;
				pEsFilterId->supplementalInfo1 = 0x01;
				pEsFilterId->supplementalInfo2 = 0;
			}
			break;
		case 1:
			{
				pEsFilterId->filterIdMajor = 0xe1;
				pEsFilterId->filterIdMinor = 0;
				pEsFilterId->supplementalInfo1 = 0x01;
				pEsFilterId->supplementalInfo2 = 0;
			}
			break;
		default:
			cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_AVC (ch=%d)", ch);
		}
		break;
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS:
		if (ch == 0)
		{
			pEsFilterId->filterIdMajor = 0xbd;
			pEsFilterId->filterIdMinor = 0;
			pEsFilterId->supplementalInfo1 = 0;
			pEsFilterId->supplementalInfo2 = 0;
		}
		else
			cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_ATRAC3PLUS (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
		if (ch == 0)
		{
			pEsFilterId->filterIdMajor = 0xbd;
			pEsFilterId->filterIdMinor = 0x40;
			pEsFilterId->supplementalInfo1 = 0;
			pEsFilterId->supplementalInfo2 = 0;
		}
		else
			cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_LPCM (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_USER_DATA:
		if (ch == 0)
		{
			pEsFilterId->filterIdMajor = 0xbd;
			pEsFilterId->filterIdMinor = 0x20;
			pEsFilterId->supplementalInfo1 = 0;
			pEsFilterId->supplementalInfo2 = 0;
		}
		else
			cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_USER_DATA (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_AC3:
		cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_AC3 (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_M2V:
		cellPamf.Error("*** TODO: pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_M2V (ch=%d)", ch);
		break;
	default:
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	return CELL_OK;
}

u8 pamfGetStreamType(mem_ptr_t<CellPamfReader> pSelf, u8 stream)
{
	//TODO: get stream type correctly
	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	switch (pAddr->stream_headers[stream].type)
	{
	case 0x1b: return CELL_PAMF_STREAM_TYPE_AVC;
	case 0xdc: return CELL_PAMF_STREAM_TYPE_ATRAC3PLUS;
	case 0x80: return CELL_PAMF_STREAM_TYPE_PAMF_LPCM;
	case 0xdd: return CELL_PAMF_STREAM_TYPE_USER_DATA;
	default:
		cellPamf.Error("pamfGetStreamType: unsupported stream type found(0x%x)",
			pAddr->stream_headers[stream].type);
		return 0;
	}
}

u8 pamfGetStreamChannel(mem_ptr_t<CellPamfReader> pSelf, u8 stream)
{
	cellPamf.Error("TODO: pamfGetStreamChannel");
	//TODO: get stream channel correctly
	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	if ((pAddr->stream_headers[stream].type == 0x1b) &&
		(pAddr->stream_headers[stream].stream_id == 0xe1)) return 1;
	return 0;
}

int cellPamfGetHeaderSize(mem_ptr_t<PamfHeader> pAddr, u64 fileSize, mem64_t pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize(pAddr=0x%x, fileSize=%d, pSize_addr=0x%x)",
		pAddr.GetAddr(), fileSize, pSize.GetAddr());

	const u64 offset = (u64)pAddr->data_offset << 11;
	pSize = offset /*? offset : 2048*/; //hack
	return CELL_OK;
}

int cellPamfGetHeaderSize2(mem_ptr_t<PamfHeader> pAddr, u64 fileSize, u32 attribute, mem64_t pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize2(pAddr=0x%x, fileSize=%d, attribute=0x%x, pSize_addr=0x%x)",
		pAddr.GetAddr(), fileSize, attribute, pSize.GetAddr());

	const u64 offset = (u64)pAddr->data_offset << 11;
	pSize = offset /*? offset : 2048*/; //hack
	return CELL_OK;
}

int cellPamfGetStreamOffsetAndSize(mem_ptr_t<PamfHeader> pAddr, u64 fileSize, mem64_t pOffset, mem64_t pSize)
{
	cellPamf.Warning("cellPamfGetStreamOffsetAndSize(pAddr=0x%x, fileSize=%d, pOffset_addr=0x%x, pSize_addr=0x%x)",
		pAddr.GetAddr(), fileSize, pOffset.GetAddr(), pSize.GetAddr());

	const u64 offset = (u64)pAddr->data_offset << 11;
	pOffset = offset /*? offset : 2048*/; //hack
	const u64 size = (u64)pAddr->data_size << 11;
	pSize = size /*? size : 4096*/; //hack
	return CELL_OK;
}

int cellPamfVerify(mem_ptr_t<PamfHeader> pAddr, u64 fileSize)
{
	cellPamf.Warning("cellPamfVerify(pAddr=0x%x, fileSize=%d)", pAddr.GetAddr(), fileSize);
	return CELL_OK;
}

int cellPamfReaderInitialize(mem_ptr_t<CellPamfReader> pSelf, mem_ptr_t<PamfHeader> pAddr, u64 fileSize, u32 attribute)
{
	cellPamf.Warning("cellPamfReaderInitialize(pSelf=0x%x, pAddr=0x%x, fileSize=%d, attribute=0x%x)",
		pSelf.GetAddr(), pAddr.GetAddr(), fileSize, attribute);

	if (fileSize)
	{
		pSelf->fileSize = fileSize;
	}
	else //if fileSize is unknown
	{
		pSelf->fileSize = ((u64)pAddr->data_offset << 11) + ((u64)pAddr->data_size << 11);
	}
	pSelf->pAddr = pAddr.GetAddr();

	if (attribute & CELL_PAMF_ATTRIBUTE_VERIFY_ON)
	{
		//TODO
	}

	pSelf->stream = 0; //??? currently set stream
	return CELL_OK;
}

int cellPamfReaderGetPresentationStartTime(mem_ptr_t<CellPamfReader> pSelf, mem_ptr_t<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.Warning("cellPamfReaderGetPresentationStartTime(pSelf=0x%x, pTimeStamp_addr=0x%x)",
		pSelf.GetAddr(), pTimeStamp.GetAddr());

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);
	const u32 upper = (u16)pAddr->start_pts_high;
	pTimeStamp->upper = upper;
	pTimeStamp->lower = pAddr->start_pts_low;
	return CELL_OK;
}

int cellPamfReaderGetPresentationEndTime(mem_ptr_t<CellPamfReader> pSelf, mem_ptr_t<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.Warning("cellPamfReaderGetPresentationEndTime(pSelf=0x%x, pTimeStamp_addr=0x%x)",
		pSelf.GetAddr(), pTimeStamp.GetAddr());

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);
	const u32 upper = (u16)pAddr->end_pts_high;
	pTimeStamp->upper = upper;
	pTimeStamp->lower = pAddr->end_pts_low;
	return CELL_OK;
}

int cellPamfReaderGetMuxRateBound(mem_ptr_t<CellPamfReader> pSelf)
{
	cellPamf.Warning("cellPamfReaderGetMuxRateBound(pSelf=0x%x)", pSelf.GetAddr());

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);
	return pAddr->mux_rate_max;
}

int cellPamfReaderGetNumberOfStreams(mem_ptr_t<CellPamfReader> pSelf)
{
	cellPamf.Warning("cellPamfReaderGetNumberOfStreams(pSelf=0x%x)", pSelf.GetAddr());

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);
	return pAddr->stream_count;
}

int cellPamfReaderGetNumberOfSpecificStreams(mem_ptr_t<CellPamfReader> pSelf, u8 streamType)
{
	cellPamf.Warning("cellPamfReaderGetNumberOfSpecificStreams(pSelf=0x%x, streamType=%d)",
		pSelf.GetAddr(), streamType);

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	int counts[6] = {0, 0, 0, 0, 0, 0};

	for (u8 i = 0; i < pAddr->stream_count; i++)
	{
		counts[pamfGetStreamType(pSelf, i)]++;
	}

	switch (streamType)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
	case CELL_PAMF_STREAM_TYPE_M2V:
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS:
	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
	case CELL_PAMF_STREAM_TYPE_AC3:
	case CELL_PAMF_STREAM_TYPE_USER_DATA:
		return counts[streamType];
	case CELL_PAMF_STREAM_TYPE_VIDEO:
		return counts[CELL_PAMF_STREAM_TYPE_AVC] + counts[CELL_PAMF_STREAM_TYPE_M2V];
	case CELL_PAMF_STREAM_TYPE_AUDIO:
		return counts[CELL_PAMF_STREAM_TYPE_ATRAC3PLUS] + 
			counts[CELL_PAMF_STREAM_TYPE_PAMF_LPCM] + counts[CELL_PAMF_STREAM_TYPE_AC3];
	default:
		return 0;
	}
}

int cellPamfReaderSetStreamWithIndex(mem_ptr_t<CellPamfReader> pSelf, u8 streamIndex)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithIndex(pSelf=0x%x, streamIndex=%d)",
		pSelf.GetAddr(), streamIndex);

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	if (streamIndex < pAddr->stream_count)
	{
		pSelf->stream = streamIndex;
		return CELL_OK;
	}
	else
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}	
}

int cellPamfReaderSetStreamWithTypeAndChannel(mem_ptr_t<CellPamfReader> pSelf, u8 streamType, u8 ch)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithTypeAndChannel(pSelf=0x%x, streamType=%d, ch=%d)",
		pSelf.GetAddr(), streamType, ch);
	
	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	if (streamType > 5)
	{
		cellPamf.Error("cellPamfReaderSetStreamWithTypeAndChannel: invalid stream type(%d)", streamType);
		//it probably doesn't support "any audio" or "any video" argument
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	for (u8 i = 0; i < pAddr->stream_count; i++)
	{
		if (pamfGetStreamType(pSelf, i) == streamType) 
		{
			if (pamfGetStreamChannel(pSelf, i) == ch)
			{
				pSelf->stream = i;
				return i;
			}
		}
	}

	return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
}

int cellPamfReaderSetStreamWithTypeAndIndex(mem_ptr_t<CellPamfReader> pSelf, u8 streamType, u8 streamIndex)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithTypeAndIndex(pSelf=0x%x, streamType=%d, streamIndex=%d)",
		pSelf.GetAddr(), streamType, streamIndex);

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	u32 found = 0;

	for (u8 i = 0; i < pAddr->stream_count; i++)
	{
		const u8 type = pamfGetStreamType(pSelf, i);

		if (type == streamType)
		{
			found++;
		}
		else switch(streamType)
		{
		case CELL_PAMF_STREAM_TYPE_VIDEO:
			if (type == CELL_PAMF_STREAM_TYPE_AVC || type == CELL_PAMF_STREAM_TYPE_M2V) 
			{
				found++;
			}
			break;
		case CELL_PAMF_STREAM_TYPE_AUDIO:
			if (type == CELL_PAMF_STREAM_TYPE_ATRAC3PLUS || type == CELL_PAMF_STREAM_TYPE_AC3 || type == CELL_PAMF_STREAM_TYPE_PAMF_LPCM)
			{
				found++;
			}
			break;
		default:
			if (streamType > 5)
			{
				return CELL_PAMF_ERROR_INVALID_ARG;
			}
		}

		if (found > streamIndex)
		{
			pSelf->stream = i;
			return i;
		}
	}

	return CELL_PAMF_ERROR_STREAM_NOT_FOUND;
}

int cellPamfStreamTypeToEsFilterId(u8 type, u8 ch, mem_ptr_t<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.Warning("cellPamfStreamTypeToEsFilterId(type=%d, ch=%d, pEsFilterId_addr=0x%x)",
		type, ch, pEsFilterId.GetAddr());
	
	return pamfStreamTypeToEsFilterId(type, ch, pEsFilterId);
}

int cellPamfReaderGetStreamIndex(mem_ptr_t<CellPamfReader> pSelf)
{
	return pSelf->stream;
}

int cellPamfReaderGetStreamTypeAndChannel(mem_ptr_t<CellPamfReader> pSelf, mem8_t pType, mem8_t pCh)
{
	cellPamf.Warning("cellPamfReaderGetStreamTypeAndChannel(pSelf=0x%x (stream=%d), pType_addr=0x%x, pCh_addr=0x%x",
		pSelf.GetAddr(), pSelf->stream, pType.GetAddr(), pCh.GetAddr());

	pType = pamfGetStreamType(pSelf, pSelf->stream);
	pCh = pamfGetStreamChannel(pSelf, pSelf->stream);
	return CELL_OK;
}

int cellPamfReaderGetEsFilterId(mem_ptr_t<CellPamfReader> pSelf, mem_ptr_t<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.Warning("cellPamfReaderGetEsFilterId(pSelf=0x%x (stream=%d), pEsFilterId_addr=0x%x)",
		pSelf.GetAddr(), pSelf->stream, pEsFilterId.GetAddr());

	return pamfStreamTypeToEsFilterId(pamfGetStreamType(pSelf, pSelf->stream), 
		pamfGetStreamChannel(pSelf, pSelf->stream), pEsFilterId);
}

int cellPamfReaderGetStreamInfo(mem_ptr_t<CellPamfReader> pSelf, u32 pInfo_addr, u32 size)
{
	cellPamf.Warning("cellPamfReaderGetStreamInfo(pSelf=0x%x (stream=%d), pInfo_addr=0x%x, size=%d)",
		pSelf.GetAddr(), pSelf->stream, pInfo_addr, size);

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);

	memset(Memory + pInfo_addr, 0, size);

	switch (pamfGetStreamType(pSelf, pSelf->stream))
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
		{
			mem_ptr_t<CellPamfAvcInfo> pInfo(pInfo_addr);
			mem_ptr_t<PamfStreamHeader_AVC> pAVC(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAvcInfo))
			{
				cellPamf.Error("cellPamfReaderGetStreamInfo: wrong AVC data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->profileIdc = pAVC->profileIdc;
			pInfo->levelIdc = pAVC->levelIdc;

			pInfo->frameMbsOnlyFlag = 1; //fake
			pInfo->frameRateInfo = pAVC->unk0 - 0xc1;
			pInfo->aspectRatioIdc = 1; //fake

			pInfo->horizontalSize = 16 * (u16)pAVC->horizontalSize;
			pInfo->verticalSize = 16 * (u16)pAVC->verticalSize;

			pInfo->videoSignalInfoFlag = 1; //fake
			pInfo->colourPrimaries = 1; //fake
			pInfo->transferCharacteristics = 1; //fake
			pInfo->matrixCoefficients = 1; //fake
			//pInfo->deblockingFilterFlag = 1; //???

			cellPamf.Error("TODO: cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_AVC");
		}
		break;
	case CELL_PAMF_STREAM_TYPE_M2V:
		{
			//TODO
			cellPamf.Error("TODO: cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_M2V");
		}
		break;
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS: 
		{
			mem_ptr_t<CellPamfAtrac3plusInfo> pInfo(pInfo_addr);
			mem_ptr_t<PamfStreamHeader_Audio> pAudio(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAtrac3plusInfo))
			{
				cellPamf.Error("cellPamfReaderGetStreamInfo: wrong ATRAC3+ data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;
		}
		break;
	case CELL_PAMF_STREAM_TYPE_AC3:
		{
			mem_ptr_t<CellPamfAc3Info> pInfo(pInfo_addr);
			mem_ptr_t<PamfStreamHeader_Audio> pAudio(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAc3Info))
			{
				cellPamf.Error("cellPamfReaderGetStreamInfo: wrong AC3 data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;
		}
		break;
	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
		{
			mem_ptr_t<CellPamfLpcmInfo> pInfo(pInfo_addr);
			mem_ptr_t<PamfStreamHeader_Audio> pAudio(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfLpcmInfo))
			{
				cellPamf.Error("cellPamfReaderGetStreamInfo: wrong LPCM data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;

			if (pAudio->bps = 0x40)
				pInfo->bitsPerSample = CELL_PAMF_BIT_LENGTH_16;
			else
				//TODO: CELL_PAMF_BIT_LENGTH_24
				cellPamf.Error("cellPamfReaderGetStreamInfo: unknown bps(0x%x)", (u8)pAudio->bps);
		}
		break;
	case CELL_PAMF_STREAM_TYPE_USER_DATA: 
		{
			cellPamf.Error("cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_USER_DATA");
			return CELL_PAMF_ERROR_INVALID_ARG;
		}
	}
	
	return CELL_OK;
}

int cellPamfReaderGetNumberOfEp(mem_ptr_t<CellPamfReader> pSelf)
{
	cellPamf.Warning("cellPamfReaderGetNumberOfEp(pSelf=0x%x (stream=%d))",
		pSelf.GetAddr(), pSelf->stream);

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);	
	return pAddr->stream_headers[pSelf->stream].ep_num;
}

int cellPamfReaderGetEpIteratorWithIndex(mem_ptr_t<CellPamfReader> pSelf, u32 epIndex, mem_ptr_t<CellPamfEpIterator> pIt)
{
	cellPamf.Warning("cellPamfReaderGetEpIteratorWithIndex(pSelf=0x%x (stream=%d), epIndex=%d, pIt_addr=0x%x)",
		pSelf.GetAddr(), pSelf->stream, epIndex, pIt.GetAddr());

	const mem_ptr_t<PamfHeader> pAddr(pSelf->pAddr);
	//TODO:
	return CELL_OK;
}

int cellPamfReaderGetEpIteratorWithTimeStamp(mem_ptr_t<CellPamfReader> pSelf, mem_ptr_t<CellCodecTimeStamp> pTimeStamp, mem_ptr_t<CellPamfEpIterator> pIt)
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfEpIteratorGetEp(mem_ptr_t<CellPamfEpIterator> pIt, mem_ptr_t<CellPamfEp> pEp)
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

int cellPamfEpIteratorMove(mem_ptr_t<CellPamfEpIterator> pIt, int steps, mem_ptr_t<CellPamfEp> pEp)
{
	UNIMPLEMENTED_FUNC(cellPamf);
	return CELL_OK;
}

void cellPamf_init()
{
	cellPamf.AddFunc(0xca8181c1, cellPamfGetHeaderSize);
	cellPamf.AddFunc(0x90fc9a59, cellPamfGetHeaderSize2);
	cellPamf.AddFunc(0x44f5c9e3, cellPamfGetStreamOffsetAndSize);
	cellPamf.AddFunc(0xd1a40ef4, cellPamfVerify);
	cellPamf.AddFunc(0xb8436ee5, cellPamfReaderInitialize);
	cellPamf.AddFunc(0x4de501b1, cellPamfReaderGetPresentationStartTime);
	cellPamf.AddFunc(0xf61609d6, cellPamfReaderGetPresentationEndTime);
	cellPamf.AddFunc(0xdb70296c, cellPamfReaderGetMuxRateBound);
	cellPamf.AddFunc(0x37f723f7, cellPamfReaderGetNumberOfStreams);
	cellPamf.AddFunc(0xd0230671, cellPamfReaderGetNumberOfSpecificStreams);
	cellPamf.AddFunc(0x461534b4, cellPamfReaderSetStreamWithIndex);
	cellPamf.AddFunc(0x03fd2caa, cellPamfReaderSetStreamWithTypeAndChannel);
	cellPamf.AddFunc(0x28b4e2c1, cellPamfReaderSetStreamWithTypeAndIndex);
	cellPamf.AddFunc(0x01067e22, cellPamfStreamTypeToEsFilterId);
	cellPamf.AddFunc(0x041cc708, cellPamfReaderGetStreamIndex);
	cellPamf.AddFunc(0x9ab20793, cellPamfReaderGetStreamTypeAndChannel);
	cellPamf.AddFunc(0x71df326a, cellPamfReaderGetEsFilterId);
	cellPamf.AddFunc(0x67fd273b, cellPamfReaderGetStreamInfo);
	cellPamf.AddFunc(0xd9ea3457, cellPamfReaderGetNumberOfEp);
	cellPamf.AddFunc(0xe8586ec6, cellPamfReaderGetEpIteratorWithIndex);
	cellPamf.AddFunc(0x439fba17, cellPamfReaderGetEpIteratorWithTimeStamp);
	cellPamf.AddFunc(0x1abeb9d6, cellPamfEpIteratorGetEp);
	cellPamf.AddFunc(0x50b83205, cellPamfEpIteratorMove);
}