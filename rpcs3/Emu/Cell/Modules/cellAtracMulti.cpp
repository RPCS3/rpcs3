#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellAtracMulti.h"

LOG_CHANNEL(cellAtracMulti);

template <>
void fmt_class_string<CellAtracMultiError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellAtracMultiError value)
	{
		switch (value)
		{
		STR_CASE(CELL_ATRACMULTI_ERROR_API_FAIL);
		STR_CASE(CELL_ATRACMULTI_ERROR_READSIZE_OVER_BUFFER);
		STR_CASE(CELL_ATRACMULTI_ERROR_UNKNOWN_FORMAT);
		STR_CASE(CELL_ATRACMULTI_ERROR_READSIZE_IS_TOO_SMALL);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_SAMPLING_RATE);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_DATA);
		STR_CASE(CELL_ATRACMULTI_ERROR_NO_DECODER);
		STR_CASE(CELL_ATRACMULTI_ERROR_UNSET_DATA);
		STR_CASE(CELL_ATRACMULTI_ERROR_DECODER_WAS_CREATED);
		STR_CASE(CELL_ATRACMULTI_ERROR_ALLDATA_WAS_DECODED);
		STR_CASE(CELL_ATRACMULTI_ERROR_NODATA_IN_BUFFER);
		STR_CASE(CELL_ATRACMULTI_ERROR_NOT_ALIGNED_OUT_BUFFER);
		STR_CASE(CELL_ATRACMULTI_ERROR_NEED_SECOND_BUFFER);
		STR_CASE(CELL_ATRACMULTI_ERROR_ALLDATA_IS_ONMEMORY);
		STR_CASE(CELL_ATRACMULTI_ERROR_ADD_DATA_IS_TOO_BIG);
		STR_CASE(CELL_ATRACMULTI_ERROR_NONEED_SECOND_BUFFER);
		STR_CASE(CELL_ATRACMULTI_ERROR_UNSET_LOOP_NUM);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_SAMPLE);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_RESET_BYTE);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_PPU_THREAD_PRIORITY);
		STR_CASE(CELL_ATRACMULTI_ERROR_ILLEGAL_SPU_THREAD_PRIORITY);
		STR_CASE(CELL_ATRACMULTI_ERROR_API_PARAMETER);
		}

		return unknown;
	});
}

error_code cellAtracMultiSetDataAndGetMemSize(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucBufferAddr, u32 uiReadByte, u32 uiBufferByte, u32 uiOutputChNum, vm::ptr<s32> piTrackArray, vm::ptr<u32> puiWorkMemByte)
{
	cellAtracMulti.warning("cellAtracMultiSetDataAndGetMemSize(pHandle=*0x%x, pucBufferAddr=*0x%x, uiReadByte=0x%x, uiBufferByte=0x%x, uiOutputChNum=%d, piTrackArray=*0x%x, puiWorkMemByte=*0x%x)",
		pHandle, pucBufferAddr, uiReadByte, uiBufferByte, uiOutputChNum, piTrackArray, puiWorkMemByte);

	*puiWorkMemByte = 0x1000;
	return CELL_OK;
}

error_code cellAtracMultiCreateDecoder(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, u32 uiSpuThreadPriority)
{
	cellAtracMulti.warning("cellAtracMultiCreateDecoder(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, uiSpuThreadPriority=%d)", pHandle, pucWorkMem, uiPpuThreadPriority, uiSpuThreadPriority);

	std::memcpy(pHandle->ucWorkMem, pucWorkMem.get_ptr(), CELL_ATRACMULTI_HANDLE_SIZE);
	return CELL_OK;
}

error_code cellAtracMultiCreateDecoderExt(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucWorkMem, u32 uiPpuThreadPriority, vm::ptr<CellAtracMultiExtRes> pExtRes)
{
	cellAtracMulti.warning("cellAtracMultiCreateDecoderExt(pHandle=*0x%x, pucWorkMem=*0x%x, uiPpuThreadPriority=%d, pExtRes=*0x%x)", pHandle, pucWorkMem, uiPpuThreadPriority, pExtRes);

	std::memcpy(pHandle->ucWorkMem, pucWorkMem.get_ptr(), CELL_ATRACMULTI_HANDLE_SIZE);
	return CELL_OK;
}

