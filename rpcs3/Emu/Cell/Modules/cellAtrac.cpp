#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellAtrac.h"

LOG_CHANNEL(cellAtrac);

template <>
void fmt_class_string<CellAtracError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellAtracError value)
	{
		switch (value)
		{
		STR_CASE(CELL_ATRAC_ERROR_API_FAIL);
		STR_CASE(CELL_ATRAC_ERROR_READSIZE_OVER_BUFFER);
		STR_CASE(CELL_ATRAC_ERROR_UNKNOWN_FORMAT);
		STR_CASE(CELL_ATRAC_ERROR_READSIZE_IS_TOO_SMALL);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_SAMPLING_RATE);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_DATA);
		STR_CASE(CELL_ATRAC_ERROR_NO_DECODER);
		STR_CASE(CELL_ATRAC_ERROR_UNSET_DATA);
		STR_CASE(CELL_ATRAC_ERROR_DECODER_WAS_CREATED);
		STR_CASE(CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED);
		STR_CASE(CELL_ATRAC_ERROR_NODATA_IN_BUFFER);
		STR_CASE(CELL_ATRAC_ERROR_NOT_ALIGNED_OUT_BUFFER);
		STR_CASE(CELL_ATRAC_ERROR_NEED_SECOND_BUFFER);
		STR_CASE(CELL_ATRAC_ERROR_ALLDATA_IS_ONMEMORY);
		STR_CASE(CELL_ATRAC_ERROR_ADD_DATA_IS_TOO_BIG);
		STR_CASE(CELL_ATRAC_ERROR_NONEED_SECOND_BUFFER);
		STR_CASE(CELL_ATRAC_ERROR_UNSET_LOOP_NUM);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_SAMPLE);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_RESET_BYTE);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_PPU_THREAD_PRIORITY);
		STR_CASE(CELL_ATRAC_ERROR_ILLEGAL_SPU_THREAD_PRIORITY);
		}

		return unknown;
	});
}

error_code cellAtracSetDataAndGetMemSize(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u8> pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, vm::ptr<u32> puiWorkMemByte)
{
	cellAtrac.warning("cellAtracSetDataAndGetMemSize(pHandle=*0x%x, pucBufferAddr=*0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, puiWorkMemByte=*0x%x)", pHandle, pucBufferAddr, uiReadByte, uiBufferByte, puiWorkMemByte);

	*puiWorkMemByte = 0x1000;
	return CELL_OK;
}

error_code cellAtracCreateDecoder(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtrac.warning("cellAtracCreateDecoder(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)", pHandle, pucWorkMem, uiPpuThreadPriority, uiSpuThreadPriority);

	std::memcpy(pHandle->ucWorkMem, pucWorkMem.get_ptr(), CELL_ATRAC_HANDLE_SIZE);
	return CELL_OK;
}

error_code cellAtracCreateDecoderExt(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, vm::ptr<CellAtracExtRes> pExtRes)
{
	cellAtrac.warning("cellAtracCreateDecoderExt(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, pExtRes=*0x%x)", pHandle, pucWorkMem, uiPpuThreadPriority, pExtRes);

	std::memcpy(pHandle->ucWorkMem, pucWorkMem.get_ptr(), CELL_ATRAC_HANDLE_SIZE);
	return CELL_OK;
}

error_code cellAtracDeleteDecoder(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac.warning("cellAtracDeleteDecoder(pHandle=*0x%x)", pHandle);

	return CELL_OK;
}

