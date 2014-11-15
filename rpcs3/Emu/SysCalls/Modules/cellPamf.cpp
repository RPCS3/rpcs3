#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "cellPamf.h"

Module *cellPamf = nullptr;

int pamfStreamTypeToEsFilterId(u8 type, u8 ch, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	//TODO: convert type and ch to EsFilterId
	pEsFilterId->filterIdMajor = 0;
	pEsFilterId->filterIdMinor = 0;
	pEsFilterId->supplementalInfo1 = 0;
	pEsFilterId->supplementalInfo2 = 0;

	switch (type)
	{
	case CELL_PAMF_STREAM_TYPE_AVC:
		{
			if (ch < 16)
			{
				pEsFilterId->filterIdMajor = 0xe0 + ch;
				pEsFilterId->filterIdMinor = 0;
				pEsFilterId->supplementalInfo1 = 0x01;
				pEsFilterId->supplementalInfo2 = 0;
			}
			else
				cellPamf->Error("pamfStreamTypeToEsFilterId: invalid CELL_PAMF_STREAM_TYPE_AVC channel (ch=%d)", ch);
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
			cellPamf->Todo("pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_ATRAC3PLUS (ch=%d)", ch);
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
			cellPamf->Todo("pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_LPCM (ch=%d)", ch);
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
			cellPamf->Todo("pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_USER_DATA (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_AC3:
		cellPamf->Todo("pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_AC3 (ch=%d)", ch);
		break;
	case CELL_PAMF_STREAM_TYPE_M2V:
		cellPamf->Todo("pamfStreamTypeToEsFilterId: CELL_PAMF_STREAM_TYPE_M2V (ch=%d)", ch);
		break;
	default:
		return CELL_PAMF_ERROR_INVALID_ARG;
	}
	return CELL_OK;
}

u8 pamfGetStreamType(vm::ptr<CellPamfReader> pSelf, u8 stream)
{
	//TODO: get stream type correctly
	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

	switch (pAddr->stream_headers[stream].type)
	{
	case 0x1b: return CELL_PAMF_STREAM_TYPE_AVC;
	case 0xdc: return CELL_PAMF_STREAM_TYPE_ATRAC3PLUS;
	case 0x80: return CELL_PAMF_STREAM_TYPE_PAMF_LPCM;
	case 0xdd: return CELL_PAMF_STREAM_TYPE_USER_DATA;
	default:
		cellPamf->Todo("pamfGetStreamType: unsupported stream type found(0x%x)", pAddr->stream_headers[stream].type);
		return 0;
	}
}

u8 pamfGetStreamChannel(vm::ptr<CellPamfReader> pSelf, u8 stream)
{
	//TODO: get stream channel correctly
	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

	switch (pAddr->stream_headers[stream].type)
	{
	case 0x1b:
		if ((pAddr->stream_headers[stream].stream_id >= 0xe0) && (pAddr->stream_headers[stream].stream_id <= 0xef))
		{
			return pAddr->stream_headers[stream].stream_id - 0xe0;
		}
		else
		{
			cellPamf->Error("pamfGetStreamChannel: stream type 0x%x got invalid stream id=0x%x", pAddr->stream_headers[stream].type, pAddr->stream_headers[stream].stream_id);
			return 0;
		}
	case 0xdc:
		cellPamf->Todo("pamfGetStreamChannel: CELL_PAMF_STREAM_TYPE_ATRAC3PLUS");
		return 0;
	case 0x80:
		cellPamf->Todo("pamfGetStreamChannel: CELL_PAMF_STREAM_TYPE_PAMF_LPCM");
		return 0;
	case 0xdd:
		cellPamf->Todo("pamfGetStreamChannel: CELL_PAMF_STREAM_TYPE_USER_DATA");
		return 0;
	default:
		cellPamf->Todo("pamfGetStreamType: unsupported stream type found(0x%x)", pAddr->stream_headers[stream].type);
		return 0;
	}

}

int cellPamfGetHeaderSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetHeaderSize(pAddr=0x%x, fileSize=%d, pSize_addr=0x%x)", pAddr.addr(), fileSize, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150)
		//return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

int cellPamfGetHeaderSize2(vm::ptr<PamfHeader> pAddr, u64 fileSize, u32 attribute, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetHeaderSize2(pAddr=0x%x, fileSize=%d, attribute=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, attribute, pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150)
		//return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pSize = offset;
	return CELL_OK;
}

int cellPamfGetStreamOffsetAndSize(vm::ptr<PamfHeader> pAddr, u64 fileSize, vm::ptr<u64> pOffset, vm::ptr<u64> pSize)
{
	cellPamf->Warning("cellPamfGetStreamOffsetAndSize(pAddr=0x%x, fileSize=%d, pOffset_addr=0x%x, pSize_addr=0x%x)", pAddr.addr(), fileSize, pOffset.addr(), pSize.addr());

	//if ((u32)pAddr->magic != 0x464d4150)
		//return CELL_PAMF_ERROR_UNKNOWN_TYPE;

	const u64 offset = (u64)pAddr->data_offset << 11;
	*pOffset = offset;
	const u64 size = (u64)pAddr->data_size << 11;
	*pSize = size;
	return CELL_OK;
}

int cellPamfVerify(vm::ptr<PamfHeader> pAddr, u64 fileSize)
{
	cellPamf->Warning("cellPamfVerify(pAddr=0x%x, fileSize=%d)", pAddr.addr(), fileSize);

	return CELL_OK;
}

int cellPamfReaderInitialize(vm::ptr<CellPamfReader> pSelf, vm::ptr<const PamfHeader> pAddr, u64 fileSize, u32 attribute)
{
	cellPamf->Warning("cellPamfReaderInitialize(pSelf=0x%x, pAddr=0x%x, fileSize=%d, attribute=0x%x)", pSelf.addr(), pAddr.addr(), fileSize, attribute);
	
	if (fileSize)
	{
		pSelf->fileSize = fileSize;
	}
	else //if fileSize is unknown
	{
		pSelf->fileSize = ((u64)pAddr->data_offset << 11) + ((u64)pAddr->data_size << 11);
	}
	pSelf->pAddr = pAddr;

	if (attribute & CELL_PAMF_ATTRIBUTE_VERIFY_ON)
	{
		//TODO
	}

	pSelf->stream = 0; //??? currently set stream
	return CELL_OK;
}

int cellPamfReaderGetPresentationStartTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf->Warning("cellPamfReaderGetPresentationStartTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);
	const u32 upper = (u16)pAddr->start_pts_high;
	pTimeStamp->upper = upper;
	pTimeStamp->lower = pAddr->start_pts_low;
	return CELL_OK;
}

int cellPamfReaderGetPresentationEndTime(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp)
{
	cellPamf->Warning("cellPamfReaderGetPresentationEndTime(pSelf=0x%x, pTimeStamp_addr=0x%x)", pSelf.addr(), pTimeStamp.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);
	const u32 upper = (u16)pAddr->end_pts_high;
	pTimeStamp->upper = upper;
	pTimeStamp->lower = pAddr->end_pts_low;
	return CELL_OK;
}

int cellPamfReaderGetMuxRateBound(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetMuxRateBound(pSelf=0x%x)", pSelf.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);
	return pAddr->mux_rate_max;
}

int cellPamfReaderGetNumberOfStreams(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfStreams(pSelf=0x%x)", pSelf.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);
	return pAddr->stream_count;
}

int cellPamfReaderGetNumberOfSpecificStreams(vm::ptr<CellPamfReader> pSelf, u8 streamType)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfSpecificStreams(pSelf=0x%x, streamType=%d)", pSelf.addr(), streamType);
	
	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

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
		return counts[CELL_PAMF_STREAM_TYPE_ATRAC3PLUS] + counts[CELL_PAMF_STREAM_TYPE_PAMF_LPCM] + counts[CELL_PAMF_STREAM_TYPE_AC3];
	default:
		return 0;
	}
}