error_code cellAtracMultiDeleteDecoder(vm::ptr<CellAtracMultiHandle> pHandle)
{
	cellAtracMulti.warning("cellAtracMultiDeleteDecoder(pHandle=*0x%x)", pHandle);

	return CELL_OK;
}

error_code cellAtracMultiDecode(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<float> pfOutAddr, vm::ptr<u32> puiSamples, vm::ptr<u32> puiFinishflag, vm::ptr<s32> piRemainFrame)
{
	cellAtracMulti.warning("cellAtracMultiDecode(pHandle=*0x%x, pfOutAddr=*0x%x, puiSamples=*0x%x, puiFinishFlag=*0x%x, piRemainFrame=*0x%x)", pHandle, pfOutAddr, puiSamples, puiFinishflag, piRemainFrame);

	*puiSamples = 0;
	*puiFinishflag = 1;
	*piRemainFrame = CELL_ATRACMULTI_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

error_code cellAtracMultiGetStreamDataInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::pptr<u8> ppucWritePointer, vm::ptr<u32> puiWritableByte, vm::ptr<u32> puiReadPosition)
{
	cellAtracMulti.warning("cellAtracMultiGetStreamDataInfo(pHandle=*0x%x, ppucWritePointer=**0x%x, puiWritableByte=*0x%x, puiReadPosition=*0x%x)", pHandle, ppucWritePointer, puiWritableByte, puiReadPosition);

	ppucWritePointer->set(pHandle.addr());
	*puiWritableByte = 0x1000;
	*puiReadPosition = 0;
	return CELL_OK;
}

error_code cellAtracMultiAddStreamData(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiAddByte)
{
	cellAtracMulti.warning("cellAtracMultiAddStreamData(pHandle=*0x%x, uiAddByte=0x%x)", pHandle, uiAddByte);

	return CELL_OK;
}

error_code cellAtracMultiGetRemainFrame(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piRemainFrame)
{
	cellAtracMulti.warning("cellAtracMultiGetRemainFrame(pHandle=*0x%x, piRemainFrame=*0x%x)", pHandle, piRemainFrame);

	*piRemainFrame = CELL_ATRACMULTI_ALLDATA_IS_ON_MEMORY;
	return CELL_OK;
}

error_code cellAtracMultiGetVacantSize(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiVacantSize)
{
	cellAtracMulti.warning("cellAtracMultiGetVacantSize(pHandle=*0x%x, puiVacantSize=*0x%x)", pHandle, puiVacantSize);

	*puiVacantSize = 0x1000;
	return CELL_OK;
}

error_code cellAtracMultiIsSecondBufferNeeded(vm::ptr<CellAtracMultiHandle> pHandle)
{
	cellAtracMulti.warning("cellAtracMultiIsSecondBufferNeeded(pHandle=*0x%x)", pHandle);

	return not_an_error(0);
}

error_code cellAtracMultiGetSecondBufferInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiReadPosition, vm::ptr<u32> puiDataByte)
{
	cellAtracMulti.warning("cellAtracMultiGetSecondBufferInfo(pHandle=*0x%x, puiReadPosition=*0x%x, puiDataByte=*0x%x)", pHandle, puiReadPosition, puiDataByte);

	*puiReadPosition = 0;
	*puiDataByte = 0; // write to null block will occur
	return CELL_OK;
}

error_code cellAtracMultiSetSecondBuffer(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u8> pucSecondBufferAddr, u32 uiSecondBufferByte)
{
	cellAtracMulti.warning("cellAtracMultiSetSecondBuffer(pHandle=*0x%x, pucSecondBufferAddr=*0x%x, uiSecondBufferByte=0x%x)", pHandle, pucSecondBufferAddr, uiSecondBufferByte);

	return CELL_OK;
}

error_code cellAtracMultiGetChannel(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiChannel)
{
	cellAtracMulti.warning("cellAtracMultiGetChannel(pHandle=*0x%x, puiChannel=*0x%x)", pHandle, puiChannel);

	*puiChannel = 2;
	return CELL_OK;
}

error_code cellAtracMultiGetMaxSample(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiMaxSample)
{
	cellAtracMulti.warning("cellAtracMultiGetMaxSample(pHandle=*0x%x, puiMaxSample=*0x%x)", pHandle, puiMaxSample);

	*puiMaxSample = 512;
	return CELL_OK;
}

