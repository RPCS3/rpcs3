#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

Module *cellAtrac = nullptr;

#include "cellAtrac.h"

#ifdef PRX_DEBUG
#include "prx_libatrac3plus.h"
u32 libatrac3plus;
u32 libatrac3plus_rtoc;
#endif

s32 cellAtracSetDataAndGetMemSize(vm::ptr<CellAtracHandle> pHandle, u32 pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, vm::ptr<u32> puiWorkMemByte)
{
	cellAtrac->Todo("cellAtracSetDataAndGetMemSize(pHandle=0x%x, pucBufferAddr=0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, puiWorkMemByte_addr=0x%x)",
		pHandle.addr(), pucBufferAddr, uiReadByte, uiBufferByte, puiWorkMemByte.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x11F4, libatrac3plus_rtoc);
#endif

	*puiWorkMemByte = 0x1000; // unproved
	return CELL_OK;
}

s32 cellAtracCreateDecoder(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtrac->Todo("cellAtracCreateDecoder(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, uiSpuThreadPriority);
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0FF0, libatrac3plus_rtoc);
#endif

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

s32 cellAtracCreateDecoderExt(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, vm::ptr<CellAtracExtRes> pExtRes)
{
	cellAtrac->Todo("cellAtracCreateDecoderExt(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, pExtRes_addr=0x%x)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, pExtRes.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0DB0, libatrac3plus_rtoc);
#endif

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

s32 cellAtracDeleteDecoder(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac->Todo("cellAtracDeleteDecoder(pHandle=0x%x)", pHandle.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0D08, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracDecode(vm::ptr<CellAtracHandle> pHandle, u32 pfOutAddr, vm::ptr<u32> puiSamples, vm::ptr<u32> puiFinishflag, vm::ptr<u32> piRemainFrame)
{
	cellAtrac->Todo("cellAtracDecode(pHandle=0x%x, pfOutAddr=0x%x, puiSamples_addr=0x%x, puiFinishFlag_addr=0x%x, piRemainFrame_addr=0x%x)",
		pHandle.addr(), pfOutAddr, puiSamples.addr(), puiFinishflag.addr(), piRemainFrame.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x09A8, libatrac3plus_rtoc);
#endif

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracGetStreamDataInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> ppucWritePointer, vm::ptr<u32> puiWritableByte, vm::ptr<u32> puiReadPosition)
{
	cellAtrac->Todo("cellAtracGetStreamDataInfo(pHandle=0x%x, ppucWritePointer_addr=0x%x, puiWritableByte_addr=0x%x, puiReadPosition_addr=0x%x)",
		pHandle.addr(), ppucWritePointer.addr(), puiWritableByte.addr(), puiReadPosition.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0BE8, libatrac3plus_rtoc);
#endif

	*ppucWritePointer = pHandle->data.pucWorkMem_addr;
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracAddStreamData(vm::ptr<CellAtracHandle> pHandle, u32 uiAddByte)
{
	cellAtrac->Todo("cellAtracAddStreamData(pHandle=0x%x, uiAddByte=0x%x)", pHandle.addr(), uiAddByte);
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0AFC, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracGetRemainFrame(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piRemainFrame)
{
	cellAtrac->Todo("cellAtracGetRemainFrame(pHandle=0x%x, piRemainFrame_addr=0x%x)", pHandle.addr(), piRemainFrame.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x092C, libatrac3plus_rtoc);
#endif

	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracGetVacantSize(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiVacantSize)
{
	cellAtrac->Todo("cellAtracGetVacantSize(pHandle=0x%x, puiVacantSize_addr=0x%x)", pHandle.addr(), puiVacantSize.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x08B0, libatrac3plus_rtoc);
#endif

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

s32 cellAtracIsSecondBufferNeeded(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac->Todo("cellAtracIsSecondBufferNeeded(pHandle=0x%x)", pHandle.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0010, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracGetSecondBufferInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiReadPosition, vm::ptr<u32> puiDataByte)
{
	cellAtrac->Todo("cellAtracGetSecondBufferInfo(pHandle=0x%x, puiReadPosition_addr=0x%x, puiDataByte_addr=0x%x)",
		pHandle.addr(), puiReadPosition.addr(), puiDataByte.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x07E8, libatrac3plus_rtoc);
#endif

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

s32 cellAtracSetSecondBuffer(vm::ptr<CellAtracHandle> pHandle, u32 pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtrac->Todo("cellAtracSetSecondBuffer(pHandle=0x%x, pucSecondBufferAddr=0x%x, uiSecondBufferByte=0x%x)",
		pHandle.addr(), pucSecondBufferAddr, uiSecondBufferByte);
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0704, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracGetChannel(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiChannel)
{
	cellAtrac->Todo("cellAtracGetChannel(pHandle=0x%x, puiChannel_addr=0x%x)", pHandle.addr(), puiChannel.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0060, libatrac3plus_rtoc);
#endif

	*puiChannel = 2;
	return CELL_OK;
}

s32 cellAtracGetMaxSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiMaxSample)
{
	cellAtrac->Todo("cellAtracGetMaxSample(pHandle=0x%x, puiMaxSample_addr=0x%x)", pHandle.addr(), puiMaxSample.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x00AC, libatrac3plus_rtoc);
#endif

	*puiMaxSample = 512;
	return CELL_OK;
}

s32 cellAtracGetNextSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiNextSample)
{
	cellAtrac->Todo("cellAtracGetNextSample(pHandle=0x%x, puiNextSample_addr=0x%x)", pHandle.addr(), puiNextSample.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0688, libatrac3plus_rtoc);
#endif

	*puiNextSample = 0;
	return CELL_OK;
}

s32 cellAtracGetSoundInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piEndSample, vm::ptr<u32> piLoopStartSample, vm::ptr<u32> piLoopEndSample)
{
	cellAtrac->Todo("cellAtracGetSoundInfo(pHandle=0x%x, piEndSample_addr=0x%x, piLoopStartSample_addr=0x%x, piLoopEndSample_addr=0x%x)",
		pHandle.addr(), piEndSample.addr(), piLoopStartSample.addr(), piLoopEndSample.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0104, libatrac3plus_rtoc);
#endif

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

s32 cellAtracGetNextDecodePosition(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiSamplePosition)
{
	cellAtrac->Todo("cellAtracGetNextDecodePosition(pHandle=0x%x, puiSamplePosition_addr=0x%x)",
		pHandle.addr(), puiSamplePosition.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0190, libatrac3plus_rtoc);
#endif

	*puiSamplePosition = 0;
	return CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED;
}

s32 cellAtracGetBitrate(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiBitrate)
{
	cellAtrac->Todo("cellAtracGetBitrate(pHandle=0x%x, puiBitrate_addr=0x%x)",
		pHandle.addr(), puiBitrate.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x0374, libatrac3plus_rtoc);
#endif

	*puiBitrate = 128;
	return CELL_OK;
}

s32 cellAtracGetLoopInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piLoopNum, vm::ptr<u32> puiLoopStatus)
{
	cellAtrac->Todo("cellAtracGetLoopInfo(pHandle=0x%x, piLoopNum_addr=0x%x, puiLoopStatus_addr=0x%x)",
		pHandle.addr(), piLoopNum.addr(), puiLoopStatus.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x025C, libatrac3plus_rtoc);
#endif

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

s32 cellAtracSetLoopNum(vm::ptr<CellAtracHandle> pHandle, int iLoopNum)
{
	cellAtrac->Todo("cellAtracSetLoopNum(pHandle=0x%x, iLoopNum=0x%x)", pHandle.addr(), iLoopNum);
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x1538, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracGetBufferInfoForResetting(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, vm::ptr<CellAtracBufferInfo> pBufferInfo)
{
	cellAtrac->Todo("cellAtracGetBufferInfoForResetting(pHandle=0x%x, uiSample=0x%x, pBufferInfo_addr=0x%x)",
		pHandle.addr(), uiSample, pBufferInfo.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x05BC, libatrac3plus_rtoc);
#endif

	pBufferInfo->pucWriteAddr = pHandle->data.pucWorkMem_addr;
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracResetPlayPosition(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, u32 uiWriteByte)
{
	cellAtrac->Todo("cellAtracResetPlayPosition(pHandle=0x%x, uiSample=0x%x, uiWriteByte=0x%x)",
		pHandle.addr(), uiSample, uiWriteByte);
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x04E4, libatrac3plus_rtoc);
#endif

	return CELL_OK;
}

s32 cellAtracGetInternalErrorInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piResult)
{
	cellAtrac->Todo("cellAtracGetInternalErrorInfo(pHandle=0x%x, piResult_addr=0x%x)",
		pHandle.addr(), piResult.addr());
#ifdef PRX_DEBUG
	return GetCurrentPPUThread().FastCall2(libatrac3plus + 0x02E4, libatrac3plus_rtoc);
#endif

	*piResult = 0;
	return CELL_OK;
}

void cellAtrac_init(Module *pxThis)
{
	cellAtrac = pxThis;

	cellAtrac->AddFunc(0x66afc68e, cellAtracSetDataAndGetMemSize);

	cellAtrac->AddFunc(0xfa293e88, cellAtracCreateDecoder);
	cellAtrac->AddFunc(0x2642d4cc, cellAtracCreateDecoderExt);
	cellAtrac->AddFunc(0x761cb9be, cellAtracDeleteDecoder);

	cellAtrac->AddFunc(0x8eb0e65f, cellAtracDecode);

	cellAtrac->AddFunc(0x2bfff084, cellAtracGetStreamDataInfo);
	cellAtrac->AddFunc(0x46cfc013, cellAtracAddStreamData);
	cellAtrac->AddFunc(0xdfab73aa, cellAtracGetRemainFrame);
	cellAtrac->AddFunc(0xc9a95fcb, cellAtracGetVacantSize);
	cellAtrac->AddFunc(0x99efe171, cellAtracIsSecondBufferNeeded);
	cellAtrac->AddFunc(0xbe07f05e, cellAtracGetSecondBufferInfo);
	cellAtrac->AddFunc(0x06ddb53e, cellAtracSetSecondBuffer);

	cellAtrac->AddFunc(0x0f9667b6, cellAtracGetChannel);
	cellAtrac->AddFunc(0x5f62d546, cellAtracGetMaxSample);
	cellAtrac->AddFunc(0x4797d1ff, cellAtracGetNextSample);
	cellAtrac->AddFunc(0xcf01d5d4, cellAtracGetSoundInfo);
	cellAtrac->AddFunc(0x7b22e672, cellAtracGetNextDecodePosition);
	cellAtrac->AddFunc(0x006016da, cellAtracGetBitrate);

	cellAtrac->AddFunc(0xab6b6dbf, cellAtracGetLoopInfo);
	cellAtrac->AddFunc(0x78ba5c41, cellAtracSetLoopNum);

	cellAtrac->AddFunc(0x99fb73d1, cellAtracGetBufferInfoForResetting);
	cellAtrac->AddFunc(0x7772eb2b, cellAtracResetPlayPosition);

	cellAtrac->AddFunc(0xb5c11938, cellAtracGetInternalErrorInfo);

#ifdef PRX_DEBUG
	CallAfter([]()
	{
		libatrac3plus = (u32)Memory.MainMem.AllocAlign(sizeof(libatrac3plus_data), 0x100000);
		memcpy(vm::get_ptr<void>(libatrac3plus), libatrac3plus_data, sizeof(libatrac3plus_data));
		libatrac3plus_rtoc = libatrac3plus + 0xBED0;

		extern Module* cellAdec;

		FIX_IMPORT(cellAdec, cellAdecDecodeAu,  libatrac3plus + 0x399C);
		FIX_IMPORT(cellAdec, cellAdecStartSeq,  libatrac3plus + 0x39BC);
		FIX_IMPORT(cellAdec, cellAdecQueryAttr, libatrac3plus + 0x39DC);
		FIX_IMPORT(cellAdec, cellAdecClose,     libatrac3plus + 0x39FC);
		FIX_IMPORT(cellAdec, cellAdecGetPcm,    libatrac3plus + 0x3A1C);
		FIX_IMPORT(cellAdec, cellAdecOpen,      libatrac3plus + 0x3A3C);
		fix_import(cellAdec, 0xDF982D2C,        libatrac3plus + 0x3A5C);

		fix_relocs(cellAtrac, libatrac3plus, 0x3EF0, 0x5048, 0x3CE0);
	});
#endif
}
