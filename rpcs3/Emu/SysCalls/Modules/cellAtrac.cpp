#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

Module *cellAtrac = nullptr;

#include "cellAtrac.h"

int cellAtracSetDataAndGetMemSize(vm::ptr<CellAtracHandle> pHandle, u32 pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, vm::ptr<be_t<u32>> puiWorkMemByte)
{
	cellAtrac->Todo("cellAtracSetDataAndGetMemSize(pHandle=0x%x, pucBufferAddr=0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, puiWorkMemByte_addr=0x%x)",
		pHandle.addr(), pucBufferAddr, uiReadByte, uiBufferByte, puiWorkMemByte.addr());

	*puiWorkMemByte = 0x1000; // unproved
	return CELL_OK;
}

int cellAtracCreateDecoder(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtrac->Todo("cellAtracCreateDecoder(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, uiSpuThreadPriority);

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

int cellAtracCreateDecoderExt(vm::ptr<CellAtracHandle> pHandle, u32 pucWorkMem_addr, u32 uiPpuThreadPriority, vm::ptr<CellAtracExtRes> pExtRes)
{
	cellAtrac->Todo("cellAtracCreateDecoderExt(pHandle=0x%x, pucWorkMem_addr=0x%x, uiPpuThreadPriority=%d, pExtRes_addr=0x%x)",
		pHandle.addr(), pucWorkMem_addr, uiPpuThreadPriority, pExtRes.addr());

	pHandle->data.pucWorkMem_addr = pucWorkMem_addr;
	return CELL_OK;
}

int cellAtracDeleteDecoder(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac->Todo("cellAtracDeleteDecoder(pHandle=0x%x)", pHandle.addr());
	return CELL_OK;
}

int cellAtracDecode(vm::ptr<CellAtracHandle> pHandle, u32 pfOutAddr, vm::ptr<be_t<u32>> puiSamples, vm::ptr<be_t<u32>> puiFinishflag, vm::ptr<be_t<u32>> piRemainFrame)
{
	cellAtrac->Todo("cellAtracDecode(pHandle=0x%x, pfOutAddr=0x%x, puiSamples_addr=0x%x, puiFinishFlag_addr=0x%x, piRemainFrame_addr=0x%x)",
		pHandle.addr(), pfOutAddr, puiSamples.addr(), puiFinishflag.addr(), piRemainFrame.addr());

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

int cellAtracGetStreamDataInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> ppucWritePointer, vm::ptr<be_t<u32>> puiWritableByte, vm::ptr<be_t<u32>> puiReadPosition)
{
	cellAtrac->Todo("cellAtracGetStreamDataInfo(pHandle=0x%x, ppucWritePointer_addr=0x%x, puiWritableByte_addr=0x%x, puiReadPosition_addr=0x%x)",
		pHandle.addr(), ppucWritePointer.addr(), puiWritableByte.addr(), puiReadPosition.addr());

	*ppucWritePointer = pHandle->data.pucWorkMem_addr;
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

int cellAtracAddStreamData(vm::ptr<CellAtracHandle> pHandle, u32 uiAddByte)
{
	cellAtrac->Todo("cellAtracAddStreamData(pHandle=0x%x, uiAddByte=0x%x)", pHandle.addr(), uiAddByte);
	return CELL_OK;
}

int cellAtracGetRemainFrame(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> piRemainFrame)
{
	cellAtrac->Todo("cellAtracGetRemainFrame(pHandle=0x%x, piRemainFrame_addr=0x%x)", pHandle.addr(), piRemainFrame.addr());

	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

int cellAtracGetVacantSize(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiVacantSize)
{
	cellAtrac->Todo("cellAtracGetVacantSize(pHandle=0x%x, puiVacantSize_addr=0x%x)", pHandle.addr(), puiVacantSize.addr());

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

int cellAtracIsSecondBufferNeeded(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac->Todo("cellAtracIsSecondBufferNeeded(pHandle=0x%x)", pHandle.addr());
	return CELL_OK;
}

int cellAtracGetSecondBufferInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiReadPosition, vm::ptr<be_t<u32>> puiDataByte)
{
	cellAtrac->Todo("cellAtracGetSecondBufferInfo(pHandle=0x%x, puiReadPosition_addr=0x%x, puiDataByte_addr=0x%x)",
		pHandle.addr(), puiReadPosition.addr(), puiDataByte.addr());

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

int cellAtracSetSecondBuffer(vm::ptr<CellAtracHandle> pHandle, u32 pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtrac->Todo("cellAtracSetSecondBuffer(pHandle=0x%x, pucSecondBufferAddr=0x%x, uiSecondBufferByte=0x%x)",
		pHandle.addr(), pucSecondBufferAddr, uiSecondBufferByte);
	return CELL_OK;
}

int cellAtracGetChannel(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiChannel)
{
	cellAtrac->Todo("cellAtracGetChannel(pHandle=0x%x, puiChannel_addr=0x%x)", pHandle.addr(), puiChannel.addr());

	*puiChannel = 2;
	return CELL_OK;
}

int cellAtracGetMaxSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiMaxSample)
{
	cellAtrac->Todo("cellAtracGetMaxSample(pHandle=0x%x, puiMaxSample_addr=0x%x)", pHandle.addr(), puiMaxSample.addr());

	*puiMaxSample = 512;
	return CELL_OK;
}

int cellAtracGetNextSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiNextSample)
{
	cellAtrac->Todo("cellAtracGetNextSample(pHandle=0x%x, puiNextSample_addr=0x%x)", pHandle.addr(), puiNextSample.addr());

	*puiNextSample = 0;
	return CELL_OK;
}

int cellAtracGetSoundInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> piEndSample, vm::ptr<be_t<u32>> piLoopStartSample, vm::ptr<be_t<u32>> piLoopEndSample)
{
	cellAtrac->Todo("cellAtracGetSoundInfo(pHandle=0x%x, piEndSample_addr=0x%x, piLoopStartSample_addr=0x%x, piLoopEndSample_addr=0x%x)",
		pHandle.addr(), piEndSample.addr(), piLoopStartSample.addr(), piLoopEndSample.addr());

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

int cellAtracGetNextDecodePosition(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiSamplePosition)
{
	cellAtrac->Todo("cellAtracGetNextDecodePosition(pHandle=0x%x, puiSamplePosition_addr=0x%x)",
		pHandle.addr(), puiSamplePosition.addr());

	*puiSamplePosition = 0;
	return CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED;
}

int cellAtracGetBitrate(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> puiBitrate)
{
	cellAtrac->Todo("cellAtracGetBitrate(pHandle=0x%x, puiBitrate_addr=0x%x)",
		pHandle.addr(), puiBitrate.addr());

	*puiBitrate = 128;
	return CELL_OK;
}

int cellAtracGetLoopInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> piLoopNum, vm::ptr<be_t<u32>> puiLoopStatus)
{
	cellAtrac->Todo("cellAtracGetLoopInfo(pHandle=0x%x, piLoopNum_addr=0x%x, puiLoopStatus_addr=0x%x)",
		pHandle.addr(), piLoopNum.addr(), puiLoopStatus.addr());

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

int cellAtracSetLoopNum(vm::ptr<CellAtracHandle> pHandle, int iLoopNum)
{
	cellAtrac->Todo("cellAtracSetLoopNum(pHandle=0x%x, iLoopNum=0x%x)", pHandle.addr(), iLoopNum);
	return CELL_OK;
}

int cellAtracGetBufferInfoForResetting(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, vm::ptr<CellAtracBufferInfo> pBufferInfo)
{
	cellAtrac->Todo("cellAtracGetBufferInfoForResetting(pHandle=0x%x, uiSample=0x%x, pBufferInfo_addr=0x%x)",
		pHandle.addr(), uiSample, pBufferInfo.addr());

	pBufferInfo->pucWriteAddr = pHandle->data.pucWorkMem_addr;
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

int cellAtracResetPlayPosition(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, u32 uiWriteByte)
{
	cellAtrac->Todo("cellAtracResetPlayPosition(pHandle=0x%x, uiSample=0x%x, uiWriteByte=0x%x)",
		pHandle.addr(), uiSample, uiWriteByte);
	return CELL_OK;
}

int cellAtracGetInternalErrorInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<be_t<u32>> piResult)
{
	cellAtrac->Todo("cellAtracGetInternalErrorInfo(pHandle=0x%x, piResult_addr=0x%x)",
		pHandle.addr(), piResult.addr());

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
}
