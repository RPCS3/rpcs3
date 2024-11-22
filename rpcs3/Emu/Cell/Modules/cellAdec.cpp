#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/savestate_utils.hpp"

#include "cellAdec.h"

#include <mutex>
#include "util/asm.hpp"

LOG_CHANNEL(cellAdec);

// These need to be defined somewhere to access the LLE functions
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_ac3;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_atrac3;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_atrac3multi;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Celp8;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Celp;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Ddp;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_DtsCore;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_DtsHd_Core;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_DtsHd;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_DtsLbr;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Aac;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_mpmc;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_M4Aac;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_M4Aac2ch;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_M4Aac2chmod;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Mp3;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_Mp3s;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_mpmcl1;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_truehd;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_wma;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_WmaLsl;
vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_WmaPro;
error_code _SceAdecCorrectPtsValue_Celp8(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_Celp(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_Ddp(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_DtsCore(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_DtsHd_Core(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_DtsHd(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_DtsLbr(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_Aac(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_mpmc(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_M4Aac(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_M4Aac2ch(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_M4Aac2chmod(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_Mp3s(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_mpmcl1(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_truehd(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_wma(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_WmaLsl(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }
error_code _SceAdecCorrectPtsValue_WmaPro(ppu_thread&, vm::ptr<void>, vm::ptr<CellCodecTimeStamp>){ UNIMPLEMENTED_FUNC(cellAdec); return CELL_OK; }

template <>
void fmt_class_string<CellAdecError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellAdecError value)
	{
		switch (value)
		{
		STR_CASE(CELL_ADEC_ERROR_FATAL);
		STR_CASE(CELL_ADEC_ERROR_SEQ);
		STR_CASE(CELL_ADEC_ERROR_ARG);
		STR_CASE(CELL_ADEC_ERROR_BUSY);
		STR_CASE(CELL_ADEC_ERROR_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_CELP_BUSY);
		STR_CASE(CELL_ADEC_ERROR_CELP_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_CELP_ARG);
		STR_CASE(CELL_ADEC_ERROR_CELP_SEQ);
		STR_CASE(CELL_ADEC_ERROR_CELP_CORE_FATAL);
		STR_CASE(CELL_ADEC_ERROR_CELP_CORE_ARG);
		STR_CASE(CELL_ADEC_ERROR_CELP_CORE_SEQ);
		STR_CASE(CELL_ADEC_ERROR_CELP8_BUSY);
		STR_CASE(CELL_ADEC_ERROR_CELP8_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_CELP8_ARG);
		STR_CASE(CELL_ADEC_ERROR_CELP8_SEQ);
		STR_CASE(CELL_ADEC_ERROR_CELP8_CORE_FATAL);
		STR_CASE(CELL_ADEC_ERROR_CELP8_CORE_ARG);
		STR_CASE(CELL_ADEC_ERROR_CELP8_CORE_SEQ);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_FATAL);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SEQ);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_ARG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_BUSY);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_BUFFER_OVERFLOW);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_END_OF_BITSTREAM);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_CH_CONFIG_INCONSISTENCY);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_NO_CH_DEFAULT_POS);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_CH_POS);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_UNANTICIPATED_COUPLING_CH);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_LAYER_ID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_ADTS_SYNCWORD_ERROR);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_ADTS_ID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_CH_CHANGED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SAMPLING_FREQ_CHANGED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_WRONG_SBR_CH);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_WRONG_SCALE_FACTOR);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_BOOKS);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_SECTION_DATA);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_PULSE_IS_NOT_LONG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GC_IS_NOT_SUPPORTED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_ELEMENT_ID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_NO_CH_CONFIG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_UNEXPECTED_OVERLAP_CRC);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_CRC_BUFFER_EXCEEDED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_CRC);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_BAD_WINDOW_CODE);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_ADIF_HEADER_ID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_NOT_SUPPORTED_PROFILE);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_PROG_NUMBER_NOT_FOUND);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_SAMP_RATE_INDEX);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_UNANTICIPATED_CH_CONFIG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_PULSE_OVERFLOWED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_CAN_NOT_UNPACK_INDEX);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_DEINTERLEAVE_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_CALC_BAND_OFFSET_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_SCALE_FACTOR_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_CC_GAIN_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_MIX_COUPLING_CH_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GROUP_IS_INVALID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_PREDICT_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_PREDICT_RESET_PATTERN);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVALID_TNS_FRAME_INFO);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_MASK_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_GROUP_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_LPFLAG_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_INVERSE_QUANTIZATION_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_CB_MAP_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_GET_PULSE_FAILED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_MONO_MIXDOWN_ELEMENT_IS_NOT_SUPPORTED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_STEREO_MIXDOWN_ELEMENT_IS_NOT_SUPPORTED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_CH_OVERFLOW);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_NOSYNCH);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PROGRAM);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_TAG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_CHN_CONFIG);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_SECTION);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_SCFACTORS);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PULSE_DATA);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_MAIN_PROFILE_NOT_IMPLEMENTED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_GC_NOT_IMPLEMENTED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_ILLEGAL_PLUS_ELE_ID);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_CREATE_ERROR);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_NOT_INITIALIZED);
		STR_CASE(CELL_ADEC_ERROR_M4AAC_SBR_INVALID_ENVELOPE);
		STR_CASE(CELL_ADEC_ERROR_AC3_BUSY);
		STR_CASE(CELL_ADEC_ERROR_AC3_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_AC3_PARAM);
		STR_CASE(CELL_ADEC_ERROR_AC3_FRAME);
		STR_CASE(CELL_ADEC_ERROR_AT3_OK); // CELL_ADEC_ERROR_AT3_OFFSET
		STR_CASE(CELL_ADEC_ERROR_AT3_BUSY);
		STR_CASE(CELL_ADEC_ERROR_AT3_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_AT3_ERROR);
		STR_CASE(CELL_ADEC_ERROR_LPCM_FATAL);
		STR_CASE(CELL_ADEC_ERROR_LPCM_SEQ);
		STR_CASE(CELL_ADEC_ERROR_LPCM_ARG);
		STR_CASE(CELL_ADEC_ERROR_LPCM_BUSY);
		STR_CASE(CELL_ADEC_ERROR_LPCM_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_MP3_OK); // CELL_ADEC_ERROR_MP3_OFFSET
		STR_CASE(CELL_ADEC_ERROR_MP3_BUSY);
		STR_CASE(CELL_ADEC_ERROR_MP3_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_MP3_ERROR);
		STR_CASE(CELL_ADEC_ERROR_MP3_LOST_SYNC);
		STR_CASE(CELL_ADEC_ERROR_MP3_NOT_L3);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_BITRATE);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_SFREQ);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_EMPHASIS);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_BLKTYPE);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_VERSION);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_MODE);
		STR_CASE(CELL_ADEC_ERROR_MP3_BAD_MODE_EXT);
		STR_CASE(CELL_ADEC_ERROR_MP3_HUFFMAN_NUM);
		STR_CASE(CELL_ADEC_ERROR_MP3_HUFFMAN_CASE_ID);
		STR_CASE(CELL_ADEC_ERROR_MP3_SCALEFAC_COMPRESS);
		STR_CASE(CELL_ADEC_ERROR_MP3_HGETBIT);
		STR_CASE(CELL_ADEC_ERROR_MP3_FLOATING_EXCEPTION);
		STR_CASE(CELL_ADEC_ERROR_MP3_ARRAY_OVERFLOW);
		STR_CASE(CELL_ADEC_ERROR_MP3_STEREO_PROCESSING);
		STR_CASE(CELL_ADEC_ERROR_MP3_JS_BOUND);
		STR_CASE(CELL_ADEC_ERROR_MP3_PCMOUT);
		STR_CASE(CELL_ADEC_ERROR_M2BC_FATAL);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SEQ);
		STR_CASE(CELL_ADEC_ERROR_M2BC_ARG);
		STR_CASE(CELL_ADEC_ERROR_M2BC_BUSY);
		STR_CASE(CELL_ADEC_ERROR_M2BC_EMPTY);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SYNCF);
		STR_CASE(CELL_ADEC_ERROR_M2BC_LAYER);
		STR_CASE(CELL_ADEC_ERROR_M2BC_BITRATE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SAMPLEFREQ);
		STR_CASE(CELL_ADEC_ERROR_M2BC_VERSION);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MODE_EXT);
		STR_CASE(CELL_ADEC_ERROR_M2BC_UNSUPPORT);
		STR_CASE(CELL_ADEC_ERROR_M2BC_OPENBS_EX);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SYNCF_EX);
		STR_CASE(CELL_ADEC_ERROR_M2BC_CRCGET_EX);
		STR_CASE(CELL_ADEC_ERROR_M2BC_CRC_EX);
		STR_CASE(CELL_ADEC_ERROR_M2BC_CRCGET);
		STR_CASE(CELL_ADEC_ERROR_M2BC_CRC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_BITALLOC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SCALE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_SAMPLE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_OPENBS);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_CRCGET);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_CRC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_BITALLOC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_SCALE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_SAMPLE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_HEADER);
		STR_CASE(CELL_ADEC_ERROR_M2BC_MC_STATUS);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_CCRCGET);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_CRC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_BITALLOC);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_SCALE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_SAMPLE);
		STR_CASE(CELL_ADEC_ERROR_M2BC_AG_STATUS);
		}

		return unknown;
	});
}


