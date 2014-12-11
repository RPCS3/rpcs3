#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellPamf.h"

Module *cellPamf = nullptr;

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
		cellPamf->Error("pamfStreamTypeToEsFilterId(): unknown type (%d, ch=%d)", type, ch);
		Emu.Pause();
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	}

	return CELL_OK;
}

u8 pamfGetStreamType(vm::ptr<CellPamfReader> pSelf, u8 stream)
{
	// TODO: get stream type correctly
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

	cellPamf->Todo("pamfGetStreamType(): unsupported stream type found(0x%x)", header.type);
	Emu.Pause();
	return 0xff;
}

u8 pamfGetStreamChannel(vm::ptr<CellPamfReader> pSelf, u8 stream)
{
	// TODO: get stream channel correctly
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

	cellPamf->Todo("pamfGetStreamChannel(): unsupported stream type found(0x%x)", header.type);
	Emu.Pause();
	return 0xff;
}

s32 cellPamfGetHeaderSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetHeaderSize(pAddr=0x%x, fileSize=%d, pSize_addr=0x%x)", pAddr.addr(), fileSize, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

s32 cellPamfGetHeaderSize2(vm::ptr<PamfHeader> pAddr, u64 fileSize, u32 attribute, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetHeaderSize2(pAddr=0x%x, fileSize=%d, attribute=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, attribute, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

s32 cellPamfGetStreamOffsetAndSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pOffset, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetStreamOffsetAndSize(pAddr=0x%x, fileSize=%d, pOffset_addr=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, pOffset.addr(), pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150) return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pOffset = offset;
	const u64 size = (u64)pAddr->data_size << 11;
	*pSize = size;
	return CELL_OK;
}

s32 cellPamfVerify(vm::ptr<PamfHeader> pAddr, u64 fileSize)
{
	cellPamf->Todo("cellPamfVerify(pAddr=0x%x, fileSize=%d)", pAddr.addr(), fileSize);

	// TODO
	return CELL_OK;
}

s32 cellPamfReaderInitialize(vm::ptr<CellPamfReader> pSelf, vm::ptr<const PamfHeader> pAddr, u64 fileSize, u32 attribute)
{
	cellPamf->Warning("cellPamfReaderInitialize(pSelf=0x%x, pAddr=0x%x, fileSize=%d, attribute=0x%x)", pSelf.addr(), pAddr.addr(), fileSize, attribute);
	
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
		cellPamf->Todo("cellPamfReaderInitialize(): verification");
	}

	pSelf->stream = 0; // currently set stream
	return CELL_OK;
}

s32 cellPamfReaderGetPresentationStartTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf->Warning("cellPamfReaderGetPresentationStartTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	// always returns CELL_OK

	pTimeStamp->upper = (u32)(u16)pSelf->pAddr->start_pts_high;
	pTimeStamp->lower = pSelf->pAddr->start_pts_low;
	return CELL_OK;
}

s32 cellPamfReaderGetPresentationEndTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf->Warning("cellPamfReaderGetPresentationEndTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	// always returns CELL_OK

	pTimeStamp->upper = (u32)(u16)pSelf->pAddr->end_pts_high;
	pTimeStamp->lower = pSelf->pAddr->end_pts_low;
	return CELL_OK;
}

u32 cellPamfReaderGetMuxRateBound(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetMuxRateBound(pSelf=0x%x)", pSelf.addr());

	// cannot return error code
	return pSelf->pAddr->mux_rate_max;
}

u8 cellPamfReaderGetNumberOfStreams(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfStreams(pSelf=0x%x)", pSelf.addr());

	// cannot return error code
	return pSelf->pAddr->stream_count;
}

u8 cellPamfReaderGetNumberOfSpecificStreams(vm::ptr<CellPamfReader> pSelf, u8 streamType)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfSpecificStreams(pSelf=0x%x, streamType=%d)", pSelf.addr(), streamType);
	
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

	cellPamf->Todo("cellPamfReaderGetNumberOfSpecificStreams(): unsupported stream type (0x%x)", streamType);
	Emu.Pause();
	return 0;
}

