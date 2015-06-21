#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellPamf.h"

extern Module cellPamf;

s32 pamfStreamTypeToEsFilterId(u8 type, u8 ch, CellCodecEsFilterId& pEsFilterId)
{
	// convert type and ch to EsFilterId
	assert(ch < 16);
	pEsFilterId.supplementalInfo1 = type == CELL_PAMF_STREAM_TYPE_AVC;
	pEsFilterId.supplementalInfo2 = 0;
	
	switch (type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
	{
		// code = 0x1b
		pEsFilterId.filterIdMajor = 0xe0 | ch;
		pEsFilterId.filterIdMinor = 0;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_M2V:
	{
		// code = 0x02
		pEsFilterId.filterIdMajor = 0xe0 | ch;
		pEsFilterId.filterIdMinor = 0;
		break;
	}
		
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS:
	{
		// code = 0xdc
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = ch;
		break;
	}
		
	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
	{
		// code = 0x80
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = 0x40 | ch;
		break;
	}

	case CELL_PAMF_STREAM_TYPE_AC3:
	{
		// code = 0x81
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = 0x30 | ch;
		break;
	}
		
	case CELL_PAMF_STREAM_TYPE_USER_DATA:
	{
		// code = 0xdd
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = 0x20 | ch;
		break;
	}

	case 6:
	{
		// code = 0xff
		pEsFilterId.filterIdMajor = 0xe0 | ch;
		pEsFilterId.filterIdMinor = 0;
		break;
	}

	case 7:
	{
		// code = 0xff
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = ch;
		break;
	}

	case 8:
	{
		// code = 0xff
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = 0x10 | ch;
		break;
	}

	case 9:
	{
		// code = 0xff
		pEsFilterId.filterIdMajor = 0xbd;
		pEsFilterId.filterIdMinor = 0x20 | ch;
		break;
	}
		
	default:
	{
		cellPamf.Error("pamfStreamTypeToEsFilterId(): unknown type (%d, ch=%d)", type, ch);
		Emu.Pause();
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	}

	return CELL_OK;
}

u8 pamfGetStreamType(vm::ptr<CellPamfReader> pSelf, u32 stream)
{
	// TODO: get stream type correctly
	assert(stream < (u32)pSelf->pAddr->stream_count);
	auto& header = pSelf->pAddr->stream_headers[stream];

	switch (header.type)
	{
	case 0x1b: return CELL_PAMF_STREAM_TYPE_AVC;
	case 0x02: return CELL_PAMF_STREAM_TYPE_M2V;
	case 0xdc: return CELL_PAMF_STREAM_TYPE_ATRAC3PLUS;
	case 0x80: return CELL_PAMF_STREAM_TYPE_PAMF_LPCM;
	case 0x81: return CELL_PAMF_STREAM_TYPE_AC3;
	case 0xdd: return CELL_PAMF_STREAM_TYPE_USER_DATA;
	}

	cellPamf.Todo("pamfGetStreamType(): unsupported stream type found(0x%x)", header.type);
	Emu.Pause();
	return 0xff;
}

u8 pamfGetStreamChannel(vm::ptr<CellPamfReader> pSelf, u32 stream)
{
	// TODO: get stream channel correctly
	assert(stream < (u32)pSelf->pAddr->stream_count);
	auto& header = pSelf->pAddr->stream_headers[stream];

	switch (header.type)
	{
	case 0x1b: // AVC
	case 0x02: // M2V
	{
		assert((header.fid_major & 0xf0) == 0xe0 && header.fid_minor == 0);
		return header.fid_major % 16;
	}
		
	case 0xdc: // ATRAC3PLUS
	{
		assert(header.fid_major == 0xbd && (header.fid_minor & 0xf0) == 0);
		return header.fid_minor % 16;
	}
		
	case 0x80: // LPCM
	{
		assert(header.fid_major == 0xbd && (header.fid_minor & 0xf0) == 0x40);
		return header.fid_minor % 16;
	}
	case 0x81: // AC3
	{
		assert(header.fid_major == 0xbd && (header.fid_minor & 0xf0) == 0x30);
		return header.fid_minor % 16;
	}
	case 0xdd:
	{
		assert(header.fid_major == 0xbd && (header.fid_minor & 0xf0) == 0x20);
		return header.fid_minor % 16;
	}
	}

	cellPamf.Todo("pamfGetStreamChannel(): unsupported stream type found(0x%x)", header.type);
	Emu.Pause();
	return 0xff;
}

s32 cellPamfGetHeaderSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize(pAddr=0x%x, fileSize=%d, pSize_addr=0x%x)", pAddr.addr(), fileSize, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

s32 cellPamfGetHeaderSize2(vm::ptr<PamfHeader> pAddr, u64 fileSize, u32 attribute, vm::ptr<u64> pSize)
{
	cellPamf.Warning("cellPamfGetHeaderSize2(pAddr=0x%x, fileSize=%d, attribute=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, attribute, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

s32 cellPamfGetStreamOffsetAndSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pOffset, vm::ptr<u64> pSize)
{
	cellPamf.Warning("cellPamfGetStreamOffsetAndSize(pAddr=0x%x, fileSize=%d, pOffset_addr=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, pOffset.addr(), pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pOffset = offset;
	const u64 size = (u64)pAddr->data_size << 11;
	*pSize = size;
	return CELL_OK;
}

s32 cellPamfVerify(vm::ptr<PamfHeader> pAddr, u64 fileSize)
{
	cellPamf.Todo("cellPamfVerify(pAddr=0x%x, fileSize=%d)", pAddr.addr(), fileSize);

	// TODO
	return CELL_OK;
}

s32 cellPamfReaderInitialize(vm::ptr<CellPamfReader> pSelf, vm::cptr<PamfHeader> pAddr, u64 fileSize, u32 attribute)
{
	cellPamf.Warning("cellPamfReaderInitialize(pSelf=0x%x, pAddr=0x%x, fileSize=%d, attribute=0x%x)", pSelf.addr(), pAddr.addr(), fileSize, attribute);
	
	if (fileSize)
	{
		pSelf->fileSize = fileSize;
	}
	else // if fileSize is unknown
	{
		pSelf->fileSize = ((u64)pAddr->data_offset << 11) + ((u64)pAddr->data_size << 11);
	}
	pSelf->pAddr = pAddr;

	if (attribute & CELL_PAMF_ATTRIBUTE_VERIFY_ON)
	{
		// TODO
		cellPamf.Todo("cellPamfReaderInitialize(): verification");
	}

	pSelf->stream = 0; // currently set stream
	return CELL_OK;
}

s32 cellPamfReaderGetPresentationStartTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.Warning("cellPamfReaderGetPresentationStartTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	// always returns CELL_OK

	pTimeStamp->upper = (u32)(u16)pSelf->pAddr->start_pts_high;
	pTimeStamp->lower = pSelf->pAddr->start_pts_low;
	return CELL_OK;
}

s32 cellPamfReaderGetPresentationEndTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf.Warning("cellPamfReaderGetPresentationEndTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	// always returns CELL_OK

	pTimeStamp->upper = (u32)(u16)pSelf->pAddr->end_pts_high;
	pTimeStamp->lower = pSelf->pAddr->end_pts_low;
	return CELL_OK;
}

u32 cellPamfReaderGetMuxRateBound(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.Warning("cellPamfReaderGetMuxRateBound(pSelf=0x%x)", pSelf.addr());

	// cannot return error code
	return pSelf->pAddr->mux_rate_max;
}

u8 cellPamfReaderGetNumberOfStreams(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.Warning("cellPamfReaderGetNumberOfStreams(pSelf=0x%x)", pSelf.addr());

	// cannot return error code
	return pSelf->pAddr->stream_count;
}

u8 cellPamfReaderGetNumberOfSpecificStreams(vm::ptr<CellPamfReader> pSelf, u8 streamType)
{
	cellPamf.Warning("cellPamfReaderGetNumberOfSpecificStreams(pSelf=0x%x, streamType=%d)", pSelf.addr(), streamType);
	
	// cannot return error code

	u8 counts[256] = {};

	for (u8 i = 0; i < pSelf->pAddr->stream_count; i++)
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
	{
		return counts[streamType];
	}
		
	case CELL_PAMF_STREAM_TYPE_VIDEO:
	{
		return counts[CELL_PAMF_STREAM_TYPE_AVC] + counts[CELL_PAMF_STREAM_TYPE_M2V];
	}
		
	case CELL_PAMF_STREAM_TYPE_AUDIO:
	{
		return counts[CELL_PAMF_STREAM_TYPE_ATRAC3PLUS] + counts[CELL_PAMF_STREAM_TYPE_PAMF_LPCM] + counts[CELL_PAMF_STREAM_TYPE_AC3];
	}
	}

	cellPamf.Todo("cellPamfReaderGetNumberOfSpecificStreams(): unsupported stream type (0x%x)", streamType);
	Emu.Pause();
	return 0;
}

s32 cellPamfReaderSetStreamWithIndex(vm::ptr<CellPamfReader> pSelf, u8 streamIndex)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithIndex(pSelf=0x%x, streamIndex=%d)", pSelf.addr(), streamIndex);

	if (streamIndex >= pSelf->pAddr->stream_count)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	pSelf->stream = streamIndex;
	return CELL_OK;
}

s32 cellPamfReaderSetStreamWithTypeAndChannel(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 ch)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithTypeAndChannel(pSelf=0x%x, streamType=%d, ch=%d)", pSelf.addr(), streamType, ch);

	// it probably doesn't support "any audio" or "any video" argument
	if (streamType > 5 || ch >= 16)
	{
		cellPamf.Error("cellPamfReaderSetStreamWithTypeAndChannel(): invalid arguments (streamType=%d, ch=%d)", streamType, ch);
		Emu.Pause();
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	for (u8 i = 0; i < pSelf->pAddr->stream_count; i++)
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

s32 cellPamfReaderSetStreamWithTypeAndIndex(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 streamIndex)
{
	cellPamf.Warning("cellPamfReaderSetStreamWithTypeAndIndex(pSelf=0x%x, streamType=%d, streamIndex=%d)", pSelf.addr(), streamType, streamIndex);

	u32 found = 0;

	for (u8 i = 0; i < pSelf->pAddr->stream_count; i++)
	{
		const u8 type = pamfGetStreamType(pSelf, i);

		if (type == streamType)
		{
			found++;
		}
		else switch(streamType)
		{
		case CELL_PAMF_STREAM_TYPE_VIDEO:
		{
			if (type == CELL_PAMF_STREAM_TYPE_AVC || type == CELL_PAMF_STREAM_TYPE_M2V)
			{
				found++;
			}
			break;
		}
			
		case CELL_PAMF_STREAM_TYPE_AUDIO:
		{
			if (type == CELL_PAMF_STREAM_TYPE_ATRAC3PLUS || type == CELL_PAMF_STREAM_TYPE_AC3 || type == CELL_PAMF_STREAM_TYPE_PAMF_LPCM)
			{
				found++;
			}
			break;
		}
			
		default:
		{
			if (streamType > 5)
			{
				return CELL_PAMF_ERROR_INVALID_ARG;
			}
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

s32 cellPamfStreamTypeToEsFilterId(u8 type, u8 ch, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.Warning("cellPamfStreamTypeToEsFilterId(type=%d, ch=%d, pEsFilterId_addr=0x%x)", type, ch, pEsFilterId.addr());

	if (!pEsFilterId)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	
	return pamfStreamTypeToEsFilterId(type, ch, *pEsFilterId);
}

s32 cellPamfReaderGetStreamIndex(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.Log("cellPamfReaderGetStreamIndex(pSelf=0x%x)", pSelf.addr());

	// seems that CELL_PAMF_ERROR_INVALID_PAMF must be already written in pSelf->stream if it's the case
	return pSelf->stream;
}

s32 cellPamfReaderGetStreamTypeAndChannel(vm::ptr<CellPamfReader> pSelf, vm::ptr<u8> pType, vm::ptr<u8> pCh)
{
	cellPamf.Warning("cellPamfReaderGetStreamTypeAndChannel(pSelf=0x%x (stream=%d), pType_addr=0x%x, pCh_addr=0x%x",
		pSelf.addr(), pSelf->stream, pType.addr(), pCh.addr());

	// unclear

	*pType = pamfGetStreamType(pSelf, pSelf->stream);
	*pCh = pamfGetStreamChannel(pSelf, pSelf->stream);
	return CELL_OK;
}

s32 cellPamfReaderGetEsFilterId(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf.Warning("cellPamfReaderGetEsFilterId(pSelf=0x%x (stream=%d), pEsFilterId_addr=0x%x)", pSelf.addr(), pSelf->stream, pEsFilterId.addr());

	// always returns CELL_OK

	assert((u32)pSelf->stream < (u32)pSelf->pAddr->stream_count);
	auto& header = pSelf->pAddr->stream_headers[pSelf->stream];
	pEsFilterId->filterIdMajor = header.fid_major;
	pEsFilterId->filterIdMinor = header.fid_minor;
	pEsFilterId->supplementalInfo1 = header.type == 0x1b ? 1 : 0;
	pEsFilterId->supplementalInfo2 = 0;
	return CELL_OK;
}

s32 cellPamfReaderGetStreamInfo(vm::ptr<CellPamfReader> pSelf, u32 pInfo_addr, u32 size)
{
	cellPamf.Warning("cellPamfReaderGetStreamInfo(pSelf=0x%x, stream=%d, pInfo_addr=0x%x, size=%d)", pSelf.addr(), pSelf->stream, pInfo_addr, size);

	assert((u32)pSelf->stream < (u32)pSelf->pAddr->stream_count);
	auto& header = pSelf->pAddr->stream_headers[pSelf->stream];
	const u8 type = pamfGetStreamType(pSelf, pSelf->stream);
	const u8 ch = pamfGetStreamChannel(pSelf, pSelf->stream);

	switch (type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
	{
		if (size < sizeof(CellPamfAvcInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::ptr<CellPamfAvcInfo>::make(pInfo_addr);

		info->profileIdc = header.AVC.profileIdc;
		info->levelIdc = header.AVC.levelIdc;
		info->frameMbsOnlyFlag = (header.AVC.x2 & 0x80) >> 7;
		info->videoSignalInfoFlag = (header.AVC.x2 & 0x40) >> 6;
		info->frameRateInfo = (header.AVC.x2 & 0x0f) - 1;
		info->aspectRatioIdc = header.AVC.aspectRatioIdc;

		if (header.AVC.aspectRatioIdc == 0xff)
		{
			info->sarWidth = header.AVC.sarWidth;
			info->sarHeight = header.AVC.sarHeight;
		}
		else
		{
			info->sarWidth = 0;
			info->sarHeight = 0;
		}

		info->horizontalSize = ((u16)header.AVC.horizontalSize & 0xff) * 16;
		info->verticalSize = ((u16)header.AVC.verticalSize & 0xff) * 16;
		info->frameCropLeftOffset = header.AVC.frameCropLeftOffset;
		info->frameCropRightOffset = header.AVC.frameCropRightOffset;
		info->frameCropTopOffset = header.AVC.frameCropTopOffset;
		info->frameCropBottomOffset = header.AVC.frameCropBottomOffset;

		if (info->videoSignalInfoFlag)
		{
			info->videoFormat = header.AVC.x14 >> 5;
			info->videoFullRangeFlag = (header.AVC.x14 & 0x10) >> 4;
			info->colourPrimaries = header.AVC.colourPrimaries;
			info->transferCharacteristics = header.AVC.transferCharacteristics;
			info->matrixCoefficients = header.AVC.matrixCoefficients;
		}
		else
		{
			info->videoFormat = 0;
			info->videoFullRangeFlag = 0;
			info->colourPrimaries = 0;
			info->transferCharacteristics = 0;
			info->matrixCoefficients = 0;
		}
		
		info->entropyCodingModeFlag = (header.AVC.x18 & 0x80) >> 7;
		info->deblockingFilterFlag = (header.AVC.x18 & 0x40) >> 6;
		info->minNumSlicePerPictureIdc = (header.AVC.x18 & 0x30) >> 4;
		info->nfwIdc = header.AVC.x18 & 0x03;
		info->maxMeanBitrate = header.AVC.maxMeanBitrate;

		cellPamf.Notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_AVC");
		break;
	}
		
	case CELL_PAMF_STREAM_TYPE_M2V:
	{
		if (size < sizeof(CellPamfM2vInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::ptr<CellPamfM2vInfo>::make(pInfo_addr);

		switch (header.M2V.x0)
		{
		case 0x44: info->profileAndLevelIndication = 3; break;
		case 0x48: info->profileAndLevelIndication = 1; break;
		default: info->profileAndLevelIndication = CELL_PAMF_M2V_UNKNOWN;
		}

		info->progressiveSequence = (header.M2V.x2 & 0x80) >> 7;
		info->videoSignalInfoFlag = (header.M2V.x2 & 0x40) >> 6;
		info->frameRateInfo = header.M2V.x2 & 0xf;
		info->aspectRatioIdc = header.M2V.aspectRatioIdc;

		if (header.M2V.aspectRatioIdc == 0xff)
		{
			info->sarWidth = header.M2V.sarWidth;
			info->sarHeight = header.M2V.sarHeight;
		}
		else
		{
			info->sarWidth = 0;
			info->sarHeight = 0;
		}

		info->horizontalSize = ((u16)header.M2V.horizontalSize & 0xff) * 16;
		info->verticalSize = ((u16)header.M2V.verticalSize & 0xff) * 16;
		info->horizontalSizeValue = header.M2V.horizontalSizeValue;
		info->verticalSizeValue = header.M2V.verticalSizeValue;

		if (info->videoSignalInfoFlag)
		{
			info->videoFormat = header.M2V.x14 >> 5;
			info->videoFullRangeFlag = (header.M2V.x14 & 0x10) >> 4;
			info->colourPrimaries = header.M2V.colourPrimaries;
			info->transferCharacteristics = header.M2V.transferCharacteristics;
			info->matrixCoefficients = header.M2V.matrixCoefficients;
		}
		else
		{
			info->videoFormat = 0;
			info->videoFullRangeFlag = 0;
			info->colourPrimaries = 0;
			info->transferCharacteristics = 0;
			info->matrixCoefficients = 0;
		}

		cellPamf.Notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_M2V");
		break;
	}
		
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS: 
	{
		if (size < sizeof(CellPamfAtrac3plusInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::ptr<CellPamfAtrac3plusInfo>::make(pInfo_addr);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;

		cellPamf.Notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_ATRAC3PLUS");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
	{
		if (size < sizeof(CellPamfLpcmInfo))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::ptr<CellPamfLpcmInfo>::make(pInfo_addr);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;
		info->bitsPerSample = header.audio.bps >> 6;

		cellPamf.Notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_PAMF_LPCM");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_AC3:
	{
		if (size < sizeof(CellPamfAc3Info))
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		auto info = vm::ptr<CellPamfAc3Info>::make(pInfo_addr);

		info->samplingFrequency = header.audio.freq & 0xf;
		info->numberOfChannels = header.audio.channels & 0xf;

		cellPamf.Notice("cellPamfReaderGetStreamInfo(): CELL_PAMF_STREAM_TYPE_AC3");
		break;
	}

	case CELL_PAMF_STREAM_TYPE_USER_DATA:
	{
		cellPamf.Error("cellPamfReaderGetStreamInfo(): invalid type CELL_PAMF_STREAM_TYPE_USER_DATA");
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	case 6:
	{
		if (size < 4)
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		cellPamf.Todo("cellPamfReaderGetStreamInfo(): type 6");
		break;
	}

	case 7:
	{
		if (size < 2)
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		cellPamf.Todo("cellPamfReaderGetStreamInfo(): type 7");
		break;
	}

	case 8:
	{
		if (size < 2)
		{
			return CELL_PAMF_ERROR_INVALID_ARG;
		}

		cellPamf.Todo("cellPamfReaderGetStreamInfo(): type 8");
		break;
	}

	case 9:
	{
		cellPamf.Error("cellPamfReaderGetStreamInfo(): invalid type 9");
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	default:
	{
		// invalid type or getting type/ch failed
		cellPamf.Error("cellPamfReaderGetStreamInfo(): invalid type %d (ch=%d)", type, ch);
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}
	}
	
	return CELL_OK;
}

u32 cellPamfReaderGetNumberOfEp(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf.Todo("cellPamfReaderGetNumberOfEp(pSelf=0x%x, stream=%d)", pSelf.addr(), pSelf->stream);

	// cannot return error code
	return 0; //pSelf->pAddr->stream_headers[pSelf->stream].ep_num;
}

s32 cellPamfReaderGetEpIteratorWithIndex(vm::ptr<CellPamfReader> pSelf, u32 epIndex, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf.Todo("cellPamfReaderGetEpIteratorWithIndex(pSelf=0x%x, stream=%d, epIndex=%d, pIt_addr=0x%x)", pSelf.addr(), pSelf->stream, epIndex, pIt.addr());

	// TODO
	return CELL_OK;
}

s32 cellPamfReaderGetEpIteratorWithTimeStamp(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf.Todo("cellPamfReaderGetEpIteratorWithTimeStamp(pSelf=0x%x, pTimeStamp_addr=0x%x, pIt_addr=0x%x)", pSelf.addr(), pTimeStamp.addr(), pIt.addr());

	// TODO
	return CELL_OK;
}

s32 cellPamfEpIteratorGetEp(vm::ptr<CellPamfEpIterator> pIt, vm::ptr<CellPamfEp> pEp)
{
	cellPamf.Todo("cellPamfEpIteratorGetEp(pIt_addr=0x%x, pEp_addr=0x%x)", pIt.addr(), pEp.addr());

	// always returns CELL_OK
	// TODO
	return CELL_OK;
}

s32 cellPamfEpIteratorMove(vm::ptr<CellPamfEpIterator> pIt, s32 steps, vm::ptr<CellPamfEp> pEp)
{
	cellPamf.Todo("cellPamfEpIteratorMove(pIt_addr=0x%x, steps=%d, pEp_addr=0x%x)", pIt.addr(), steps, pEp.addr());

	// cannot return error code
	// TODO
	return 0;
}

Module cellPamf("cellPamf", []()
{
	REG_FUNC(cellPamf, cellPamfGetHeaderSize);
	REG_FUNC(cellPamf, cellPamfGetHeaderSize2);
	REG_FUNC(cellPamf, cellPamfGetStreamOffsetAndSize);
	REG_FUNC(cellPamf, cellPamfVerify);
	REG_FUNC(cellPamf, cellPamfReaderInitialize);
	REG_FUNC(cellPamf, cellPamfReaderGetPresentationStartTime);
	REG_FUNC(cellPamf, cellPamfReaderGetPresentationEndTime);
	REG_FUNC(cellPamf, cellPamfReaderGetMuxRateBound);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfStreams);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfSpecificStreams);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithIndex);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithTypeAndChannel);
	REG_FUNC(cellPamf, cellPamfReaderSetStreamWithTypeAndIndex);
	REG_FUNC(cellPamf, cellPamfStreamTypeToEsFilterId);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamIndex);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamTypeAndChannel);
	REG_FUNC(cellPamf, cellPamfReaderGetEsFilterId);
	REG_FUNC(cellPamf, cellPamfReaderGetStreamInfo);
	REG_FUNC(cellPamf, cellPamfReaderGetNumberOfEp);
	REG_FUNC(cellPamf, cellPamfReaderGetEpIteratorWithIndex);
	REG_FUNC(cellPamf, cellPamfReaderGetEpIteratorWithTimeStamp);
	REG_FUNC(cellPamf, cellPamfEpIteratorGetEp);
	REG_FUNC(cellPamf, cellPamfEpIteratorMove);
});