// LPCM decoder (included in cellAdec/libadec.sprx)

vm::gvar<CellAdecCoreOps> g_cell_adec_core_ops_lpcm;

error_code _CellAdecCoreOpGetMemSize_lpcm(vm::ptr<CellAdecAttr> attr)
{
	cellAdec.todo("_CellAdecCoreOpGetMemSize_lpcm(attr=*0x%x)", attr);

	return CELL_OK;
}

[[noreturn]] error_code _CellAdecCoreOpOpenExt_lpcm(vm::ptr<void> handle, vm::ptr<AdecNotifyAuDone> notifyAuDone, vm::ptr<void> notifyAuDoneArg, vm::ptr<AdecNotifyPcmOut> notifyPcmOut, vm::ptr<void> notifyPcmOutArg,
	vm::ptr<AdecNotifyError> notifyError, vm::ptr<void> notifyErrorArg, vm::ptr<AdecNotifySeqDone> notifySeqDone, vm::ptr<void> notifySeqDoneArg, vm::cptr<CellAdecResource> res, vm::cptr<CellAdecResourceSpurs> spursRes)
{
	cellAdec.todo("_CellAdecCoreOpOpenExt_lpcm(handle=*0x%x, notifyAuDone=*0x%x, notifyAuDoneArg=0x%x, notifyPcmOut=*0x%x, notifyPcmOutArg=0x%x, notifyError=*0x%x, notifyErrorArg=0x%x, notifySeqDone=*0x%x, notifySeqDoneArg=0x%x, res=*0x%x, spursRes=*0x%x)",
		handle, notifyAuDone, notifyAuDoneArg, notifyPcmOut, notifyPcmOutArg, notifyError, notifyErrorArg, notifySeqDone, notifySeqDoneArg, res, spursRes);

	fmt::throw_exception("LPCM decoder not implemented, please disable HLE libadec.sprx");
}

[[noreturn]] error_code _CellAdecCoreOpOpen_lpcm(vm::ptr<void> handle, vm::ptr<AdecNotifyAuDone> notifyAuDone, vm::ptr<void> notifyAuDoneArg, vm::ptr<AdecNotifyPcmOut> notifyPcmOut, vm::ptr<void> notifyPcmOutArg,
	vm::ptr<AdecNotifyError> notifyError, vm::ptr<void> notifyErrorArg, vm::ptr<AdecNotifySeqDone> notifySeqDone, vm::ptr<void> notifySeqDoneArg, vm::cptr<CellAdecResource> res)
{
	cellAdec.todo("_CellAdecCoreOpOpen_lpcm(handle=*0x%x, notifyAuDone=*0x%x, notifyAuDoneArg=*0x%x, notifyPcmOut=*0x%x, notifyPcmOutArg=*0x%x, notifyError=*0x%x, notifyErrorArg=*0x%x, notifySeqDone=*0x%x, notifySeqDoneArg=*0x%x, res=*0x%x)",
		handle, notifyAuDone, notifyAuDoneArg, notifyPcmOut, notifyPcmOutArg, notifyError, notifyErrorArg, notifySeqDone, notifySeqDoneArg, res);

	fmt::throw_exception("LPCM decoder not implemented, please disable HLE libadec.sprx");
}

