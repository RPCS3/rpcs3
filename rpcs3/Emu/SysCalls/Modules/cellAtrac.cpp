#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellAtrac_init();
Module cellAtrac(0x0013, cellAtrac_init);

#include "cellAtrac.h"

int cellAtracSetDataAndGetMemSize(mem_ptr_t<CellAtracHandle> pHandle, u32 pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, mem32_t puiWorkMemByte)
{
	cellAtrac.Error("cellAtracSetDataAndGetMemSize(pHandle=0x%x, pucBufferAddr=0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, puiWorkMemByte_addr=0x%x)",
		pHandle.GetAddr(), pucBufferAddr, uiReadByte, uiBufferByte, puiWorkMemByte.GetAddr());

	puiWorkMemByte = 0x1000; // unproved
	return CELL_OK;
}

int cellAtracCreateDecoder(mem_ptr_t<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtrac.Error("cellAtracCreateDecoder(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)",
		pHandle.GetAddr(), pucWorkMem_addr, uiPpuThreadPriority, uiSpuThreadPriority);

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

int cellAtracCreateDecoderExt(mem_ptr_t<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, mem_ptr_t<CellAtracExtRes> pExtRes)
{
	cellAtrac.Error("cellAtracCreateDecoderExt(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, pExtRes_addr=0x%x)",
		pHandle.GetAddr(), pucWorkMem_addr, uiPpuThreadPriority, pExtRes.GetAddr());

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

int cellAtracDeleteDecoder(mem_ptr_t<CellAtracHandle> pHandle)
{
	cellAtrac.Error("cellAtracDeleteDecoder(pHandle=0x%x)", pHandle.GetAddr());
	return CELL_OK;
}

int cellAtracDecode(mem_ptr_t<CellAtracHandle> pHandle, u32 pfOutAddr, mem32_t puiSamples, mem32_t puiFinishflag, mem32_t piRemainFrame)
{
	cellAtrac.Error("cellAtracDecode(pHandle=0x%x, pfOutAddr=0x%x, puiSamples_addr=0x%x, puiFinishFlag_addr=0x%x, piRemainFrame_addr=0x%x)",
		pHandle.GetAddr(), pfOutAddr, puiSamples.GetAddr(), puiFinishflag.GetAddr(), piRemainFrame.GetAddr());

	puiSamples = 0;
	puiFinishflag = 1;
	piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

int cellAtracGetStreamDataInfo(mem_ptr_t<CellAtracHandle> pHandle, mem32_t ppucWritePointer, mem32_t puiWritableByte, mem32_t puiReadPosition)
{
	cellAtrac.Error("cellAtracGetStreamDataInfo(pHandle=0x%x, ppucWritePointer_addr=0x%x, puiWritableByte_addr=0x%x, puiReadPosition_addr=0x%x)",
		pHandle.GetAddr(), ppucWritePointer.GetAddr(), puiWritableByte.GetAddr(), puiReadPosition.GetAddr());

	ppucWritePointer = pHandle->data.pucWorkMem_addr;
	puiWritableByte = 0x1000;
	puiReadPosition = 0;
	return CELL_OK;
}

int cellAtracAddStreamData(mem_ptr_t<CellAtracHandle> pHandle, u32 uiAddByte)
{
	cellAtrac.Error("cellAtracAddStreamData(pHandle=0x%x, uiAddByte=0x%x)", pHandle.GetAddr(), uiAddByte);
	return CELL_OK;
}

int cellAtracGetRemainFrame(mem_ptr_t<CellAtracHandle> pHandle, mem32_t piRemainFrame)
{
	cellAtrac.Error("cellAtracGetRemainFrame(pHandle=0x%x, piRemainFrame_addr=0x%x)", pHandle.GetAddr(), piRemainFrame.GetAddr());

	piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

int cellAtracGetVacantSize(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiVacantSize)
{
	cellAtrac.Error("cellAtracGetVacantSize(pHandle=0x%x, puiVacantSize_addr=0x%x)", pHandle.GetAddr(), puiVacantSize.GetAddr());

	puiVacantSize = 0x1000;
	return CELL_OK;
}

int cellAtracIsSecondBufferNeeded(mem_ptr_t<CellAtracHandle> pHandle)
{
	cellAtrac.Error("cellAtracIsSecondBufferNeeded(pHandle=0x%x)", pHandle.GetAddr());
	return CELL_OK;
}

int cellAtracGetSecondBufferInfo(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiReadPosition, mem32_t puiDataByte)
{
	cellAtrac.Error("cellAtracGetSecondBufferInfo(pHandle=0x%x, puiReadPosition_addr=0x%x, puiDataByte_addr=0x%x)",
		pHandle.GetAddr(), puiReadPosition.GetAddr(), puiDataByte.GetAddr());

	puiReadPosition = 0;
	puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

int cellAtracSetSecondBuffer(mem_ptr_t<CellAtracHandle> pHandle, u32 pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtrac.Error("cellAtracSetSecondBuffer(pHandle=0x%x, pucSecondBufferAddr=0x%x, uiSecondBufferByte=0x%x)",
		pHandle.GetAddr(), pucSecondBufferAddr, uiSecondBufferByte);
	return CELL_OK;
}

int cellAtracGetChannel(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiChannel)
{
	cellAtrac.Error("cellAtracGetChannel(pHandle=0x%x, puiChannel_addr=0x%x)", pHandle.GetAddr(), puiChannel.GetAddr());

	puiChannel = 2;
	return CELL_OK;
}

int cellAtracGetMaxSample(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiMaxSample)
{
	cellAtrac.Error("cellAtracGetMaxSample(pHandle=0x%x, puiMaxSample_addr=0x%x)", pHandle.GetAddr(), puiMaxSample.GetAddr());

	puiMaxSample = 512;
	return CELL_OK;
}

int cellAtracGetNextSample(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiNextSample)
{
	cellAtrac.Error("cellAtracGetNextSample(pHandle=0x%x, puiNextSample_addr=0x%x)", pHandle.GetAddr(), puiNextSample.GetAddr());

	puiNextSample = 0;
	return CELL_OK;
}

int cellAtracGetSoundInfo(mem_ptr_t<CellAtracHandle> pHandle, mem32_t piEndSample, mem32_t piLoopStartSample, mem32_t piLoopEndSample)
{
	cellAtrac.Error("cellAtracGetSoundInfo(pHandle=0x%x, piEndSample_addr=0x%x, piLoopStartSample_addr=0x%x, piLoopEndSample_addr=0x%x)",
		pHandle.GetAddr(), piEndSample.GetAddr(), piLoopStartSample.GetAddr(), piLoopEndSample.GetAddr());

	piEndSample = 0;
	piLoopStartSample = 0;
	piLoopEndSample = 0;
	return CELL_OK;
}

int cellAtracGetNextDecodePosition(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiSamplePosition)
{
	cellAtrac.Error("cellAtracGetNextDecodePosition(pHandle=0x%x, puiSamplePosition_addr=0x%x)",
		pHandle.GetAddr(), puiSamplePosition.GetAddr());

	puiSamplePosition = 0;
	return CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED;
}

int cellAtracGetBitrate(mem_ptr_t<CellAtracHandle> pHandle, mem32_t puiBitrate)
{
	cellAtrac.Error("cellAtracGetBitrate(pHandle=0x%x, puiBitrate_addr=0x%x)",
		pHandle.GetAddr(), puiBitrate.GetAddr());

	puiBitrate = 128;
	return CELL_OK;
}

int cellAtracGetLoopInfo(mem_ptr_t<CellAtracHandle> pHandle, mem32_t piLoopNum, mem32_t puiLoopStatus)
{
	cellAtrac.Error("cellAtracGetLoopInfo(pHandle=0x%x, piLoopNum_addr=0x%x, puiLoopStatus_addr=0x%x)",
		pHandle.GetAddr(), piLoopNum.GetAddr(), puiLoopStatus.GetAddr());

	piLoopNum = 0;
	puiLoopStatus = 0;
	return CELL_OK;
}

int cellAtracSetLoopNum(mem_ptr_t<CellAtracHandle> pHandle, int iLoopNum)
{
	cellAtrac.Error("cellAtracSetLoopNum(pHandle=0x%x, iLoopNum=0x%x)", pHandle.GetAddr(), iLoopNum);
	return CELL_OK;
}

int cellAtracGetBufferInfoForResetting(mem_ptr_t<CellAtracHandle> pHandle, u32 uiSample, mem_ptr_t<CellAtracBufferInfo> pBufferInfo)
{
	cellAtrac.Error("cellAtracGetBufferInfoForResetting(pHandle=0x%x, uiSample=0x%x, pBufferInfo_addr=0x%x)",
		pHandle.GetAddr(), uiSample, pBufferInfo.GetAddr());

	pBufferInfo->pucWriteAddr = pHandle->data.pucWorkMem_addr;
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

int cellAtracResetPlayPosition(mem_ptr_t<CellAtracHandle> pHandle, u32 uiSample, u32 uiWriteByte)
{
	cellAtrac.Error("cellAtracResetPlayPosition(pHandle=0x%x, uiSample=0x%x, uiWriteByte=0x%x)",
		pHandle.GetAddr(), uiSample, uiWriteByte);
	return CELL_OK;
}

int cellAtracGetInternalErrorInfo(mem_ptr_t<CellAtracHandle> pHandle, mem32_t piResult)
{
	cellAtrac.Error("cellAtracGetInternalErrorInfo(pHandle=0x%x, piResult_addr=0x%x)",
		pHandle.GetAddr(), piResult.GetAddr());

	piResult = 0;
	return CELL_OK;
}

void cellAtrac_init()
{
	cellAtrac.AddFunc(0x66afc68e, cellAtracSetDataAndGetMemSize);

	cellAtrac.AddFunc(0xfa293e88, cellAtracCreateDecoder);
	cellAtrac.AddFunc(0x2642d4cc, cellAtracCreateDecoderExt);
	cellAtrac.AddFunc(0x761cb9be, cellAtracDeleteDecoder);

	cellAtrac.AddFunc(0x8eb0e65f, cellAtracDecode);

	cellAtrac.AddFunc(0x2bfff084, cellAtracGetStreamDataInfo);
	cellAtrac.AddFunc(0x46cfc013, cellAtracAddStreamData);
	cellAtrac.AddFunc(0xdfab73aa, cellAtracGetRemainFrame);
	cellAtrac.AddFunc(0xc9a95fcb, cellAtracGetVacantSize);
	cellAtrac.AddFunc(0x99efe171, cellAtracIsSecondBufferNeeded);
	cellAtrac.AddFunc(0xbe07f05e, cellAtracGetSecondBufferInfo);
	cellAtrac.AddFunc(0x06ddb53e, cellAtracSetSecondBuffer);

	cellAtrac.AddFunc(0x0f9667b6, cellAtracGetChannel);
	cellAtrac.AddFunc(0x5f62d546, cellAtracGetMaxSample);
	cellAtrac.AddFunc(0x4797d1ff, cellAtracGetNextSample);
	cellAtrac.AddFunc(0xcf01d5d4, cellAtracGetSoundInfo);
	cellAtrac.AddFunc(0x7b22e672, cellAtracGetNextDecodePosition);
	cellAtrac.AddFunc(0x006016da, cellAtracGetBitrate);

	cellAtrac.AddFunc(0xab6b6dbf, cellAtracGetLoopInfo);
	cellAtrac.AddFunc(0x78ba5c41, cellAtracSetLoopNum);

	cellAtrac.AddFunc(0x99fb73d1, cellAtracGetBufferInfoForResetting);
	cellAtrac.AddFunc(0x7772eb2b, cellAtracResetPlayPosition);

	cellAtrac.AddFunc(0xb5c11938, cellAtracGetInternalErrorInfo);
}