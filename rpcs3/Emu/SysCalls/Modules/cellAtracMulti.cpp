#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellAtracMulti.h"

s32 cellAtracMultiSetDataAndGetMemSize(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, u32 uiOutputChNum, vm::ptr<s32> piTrackArray, vm::ptr<u32> puiWorkMemByte)
{
	cellAtracMulti.Warning("cellAtracMultiSetDataAndGetMemSize(pHandle=*0x%x, pucBufferAddr=*0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, uiOutputChNum=%d, piTrackArray=*0x%x, puiWorkMemByte=*0x%x)",
		pHandle, pucBufferAddr, uiReadByte, uiBufferByte, uiOutputChNum, piTrackArray, puiWorkMemByte);

	*puiWorkMemByte = 0x1000;
	return CELL_OK;
}

s32 cellAtracMultiCreateDecoder(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtracMulti.Warning("cellAtracMultiCreateDecoder(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)", pHandle, pucWorkMem, uiPpuThreadPriority, uiSpuThreadPriority);

	pHandle->pucWorkMem = pucWorkMem;
	return CELL_OK;
}

s32 cellAtracMultiCreateDecoderExt(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, vm::ptr<CellAtracMultiExtRes> pExtRes)
{
	cellAtracMulti.Warning("cellAtracMultiCreateDecoderExt(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, pExtRes=*0x%x)", pHandle, pucWorkMem, uiPpuThreadPriority, pExtRes);

	pHandle->pucWorkMem = pucWorkMem;
	return CELL_OK;
}

s32 cellAtracMultiDeleteDecoder(vm::ptr<CellAtracMultiHandle> pHandle)
{
	cellAtracMulti.Warning("cellAtracMultiDeleteDecoder(pHandle=*0x%x)", pHandle);

	return CELL_OK;
}