error_code _CellAdecCoreOpClose_lpcm(vm::ptr<void> handle)
{
	cellAdec.todo("_CellAdecCoreOpClose_lpcm(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code _CellAdecCoreOpStartSeq_lpcm(vm::ptr<void> handle, vm::ptr<CellAdecParamLpcm> lpcmParam)
{
	cellAdec.todo("_CellAdecCoreOpStartSeq_lpcm(handle=*0x%x, lpcmParam=*0x%x)", handle, lpcmParam);

	return CELL_OK;
}

error_code _CellAdecCoreOpEndSeq_lpcm(vm::ptr<void> handle)
{
	cellAdec.todo("_CellAdecCoreOpEndSeq_lpcm(handle=*0x%x)", handle);

	return CELL_OK;
}

error_code _CellAdecCoreOpDecodeAu_lpcm(vm::ptr<void> handle, s32 pcmHandle, vm::ptr<CellAdecAuInfo> auInfo)
{
	cellAdec.todo("_CellAdecCoreOpDecodeAu_lpcm(handle=*0x%x, pcmHandle=%d, auInfo=*0x%x)", handle, pcmHandle, auInfo);

	return CELL_OK;
}

void _CellAdecCoreOpGetVersion_lpcm(vm::ptr<be_t<u32, 1>> version)
{
	cellAdec.notice("_CellAdecCoreOpGetVersion_lpcm(version=*0x%x)", version);

	*version = 0x20070323;
}

error_code _CellAdecCoreOpRealign_lpcm(vm::ptr<void> handle, vm::ptr<void> outBuffer, vm::cptr<void> pcmStartAddr)
{
	cellAdec.todo("_CellAdecCoreOpRealign_lpcm(handle=*0x%x, outBuffer=*0x%x, pcmStartAddr=*0x%x)", handle, outBuffer, pcmStartAddr);

	return CELL_OK;
}

error_code _CellAdecCoreOpReleasePcm_lpcm(vm::ptr<void> handle, s32 pcmHandle, vm::ptr<void> outBuffer)
{
	cellAdec.todo("_CellAdecCoreOpReleasePcm_lpcm(handle=*0x%x, pcmHandle=%d, outBuffer=*0x%x)", handle, pcmHandle, outBuffer);

	return CELL_OK;
}

s32 _CellAdecCoreOpGetPcmHandleNum_lpcm()
{
	cellAdec.notice("_CellAdecCoreOpGetPcmHandleNum_lpcm()");

	return 8;
}

u32 _CellAdecCoreOpGetBsiInfoSize_lpcm()
{
	cellAdec.notice("_CellAdecCoreOpGetBsiInfoSize_lpcm()");

	return sizeof(CellAdecLpcmInfo);
}


// cellAdec abstraction layer, operates the individual decoders

error_code AdecContext::get_new_pcm_handle(vm::ptr<CellAdecAuInfo> au_info) const
{
	for (s32 i = 0; i < frames_num; i++)
	{
		if (!frames[i].in_use)
		{
			frames[i].in_use = true;
			frames[i].au_info = *au_info;
			return not_an_error(i);
		}
	}

	return CELL_ADEC_ERROR_BUSY;
}

error_code AdecContext::verify_pcm_handle(s32 pcm_handle) const
{
	ensure(pcm_handle >= 0 && pcm_handle < frames_num); // Not properly checked on LLE, see below

	if ((pcm_handle < 0 && /* LLE uses && instead of || */ pcm_handle >= frames_num) || !frames[pcm_handle].in_use)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	return CELL_OK;
}

vm::ptr<CellAdecAuInfo> AdecContext::get_au_info(s32 pcm_handle) const
{
	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return vm::null;
	}

	return (frames + pcm_handle).ptr(&AdecFrame::au_info);
}

void AdecContext::set_state(s32 pcm_handle, u32 state) const
{
	if (state == 1u << 9)
	{
		frames[pcm_handle].au_done = true;
	}
	else if (state == 1u << 11)
	{
		frames[pcm_handle].pcm_out = true;
	}
}

error_code AdecContext::get_pcm_item(s32 pcm_handle, vm::ptr<CellAdecPcmItem>& pcm_item) const
{
	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	pcm_item = (frames + pcm_handle).ptr(&AdecFrame::pcm_item);

	return CELL_OK;
}

error_code AdecContext::set_pcm_item(s32 pcm_handle, vm::ptr<void> pcm_addr, u32 pcm_size, vm::cpptr<void> bitstream_info) const
{
	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	frames[pcm_handle].pcm_item.pcmHandle = pcm_handle;
	frames[pcm_handle].pcm_item.status = CELL_OK;
	frames[pcm_handle].pcm_item.startAddr = pcm_addr;
	frames[pcm_handle].pcm_item.size = pcm_size;
	frames[pcm_handle].pcm_item.auInfo = frames[pcm_handle].au_info;
	std::memcpy(frames[pcm_handle].pcm_item.pcmAttr.bsiInfo.get_ptr(), bitstream_info->get_ptr(), bitstream_info_size);

	return CELL_OK;
}

error_code AdecContext::link_frame(ppu_thread& ppu, s32 pcm_handle)
{
	ensure(sys_mutex_lock(ppu, mutex, 0) == CELL_OK); // Error code isn't checked on LLE

	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_ADEC_ERROR_FATAL;
	}

	if (frames_tail == -1 && frames_head == -1)
	{
		frames[pcm_handle].next = pcm_handle;
		frames[pcm_handle].prev = pcm_handle;
		frames_head = pcm_handle;
		frames_tail = pcm_handle;
	}
	else if (frames_tail != -1 && frames_head != -1)
	{
		frames[pcm_handle].next = pcm_handle;
		frames[pcm_handle].prev = frames_tail;
		frames[frames_tail].next = pcm_handle;
		frames_tail = pcm_handle;
	}
	else
	{
		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_ADEC_ERROR_FATAL;
	}

	ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
	return CELL_OK;
}

error_code AdecContext::unlink_frame(ppu_thread& ppu, s32 pcm_handle)
{
	ensure(sys_mutex_lock(ppu, mutex, 0) == CELL_OK); // Error code isn't checked on LLE

	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_ADEC_ERROR_FATAL;
	}

	if (frames_head == -1 || frames_tail == -1)
	{
		ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_ADEC_ERROR_FATAL;
	}

	const s32 next = frames[pcm_handle].next;
	const s32 prev = frames[pcm_handle].prev;

	if (frames_head == frames_tail)
	{
		if (pcm_handle != frames_tail)
		{
			ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
			return CELL_ADEC_ERROR_FATAL;
		}

		frames_head = -1;
		frames_tail = -1;
		frames[pcm_handle].next = -1;
		frames[pcm_handle].prev = -1;
	}
	else if (pcm_handle == frames_head)
	{
		frames_head = next;
		frames[prev].next = next;
	}
	else if (pcm_handle == frames_tail)
	{
		frames_tail = prev;
		frames[next].prev = prev;
	}
	else
	{
		frames[next].prev = prev;
		frames[prev].next = next;
	}

	ensure(sys_mutex_unlock(ppu, mutex) == CELL_OK); // Error code isn't checked on LLE
	return CELL_OK;
}

void AdecContext::reset_frame(s32 pcm_handle) const
{
	frames[pcm_handle].in_use = false;
	frames[pcm_handle].au_done = false;
	frames[pcm_handle].pcm_out = false;
	frames[pcm_handle].next = -1;
	frames[pcm_handle].prev = -1;
}