s32 cellPamfReaderSetStreamWithIndex(vm::ptr<CellPamfReader> pSelf, u8 streamIndex)
{
	cellPamf->Warning("cellPamfReaderSetStreamWithIndex(pSelf=0x%x, streamIndex=%d)", pSelf.addr(), streamIndex);

	if (streamIndex >= pSelf->pAddr->stream_count)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}

	pSelf->stream = streamIndex;
	return CELL_OK;
}

s32 cellPamfReaderSetStreamWithTypeAndChannel(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 ch)
{
	cellPamf->Warning("cellPamfReaderSetStreamWithTypeAndChannel(pSelf=0x%x, streamType=%d, ch=%d)", pSelf.addr(), streamType, ch);

	// it probably doesn't support "any audio" or "any video" argument
	if (streamType > 5 || ch >= 16)
	{
		cellPamf->Error("cellPamfReaderSetStreamWithTypeAndChannel(): invalid arguments (streamType=%d, ch=%d)", streamType, ch);
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
	cellPamf->Warning("cellPamfReaderSetStreamWithTypeAndIndex(pSelf=0x%x, streamType=%d, streamIndex=%d)", pSelf.addr(), streamType, streamIndex);

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
	cellPamf->Warning("cellPamfStreamTypeToEsFilterId(type=%d, ch=%d, pEsFilterId_addr=0x%x)", type, ch, pEsFilterId.addr());

	if (!pEsFilterId)
	{
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	
	return pamfStreamTypeToEsFilterId(type, ch, *pEsFilterId);
}

s32 cellPamfReaderGetStreamIndex(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Log("cellPamfReaderGetStreamIndex(pSelf=0x%x)", pSelf.addr());

	// seems that CELL_PAMF_ERROR_INVALID_PAMF must be already written in pSelf->stream if it's the case
	return pSelf->stream;
}

s32 cellPamfReaderGetStreamTypeAndChannel(vm::ptr<CellPamfReader> pSelf, vm::ptr<u8> pType, vm::ptr<u8> pCh)
{
	cellPamf->Warning("cellPamfReaderGetStreamTypeAndChannel(pSelf=0x%x (stream=%d), pType_addr=0x%x, pCh_addr=0x%x",
		pSelf.addr(), pSelf->stream, pType.addr(), pCh.addr());

	// unclear

	*pType = pamfGetStreamType(pSelf, pSelf->stream);
	*pCh = pamfGetStreamChannel(pSelf, pSelf->stream);
	return CELL_OK;
}

s32 cellPamfReaderGetEsFilterId(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf->Warning("cellPamfReaderGetEsFilterId(pSelf=0x%x (stream=%d), pEsFilterId_addr=0x%x)", pSelf.addr(), pSelf->stream, pEsFilterId.addr());

	// always returns CELL_OK

	auto& header = pSelf->pAddr->stream_headers[pSelf->stream];
	pEsFilterId->filterIdMajor = header.fid_major;
	pEsFilterId->filterIdMinor = header.fid_minor;
	pEsFilterId->supplementalInfo1 = header.type == 0x1b ? 1 : 0;
	pEsFilterId->supplementalInfo2 = 0;
	return CELL_OK;
}

s32 cellPamfReaderGetStreamInfo(vm::ptr<CellPamfReader> pSelf, u32 pInfo_addr, u32 size)
{
	cellPamf->Warning("cellPamfReaderGetStreamInfo(pSelf=0x%x, stream=%d, pInfo_addr=0x%x, size=%d)", pSelf.addr(), pSelf->stream, pInfo_addr, size);

	// TODO (many parameters are wrong)
	memset(vm::get_ptr<void>(pInfo_addr), 0, size);

	switch (pamfGetStreamType(pSelf, pSelf->stream))
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
		{
			auto pInfo = vm::ptr<CellPamfAvcInfo>::make(pInfo_addr);
			auto pAVC = vm::ptr<PamfStreamHeader_AVC>::make(pSelf->pAddr.addr() + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAvcInfo))
			{
				cellPamf->Error("cellPamfReaderGetStreamInfo: wrong AVC data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->profileIdc = pAVC->profileIdc;
			pInfo->levelIdc = pAVC->levelIdc;

			pInfo->frameMbsOnlyFlag = 1; //fake
			pInfo->frameRateInfo = (pAVC->unk0 & 0x7) - 1;
			pInfo->aspectRatioIdc = 1; //fake

			pInfo->horizontalSize = 16 * (u16)pAVC->horizontalSize;
			pInfo->verticalSize = 16 * (u16)pAVC->verticalSize;

			pInfo->videoSignalInfoFlag = 1; //fake
			pInfo->colourPrimaries = 1; //fake
			pInfo->transferCharacteristics = 1; //fake
			pInfo->matrixCoefficients = 1; //fake
			//pInfo->deblockingFilterFlag = 1; //???

			cellPamf->Warning("cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_AVC");
		}
		break;
	case CELL_PAMF_STREAM_TYPE_M2V:
		{
			//TODO
			cellPamf->Error("TODO: cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_M2V");
		}
		break;
	case CELL_PAMF_STREAM_TYPE_ATRAC3PLUS: 
		{
			auto pInfo = vm::ptr<CellPamfAtrac3plusInfo>::make(pInfo_addr);
			auto pAudio = vm::ptr<PamfStreamHeader_Audio>::make(pSelf->pAddr.addr() + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAtrac3plusInfo))
			{
				cellPamf->Error("cellPamfReaderGetStreamInfo: wrong ATRAC3+ data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;
		}
		break;
	case CELL_PAMF_STREAM_TYPE_AC3:
		{
			auto pInfo = vm::ptr<CellPamfAc3Info>::make(pInfo_addr);
			auto pAudio = vm::ptr<PamfStreamHeader_Audio>::make(pSelf->pAddr.addr() + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfAc3Info))
			{
				cellPamf->Error("cellPamfReaderGetStreamInfo: wrong AC3 data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;
		}
		break;
	case CELL_PAMF_STREAM_TYPE_PAMF_LPCM:
		{
			auto pInfo = vm::ptr<CellPamfLpcmInfo>::make(pInfo_addr);
			auto pAudio = vm::ptr<PamfStreamHeader_Audio>::make(pSelf->pAddr.addr() + 0x98 + pSelf->stream * 0x30);

			if (size != sizeof(CellPamfLpcmInfo))
			{
				cellPamf->Error("cellPamfReaderGetStreamInfo: wrong LPCM data size(%d)", size);
				return CELL_PAMF_ERROR_INVALID_ARG;
			}

			pInfo->numberOfChannels = pAudio->channels;
			pInfo->samplingFrequency = CELL_PAMF_FS_48kHz;

			if (pAudio->bps == 0x40)
				pInfo->bitsPerSample = CELL_PAMF_BIT_LENGTH_16;
			else
				//TODO: CELL_PAMF_BIT_LENGTH_24
				cellPamf->Error("cellPamfReaderGetStreamInfo: unknown bps(0x%x)", (u8)pAudio->bps);
		}
		break;
	case CELL_PAMF_STREAM_TYPE_USER_DATA: 
		{
			cellPamf->Error("cellPamfReaderGetStreamInfo: CELL_PAMF_STREAM_TYPE_USER_DATA");
			return CELL_PAMF_ERROR_INVALID_ARG;
		}
	}
	
	return CELL_OK;
}

u32 cellPamfReaderGetNumberOfEp(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfEp(pSelf=0x%x, stream=%d)", pSelf.addr(), pSelf->stream);

	// cannot return error code
	return pSelf->pAddr->stream_headers[pSelf->stream].ep_num;
}

s32 cellPamfReaderGetEpIteratorWithIndex(vm::ptr<CellPamfReader> pSelf, u32 epIndex, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf->Todo("cellPamfReaderGetEpIteratorWithIndex(pSelf=0x%x, stream=%d, epIndex=%d, pIt_addr=0x%x)", pSelf.addr(), pSelf->stream, epIndex, pIt.addr());

	// TODO
	return CELL_OK;
}

s32 cellPamfReaderGetEpIteratorWithTimeStamp(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf->Todo("cellPamfReaderGetEpIteratorWithTimeStamp(pSelf=0x%x, pTimeStamp_addr=0x%x, pIt_addr=0x%x)", pSelf.addr(), pTimeStamp.addr(), pIt.addr());

	// TODO
	return CELL_OK;
}

s32 cellPamfEpIteratorGetEp(vm::ptr<CellPamfEpIterator> pIt, vm::ptr<CellPamfEp> pEp)
{
	cellPamf->Todo("cellPamfEpIteratorGetEp(pIt_addr=0x%x, pEp_addr=0x%x)", pIt.addr(), pEp.addr());

	// always returns CELL_OK
	// TODO
	return CELL_OK;
}

s32 cellPamfEpIteratorMove(vm::ptr<CellPamfEpIterator> pIt, s32 steps, vm::ptr<CellPamfEp> pEp)
{
	cellPamf->Todo("cellPamfEpIteratorMove(pIt_addr=0x%x, steps=%d, pEp_addr=0x%x)", pIt.addr(), steps, pEp.addr());

	// cannot return error code
	// TODO
	return 0;
}

void cellPamf_init(Module *pxThis)
{
	cellPamf = pxThis;

	cellPamf->AddFunc(0xca8181c1, cellPamfGetHeaderSize);
	cellPamf->AddFunc(0x90fc9a59, cellPamfGetHeaderSize2);
	cellPamf->AddFunc(0x44f5c9e3, cellPamfGetStreamOffsetAndSize);
	cellPamf->AddFunc(0xd1a40ef4, cellPamfVerify);
	cellPamf->AddFunc(0xb8436ee5, cellPamfReaderInitialize);
	cellPamf->AddFunc(0x4de501b1, cellPamfReaderGetPresentationStartTime);
	cellPamf->AddFunc(0xf61609d6, cellPamfReaderGetPresentationEndTime);
	cellPamf->AddFunc(0xdb70296c, cellPamfReaderGetMuxRateBound);
	cellPamf->AddFunc(0x37f723f7, cellPamfReaderGetNumberOfStreams);
	cellPamf->AddFunc(0xd0230671, cellPamfReaderGetNumberOfSpecificStreams);
	cellPamf->AddFunc(0x461534b4, cellPamfReaderSetStreamWithIndex);
	cellPamf->AddFunc(0x03fd2caa, cellPamfReaderSetStreamWithTypeAndChannel);
	cellPamf->AddFunc(0x28b4e2c1, cellPamfReaderSetStreamWithTypeAndIndex);
	cellPamf->AddFunc(0x01067e22, cellPamfStreamTypeToEsFilterId);
	cellPamf->AddFunc(0x041cc708, cellPamfReaderGetStreamIndex);
	cellPamf->AddFunc(0x9ab20793, cellPamfReaderGetStreamTypeAndChannel);
	cellPamf->AddFunc(0x71df326a, cellPamfReaderGetEsFilterId);
	cellPamf->AddFunc(0x67fd273b, cellPamfReaderGetStreamInfo);
	cellPamf->AddFunc(0xd9ea3457, cellPamfReaderGetNumberOfEp);
	cellPamf->AddFunc(0xe8586ec6, cellPamfReaderGetEpIteratorWithIndex);
	cellPamf->AddFunc(0x439fba17, cellPamfReaderGetEpIteratorWithTimeStamp);
	cellPamf->AddFunc(0x1abeb9d6, cellPamfEpIteratorGetEp);
	cellPamf->AddFunc(0x50b83205, cellPamfEpIteratorMove);
}

