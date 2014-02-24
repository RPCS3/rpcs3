#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"
#include "cellDmux.h"

extern "C"
{
#include "libavformat\avformat.h"
}

void cellDmux_init();
Module cellDmux(0x0007, cellDmux_init);

void dmuxQueryAttr(u32 info_addr /* may be 0 */, mem_ptr_t<CellDmuxAttr> attr)
{
	attr->demuxerVerLower = 0; // TODO: check values
	attr->demuxerVerUpper = 0;
	attr->memSize = 1024 * 1024; // 1M
}

void dmuxQueryEsAttr(u32 info_addr /* may be 0 */, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
					 const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> attr)
{
	attr->memSize = 1024 * 1024;
}

int cellDmuxQueryAttr(const mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr(demuxerType_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType.GetAddr(), demuxerAttr.GetAddr());

	if (!demuxerType.IsGood() || !demuxerAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(0, demuxerAttr);
	return CELL_OK;
}

int cellDmuxQueryAttr2(const mem_ptr_t<CellDmuxType2> demuxerType2, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Warning("cellDmuxQueryAttr2(demuxerType2_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType2.GetAddr(), demuxerAttr.GetAddr());

	if (!demuxerType2.IsGood() || !demuxerAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmuxQueryAttr(demuxerType2->streamSpecificInfo_addr, demuxerAttr);
	return CELL_OK;
}

u32 dmuxOpen(Demuxer* data)
{
	Demuxer& dmux = *data;

	u32 id = cellDmux.GetNewId(data);

	thread t("Demuxer [" + std::to_string(id) + "] Thread", [&]()
	{
		ConLog.Write("Demuxer enter (mem=0x%x, size=0x%x, cb=0x%x, arg=0x%x)", dmux.memAddr, dmux.memSize, dmux.cbFunc, dmux.cbArg);

		DemuxerTask task;

		while (true)
		{
			if (!dmux.job.Pop(task))
			{
				break;
			}

			switch (task.type)
			{
			case dmuxSetStream:
			case dmuxResetStream:
			case dmuxEnableEs:
			case dmuxDisableEs:
			case dmuxResetEs:
			case dmuxGetAu:
			case dmuxPeekAu:
			case dmuxReleaseAu:
			case dmuxFlushEs:
			case dmuxClose:
				dmux.is_finished = true;
				ConLog.Write("Demuxer exit");
				return;
			default:
				ConLog.Error("Demuxer error: unknown task(%d)", task.type);
			}
		}

		ConLog.Warning("Demuxer aborted");
	});

	t.detach();

	return id;
}

int cellDmuxOpen(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellDmuxResource> demuxerResource, 
				 const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen(demuxerType_addr=0x%x, demuxerResource_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResource.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType.IsGood() || !demuxerResource.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResource->memAddr, demuxerResource->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerResource and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResource->memAddr, demuxerResource->memSize, (CellDmuxCbMsg&)demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxOpenEx(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellDmuxResourceEx> demuxerResourceEx, 
				   const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpenEx(demuxerType_addr=0x%x, demuxerResourceEx_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResourceEx.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType.IsGood() || !demuxerResourceEx.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResourceEx->memAddr, demuxerResourceEx->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerResourceEx and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResourceEx->memAddr, demuxerResourceEx->memSize, (CellDmuxCbMsg&)demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxOpen2(const mem_ptr_t<CellDmuxType2> demuxerType2, const mem_ptr_t<CellDmuxResource2> demuxerResource2, 
				  const mem_ptr_t<CellDmuxCb> demuxerCb, mem32_t demuxerHandle)
{
	cellDmux.Warning("cellDmuxOpen2(demuxerType2_addr=0x%x, demuxerResource2_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType2.GetAddr(), demuxerResource2.GetAddr(), demuxerCb.GetAddr(), demuxerHandle.GetAddr());

	if (!demuxerType2.IsGood() || !demuxerResource2.IsGood() || !demuxerCb.IsGood() || !demuxerHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(demuxerResource2->memAddr, demuxerResource2->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	// TODO: check demuxerType2, demuxerResource2 and demuxerCb arguments

	demuxerHandle = dmuxOpen(new Demuxer(demuxerResource2->memAddr, demuxerResource2->memSize, (CellDmuxCbMsg&)demuxerCb->cbMsgFunc, demuxerCb->cbArg_addr));

	return CELL_OK;
}

int cellDmuxClose(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxClose(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxClose));

	while (!dmux->is_finished)
	{
		if (Emu.IsStopped())
		{
			ConLog.Warning("cellDmuxClose(%d) aborted", demuxerHandle);
			return CELL_OK;
		}

		Sleep(1);
	}

	Emu.GetIdManager().RemoveID(demuxerHandle);
	return CELL_OK;
}

int cellDmuxSetStream(u32 demuxerHandle, const u32 streamAddress, u32 streamSize, bool discontinuity, u64 userData)
{
	cellDmux.Warning("cellDmuxSetStream(demuxerHandle=%d, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx",
		demuxerHandle, streamAddress, streamSize, discontinuity, userData);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	if (!Memory.IsGoodAddr(streamAddress, streamSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!dmux->job.IsEmpty())
	{
		return CELL_DMUX_ERROR_BUSY;
	}

	DemuxerTask task(dmuxSetStream);
	auto& info = task.stream;
	info.addr = streamAddress;
	info.size = streamSize;
	info.discontinuity = discontinuity;
	info.userdata = userData;

	dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxResetStream(u32 demuxerHandle)
{
	cellDmux.Warning("cellDmuxResetStream(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStream));

	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone(u32 demuxerHandle)
{
	cellDmux.Error("cellDmuxResetStreamAndWaitDone(demuxerHandle=%d)", demuxerHandle);

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	dmux->job.Push(DemuxerTask(dmuxResetStream));

	// TODO: wait done

	return CELL_OK;
}

int cellDmuxQueryEsAttr(const mem_ptr_t<CellDmuxType> demuxerType, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
						const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Warning("cellDmuxQueryEsAttr(demuxerType_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());

	if (!demuxerType.IsGood() || !esFilterId.IsGood() || !esAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxQueryEsAttr: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId and esSpecificInfo correctly

	dmuxQueryEsAttr(0, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxQueryEsAttr2(const mem_ptr_t<CellDmuxType2> demuxerType2, const mem_ptr_t<CellCodecEsFilterId> esFilterId,
						 const u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Error("cellDmuxQueryEsAttr2(demuxerType2_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType2.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());

	if (!demuxerType2.IsGood() || !esFilterId.IsGood() || !esAttr.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxQueryEsAttr2: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (demuxerType2->streamType != CELL_DMUX_STREAM_TYPE_PAMF)
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check demuxerType2, esFilterId and esSpecificInfo correctly

	dmuxQueryEsAttr(demuxerType2->streamSpecificInfo_addr, esFilterId, esSpecificInfo_addr, esAttr);
	return CELL_OK;
}

int cellDmuxEnableEs(u32 demuxerHandle, const mem_ptr_t<CellCodecEsFilterId> esFilterId, 
					 const mem_ptr_t<CellDmuxEsResource> esResourceInfo, const mem_ptr_t<CellDmuxEsCb> esCb,
					 const u32 esSpecificInfo_addr, mem32_t esHandle)
{
	cellDmux.Warning("cellDmuxEnableEs(demuxerHandle=%d, esFilterId_addr=0x%x, esResourceInfo_addr=0x%x, esCb_addr=0x%x, "
		"esSpecificInfo_addr=0x%x, esHandle_addr=0x%x)", demuxerHandle, esFilterId.GetAddr(), esResourceInfo.GetAddr(),
		esCb.GetAddr(), esSpecificInfo_addr, esHandle.GetAddr());

	if (!esFilterId.IsGood() || !esResourceInfo.IsGood() || !esCb.IsGood() || !esHandle.IsGood())
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esSpecificInfo_addr, 12))
	{
		cellDmux.Error("cellDmuxEnableEs: invalid specific info addr (0x%x)", esSpecificInfo_addr);
		return CELL_DMUX_ERROR_FATAL;
	}

	if (!Memory.IsGoodAddr(esResourceInfo->memAddr, esResourceInfo->memSize))
	{
		return CELL_DMUX_ERROR_FATAL;
	}

	Demuxer* dmux;
	if (!Emu.GetIdManager().GetIDData(demuxerHandle, dmux))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	// TODO: check esFilterId, esResourceInfo, esCb and esSpecificInfo correctly

	ElementaryStream* es = new ElementaryStream(dmux, esResourceInfo->memAddr, esResourceInfo->memSize,
		esFilterId->filterIdMajor, esFilterId->filterIdMinor, esFilterId->supplementalInfo1, esFilterId->supplementalInfo2,
		(CellDmuxCbEsMsg&)esCb->cbEsMsgFunc, esCb->cbArg_addr, esSpecificInfo_addr);

	u32 id = cellDmux.GetNewId(es);

	DemuxerTask task(dmuxEnableEs);
	task.au.es = id;
	task.au.es_ptr = es;

	dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxDisableEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxDisableEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxDisableEs);
	task.esHandle = esHandle;
	task.au.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxResetEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxResetEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxResetEs);
	task.esHandle = esHandle;
	task.au.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxGetAu(u32 esHandle, const u32 auInfo_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxGetAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxPeekAu(u32 esHandle, const u32 auInfo_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxPeekAu(esHandle=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfo_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxGetAuEx(u32 esHandle, const u32 auInfoEx_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxGetAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxPeekAuEx(u32 esHandle, const u32 auInfoEx_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxPeekAuEx(esHandle=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle, auInfoEx_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxReleaseAu(u32 esHandle)
{
	cellDmux.Warning("cellDmuxReleaseAu(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxReleaseAu);
	task.esHandle = esHandle;
	task.au.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

int cellDmuxFlushEs(u32 esHandle)
{
	cellDmux.Warning("cellDmuxFlushEs(esHandle=0x%x)", esHandle);

	ElementaryStream* es;
	if (!Emu.GetIdManager().GetIDData(esHandle, es))
	{
		return CELL_DMUX_ERROR_ARG;
	}

	DemuxerTask task(dmuxFlushEs);
	task.esHandle = esHandle;
	task.au.es_ptr = es;

	es->dmux->job.Push(task);
	return CELL_OK;
}

void cellDmux_init()
{
	cellDmux.AddFunc(0xa2d4189b, cellDmuxQueryAttr);
	cellDmux.AddFunc(0x3f76e3cd, cellDmuxQueryAttr2);
	cellDmux.AddFunc(0x68492de9, cellDmuxOpen);
	cellDmux.AddFunc(0xf6c23560, cellDmuxOpenEx);
	cellDmux.AddFunc(0x11bc3a6c, cellDmuxOpen2);
	cellDmux.AddFunc(0x8c692521, cellDmuxClose);
	cellDmux.AddFunc(0x04e7499f, cellDmuxSetStream);
	cellDmux.AddFunc(0x5d345de9, cellDmuxResetStream);
	cellDmux.AddFunc(0xccff1284, cellDmuxResetStreamAndWaitDone);
	cellDmux.AddFunc(0x02170d1a, cellDmuxQueryEsAttr);
	cellDmux.AddFunc(0x52911bcf, cellDmuxQueryEsAttr2);
	cellDmux.AddFunc(0x7b56dc3f, cellDmuxEnableEs);
	cellDmux.AddFunc(0x05371c8d, cellDmuxDisableEs);
	cellDmux.AddFunc(0x21d424f0, cellDmuxResetEs);
	cellDmux.AddFunc(0x42c716b5, cellDmuxGetAu);
	cellDmux.AddFunc(0x2750c5e0, cellDmuxPeekAu);
	cellDmux.AddFunc(0x2c9a5857, cellDmuxGetAuEx);
	cellDmux.AddFunc(0x002e8da2, cellDmuxPeekAuEx);
	cellDmux.AddFunc(0x24ea6474, cellDmuxReleaseAu);
	cellDmux.AddFunc(0xebb3b2bd, cellDmuxFlushEs);
}