error_code AdecContext::correct_pts_value(ppu_thread& ppu, s32 pcm_handle, s8 correct_pts_type)
{
	if (verify_pcm_handle(pcm_handle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	const vm::ptr<CellCodecTimeStamp> au_pts = (frames + pcm_handle).ptr(&AdecFrame::au_info).ptr(&CellAdecAuInfo::pts);

	switch (correct_pts_type)
	{
	case ADEC_CORRECT_PTS_VALUE_TYPE_EAC3:    return ppu_execute<&_SceAdecCorrectPtsValue_Ddp>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_DTSHD:   return ppu_execute<&_SceAdecCorrectPtsValue_DtsHd>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_CELP:    return ppu_execute<&_SceAdecCorrectPtsValue_Celp>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_M2AAC:   return ppu_execute<&_SceAdecCorrectPtsValue_Aac>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_MPEG_L2: return ppu_execute<&_SceAdecCorrectPtsValue_mpmc>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_TRUEHD:  return ppu_execute<&_SceAdecCorrectPtsValue_truehd>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_DTS:     return ppu_execute<&_SceAdecCorrectPtsValue_DtsCore>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_M4AAC:
		switch (type.audioCodecType)
		{
		case CELL_ADEC_TYPE_M4AAC:         return ppu_execute<&_SceAdecCorrectPtsValue_M4Aac>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
		case CELL_ADEC_TYPE_M4AAC_2CH:     return ppu_execute<&_SceAdecCorrectPtsValue_M4Aac2ch>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
		case CELL_ADEC_TYPE_M4AAC_2CH_MOD: return ppu_execute<&_SceAdecCorrectPtsValue_M4Aac2chmod>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
		default:                           return CELL_OK;
		}

	case ADEC_CORRECT_PTS_VALUE_TYPE_WMA:            return ppu_execute<&_SceAdecCorrectPtsValue_wma>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_DTSLBR:         return ppu_execute<&_SceAdecCorrectPtsValue_DtsLbr>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_MPEG_L1:        return ppu_execute<&_SceAdecCorrectPtsValue_mpmcl1>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_MP3S:           return ppu_execute<&_SceAdecCorrectPtsValue_Mp3s>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_CELP8:          return ppu_execute<&_SceAdecCorrectPtsValue_Celp8>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_WMAPRO:         return ppu_execute<&_SceAdecCorrectPtsValue_WmaPro>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_WMALSL:         return ppu_execute<&_SceAdecCorrectPtsValue_WmaLsl>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	case ADEC_CORRECT_PTS_VALUE_TYPE_DTSHDCORE_UNK1: // Same as below
	case ADEC_CORRECT_PTS_VALUE_TYPE_DTSHDCORE_UNK2: return ppu_execute<&_SceAdecCorrectPtsValue_DtsHd_Core>(ppu, +core_handle, au_pts) != CELL_OK ? static_cast<error_code>(CELL_ADEC_ERROR_FATAL) : static_cast<error_code>(CELL_OK);
	}

	// If the user didn't set a PTS, we need to interpolate from the previous PTS
	if (au_pts->upper == CODEC_TS_INVALID)
	{
		if (au_pts->lower != CODEC_TS_INVALID)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		if (previous_pts.upper == CODEC_TS_INVALID && previous_pts.lower == CODEC_TS_INVALID)
		{
			return CELL_OK;
		}

		const u32 pts_adjust = [&]
		{
			switch (correct_pts_type)
			{
			case ADEC_CORRECT_PTS_VALUE_TYPE_LPCM:           return 450;
			case 1:                                          return 150;
			case ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_48000Hz: return 3840;
			case ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_44100Hz: return 4180;
			case ADEC_CORRECT_PTS_VALUE_TYPE_ATRACX_32000Hz: return 5760;
			case ADEC_CORRECT_PTS_VALUE_TYPE_AC3:            return 2880;
			case ADEC_CORRECT_PTS_VALUE_TYPE_ATRAC3:         return 4180;
			case ADEC_CORRECT_PTS_VALUE_TYPE_MP3_48000Hz:    return 2160;
			case ADEC_CORRECT_PTS_VALUE_TYPE_MP3_44100Hz:    return 2351;
			case ADEC_CORRECT_PTS_VALUE_TYPE_MP3_32000Hz:    return 3240;
			case ADEC_CORRECT_PTS_VALUE_TYPE_ATRAC3MULTI:    return 3840;
			default:                                         return 0;
			}
		}();

		au_pts->upper = previous_pts.upper;
		au_pts->lower = previous_pts.lower + pts_adjust;

		if (au_pts->lower < previous_pts.lower) // overflow
		{
			au_pts->upper = (au_pts->upper + 1) & 1;
		}

		if (au_pts->upper == CODEC_TS_INVALID)
		{
			return CELL_ADEC_ERROR_FATAL;
		}
	}

	if (au_pts->lower == CODEC_TS_INVALID)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	previous_pts = *au_pts;

	return CELL_OK;
}

static vm::cptr<CellAdecCoreOps> get_core_ops(s32 type)
{
	switch (type)
	{
	case CELL_ADEC_TYPE_INVALID1:      fmt::throw_exception("Invalid audio codec: CELL_ADEC_TYPE_INVALID1");
	case CELL_ADEC_TYPE_LPCM_PAMF:     return g_cell_adec_core_ops_lpcm;
	case CELL_ADEC_TYPE_AC3:           return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cell_libac3dec.variables.find(0xc58bb170)->second.export_addr);
	case CELL_ADEC_TYPE_ATRACX:        return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtracXdec.variables.find(0x4944af9a)->second.export_addr);
	case CELL_ADEC_TYPE_MP3:           return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellMP3dec.variables.find(0x84276a23)->second.export_addr);
	case CELL_ADEC_TYPE_ATRAC3:        return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtrac3dec.variables.find(0x60487d65)->second.export_addr);
	case CELL_ADEC_TYPE_MPEG_L2:       return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellM2BCdec.variables.find(0x300afe4d)->second.export_addr);
	case CELL_ADEC_TYPE_M2AAC:         return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellM2AACdec.variables.find(0xac2f0831)->second.export_addr);
	case CELL_ADEC_TYPE_EAC3:          return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellDDPdec.variables.find(0xf5e9c15c)->second.export_addr);
	case CELL_ADEC_TYPE_TRUEHD:        return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellTRHDdec.variables.find(0xe88e381b)->second.export_addr);
	case CELL_ADEC_TYPE_DTS:           return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellDTSdec.variables.find(0x15248ec5)->second.export_addr);
	case CELL_ADEC_TYPE_CELP:          return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellCelpDec.variables.find(0xffe42c22)->second.export_addr);
	case CELL_ADEC_TYPE_LPCM_BLURAY:   return g_cell_adec_core_ops_lpcm;
	case CELL_ADEC_TYPE_ATRACX_2CH:    return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtracXdec.variables.find(0x076b33ab)->second.export_addr);
	case CELL_ADEC_TYPE_ATRACX_6CH:    return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtracXdec.variables.find(0x1d210eaa)->second.export_addr);
	case CELL_ADEC_TYPE_ATRACX_8CH:    return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtracXdec.variables.find(0xe9a86e54)->second.export_addr);
	case CELL_ADEC_TYPE_M4AAC:         return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellM4AacDec.variables.find(0xab61278d)->second.export_addr);
	case CELL_ADEC_TYPE_LPCM_DVD:      return g_cell_adec_core_ops_lpcm;
	case CELL_ADEC_TYPE_WMA:           return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellWMAdec.variables.find(0x88320b10)->second.export_addr);
	case CELL_ADEC_TYPE_DTSLBR:        return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellDTSLBRdec.variables.find(0x8c50af52)->second.export_addr);
	case CELL_ADEC_TYPE_M4AAC_2CH:     return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellM4AacDec2ch.variables.find(0xe996c664)->second.export_addr);
	case CELL_ADEC_TYPE_DTSHD:         return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellDTSHDdec.variables.find(0x51ac2b6c)->second.export_addr);
	case CELL_ADEC_TYPE_MPEG_L1:       return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellMPL1dec.variables.find(0xbbc70551)->second.export_addr);
	case CELL_ADEC_TYPE_MP3S:          return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellMP3Sdec.variables.find(0x292cdf0a)->second.export_addr);
	case CELL_ADEC_TYPE_M4AAC_2CH_MOD: return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellM4AacDec2chmod.variables.find(0xdbd26836)->second.export_addr);
	case CELL_ADEC_TYPE_CELP8:         return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellCelp8Dec.variables.find(0xf0190c6c)->second.export_addr);
	case CELL_ADEC_TYPE_INVALID2:      fmt::throw_exception("Invalid audio codec: CELL_ADEC_TYPE_INVALID2");
	case CELL_ADEC_TYPE_INVALID3:      fmt::throw_exception("Invalid audio codec: CELL_ADEC_TYPE_INVALID3");
	case CELL_ADEC_TYPE_RESERVED22:    fmt::throw_exception("Invalid audio codec: CELL_ADEC_TYPE_RESERVED22");
	case CELL_ADEC_TYPE_RESERVED23:    fmt::throw_exception("Invalid audio codec: CELL_ADEC_TYPE_RESERVED23");
	case CELL_ADEC_TYPE_DTSHDCORE:     return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellDTSHDCOREdec.variables.find(0x6c8f4f1c)->second.export_addr);
	case CELL_ADEC_TYPE_ATRAC3MULTI:   return vm::cptr<CellAdecCoreOps>::make(*ppu_module_manager::cellAtrac3multidec.variables.find(0xc20c6bd7)->second.export_addr);
	default:                           fmt::throw_exception("Invalid audio codec: %d", type);
	}
}