s32 cellAtracMultiDecode(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<float> pfOutAddr, vm::ptr<u32> puiSamples, vm::ptr<u32> puiFinishflag, vm::ptr<s32> piRemainFrame)
{
	cellAtracMulti.Warning("cellAtracMultiDecode(pHandle=*0x%x, pfOutAddr=*0x%x, puiSamples=*0x%x, puiFinishFlag=*0x%x, piRemainFrame=*0x%x)", pHandle, pfOutAddr, puiSamples, puiFinishflag, piRemainFrame);

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRACMULTI_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracMultiGetStreamDataInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::pptr<u8> ppucWritePointer, vm::ptr<u32> puiWritableByte, vm::ptr<u32> puiReadPosition)
{
	cellAtracMulti.Warning("cellAtracMultiGetStreamDataInfo(pHandle=*0x%x, ppucWritePointer=**0x%x, puiWritableByte=*0x%x, puiReadPosition=*0x%x)", pHandle, ppucWritePointer, puiWritableByte, puiReadPosition);

	*ppucWritePointer = pHandle->pucWorkMem;
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracMultiAddStreamData(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiAddByte)
{
	cellAtracMulti.Warning("cellAtracMultiAddStreamData(pHandle=*0x%x, uiAddByte=0x%x)", pHandle, uiAddByte);

	return CELL_OK;
}

s32 cellAtracMultiGetRemainFrame(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piRemainFrame)
{
	cellAtracMulti.Warning("cellAtracMultiGetRemainFrame(pHandle=*0x%x, piRemainFrame=*0x%x)", pHandle, piRemainFrame);

	*piRemainFrame = CELL_ATRACMULTI_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

s32 cellAtracMultiGetVacantSize(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiVacantSize)
{
	cellAtracMulti.Warning("cellAtracMultiGetVacantSize(pHandle=*0x%x, puiVacantSize=*0x%x)", pHandle, puiVacantSize);

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

s32 cellAtracMultiIsSecondBufferNeeded(vm::ptr<CellAtracMultiHandle> pHandle)
{
	cellAtracMulti.Warning("cellAtracMultiIsSecondBufferNeeded(pHandle=*0x%x)", pHandle);

	return 0;
}

s32 cellAtracMultiGetSecondBufferInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiReadPosition, vm::ptr<u32> puiDataByte)
{
	cellAtracMulti.Warning("cellAtracMultiGetSecondBufferInfo(pHandle=*0x%x, puiReadPosition=*0x%x, puiDataByte=*0x%x)", pHandle, puiReadPosition, puiDataByte);

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

s32 cellAtracMultiSetSecondBuffer(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtracMulti.Warning("cellAtracMultiSetSecondBuffer(pHandle=*0x%x, pucSecondBufferAddr=*0x%x, uiSecondBufferByte=0x%x)", pHandle, pucSecondBufferAddr, uiSecondBufferByte);

	return CELL_OK;
}

s32 cellAtracMultiGetChannel(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiChannel)
{
	cellAtracMulti.Warning("cellAtracMultiGetChannel(pHandle=*0x%x, puiChannel=*0x%x)", pHandle, puiChannel);

	*puiChannel = 2;
	return CELL_OK;
}

s32 cellAtracMultiGetMaxSample(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiMaxSample)
{
	cellAtracMulti.Warning("cellAtracMultiGetMaxSample(pHandle=*0x%x, puiMaxSample=*0x%x)", pHandle, puiMaxSample);

	*puiMaxSample = 512;
	return CELL_OK;
}

s32 cellAtracMultiGetNextSample(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiNextSample)
{
	cellAtracMulti.Warning("cellAtracMultiGetNextSample(pHandle=*0x%x, puiNextSample=*0x%x)", pHandle, puiNextSample);

	*puiNextSample = 0;
	return CELL_OK;
}

s32 cellAtracMultiGetSoundInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piEndSample, vm::ptr<s32> piLoopStartSample, vm::ptr<s32> piLoopEndSample)
{
	cellAtracMulti.Warning("cellAtracMultiGetSoundInfo(pHandle=*0x%x, piEndSample=*0x%x, piLoopStartSample=*0x%x, piLoopEndSample=*0x%x)", pHandle, piEndSample, piLoopStartSample, piLoopEndSample);

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

s32 cellAtracMultiGetNextDecodePosition(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiSamplePosition)
{
	cellAtracMulti.Warning("cellAtracMultiGetNextDecodePosition(pHandle=*0x%x, puiSamplePosition=*0x%x)", pHandle, puiSamplePosition);

	*puiSamplePosition = 0;
	return CELL_ATRACMULTI_ERROR_ALLDATA_WAS_DECODED;
}

s32 cellAtracMultiGetBitrate(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiBitrate)
{
	cellAtracMulti.Warning("cellAtracMultiGetBitrate(pHandle=*0x%x, puiBitrate=*0x%x)", pHandle, puiBitrate);

	*puiBitrate = 128;
	return CELL_OK;
}

s32 cellAtracMultiGetTrackArray(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piTrackArray)
{
	cellAtracMulti.Error("cellAtracMultiGetTrackArray(pHandle=*0x%x, piTrackArray=*0x%x)", pHandle, piTrackArray);

	return CELL_OK;
}

s32 cellAtracMultiGetLoopInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piLoopNum, vm::ptr<u32> puiLoopStatus)
{
	cellAtracMulti.Warning("cellAtracMultiGetLoopInfo(pHandle=*0x%x, piLoopNum=*0x%x, puiLoopStatus=*0x%x)", pHandle, piLoopNum, puiLoopStatus);

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

s32 cellAtracMultiSetLoopNum(vm::ptr<CellAtracMultiHandle> pHandle, s32 iLoopNum)
{
	cellAtracMulti.Warning("cellAtracMultiSetLoopNum(pHandle=*0x%x, iLoopNum=%d)", pHandle, iLoopNum);

	return CELL_OK;
}

s32 cellAtracMultiGetBufferInfoForResetting(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiSample, vm::ptr<CellAtracMultiBufferInfo> pBufferInfo)
{
	cellAtracMulti.Warning("cellAtracMultiGetBufferInfoForResetting(pHandle=*0x%x, uiSample=0x%x, pBufferInfo=*0x%x)", pHandle, uiSample, pBufferInfo);

	pBufferInfo->pucWriteAddr = pHandle->pucWorkMem;
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

s32 cellAtracMultiResetPlayPosition(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiSample, u32 uiWriteByte, vm::ptr<s32> piTrackArray)
{
	cellAtracMulti.Warning("cellAtracMultiResetPlayPosition(pHandle=*0x%x, uiSample=0x%x, uiWriteByte=0x%x, piTrackArray=*0x%x)", pHandle, uiSample, uiWriteByte, piTrackArray);

	return CELL_OK;
}

s32 cellAtracMultiGetInternalErrorInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piResult)
{
	cellAtracMulti.Warning("cellAtracMultiGetInternalErrorInfo(pHandle=*0x%x, piResult=*0x%x)", pHandle, piResult);

	*piResult = 0;
	return CELL_OK;
}

Module cellAtracMulti("cellAtrac", []()
{
	REG_FUNC(cellAtracMulti, cellAtracMultiSetDataAndGetMemSize);

	REG_FUNC(cellAtracMulti, cellAtracMultiCreateDecoder);
	REG_FUNC(cellAtracMulti, cellAtracMultiCreateDecoderExt);
	REG_FUNC(cellAtracMulti, cellAtracMultiDeleteDecoder);

	REG_FUNC(cellAtracMulti, cellAtracMultiDecode);

	REG_FUNC(cellAtracMulti, cellAtracMultiGetStreamDataInfo);
	REG_FUNC(cellAtracMulti, cellAtracMultiAddStreamData);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetRemainFrame);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetVacantSize);
	REG_FUNC(cellAtracMulti, cellAtracMultiIsSecondBufferNeeded);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetSecondBufferInfo);
	REG_FUNC(cellAtracMulti, cellAtracMultiSetSecondBuffer);

	REG_FUNC(cellAtracMulti, cellAtracMultiGetChannel);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetMaxSample);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetNextSample);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetSoundInfo);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetNextDecodePosition);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetBitrate);
	REG_FUNC(cellAtracMulti, cellAtracMultiGetTrackArray);

	REG_FUNC(cellAtracMulti, cellAtracMultiGetLoopInfo);
	REG_FUNC(cellAtracMulti, cellAtracMultiSetLoopNum);

	REG_FUNC(cellAtracMulti, cellAtracMultiGetBufferInfoForResetting);
	REG_FUNC(cellAtracMulti, cellAtracMultiResetPlayPosition);

	REG_FUNC(cellAtracMulti, cellAtracMultiGetInternalErrorInfo);
});
