#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

extern Module cellAtrac;

#include "cellAtrac.h"

s32 cellAtracSetDataAndGetMemSize(vm::ptr<CellAtracHandle> pHandle, u32 pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, vm::ptr<u32> puiWorkMemByte)
{
	cellAtrac.Warning("cellAtracSetDataAndGetMemSize(pHandle=0x%x, pucBufferAddr=0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, puiWorkMemByte_addr=0x%x)",
		pHandle.addr(), pucBufferAddr, uiReadByte, uiBufferByte, puiWorkMemByte.addr());

	*puiWorkMemByte = 0x1000; // unproved
	return CELL_OK;
}

s32 cellAtracCreateDecoder(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtrac.Warning("cellAtracCreateDecoder(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, uiSpuThreadPriority);

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

s32 cellAtracCreateDecoderExt(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, vm::ptr<CellAtracExtRes> pExtRes)
{
	cellAtrac.Warning("cellAtracCreateDecoderExt(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, pExtRes_addr=0x%x)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, pExtRes.addr());

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

s32 cellAtracDeleteDecoder(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac.Warning("cellAtracDeleteDecoder(pHandle=0x%x)", pHandle.addr());

	return CELL_OK;
}

s32 cellAtracDecode(vm::ptr<CellAtracHandle> pHandle, u32 pfOutAddr, vm::ptr<u32> puiSamples, vm::ptr<u32> puiFinishflag, vm::ptr<u32> piRemainFrame)
{
	cellAtrac.Warning("cellAtracDecode(pHandle=0x%x, pfOutAddr=0x%x, puiSamples_addr=0x%x, puiFinishFlag_addr=0x%x, piRemainFrame_addr=0x%x)",
		pHandle.addr(), pfOutAddr, puiSamples.addr(), puiFinishflag.addr(), piRemainFrame.addr());

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracGetStreamDataInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> ppucWritePointer, vm::ptr<u32> puiWritableByte, vm::ptr<u32> puiReadPosition)
{
	cellAtrac.Warning("cellAtracGetStreamDataInfo(pHandle=0x%x, ppucWritePointer_addr=0x%x, puiWritableByte_addr=0x%x, puiReadPosition_addr=0x%x)",
		pHandle.addr(), ppucWritePointer.addr(), puiWritableByte.addr(), puiReadPosition.addr());

	*ppucWritePointer = pHandle->data.pucWorkMem_addr;
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracAddStreamData(vm::ptr<CellAtracHandle> pHandle, u32 uiAddByte)
{
	cellAtrac.Warning("cellAtracAddStreamData(pHandle=0x%x, uiAddByte=0x%x)", pHandle.addr(), uiAddByte);

	return CELL_OK;
}

s32 cellAtracGetRemainFrame(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piRemainFrame)
{
	cellAtrac.Warning("cellAtracGetRemainFrame(pHandle=0x%x, piRemainFrame_addr=0x%x)", pHandle.addr(), piRemainFrame.addr());

	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracGetVacantSize(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiVacantSize)
{
	cellAtrac.Warning("cellAtracGetVacantSize(pHandle=0x%x, puiVacantSize_addr=0x%x)", pHandle.addr(), puiVacantSize.addr());

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

s32 cellAtracIsSecondBufferNeeded(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac.Warning("cellAtracIsSecondBufferNeeded(pHandle=0x%x)", pHandle.addr());

	return CELL_OK;
}

s32 cellAtracGetSecondBufferInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiReadPosition, vm::ptr<u32> puiDataByte)
{
	cellAtrac.Warning("cellAtracGetSecondBufferInfo(pHandle=0x%x, puiReadPosition_addr=0x%x, puiDataByte_addr=0x%x)",
		pHandle.addr(), puiReadPosition.addr(), puiDataByte.addr());

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

s32 cellAtracSetSecondBuffer(vm::ptr<CellAtracHandle> pHandle, u32 pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtrac.Warning("cellAtracSetSecondBuffer(pHandle=0x%x, pucSecondBufferAddr=0x%x, uiSecondBufferByte=0x%x)",
		pHandle.addr(), pucSecondBufferAddr, uiSecondBufferByte);

	return CELL_OK;
}

s32 cellAtracGetChannel(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiChannel)
{
	cellAtrac.Warning("cellAtracGetChannel(pHandle=0x%x, puiChannel_addr=0x%x)", pHandle.addr(), puiChannel.addr());

	*puiChannel = 2;
	return CELL_OK;
}

s32 cellAtracGetMaxSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiMaxSample)
{
	cellAtrac.Warning("cellAtracGetMaxSample(pHandle=0x%x, puiMaxSample_addr=0x%x)", pHandle.addr(), puiMaxSample.addr());

	*puiMaxSample = 512;
	return CELL_OK;
}

s32 cellAtracGetNextSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiNextSample)
{
	cellAtrac.Warning("cellAtracGetNextSample(pHandle=0x%x, puiNextSample_addr=0x%x)", pHandle.addr(), puiNextSample.addr());

	*puiNextSample = 0;
	return CELL_OK;
}

s32 cellAtracGetSoundInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piEndSample, vm::ptr<u32> piLoopStartSample, vm::ptr<u32> piLoopEndSample)
{
	cellAtrac.Warning("cellAtracGetSoundInfo(pHandle=0x%x, piEndSample_addr=0x%x, piLoopStartSample_addr=0x%x, piLoopEndSample_addr=0x%x)",
		pHandle.addr(), piEndSample.addr(), piLoopStartSample.addr(), piLoopEndSample.addr());

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

s32 cellAtracGetNextDecodePosition(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiSamplePosition)
{
	cellAtrac.Warning("cellAtracGetNextDecodePosition(pHandle=0x%x, puiSamplePosition_addr=0x%x)",
		pHandle.addr(), puiSamplePosition.addr());

	*puiSamplePosition = 0;
	return CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED;
}

s32 cellAtracGetBitrate(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiBitrate)
{
	cellAtrac.Warning("cellAtracGetBitrate(pHandle=0x%x, puiBitrate_addr=0x%x)",
		pHandle.addr(), puiBitrate.addr());

	*puiBitrate = 128;
	return CELL_OK;
}

s32 cellAtracGetLoopInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piLoopNum, vm::ptr<u32> puiLoopStatus)
{
	cellAtrac.Warning("cellAtracGetLoopInfo(pHandle=0x%x, piLoopNum_addr=0x%x, puiLoopStatus_addr=0x%x)",
		pHandle.addr(), piLoopNum.addr(), puiLoopStatus.addr());

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

s32 cellAtracSetLoopNum(vm::ptr<CellAtracHandle> pHandle, int iLoopNum)
{
	cellAtrac.Warning("cellAtracSetLoopNum(pHandle=0x%x, iLoopNum=0x%x)", pHandle.addr(), iLoopNum);

	return CELL_OK;
}

s32 cellAtracGetBufferInfoForResetting(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, vm::ptr<CellAtracBufferInfo> pBufferInfo)
{
	cellAtrac.Warning("cellAtracGetBufferInfoForResetting(pHandle=0x%x, uiSample=0x%x, pBufferInfo_addr=0x%x)",
		pHandle.addr(), uiSample, pBufferInfo.addr());

	pBufferInfo->pucWriteAddr = pHandle->data.pucWorkMem_addr;
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracResetPlayPosition(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, u32 uiWriteByte)
{
	cellAtrac.Warning("cellAtracResetPlayPosition(pHandle=0x%x, uiSample=0x%x, uiWriteByte=0x%x)",
		pHandle.addr(), uiSample, uiWriteByte);

	return CELL_OK;
}

s32 cellAtracGetInternalErrorInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> piResult)
{
	cellAtrac.Warning("cellAtracGetInternalErrorInfo(pHandle=0x%x, piResult_addr=0x%x)",
		pHandle.addr(), piResult.addr());

	*piResult = 0;
	return CELL_OK;
}

Module cellAtrac("cellAtrac", []()
{
	REG_FUNC(cellAtrac, cellAtracSetDataAndGetMemSize);

	REG_FUNC(cellAtrac, cellAtracCreateDecoder);
	REG_FUNC(cellAtrac, cellAtracCreateDecoderExt);
	REG_FUNC(cellAtrac, cellAtracDeleteDecoder);

	REG_FUNC(cellAtrac, cellAtracDecode);

	REG_FUNC(cellAtrac, cellAtracGetStreamDataInfo);
	REG_FUNC(cellAtrac, cellAtracAddStreamData);
	REG_FUNC(cellAtrac, cellAtracGetRemainFrame);
	REG_FUNC(cellAtrac, cellAtracGetVacantSize);
	REG_FUNC(cellAtrac, cellAtracIsSecondBufferNeeded);
	REG_FUNC(cellAtrac, cellAtracGetSecondBufferInfo);
	REG_FUNC(cellAtrac, cellAtracSetSecondBuffer);

	REG_FUNC(cellAtrac, cellAtracGetChannel);
	REG_FUNC(cellAtrac, cellAtracGetMaxSample);
	REG_FUNC(cellAtrac, cellAtracGetNextSample);
	REG_FUNC(cellAtrac, cellAtracGetSoundInfo);
	REG_FUNC(cellAtrac, cellAtracGetNextDecodePosition);
	REG_FUNC(cellAtrac, cellAtracGetBitrate);

	REG_FUNC(cellAtrac, cellAtracGetLoopInfo);
	REG_FUNC(cellAtrac, cellAtracSetLoopNum);

	REG_FUNC(cellAtrac, cellAtracGetBufferInfoForResetting);
	REG_FUNC(cellAtrac, cellAtracResetPlayPosition);

	REG_FUNC(cellAtrac, cellAtracGetInternalErrorInfo);
});