error_code adecNotifyAuDone(ppu_thread& ppu, s32 pcmHandle, vm::ptr<AdecContext> handle)
{
	// Block savestate creation during callbacks
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.trace("adecNotifyAuDone(pcmHandle=%d, handle=*0x%x)", pcmHandle, handle);

	ensure(!!handle); // Not checked on LLE

	const auto au_info = handle->get_au_info(pcmHandle);

	if (!au_info)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	handle->set_state(pcmHandle, 1u << 9);

	handle->callback.cbFunc(ppu, handle, CELL_ADEC_MSG_TYPE_AUDONE, static_cast<s32>(au_info.addr()), handle->callback.cbArg);

	return CELL_OK;
}

error_code adecNotifyPcmOut(ppu_thread& ppu, s32 pcmHandle, vm::ptr<void> pcmAddr, u32 pcmSize, vm::ptr<AdecContext> handle, vm::cpptr<void> bsiInfo, s8 correctPtsValueType, s32 errorCode)
{
	// Block savestate creation during callbacks
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.trace("adecNotifyPcmOut(pcmHandle=%d, pcmAddr=*0x%x, pcmSize=0x%x, handle=*0x%x, bsiInfo=**0x%x, correctPtsValueType=%d, errorCode=0x%x)", pcmHandle, pcmAddr, pcmSize, handle, bsiInfo, correctPtsValueType, errorCode);

	ensure(!!handle && !!bsiInfo && !!*bsiInfo); // Not checked on LLE

	if (!handle->get_au_info(pcmHandle))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (error_code ret = handle->correct_pts_value(ppu, pcmHandle, correctPtsValueType); ret != CELL_OK)
	{
		return ret;
	}

	if (handle->link_frame(ppu, pcmHandle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	handle->set_state(pcmHandle, 1u << 11);

	if (handle->set_pcm_item(pcmHandle, pcmAddr, pcmSize, bsiInfo) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	vm::ptr<CellAdecPcmItem> pcm_item{};

	if (handle->get_pcm_item(pcmHandle, pcm_item) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (handle->pcm_queue.push(ppu, pcm_item, pcmHandle) != CELL_OK)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (handle->pcm_item_queue.push(ppu, pcm_item, pcmHandle) != CELL_OK)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	handle->callback.cbFunc(ppu, handle, CELL_ADEC_MSG_TYPE_PCMOUT, errorCode, handle->callback.cbArg);

	return CELL_OK;
}

error_code adecNotifyError(ppu_thread& ppu, s32 errorCode, vm::ptr<AdecContext> handle)
{
	// Block savestate creation during callbacks
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.error("adecNotifyError(errorCode=0x%x, handle=*0x%x)", errorCode, handle);

	ensure(!!handle); // Not checked on LLE

	handle->callback.cbFunc(ppu, handle, CELL_ADEC_MSG_TYPE_ERROR, errorCode, handle->callback.cbArg);

	return CELL_OK;
}

error_code adecNotifySeqDone(ppu_thread& ppu, vm::ptr<AdecContext> handle)
{
	// Block savestate creation during callbacks
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.notice("adecNotifySeqDone(handle=*0x%x)", handle);

	ensure(!!handle); // Not checked on LLE

	handle->callback.cbFunc(ppu, handle, CELL_ADEC_MSG_TYPE_SEQDONE, CELL_OK, handle->callback.cbArg);

	return CELL_OK;
}

error_code cellAdecQueryAttr(ppu_thread& ppu, vm::ptr<CellAdecType> type, vm::ptr<CellAdecAttr> attr)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.notice("cellAdecQueryAttr(type=*0x%x, attr=*0x%x)", type, attr);

	if (!type || type->audioCodecType >= 32 || type->audioCodecType == CELL_ADEC_TYPE_INVALID1 || type->audioCodecType == CELL_ADEC_TYPE_INVALID2 || type->audioCodecType == CELL_ADEC_TYPE_INVALID3 || !attr)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	const auto core_ops = get_core_ops(type->audioCodecType);

	core_ops->getMemSize(ppu, attr);

	const s32 pcm_handle_num = core_ops->getPcmHandleNum(ppu);
	const u32 bitstream_info_size = core_ops->getBsiInfoSize(ppu);
	attr->workMemSize += static_cast<u32>((bitstream_info_size + sizeof(AdecFrame)) * pcm_handle_num + sizeof(AdecContext) + 0x7f);

	const vm::var<be_t<u32, 1>> ver_lower;
	core_ops->getVersion(ppu, ver_lower);

	attr->adecVerUpper = 0x491000;
	attr->adecVerLower = *ver_lower;

	return CELL_OK;
}

error_code adecOpen(ppu_thread& ppu, vm::ptr<CellAdecType> type, vm::cptr<CellAdecResource> res, vm::cptr<CellAdecCb> cb, vm::pptr<AdecContext> handle, vm::cptr<CellAdecResourceSpurs> spursRes)
{
	if (!type || type->audioCodecType >= 32 || !res || !res->startAddr || res->ppuThreadPriority >= 0xa00 || res->spuThreadPriority >= 0x100 || res->ppuThreadStackSize < 0x1000 || !cb || !cb->cbFunc || !handle)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (vm::var<CellAdecAttr> attr; cellAdecQueryAttr(ppu, type, attr), res->totalMemSize < attr->workMemSize)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	const auto core_ops = get_core_ops(type->audioCodecType);

	const s32 pcm_handle_num = core_ops->getPcmHandleNum(ppu);
	const u32 bitstream_info_size = core_ops->getBsiInfoSize(ppu);

	const auto _this = vm::ptr<AdecContext>::make(utils::align(+res->startAddr, 0x80));
	const auto frames = vm::ptr<AdecFrame>::make(_this.addr() + sizeof(AdecContext));
	const u32 bitstream_infos_addr = frames.addr() + pcm_handle_num * sizeof(AdecFrame);
	const auto core_handle = vm::ptr<void>::make(utils::align(bitstream_infos_addr + bitstream_info_size * pcm_handle_num, 0x80));

	if (type->audioCodecType == CELL_ADEC_TYPE_LPCM_DVD)
	{
		// TODO
	}
	else if (type->audioCodecType == CELL_ADEC_TYPE_LPCM_PAMF || type->audioCodecType == CELL_ADEC_TYPE_LPCM_BLURAY)
	{
		// TODO
	}

	_this->_this = _this;
	_this->this_size = sizeof(AdecContext) + (bitstream_info_size + sizeof(AdecFrame)) * pcm_handle_num;
	_this->unk = 0;
	_this->sequence_state = AdecSequenceState::dormant;
	_this->type = *type;
	_this->res = *res;
	_this->callback = *cb;
	_this->core_handle = core_handle;
	_this->core_ops = core_ops;
	_this->previous_pts = { CODEC_TS_INVALID, CODEC_TS_INVALID };
	_this->frames_num = pcm_handle_num;
	_this->reserved1 = 0;
	_this->frames_head = -1;
	_this->frames_tail = -1;
	_this->frames = frames;
	_this->bitstream_info_size = bitstream_info_size;
	_this->mutex_attribute = { SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem03"_u64 } };

	_this->pcm_queue.init(ppu, _this.ptr(&AdecContext::pcm_queue));
	_this->pcm_item_queue.init(ppu, _this.ptr(&AdecContext::pcm_item_queue));

	for (s32 i = 0; i < pcm_handle_num; i++)
	{
		frames[i].in_use = false;
		frames[i].this_index = i;
		frames[i].au_done = false;
		frames[i].unk1 = false;
		frames[i].pcm_out = false;
		frames[i].unk2 = false;
		frames[i].pcm_item.pcmAttr.bsiInfo.set(bitstream_infos_addr + bitstream_info_size * i);
		frames[i].reserved1 = 0;
		frames[i].reserved2 = 0;
		frames[i].next = 0;
		frames[i].prev = 0;
	}

	ensure(sys_mutex_create(ppu, _this.ptr(&AdecContext::mutex), _this.ptr(&AdecContext::mutex_attribute)) == CELL_OK); // Error code isn't checked on LLE

	*handle = _this;

	const auto notifyAuDone = vm::ptr<AdecNotifyAuDone>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(adecNotifyAuDone)));
	const auto notifyPcmOut = vm::ptr<AdecNotifyPcmOut>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(adecNotifyPcmOut)));
	const auto notifyError = vm::ptr<AdecNotifyError>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(adecNotifyError)));
	const auto notifySeqDone = vm::ptr<AdecNotifySeqDone>::make(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(adecNotifySeqDone)));

	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	if (spursRes)
	{
		return core_ops->openExt(ppu, _this->core_handle, notifyAuDone, _this, notifyPcmOut, _this, notifyError, _this, notifySeqDone, _this, res, spursRes);
	}

	return core_ops->open(ppu, _this->core_handle, notifyAuDone, _this, notifyPcmOut, _this, notifyError, _this, notifySeqDone, _this, res);
}