error_code cellAtracMultiGetNextSample(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiNextSample)
{
	cellAtracMulti.warning("cellAtracMultiGetNextSample(pHandle=*0x%x, puiNextSample=*0x%x)", pHandle, puiNextSample);

	*puiNextSample = 0;
	return CELL_OK;
}

error_code cellAtracMultiGetSoundInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piEndSample, vm::ptr<s32> piLoopStartSample, vm::ptr<s32> piLoopEndSample)
{
	cellAtracMulti.warning("cellAtracMultiGetSoundInfo(pHandle=*0x%x, piEndSample=*0x%x, piLoopStartSample=*0x%x, piLoopEndSample=*0x%x)", pHandle, piEndSample, piLoopStartSample, piLoopEndSample);

	*piEndSample = 0;
	*piLoopStartSample = 0;
	*piLoopEndSample = 0;
	return CELL_OK;
}

error_code cellAtracMultiGetNextDecodePosition(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiSamplePosition)
{
	cellAtracMulti.warning("cellAtracMultiGetNextDecodePosition(pHandle=*0x%x, puiSamplePosition=*0x%x)", pHandle, puiSamplePosition);

	*puiSamplePosition = 0;
	return CELL_ATRACMULTI_ERROR_ALLDATA_WAS_DECODED;
}

error_code cellAtracMultiGetBitrate(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<u32> puiBitrate)
{
	cellAtracMulti.warning("cellAtracMultiGetBitrate(pHandle=*0x%x, puiBitrate=*0x%x)", pHandle, puiBitrate);

	*puiBitrate = 128;
	return CELL_OK;
}

error_code cellAtracMultiGetTrackArray(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piTrackArray)
{
	cellAtracMulti.error("cellAtracMultiGetTrackArray(pHandle=*0x%x, piTrackArray=*0x%x)", pHandle, piTrackArray);

	return CELL_OK;
}

error_code cellAtracMultiGetLoopInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piLoopNum, vm::ptr<u32> puiLoopStatus)
{
	cellAtracMulti.warning("cellAtracMultiGetLoopInfo(pHandle=*0x%x, piLoopNum=*0x%x, puiLoopStatus=*0x%x)", pHandle, piLoopNum, puiLoopStatus);

	*piLoopNum = 0;
	*puiLoopStatus = 0;
	return CELL_OK;
}

error_code cellAtracMultiSetLoopNum(vm::ptr<CellAtracMultiHandle> pHandle, s32 iLoopNum)
{
	cellAtracMulti.warning("cellAtracMultiSetLoopNum(pHandle=*0x%x, iLoopNum=%d)", pHandle, iLoopNum);

	return CELL_OK;
}

error_code cellAtracMultiGetBufferInfoForResetting(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiSample, vm::ptr<CellAtracMultiBufferInfo> pBufferInfo)
{
	cellAtracMulti.warning("cellAtracMultiGetBufferInfoForResetting(pHandle=*0x%x, uiSample=0x%x, pBufferInfo=*0x%x)", pHandle, uiSample, pBufferInfo);

	pBufferInfo->pucWriteAddr.set(pHandle.addr());
	pBufferInfo->uiWritableByte = 0x1000;
	pBufferInfo->uiMinWriteByte = 0;
	pBufferInfo->uiReadPosition = 0;
	return CELL_OK;
}

error_code cellAtracMultiResetPlayPosition(vm::ptr<CellAtracMultiHandle> pHandle, u32 uiSample, u32 uiWriteByte, vm::ptr<s32> piTrackArray)
{
	cellAtracMulti.warning("cellAtracMultiResetPlayPosition(pHandle=*0x%x, uiSample=0x%x, uiWriteByte=0x%x, piTrackArray=*0x%x)", pHandle, uiSample, uiWriteByte, piTrackArray);

	return CELL_OK;
}

error_code cellAtracMultiGetInternalErrorInfo(vm::ptr<CellAtracMultiHandle> pHandle, vm::ptr<s32> piResult)
{
	cellAtracMulti.warning("cellAtracMultiGetInternalErrorInfo(pHandle=*0x%x, piResult=*0x%x)", pHandle, piResult);

	*piResult = 0;
	return CELL_OK;
}

error_code cellAtracMultiGetSamplingRate()
{
	UNIMPLEMENTED_FUNC(cellAtracMulti);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAtracMulti)("cellAtracMulti", []()
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

	REG_FUNC(cellAtracMulti, cellAtracMultiGetSamplingRate);
});
