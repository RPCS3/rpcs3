#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
}

#include "cellAdec.h"

void cellAdec_init();
Module cellAdec(0x0006, cellAdec_init);

u32 adecOpen(AudioDecoder* data)
{
	AudioDecoder& adec = *data;

	u32 adec_id = cellAdec.GetNewId(data);

	adec.id = adec_id;

	thread t("Audio Decoder[" + std::to_string(adec_id) + "] Thread", [&]()
	{
		ConLog.Write("Audio Decoder enter()");

		AdecTask task;

		while (true)
		{
			if (Emu.IsStopped())
			{
				break;
			}

			if (adec.job.IsEmpty() && adec.is_running)
			{
				Sleep(1);
				continue;
			}

			if (adec.frames.GetCount() >= 50)
			{
				Sleep(1);
				continue;
			}

			if (!adec.job.Pop(task))
			{
				break;
			}

			switch (task.type)
			{
			case adecStartSeq:
				{
				}
				break;

			case adecEndSeq:
				{
				}
				break;

			case adecDecodeAu:
				{
				}
				break;

			case adecClose:
				{
					adec.is_finished = true;
					ConLog.Write("Audio Decoder exit");
					return;
				}

			default:
				ConLog.Error("Audio Decoder error: unknown task(%d)", task.type);
				return;
			}
		}
		ConLog.Warning("Audio Decoder aborted");
	});

	t.detach();

	return adec_id;
}

bool adecCheckType(AudioCodecType type)
{
	switch (type)
	{
	case CELL_ADEC_TYPE_ATRACX: ConLog.Write("*** (???) type: ATRAC3plus"); break;
	case CELL_ADEC_TYPE_ATRACX_2CH: ConLog.Write("*** type: ATRAC3plus 2ch"); break;

	case CELL_ADEC_TYPE_ATRACX_6CH:
	case CELL_ADEC_TYPE_ATRACX_8CH:
	case CELL_ADEC_TYPE_LPCM_PAMF:
	case CELL_ADEC_TYPE_AC3:
	case CELL_ADEC_TYPE_MP3:
	case CELL_ADEC_TYPE_ATRAC3:
	case CELL_ADEC_TYPE_MPEG_L2:
	case CELL_ADEC_TYPE_CELP:
	case CELL_ADEC_TYPE_M4AAC:
	case CELL_ADEC_TYPE_CELP8:
		cellAdec.Error("Unimplemented audio codec type (%d)", type);
		break;
	default:
		return false;
	}

	return true;
}

int cellAdecQueryAttr(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecAttr> attr)
{
	cellAdec.Warning("cellAdecQueryAttr(type_addr=0x%x, attr_addr=0x%x)", type.GetAddr(), attr.GetAddr());

	if (!type.IsGood() || !attr.IsGood())
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	// TODO: check values
	attr->adecVerLower = 0x280000; // from dmux
	attr->adecVerUpper = 0x260000;
	attr->workMemSize = 4 * 1024 * 1024;

	return CELL_OK;
}

int cellAdecOpen(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResource> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec.Warning("cellAdecOpen(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!type.IsGood() || !res.IsGood() || !cb.IsGood() || !handle.IsGood())
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

int cellAdecOpenEx(mem_ptr_t<CellAdecType> type, mem_ptr_t<CellAdecResourceEx> res, mem_ptr_t<CellAdecCb> cb, mem32_t handle)
{
	cellAdec.Warning("cellAdecOpenEx(type_addr=0x%x, res_addr=0x%x, cb_addr=0x%x, handle_addr=0x%x)", 
		type.GetAddr(), res.GetAddr(), cb.GetAddr(), handle.GetAddr());

	if (!type.IsGood() || !res.IsGood() || !cb.IsGood() || !handle.IsGood())
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (!adecCheckType(type->audioCodecType)) return CELL_ADEC_ERROR_ARG;

	handle = adecOpen(new AudioDecoder(type->audioCodecType, res->startAddr, res->totalMemSize, cb->cbFunc, cb->cbArg));

	return CELL_OK;
}

int cellAdecClose(u32 handle)
{
	cellAdec.Warning("cellAdecClose(handle=%d)", handle);

	AudioDecoder* adec;
	if (!Emu.GetIdManager().GetIDData(handle, adec))
	{
		return CELL_ADEC_ERROR_ARG;
	}

	adec->job.Push(AdecTask(adecClose));

	while (!adec->is_finished || !adec->frames.IsEmpty())
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellAdecClose(%d) aborted", handle);
			break;
		}
		Sleep(1);
	}

	Emu.GetIdManager().RemoveID(handle);
	return CELL_OK;
}

int cellAdecStartSeq(u32 handle, u32 param_addr)
{
	cellAdec.Error("cellAdecStartSeq(handle=%d, param_addr=0x%x)", handle, param_addr);
	return CELL_OK;
}

int cellAdecEndSeq(u32 handle)
{
	cellAdec.Error("cellAdecEndSeq(handle=%d)", handle);
	return CELL_OK;
}

int cellAdecDecodeAu(u32 handle, mem_ptr_t<CellAdecAuInfo> auInfo)
{
	cellAdec.Error("cellAdecDecodeAu(handle=%d, auInfo_addr=0x%x)", handle, auInfo.GetAddr());
	return CELL_OK;
}

int cellAdecGetPcm(u32 handle, u32 outBuffer_addr)
{
	cellAdec.Error("cellAdecGetPcm(handle=%d, outBuffer_addr=0x%x)", handle, outBuffer_addr);
	return CELL_OK;
}

int cellAdecGetPcmItem(u32 handle, u32 pcmItem_ptr_addr)
{
	cellAdec.Error("cellAdecGetPcmItem(handle=%d, pcmItem_ptr_addr=0x%x)", handle, pcmItem_ptr_addr);
	return CELL_OK;
}

void cellAdec_init()
{
	cellAdec.AddFunc(0x7e4a4a49, cellAdecQueryAttr);
	cellAdec.AddFunc(0xd00a6988, cellAdecOpen);
	cellAdec.AddFunc(0x8b5551a4, cellAdecOpenEx);
	cellAdec.AddFunc(0x847d2380, cellAdecClose);
	cellAdec.AddFunc(0x487b613e, cellAdecStartSeq);
	cellAdec.AddFunc(0xe2ea549b, cellAdecEndSeq);
	cellAdec.AddFunc(0x1529e506, cellAdecDecodeAu);
	cellAdec.AddFunc(0x97ff2af1, cellAdecGetPcm);
	cellAdec.AddFunc(0xbd75f78b, cellAdecGetPcmItem);

	av_register_all();
	avcodec_register_all();
}