error_code cellAdecOpen(ppu_thread& ppu, vm::ptr<CellAdecType> type, vm::ptr<CellAdecResource> res, vm::ptr<CellAdecCb> cb, vm::pptr<AdecContext> handle)
{
	cellAdec.notice("cellAdecOpen(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	return adecOpen(ppu, type, res, cb, handle, vm::null);
}

error_code cellAdecOpenExt(ppu_thread& ppu, vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::pptr<AdecContext> handle)
{
	cellAdec.notice("cellAdecOpenExt(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	if (!res)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	const vm::var<CellAdecResource> _res{{ res->totalMemSize, res->startAddr, res->ppuThreadPriority, 0, res->ppuThreadStackSize }};
	const vm::var<CellAdecResourceSpurs> spursRes{{ res->spurs_addr, res->priority, res->maxContention }};

	return adecOpen(ppu, type, _res, cb, handle, spursRes);
}

error_code cellAdecOpenEx(ppu_thread& ppu, vm::ptr<CellAdecType> type, vm::ptr<CellAdecResourceEx> res, vm::ptr<CellAdecCb> cb, vm::pptr<AdecContext> handle)
{
	cellAdec.notice("cellAdecOpenEx(type=*0x%x, res=*0x%x, cb=*0x%x, handle=*0x%x)", type, res, cb, handle);

	return cellAdecOpenExt(ppu, type, res, cb, handle);
}

error_code cellAdecClose(ppu_thread& ppu, vm::ptr<AdecContext> handle)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.notice("cellAdecClose(handle=*0x%x)", handle);

	if (!handle || handle->_this != handle)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (error_code ret = handle->core_ops->close(ppu, handle->core_handle); ret != CELL_OK)
	{
		return ret;
	}

	if (error_code ret = sys_mutex_destroy(ppu, handle->mutex); ret != CELL_OK)
	{
		return ret;
	}

	if (error_code ret = handle->pcm_queue.finalize(ppu); ret != CELL_OK)
	{
		return ret;
	}

	if (error_code ret = handle->pcm_item_queue.finalize(ppu); ret != CELL_OK)
	{
		return ret;
	}

	handle->_this = vm::null;
	handle->sequence_state = AdecSequenceState::closed;

	return CELL_OK;
}

error_code cellAdecStartSeq(ppu_thread& ppu, vm::ptr<AdecContext> handle, vm::ptr<void> param)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.notice("cellAdecStartSeq(handle=*0x%x, param=*0x%x)", handle, param);

	if (!handle || !param)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (handle->sequence_state != AdecSequenceState::dormant && handle->sequence_state != AdecSequenceState::ready)
	{
		return CELL_ADEC_ERROR_SEQ;
	}

	if (error_code ret = handle->core_ops->startSeq(ppu, handle->core_handle, param); ret != CELL_OK)
	{
		return ret;
	}

	handle->sequence_state = AdecSequenceState::ready;

	return CELL_OK;
}