int cellPamfReaderSetStreamWithIndex(vm::ptr<CellPamfReader> pSelf, u8 streamIndex)
{
	cellPamf->Warning("cellPamfReaderSetStreamWithIndex(pSelf=0x%x, streamIndex=%d)", pSelf.addr(), streamIndex);

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

	if (streamIndex < pAddr->stream_count)
	{
		pSelf->stream = streamIndex;
		return CELL_OK;
	}
	else
	{
		cellPamf->Error("cellPamfReaderSetStreamWithIndex: CELL_PAMF_ERROR_INVALID_ARG");
		return CELL_PAMF_ERROR_INVALID_ARG;
	}	
}

int cellPamfReaderSetStreamWithTypeAndChannel(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 ch)
{
	cellPamf->Warning("cellPamfReaderSetStreamWithTypeAndChannel(pSelf=0x%x, streamType=%d, ch=%d)", pSelf.addr(), streamType, ch);
	
	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

	if (streamType > 5)
	{
		cellPamf->Error("cellPamfReaderSetStreamWithTypeAndChannel: invalid stream type(%d)", streamType);
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

int cellPamfReaderSetStreamWithTypeAndIndex(vm::ptr<CellPamfReader> pSelf, u8 streamType, u8 streamIndex)
{
	cellPamf->Warning("cellPamfReaderSetStreamWithTypeAndIndex(pSelf=0x%x, streamType=%d, streamIndex=%d)", pSelf.addr(), streamType, streamIndex);

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

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

int cellPamfStreamTypeToEsFilterId(u8 type, u8 ch, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf->Warning("cellPamfStreamTypeToEsFilterId(type=%d, ch=%d, pEsFilterId_addr=0x%x)", type, ch, pEsFilterId.addr());
	
	return pamfStreamTypeToEsFilterId(type, ch, pEsFilterId);
}

int cellPamfReaderGetStreamIndex(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Log("cellPamfReaderGetStreamIndex(pSelf=0x%x)", pSelf.addr());

	return pSelf->stream;
}

int cellPamfReaderGetStreamTypeAndChannel(vm::ptr<CellPamfReader> pSelf, vm::ptr<u8> pType, vm::ptr<u8> pCh)
{
	cellPamf->Warning("cellPamfReaderGetStreamTypeAndChannel(pSelf=0x%x (stream=%d), pType_addr=0x%x, pCh_addr=0x%x",
		pSelf.addr(), pSelf->stream, pType.addr(), pCh.addr());

	*pType = pamfGetStreamType(pSelf, pSelf->stream);
	*pCh = pamfGetStreamChannel(pSelf, pSelf->stream);
	return CELL_OK;
}

int cellPamfReaderGetEsFilterId(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecEsFilterId> pEsFilterId)
{
	cellPamf->Warning("cellPamfReaderGetEsFilterId(pSelf=0x%x (stream=%d), pEsFilterId_addr=0x%x)", pSelf.addr(), pSelf->stream, pEsFilterId.addr());

	return pamfStreamTypeToEsFilterId(pamfGetStreamType(pSelf, pSelf->stream), 
		pamfGetStreamChannel(pSelf, pSelf->stream), pEsFilterId);
}

int cellPamfReaderGetStreamInfo(vm::ptr<CellPamfReader> pSelf, u32 pInfo_addr, u32 size)
{
	cellPamf->Warning("cellPamfReaderGetStreamInfo(pSelf=0x%x, stream=%d, pInfo_addr=0x%x, size=%d)", pSelf.addr(), pSelf->stream, pInfo_addr, size);

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

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
			auto pAudio = vm::ptr<PamfStreamHeader_Audio>::make(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

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
			auto pAudio = vm::ptr<PamfStreamHeader_Audio>::make(pSelf->pAddr + 0x98 + pSelf->stream * 0x30);

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

int cellPamfReaderGetNumberOfEp(vm::ptr<CellPamfReader> pSelf)
{
	cellPamf->Warning("cellPamfReaderGetNumberOfEp(pSelf=0x%x, stream=%d)", pSelf.addr(), pSelf->stream);

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);	
	return pAddr->stream_headers[pSelf->stream].ep_num;
}

int cellPamfReaderGetEpIteratorWithIndex(vm::ptr<CellPamfReader> pSelf, u32 epIndex, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf->Todo("cellPamfReaderGetEpIteratorWithIndex(pSelf=0x%x, stream=%d, epIndex=%d, pIt_addr=0x%x)", pSelf.addr(), pSelf->stream, epIndex, pIt.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);
	//TODO:
	return CELL_OK;
}

int cellPamfReaderGetEpIteratorWithTimeStamp(vm::ptr<CellPamfReader> pSelf, vm::ptr<CellCodecTimeStamp> pTimeStamp, vm::ptr<CellPamfEpIterator> pIt)
{
	cellPamf->Todo("cellPamfReaderGetEpIteratorWithTimeStamp(pSelf=0x%x, pTimeStamp_addr=0x%x, pIt_addr=0x%x)", pSelf.addr(), pTimeStamp.addr(), pIt.addr());

	if (!pSelf->pAddr) {
		return CELL_PAMF_ERROR_INVALID_PAMF;
	}

	vm::ptr<const PamfHeader> pAddr(pSelf->pAddr);

	//TODO:

	return CELL_OK;
}

int cellPamfEpIteratorGetEp(vm::ptr<CellPamfEpIterator> pIt, vm::ptr<CellPamfEp> pEp)
{
	cellPamf->Todo("cellPamfEpIteratorGetEp(pIt_addr=0x%x, pEp_addr=0x%x)", pIt.addr(), pEp.addr());

	//TODO:

	return CELL_OK;
}

int cellPamfEpIteratorMove(vm::ptr<CellPamfEpIterator> pIt, s32 steps, vm::ptr<CellPamfEp> pEp)
{
	cellPamf->Todo("cellPamfEpIteratorMove(pIt_addr=0x%x, steps=%d, pEp_addr=0x%x)", pIt.addr(), steps, pEp.addr());

	//TODO:

	return CELL_OK;
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

