#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "cellPamf.h"
#include "cellDmux.h"

void cellDmux_init();
Module cellDmux(0x0007, cellDmux_init);

int cellDmuxQueryAttr(mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Error("cellDmuxQueryAttr(demuxerType_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType.GetAddr(), demuxerAttr.GetAddr());
	return CELL_OK;
}

int cellDmuxQueryAttr2(mem_ptr_t<CellDmuxType2> demuxerType2, mem_ptr_t<CellDmuxAttr> demuxerAttr)
{
	cellDmux.Error("cellDmuxQueryAttr2(demuxerType2_addr=0x%x, demuxerAttr_addr=0x%x)", demuxerType2.GetAddr(), demuxerAttr.GetAddr());
	return CELL_OK;
}

int cellDmuxOpen(mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellDmuxResource> demuxerResource, 
				 mem_ptr_t<CellDmuxCb> demuxerCb, u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxOpen(demuxerType_addr=0x%x, demuxerResource_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResource.GetAddr(), demuxerCb.GetAddr(), demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxOpenEx(mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellDmuxResourceEx> demuxerResourceEx, 
				   mem_ptr_t<CellDmuxCb> demuxerCb, u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxOpenEx(demuxerType_addr=0x%x, demuxerResourceEx_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType.GetAddr(), demuxerResourceEx.GetAddr(), demuxerCb.GetAddr(), demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxOpen2(mem_ptr_t<CellDmuxType2> demuxerType2, u32 demuxerResource2_addr, 
				  mem_ptr_t<CellDmuxCb> demuxerCb, u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxOpen2(demuxerType2_addr=0x%x, demuxerResource2_addr=0x%x, demuxerCb_addr=0x%x, demuxerHandle_addr=0x%x)",
		demuxerType2.GetAddr(), demuxerResource2_addr, demuxerCb.GetAddr(), demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxClose(u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxClose(demuxerHandle_addr=0x%x)", demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxSetStream(u32 demuxerHandle_addr, u32 streamAddress, u32 streamSize, bool discontinuity, u64 userData)
{
	cellDmux.Error("cellDmuxSetStream(demuxerHandle_addr=0x%x, streamAddress=0x%x, streamSize=%d, discontinuity=%d, userData=0x%llx",
		demuxerHandle_addr, streamAddress, streamSize, discontinuity, userData);
	return CELL_OK;
}

int cellDmuxResetStream(u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxResetStream(demuxerHandle_addr=0x%x)", demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxResetStreamAndWaitDone(u32 demuxerHandle_addr)
{
	cellDmux.Error("cellDmuxResetStreamAndWaitDone(demuxerHandle_addr=0x%x)", demuxerHandle_addr);
	return CELL_OK;
}

int cellDmuxQueryEsAttr(mem_ptr_t<CellDmuxType> demuxerType, mem_ptr_t<CellCodecEsFilterId> esFilterId,
						u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Error("cellDmuxQueryEsAttr(demuxerType_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());
	return CELL_OK;
}

int cellDmuxQueryEsAttr2(mem_ptr_t<CellDmuxType2> demuxerType2, mem_ptr_t<CellCodecEsFilterId> esFilterId,
						 u32 esSpecificInfo_addr, mem_ptr_t<CellDmuxEsAttr> esAttr)
{
	cellDmux.Error("cellDmuxQueryEsAttr2(demuxerType2_addr=0x%x, esFilterId_addr=0x%x, esSpecificInfo_addr=0x%x, esAttr_addr=0x%x)",
		demuxerType2.GetAddr(), esFilterId.GetAddr(), esSpecificInfo_addr, esAttr.GetAddr());
	return CELL_OK;
}

int cellDmuxEnableEs(u32 demuxerHandle_addr, mem_ptr_t<CellCodecEsFilterId> esFilterId, 
					 mem_ptr_t<CellDmuxEsResource> esResourceInfo, mem_ptr_t<CellDmuxEsCb> esCb,
					 u32 esSpecificInfo_addr, u32 esHandle_addr)
{
	cellDmux.Error("cellDmuxEnableEs(demuxerHandle_addr=0x%x, esFilterId_addr=0x%x, esResourceInfo_addr=0x%x, esCb_addr=0x%x, "
		"esSpecificInfo_addr=0x%x, esHandle_addr=0x%x)", demuxerHandle_addr, esFilterId.GetAddr(), esResourceInfo.GetAddr(),
		esCb.GetAddr(), esSpecificInfo_addr, esHandle_addr);
	return CELL_OK;
}

int cellDmuxDisableEs(u32 esHandle_addr)
{
	cellDmux.Error("cellDmuxDisableEs(esHandle_addr=0x%x)", esHandle_addr);
	return CELL_OK;
}

int cellDmuxResetEs(u32 esHandle_addr)
{
	cellDmux.Error("cellDmuxResetEs(esHandle_addr=0x%x)", esHandle_addr);
	return CELL_OK;
}

int cellDmuxGetAu(u32 esHandle_addr, u32 auInfo_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxGetAu(esHandle_addr=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle_addr, auInfo_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxPeekAu(u32 esHandle_addr, u32 auInfo_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxPeekAu(esHandle_addr=0x%x, auInfo_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle_addr, auInfo_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxGetAuEx(u32 esHandle_addr, u32 auInfoEx_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxGetAuEx(esHandle_addr=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle_addr, auInfoEx_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxPeekAuEx(u32 esHandle_addr, u32 auInfoEx_ptr_addr, u32 auSpecificInfo_ptr_addr)
{
	cellDmux.Error("cellDmuxPeekAuEx(esHandle_addr=0x%x, auInfoEx_ptr_addr=0x%x, auSpecificInfo_ptr_addr=0x%x)",
		esHandle_addr, auInfoEx_ptr_addr, auSpecificInfo_ptr_addr);
	return CELL_OK;
}

int cellDmuxReleaseAu(u32 esHandle_addr)
{
	cellDmux.Error("cellDmuxReleaseAu(esHandle_addr=0x%x)", esHandle_addr);
	return CELL_OK;
}

int cellDmuxFlushEs(u32 esHandle_addr)
{
	cellDmux.Error("cellDmuxFlushEs(esHandle_addr=0x%x)", esHandle_addr);
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