error_code cellAdecEndSeq(ppu_thread& ppu, vm::ptr<AdecContext> handle)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.notice("cellAdecEndSeq(handle=*0x%x)", handle);

	if (!handle || handle->_this != handle)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (handle->sequence_state != AdecSequenceState::ready)
	{
		return CELL_ADEC_ERROR_SEQ;
	}

	// LLE does not set the sequence state to dormant

	return handle->core_ops->endSeq(ppu, handle->core_handle);
}

error_code cellAdecDecodeAu(ppu_thread& ppu, vm::ptr<AdecContext> handle, vm::ptr<CellAdecAuInfo> auInfo)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	cellAdec.trace("cellAdecDecodeAu(handle=*0x%x, auInfo=*0x%x)", handle, auInfo);

	if (!handle || !auInfo || !auInfo->size)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	cellAdec.trace("cellAdecDecodeAu(): startAddr=*0x%x, size=0x%x, pts=0x%llx, userData=0x%llx", auInfo->startAddr, auInfo->size, std::bit_cast<be_t<u64>>(auInfo->pts), auInfo->userData);

	if (handle->sequence_state != AdecSequenceState::ready)
	{
		return CELL_ADEC_ERROR_SEQ;
	}

	if (!auInfo->startAddr)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	const error_code pcmHandle = handle->get_new_pcm_handle(auInfo);

	if (pcmHandle == static_cast<s32>(CELL_ADEC_ERROR_BUSY))
	{
		return CELL_ADEC_ERROR_BUSY;
	}

	if (handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_BLURAY)
	{
		// TODO
	}
	else if (handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_DVD)
	{
		// TODO
	}

	return handle->core_ops->decodeAu(ppu, handle->core_handle, pcmHandle, auInfo);
}

error_code cellAdecGetPcm(ppu_thread& ppu, vm::ptr<AdecContext> handle, vm::ptr<float> outBuffer)
{
	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	ppu.state += cpu_flag::wait;

	cellAdec.trace("cellAdecGetPcm(handle=0x%x, outBuffer=*0x%x)", handle, outBuffer);

	if (!handle)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	// If the pcm_handles are equal, then cellAdecGetPcmItem() was not called before cellAdecGetPcm(). We need to pop pcm_item_queue as well
	if (handle->pcm_item_queue.peek(ppu).pcm_handle == handle->pcm_queue.peek(ppu).pcm_handle)
	{
		handle->pcm_item_queue.pop(ppu);
	}

	const auto pcm_queue_entry = handle->pcm_queue.pop(ppu);

	if (!pcm_queue_entry)
	{
		return CELL_ADEC_ERROR_EMPTY;
	}

	const auto pcm_item = pcm_queue_entry->pcm_item;

	if (handle->verify_pcm_handle(pcm_item->pcmHandle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	if (handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_PAMF || handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_BLURAY || handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_DVD)
	{
		// TODO
	}

	if (error_code ret = handle->core_ops->realign(ppu, handle->core_handle, outBuffer, pcm_queue_entry->pcm_item->startAddr); ret != CELL_OK)
	{
		return ret;
	}

	if (error_code ret = handle->core_ops->releasePcm(ppu, handle->core_handle, pcm_item->pcmHandle, outBuffer); ret != CELL_OK)
	{
		return ret;
	}

	if (handle->unlink_frame(ppu, pcm_item->pcmHandle) == static_cast<s32>(CELL_ADEC_ERROR_FATAL))
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	handle->reset_frame(pcm_item->pcmHandle);

	return CELL_OK;
}

error_code cellAdecGetPcmItem(ppu_thread& ppu, vm::ptr<AdecContext> handle, vm::pptr<CellAdecPcmItem> pcmItem)
{
	cellAdec.trace("cellAdecGetPcmItem(handle=*0x%x, pcmItem=**0x%x)", handle, pcmItem);

	if (!handle)
	{
		return CELL_ADEC_ERROR_ARG;
	}

	if (!pcmItem)
	{
		return CELL_ADEC_ERROR_FATAL;
	}

	const auto pcm_item_entry = handle->pcm_item_queue.pop(ppu);

	if (ppu.state & cpu_flag::again) // Savestate was created while waiting on the queue mutex
	{
		return {};
	}

	if (!pcm_item_entry)
	{
		return CELL_ADEC_ERROR_EMPTY;
	}

	*pcmItem = pcm_item_entry->pcm_item;

	return CELL_OK;
}

DECLARE(ppu_module_manager::cellAdec)("cellAdec", []()
{
	REG_FUNC(cellAdec, cellAdecQueryAttr);
	REG_FUNC(cellAdec, cellAdecOpen);
	REG_FUNC(cellAdec, cellAdecOpenEx);
	REG_FUNC(cellAdec, cellAdecOpenExt); // 0xdf982d2c
	REG_FUNC(cellAdec, cellAdecClose);
	REG_FUNC(cellAdec, cellAdecStartSeq);
	REG_FUNC(cellAdec, cellAdecEndSeq);
	REG_FUNC(cellAdec, cellAdecDecodeAu);
	REG_FUNC(cellAdec, cellAdecGetPcm);
	REG_FUNC(cellAdec, cellAdecGetPcmItem);

	REG_HIDDEN_FUNC(adecNotifyAuDone);
	REG_HIDDEN_FUNC(adecNotifyPcmOut);
	REG_HIDDEN_FUNC(adecNotifyError);
	REG_HIDDEN_FUNC(adecNotifySeqDone);

	ppu_static_variable& lpcm_gvar = REG_VAR(cellAdec, g_cell_adec_core_ops_lpcm);
	lpcm_gvar.flags = MFF_HIDDEN;
	lpcm_gvar.init = []
	{
		g_cell_adec_core_ops_lpcm->getMemSize.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpGetMemSize_lpcm)));
		g_cell_adec_core_ops_lpcm->open.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpOpen_lpcm)));
		g_cell_adec_core_ops_lpcm->close.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpClose_lpcm)));
		g_cell_adec_core_ops_lpcm->startSeq.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpStartSeq_lpcm)));
		g_cell_adec_core_ops_lpcm->endSeq.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpEndSeq_lpcm)));
		g_cell_adec_core_ops_lpcm->decodeAu.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpDecodeAu_lpcm)));
		g_cell_adec_core_ops_lpcm->getVersion.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpGetVersion_lpcm)));
		g_cell_adec_core_ops_lpcm->realign.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpRealign_lpcm)));
		g_cell_adec_core_ops_lpcm->releasePcm.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpReleasePcm_lpcm)));
		g_cell_adec_core_ops_lpcm->getPcmHandleNum.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpGetPcmHandleNum_lpcm)));
		g_cell_adec_core_ops_lpcm->getBsiInfoSize.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpGetBsiInfoSize_lpcm)));
		g_cell_adec_core_ops_lpcm->openExt.set(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(_CellAdecCoreOpOpenExt_lpcm)));
	};

	REG_HIDDEN_FUNC(_CellAdecCoreOpGetMemSize_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpOpen_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpClose_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpStartSeq_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpEndSeq_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpDecodeAu_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpGetVersion_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpRealign_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpReleasePcm_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpGetPcmHandleNum_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpGetBsiInfoSize_lpcm);
	REG_HIDDEN_FUNC(_CellAdecCoreOpOpenExt_lpcm);
});