error_code cellAtracDecode(vm::ptr<CellAtracHandle> pHandle, vm::ptr<float> pfOutAddr, vm::ptr<u32> puiSamples, vm::ptr<u32> puiFinishflag, vm::ptr<s32> piRemainFrame)
{
	cellAtrac.warning("cellAtracDecode(pHandle=*0x%x, pfOutAddr=*0x%x, puiSamples=*0x%x, puiFinishFlag=*0x%x, piRemainFrame=*0x%x)", pHandle, pfOutAddr, puiSamples, puiFinishflag, piRemainFrame);

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

error_code cellAtracGetStreamDataInfo(vm::ptr<CellAtracHandle> pHandle, vm::pptr<u8> ppucWritePointer, vm::ptr<u32> puiWritableByte, vm::ptr<u32> puiReadPosition)
{
	cellAtrac.warning("cellAtracGetStreamDataInfo(pHandle=*0x%x, ppucWritePointer=**0x%x, puiWritableByte=*0x%x, puiReadPosition=*0x%x)", pHandle, ppucWritePointer, puiWritableByte, puiReadPosition);

	ppucWritePointer->set(pHandle.addr());
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

error_code cellAtracAddStreamData(vm::ptr<CellAtracHandle> pHandle, u32 uiAddByte)
{
	cellAtrac.warning("cellAtracAddStreamData(pHandle=*0x%x, uiAddByte=0x%x)", pHandle, uiAddByte);

	return CELL_OK;
}

error_code cellAtracGetRemainFrame(vm::ptr<CellAtracHandle> pHandle, vm::ptr<s32> piRemainFrame)
{
	cellAtrac.warning("cellAtracGetRemainFrame(pHandle=*0x%x, piRemainFrame=*0x%x)", pHandle, piRemainFrame);

	*piRemainFrame = CELL_ATRAC_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

error_code cellAtracGetVacantSize(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiVacantSize)
{
	cellAtrac.warning("cellAtracGetVacantSize(pHandle=*0x%x, puiVacantSize=*0x%x)", pHandle, puiVacantSize);

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

error_code cellAtracIsSecondBufferNeeded(vm::ptr<CellAtracHandle> pHandle)
{
	cellAtrac.warning("cellAtracIsSecondBufferNeeded(pHandle=*0x%x)", pHandle);

	return 0;
}

error_code cellAtracGetSecondBufferInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiReadPosition, vm::ptr<u32> puiDataByte)
{
	cellAtrac.warning("cellAtracGetSecondBufferInfo(pHandle=*0x%x, puiReadPosition=*0x%x, puiDataByte=*0x%x)", pHandle, puiReadPosition, puiDataByte);

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

error_code cellAtracSetSecondBuffer(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u8> pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtrac.warning("cellAtracSetSecondBuffer(pHandle=*0x%x, pucSecondBufferAddr=*0x%x, uiSecondBufferByte=0x%x)", pHandle, pucSecondBufferAddr, uiSecondBufferByte);

	return CELL_OK;
}

error_code cellAtracGetChannel(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiChannel)
{
	cellAtrac.warning("cellAtracGetChannel(pHandle=*0x%x, puiChannel=*0x%x)", pHandle, puiChannel);

	*puiChannel = 2;
	return CELL_OK;
}

error_code cellAtracGetMaxSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiMaxSample)
{
	cellAtrac.warning("cellAtracGetMaxSample(pHandle=*0x%x, puiMaxSample=*0x%x)", pHandle, puiMaxSample);

	*puiMaxSample = 512;
	return CELL_OK;
}

error_code cellAtracGetNextSample(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiNextSample)
{
	cellAtrac.warning("cellAtracGetNextSample(pHandle=*0x%x, puiNextSample=*0x%x)", pHandle, puiNextSample);

	*puiNextSample = 0;
	return CELL_OK;
}

error_code cellAtracGetSoundInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<s32> piEndSample, vm::ptr<s32> piLoopStartSample, vm::ptr<s32> piLoopEndSample)
{
	cellAtrac.warning("cellAtracGetSoundInfo(pHandle=*0x%x, piEndSample=*0x%x, piLoopStartSample=*0x%x, piLoopEndSample=*0x%x)", pHandle, piEndSample, piLoopStartSample, piLoopEndSample);

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

error_code cellAtracGetNextDecodePosition(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiSamplePosition)
{
	cellAtrac.warning("cellAtracGetNextDecodePosition(pHandle=*0x%x, puiSamplePosition=*0x%x)", pHandle, puiSamplePosition);

	*puiSamplePosition = 0;
	return CELL_ATRAC_ERROR_ALLDATA_WAS_DECODED;
}

error_code cellAtracGetBitrate(vm::ptr<CellAtracHandle> pHandle, vm::ptr<u32> puiBitrate)
{
	cellAtrac.warning("cellAtracGetBitrate(pHandle=*0x%x, puiBitrate=*0x%x)", pHandle, puiBitrate);

	*puiBitrate = 128;
	return CELL_OK;
}

error_code cellAtracGetLoopInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<s32> piLoopNum, vm::ptr<u32> puiLoopStatus)
{
	cellAtrac.warning("cellAtracGetLoopInfo(pHandle=*0x%x, piLoopNum=*0x%x, puiLoopStatus=*0x%x)", pHandle, piLoopNum, puiLoopStatus);

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

error_code cellAtracSetLoopNum(vm::ptr<CellAtracHandle> pHandle, s32 iLoopNum)
{
	cellAtrac.warning("cellAtracSetLoopNum(pHandle=*0x%x, iLoopNum=%d)", pHandle, iLoopNum);

	return CELL_OK;
}

error_code cellAtracGetBufferInfoForResetting(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, vm::ptr<CellAtracBufferInfo> pBufferInfo)
{
	cellAtrac.warning("cellAtracGetBufferInfoForResetting(pHandle=*0x%x, uiSample=0x%x, pBufferInfo=*0x%x)", pHandle, uiSample, pBufferInfo);

	pBufferInfo->pucWriteAddr.set(pHandle.addr());
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

error_code cellAtracResetPlayPosition(vm::ptr<CellAtracHandle> pHandle, u32 uiSample, u32 uiWriteByte)
{
	cellAtrac.warning("cellAtracResetPlayPosition(pHandle=*0x%x, uiSample=0x%x, uiWriteByte=0x%x)", pHandle, uiSample, uiWriteByte);

	return CELL_OK;
}

error_code cellAtracGetInternalErrorInfo(vm::ptr<CellAtracHandle> pHandle, vm::ptr<s32> piResult)
{
	cellAtrac.warning("cellAtracGetInternalErrorInfo(pHandle=*0x%x, piResult=*0x%x)", pHandle, piResult);

	*piResult = 0;
	return CELL_OK;
}

error_code cellAtracGetSamplingRate()
{
	UNIMPLEMENTED_FUNC(cellAtrac);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAtrac)("cellAtrac", []()
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

	REG_FUNC(cellAtrac, cellAtracGetSamplingRate);
});