DECLARE(ppu_module_manager::cell_libac3dec)("cell_libac3dec", []
{
	REG_VNID(cell_libac3dec, 0xc58bb170, g_cell_adec_core_ops_ac3);
});

DECLARE(ppu_module_manager::cellAtrac3dec)("cellAtrac3dec", []
{
	REG_VNID(cellAtrac3dec, 0x60487d65, g_cell_adec_core_ops_atrac3);
});

DECLARE(ppu_module_manager::cellAtrac3multidec)("cellAtrac3multidec", []
{
	REG_VNID(cellAtrac3multidec, 0xc20c6bd7, g_cell_adec_core_ops_atrac3multi);
});

DECLARE(ppu_module_manager::cellCelp8Dec)("cellCelp8Dec", []
{
	REG_VNID(cellCelp8Dec, 0xf0190c6c, g_cell_adec_core_ops_Celp8);
	REG_FUNC(cellCelp8Dec, _SceAdecCorrectPtsValue_Celp8);
});

DECLARE(ppu_module_manager::cellCelpDec)("cellCelpDec", []
{
	REG_VNID(cellCelpDec, 0xffe42c22, g_cell_adec_core_ops_Celp);
	REG_FUNC(cellCelpDec, _SceAdecCorrectPtsValue_Celp);
});

DECLARE(ppu_module_manager::cellDDPdec)("cellDDPdec", []
{
	REG_VNID(cellDDPdec, 0xf5e9c15c, g_cell_adec_core_ops_Ddp);
	REG_FUNC(cellDDPdec, _SceAdecCorrectPtsValue_Ddp);
});

DECLARE(ppu_module_manager::cellDTSdec)("cellDTSdec", []
{
	REG_VNID(cellDTSdec, 0x15248ec5, g_cell_adec_core_ops_DtsCore);
	REG_FUNC(cellDTSdec, _SceAdecCorrectPtsValue_DtsCore);
});

DECLARE(ppu_module_manager::cellDTSHDCOREdec)("cellDTSHDCOREdec", []
{
	REG_VNID(cellDTSHDCOREdec, 0x6c8f4f1c, g_cell_adec_core_ops_DtsHd_Core);
	REG_FUNC(cellDTSHDCOREdec, _SceAdecCorrectPtsValue_DtsHd_Core);
});

DECLARE(ppu_module_manager::cellDTSHDdec)("cellDTSHDdec", []
{
	REG_VNID(cellDTSHDdec, 0x51ac2b6c, g_cell_adec_core_ops_DtsHd);
	REG_FUNC(cellDTSHDdec, _SceAdecCorrectPtsValue_DtsHd);
});

DECLARE(ppu_module_manager::cellDTSLBRdec)("cellDTSLBRdec", []
{
	REG_VNID(cellDTSLBRdec, 0x8c50af52, g_cell_adec_core_ops_DtsLbr);
	REG_FUNC(cellDTSLBRdec, _SceAdecCorrectPtsValue_DtsLbr);
});

DECLARE(ppu_module_manager::cellM2AACdec)("cellM2AACdec", []
{
	REG_VNID(cellM2AACdec, 0xac2f0831, g_cell_adec_core_ops_Aac);
	REG_FUNC(cellM2AACdec, _SceAdecCorrectPtsValue_Aac);
});

DECLARE(ppu_module_manager::cellM2BCdec)("cellM2BCdec", []
{
	REG_VNID(cellM2BCdec, 0x300afe4d, g_cell_adec_core_ops_mpmc);
	REG_FUNC(cellM2BCdec, _SceAdecCorrectPtsValue_mpmc);
});

DECLARE(ppu_module_manager::cellM4AacDec)("cellM4AacDec", []
{
	REG_VNID(cellM4AacDec, 0xab61278d, g_cell_adec_core_ops_M4Aac);
	REG_FUNC(cellM4AacDec, _SceAdecCorrectPtsValue_M4Aac);
});

DECLARE(ppu_module_manager::cellM4AacDec2ch)("cellM4AacDec2ch", []
{
	REG_VNID(cellM4AacDec2ch, 0xe996c664, g_cell_adec_core_ops_M4Aac2ch);
	REG_FUNC(cellM4AacDec2ch, _SceAdecCorrectPtsValue_M4Aac2ch);
});

DECLARE(ppu_module_manager::cellM4AacDec2chmod)("cellM4AacDec2chmod", []
{
	REG_VNID(cellM4AacDec2chmod, 0xdbd26836, g_cell_adec_core_ops_M4Aac2chmod);
	REG_FUNC(cellM4AacDec2chmod, _SceAdecCorrectPtsValue_M4Aac2chmod);
});

DECLARE(ppu_module_manager::cellMP3dec)("cellMP3dec", []
{
	REG_VNID(cellMP3dec, 0x84276a23, g_cell_adec_core_ops_Mp3);
});

DECLARE(ppu_module_manager::cellMP3Sdec)("cellMP3Sdec", []
{
	REG_VNID(cellMP3Sdec, 0x292cdf0a, g_cell_adec_core_ops_Mp3s);
	REG_FUNC(cellMP3Sdec, _SceAdecCorrectPtsValue_Mp3s);
});

DECLARE(ppu_module_manager::cellMPL1dec)("cellMPL1dec", []
{
	REG_VNID(cellMPL1dec, 0xbbc70551, g_cell_adec_core_ops_mpmcl1);
	REG_FUNC(cellMPL1dec, _SceAdecCorrectPtsValue_mpmcl1);
});

DECLARE(ppu_module_manager::cellTRHDdec)("cellTRHDdec", []
{
	REG_VNID(cellTRHDdec, 0xe88e381b, g_cell_adec_core_ops_truehd);
	REG_FUNC(cellTRHDdec, _SceAdecCorrectPtsValue_truehd);
});

DECLARE(ppu_module_manager::cellWMAdec)("cellWMAdec", []
{
	REG_VNID(cellWMAdec, 0x88320b10, g_cell_adec_core_ops_wma);
	REG_FUNC(cellWMAdec, _SceAdecCorrectPtsValue_wma);
});

DECLARE(ppu_module_manager::cellWMALSLdec)("cellWMALSLdec", []
{
	REG_VNID(cellWMALSLdec, 0x602aab16, g_cell_adec_core_ops_WmaLsl);
	REG_FUNC(cellWMALSLdec, _SceAdecCorrectPtsValue_WmaLsl);
});

DECLARE(ppu_module_manager::cellWMAPROdec)("cellWMAPROdec", []
{
	REG_VNID(cellWMAPROdec, 0xa8bac670, g_cell_adec_core_ops_WmaPro);
	REG_FUNC(cellWMAPROdec, _SceAdecCorrectPtsValue_WmaPro);
});
