#include "stdafx.h"
#include "Emu/perf_meter.hpp"
#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_ppu_thread.h"
#include "Emu/savestate_utils.hpp"
#include "sysPrxForUser.h"

#include "cellAdec.h"

#include "util/simd.hpp"
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

void LpcmDecContext::exec(ppu_thread& ppu)
{
	perf_meter<"LPCMDEC"_u64> perf0;

	switch (savestate)
	{
	case lpcm_dec_state::waiting_for_cmd_mutex_lock: break;
	case lpcm_dec_state::waiting_for_cmd_cond_wait: break;
	case lpcm_dec_state::waiting_for_output_mutex_lock: goto output_mutex_lock;
	case lpcm_dec_state::waiting_for_output_cond_wait: goto output_cond_wait;
	case lpcm_dec_state::queue_mutex_lock: goto queue_mutex_lock;
	case lpcm_dec_state::executing_cmd: goto execute_cmd;
	}

	for (; run_thread; cmd_counter++)
	{
		cellAdec.trace("Command counter: %llu, waiting for next command...", cmd_counter);

		// Wait for a command to become available
		error_occurred |= static_cast<u32>(cmd_available.acquire(ppu, savestate) != CELL_OK);

		if (ppu.state & cpu_flag::again)
		{
			return;
		}

		cellAdec.trace("Command available, waiting for output to be consumed...");

		// Wait for the output to be consumed.
		// The output has to be consumed even if the next command is not a decode command
		savestate = lpcm_dec_state::waiting_for_output_mutex_lock;
		output_mutex_lock:

		error_occurred |= static_cast<u32>(sys_mutex_lock(ppu, output_mutex, 0) != CELL_OK);

		if (ppu.state & cpu_flag::again)
		{
			return;
		}

		while (output_locked)
		{
			savestate = lpcm_dec_state::waiting_for_output_cond_wait;
			output_cond_wait:

			ensure(sys_cond_wait(ppu, output_consumed, 0) == CELL_OK); // Error code isn't checked on LLE

			if (ppu.state & cpu_flag::again)
			{
				return;
			}
		}

		cellAdec.trace("Output consumed");

		// Pop command from queue
		savestate = lpcm_dec_state::queue_mutex_lock;
		queue_mutex_lock:

		ensure(sys_mutex_lock(ppu, queue_mutex, 0) == CELL_OK); // Error code isn't checked on LLE

		if (ppu.state & cpu_flag::again)
		{
			return;
		}

		cmd_queue.pop(cmd);

		ensure(sys_mutex_unlock(ppu, queue_mutex) == CELL_OK); // Error code isn't checked on LLE

		cellAdec.trace("Command type: %d", static_cast<u32>(cmd.type.get()));

		savestate = lpcm_dec_state::executing_cmd;
		execute_cmd:

		switch (cmd.type)
		{
		case LpcmDecCmdType::start_seq:
			// LLE sends a command to the SPU thread. The SPU thread consumes the command without doing anything, however
			error_occurred |= static_cast<u32>(sys_mutex_unlock(ppu, output_mutex) != CELL_OK);
			break;

		case LpcmDecCmdType::end_seq:
		{
			// Block savestate creation during callbacks
			std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

			if (!savestate_lock.owns_lock())
			{
				ppu.state += cpu_flag::again;
				return;
			}

			// Doesn't do anything else
			notify_seq_done.cbFunc(ppu, notify_seq_done.cbArg);

			error_occurred |= static_cast<u32>(sys_mutex_unlock(ppu, output_mutex) != CELL_OK);
			break;
		}
		case LpcmDecCmdType::close:
			ensure(sys_mutex_unlock(ppu, output_mutex) == CELL_OK); // Error code isn't checked on LLE
			return;

		case LpcmDecCmdType::decode_au:
		{
			// For 20 and 24-bit samples
			const u8* const input_u8 = static_cast<const u8*>(cmd.au_start_addr.get_ptr());
			const s64 au_size_u8 = cmd.au_size;

			// For 16-bit samples
			const be_t<s16, 1>* const input_s16 = static_cast<const be_t<s16, 1>*>(cmd.au_start_addr.get_ptr());
			const s64 au_size_s16 = static_cast<s64>(au_size_u8 / sizeof(s16));

			be_t<f32>* const _output = std::assume_aligned<0x80>(output.get_ptr());
			s64 output_size = cmd.au_size;

			s32 sample_num = static_cast<s32>(utils::align(+lpcm_param->audioPayloadSize, 0x10));
			s32 channel_num = 0;

			if (!dvd_packing)
			{
				switch (lpcm_param->sizeOfWord)
				{
				case CELL_ADEC_BIT_LENGTH_16: output_size = output_size * 32 / 16; sample_num /= 2; break;
				case CELL_ADEC_BIT_LENGTH_20: // Same as below
				case CELL_ADEC_BIT_LENGTH_24: output_size = output_size * 32 / 24; sample_num /= 3; break;
				default: ; // LLE skips decoding entirely, the output buffer isn't written to, and it outputs whatever was there before
				}

				// LPCM streams with an odd number of channels contain an empty dummy channel
				switch (lpcm_param->channelNumber)
				{
				case CELL_ADEC_CH_MONO:    channel_num = 1; output_size = output_size * 1 / 2; break;
				case CELL_ADEC_CH_STEREO:  channel_num = 2; break;
				case CELL_ADEC_CH_3_0:     // Same as below
				case CELL_ADEC_CH_2_1:     channel_num = 3; output_size = output_size * 3 / 4; break;
				case CELL_ADEC_CH_3_1:     // Same as below
				case CELL_ADEC_CH_2_2:     channel_num = 4; break;
				case CELL_ADEC_CH_3_2:     channel_num = 5; output_size = output_size * 5 / 6; break;
				case CELL_ADEC_CH_3_2_LFE: channel_num = 6; break;
				case CELL_ADEC_CH_3_4:     channel_num = 7; output_size = output_size * 7 / 8; break;
				case CELL_ADEC_CH_3_4_LFE: channel_num = 8; break;
				default: ; // Don't do anything, LLE simply skips reordering channels
				}

				// LLE doesn't check the output size
				ensure(output_size <= LPCM_DEC_OUTPUT_BUFFER_SIZE);
				ensure(sample_num * sizeof(f32) <= LPCM_DEC_OUTPUT_BUFFER_SIZE);

				// Convert to float
				if (lpcm_param->sizeOfWord == CELL_ADEC_BIT_LENGTH_16)
				{
					s64 i = 0;
					for (; i <= au_size_s16 - 8; i += 8)
					{
						const v128 s16be = v128::loadu(&input_s16[i]);

						// Convert endianess if necessary and shift left by 16
#if defined(ARCH_X64) && !defined(__SSSE3__)
						const v128 s16le = gv_rol16<8>(s16be);
						const v128 s32_1 = gv_unpacklo16(gv_bcst16(0), s16le);
						const v128 s32_2 = gv_unpackhi16(gv_bcst16(0), s16le);
#else
						const v128 s32_1 = std::endian::native == std::endian::little
							? gv_shuffle8(s16be, v128::normal_array_t<s8>{ -1, -1, 1, 0, -1, -1, 3, 2, -1, -1, 5, 4, -1, -1, 7, 6 })
							: gv_unpacklo16(s16be, gv_bcst16(0));

						const v128 s32_2 = std::endian::native == std::endian::little
							? gv_shuffle8(s16be, v128::normal_array_t<s8>{ -1, -1, 9, 8, -1, -1, 11, 10, -1, -1, 13, 12, -1, -1, 15, 14 })
							: gv_unpackhi16(s16be, gv_bcst16(0));
#endif
						// Convert to float and divide by INT32_MAX + 1
						const v128 f32_1 = gv_mulfs(gv_cvts32_tofs(s32_1), 1.f / static_cast<f32>(0x80000000u));
						const v128 f32_2 = gv_mulfs(gv_cvts32_tofs(s32_2), 1.f / static_cast<f32>(0x80000000u));

						v128::storeu(gv_to_be32(f32_1), &_output[i]);
						v128::storeu(gv_to_be32(f32_2), &_output[i + 4]);
					}

					for (; i < au_size_s16; i++)
					{
						_output[i] = static_cast<f32>(input_s16[i]) / 0x8000u;
					}
				}
				else if (lpcm_param->sizeOfWord == CELL_ADEC_BIT_LENGTH_20 || lpcm_param->sizeOfWord == CELL_ADEC_BIT_LENGTH_24)
				{
					s64 i = 0;
					for (; i * 3 <= au_size_u8 - static_cast<s64>(sizeof(v128)); i += 4)
					{
						// Load four samples, convert endianness if necessary and shift left by 8
						const v128 _s32 = std::endian::native == std::endian::little
							? gv_shuffle8(v128::loadu(&input_u8[i * 3]), v128::normal_array_t<s8>{ -1, 2, 1, 0, -1, 5, 4, 3, -1, 8, 7, 6, -1, 11, 10, 9 })
							: gv_shuffle8(v128::loadu(&input_u8[i * 3]), v128::normal_array_t<s8>{ 0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1 });

						// Convert to float and divide by INT32_MAX + 1
						const v128 _f32 = gv_mulfs(gv_cvts32_tofs(_s32), 1.f / static_cast<f32>(0x80000000u));

						v128::storeu(gv_to_be32(_f32), &_output[i]);
					}

					for (; i * 3 <= au_size_u8 - 3; i++)
					{
						alignas(alignof(s32)) const u8 s32le[4] = { 0, input_u8[i * 3 + 2], input_u8[i * 3 + 1], input_u8[i * 3] };

						_output[i] = static_cast<f32>(std::bit_cast<le_t<s32>>(s32le)) / static_cast<f32>(0x80000000u);
					}
				}

				// Reorder channels and remove the dummy channel

				// Input channel order:
				// Front Left, Front Right, Center, Side Left, Rear Left, Rear Right, Side Right, LFE

				// Output channel order:
				// - up to 3_4: Front Left, Center, Front Right, Side Left, Side Right, Rear Left, Rear Right, LFE
				// - 3_4_LFE: Front Left, Front Right, Center, LFE, Side Left, Side Right, Rear Left, Rear Right

				// The following loops can access sample_num % channel_num * sizeof(f32) bytes more than LPCM_DEC_OUTPUT_BUFFER_SIZE (up to 28 bytes).
				// This is intended, LLE does something similar. The buffer is much larger than LPCM_DEC_OUTPUT_BUFFER_SIZE (see _CellAdecCoreOpGetMemSize_lpcm())
				switch (lpcm_param->channelNumber)
				{
				case CELL_ADEC_CH_MONO:
					for (s32 i = 0; i < sample_num / 2; i += 4)
					{
						const v128 tmp1 = v128::loadu(&_output[i * 2]);
						const v128 tmp2 = v128::loadu(&_output[i * 2 + 4]);
						v128::storeu(gv_shufflefs<0, 2, 0, 2>(tmp1, tmp2), &_output[i]); // Remove every other sample
					}
					break;

				case CELL_ADEC_CH_STEREO:
				case CELL_ADEC_CH_2_2:
					// Input order == output order, no need to do anything
					break;

				case CELL_ADEC_CH_3_0:
					for (s32 i_in = 0, i_out = 0; i_in < sample_num; i_in += 4, i_out += 3)
					{
						const v128 tmp = gv_shuffle32<0, 2, 1, 3>(v128::loadu(&_output[i_in])); // Swap Front Right and Center
						v128::storeu(tmp, &_output[i_out]);
					}
					break;

				case CELL_ADEC_CH_2_1:
					for (s32 i_in = 0, i_out = 0; i_in < sample_num; i_in += 4, i_out += 3)
					{
						v128::storeu(v128::loadu(&_output[i_in]), &_output[i_out]);
					}
					break;

				case CELL_ADEC_CH_3_1:
				case CELL_ADEC_CH_3_2_LFE:
					for (s32 i = 0; i < sample_num; i += channel_num)
					{
						const u64 tmp = std::rotl(read_from_ptr<u64>(&_output[i + 1]), 0x20); // Swap Front Right and Center
						std::memcpy(&_output[i + 1], &tmp, sizeof(u64));
					}
					break;

				case CELL_ADEC_CH_3_2:
					for (s32 i_in = 0, i_out = 0; i_in < sample_num; i_in += 6, i_out += 5)
					{
						const v128 tmp = gv_shuffle32<0, 2, 1, 3>(v128::loadu(&_output[i_in])); // Swap Front Right and Center
						v128::storeu(tmp, &_output[i_out]);
						_output[i_out + 4] = _output[i_in + 4];
					}
					break;

				case CELL_ADEC_CH_3_4:
					for (s32 i_in = 0, i_out = 0; i_in < sample_num; i_in += 8, i_out += 7)
					{
						const v128 tmp1 = gv_shuffle32<0, 2, 1, 3>(v128::loadu(&_output[i_in])); // Swap Front Right and Center
						const v128 tmp2 = gv_shuffle32<2, 0, 1, 3>(v128::loadu(&_output[i_in + 4])); // Reorder Rear Left, Rear Right, Side Right -> Side Right, Rear Left, Rear Right
						v128::storeu(tmp1, &_output[i_out]);
						v128::storeu(tmp2, &_output[i_out + 4]);
					}
					break;

				case CELL_ADEC_CH_3_4_LFE:
					for (s32 i = 0; i < sample_num; i += 8)
					{
						const v128 tmp1 = gv_shuffle32<3, 2, 0, 1>(v128::loadu(&_output[i + 4])); // Reorder Rear Left, Rear Right, Side Right, LFE -> LFE, Side Right, Rear Left, Rear Right
						v128::storeu(tmp1, &_output[i + 4]);
						const u64 tmp2 = std::rotl(read_from_ptr<u64>(&_output[i + 3]), 0x20); // Swap Side Left and LFE
						std::memcpy(&_output[i + 3], &tmp2, sizeof(u64));
					}
					break;

				default:
					; // Don't do anything
				}
			}
			else
			{
				switch (lpcm_param->sizeOfWord)
				{
				case CELL_ADEC_BIT_LENGTH_16: output_size = output_size * 32 / 16; break;
				case CELL_ADEC_BIT_LENGTH_20: output_size = output_size * 32 / 20; break;
				case CELL_ADEC_BIT_LENGTH_24: output_size = output_size * 32 / 24; break;
				default: fmt::throw_exception("Unreachable"); // Parameters get verified in adecSetLpcmDvdParams()
				}

				// Only the front left and right channels are decoded, all other channels are ignored
				switch (lpcm_param->channelNumber)
				{
				case CELL_ADEC_LPCM_DVD_CH_MONO: // Set channel_num to two for mono as well
				case CELL_ADEC_LPCM_DVD_CH_STEREO:  channel_num = 2; break;
				case 4:                             channel_num = 3; output_size = output_size * 2 / 3; break;
				case 5:                             channel_num = 4; output_size = output_size * 2 / 4; break;
				case CELL_ADEC_LPCM_DVD_CH_3_2:     channel_num = 5; output_size = output_size * 2 / 5; break;
				case CELL_ADEC_LPCM_DVD_CH_3_2_LFE: channel_num = 6; output_size = output_size * 2 / 6; break;
				case CELL_ADEC_LPCM_DVD_CH_3_4:     channel_num = 7; output_size = output_size * 2 / 7; break;
				case CELL_ADEC_LPCM_DVD_CH_3_4_LFE: channel_num = 8; output_size = output_size * 2 / 8; break;
				default: fmt::throw_exception("Unreachable"); // Parameters get verified in adecSetLpcmDvdParams()
				}

				// LLE doesn't check the output size
				ensure(output_size <= LPCM_DEC_OUTPUT_BUFFER_SIZE);

				// Convert to float
				switch (lpcm_param->sizeOfWord)
				{
				case CELL_ADEC_BIT_LENGTH_16:
				{
					s64 i_in = 0;
					s64 i_out = 0;
					for (; i_in <= au_size_s16 - channel_num - 2; i_in += channel_num * 2, i_out += 4)
					{
						// Load four samples
						const v128 tmp1 = gv_loadu32(&input_s16[i_in]);
						const v128 tmp2 = gv_loadu32(&input_s16[i_in + channel_num]);
						const v128 s16be = gv_unpacklo32(tmp1, tmp2);

						// Convert endianess if necessary and shift left by 16
						const v128 _s32 = std::endian::native == std::endian::little
							? gv_shuffle8(s16be, v128::normal_array_t<s8>{ -1, -1, 1, 0, -1, -1, 3, 2, -1, -1, 5, 4, -1, -1, 7, 6 })
							: gv_unpacklo16(s16be, gv_bcst16(0));

						// Convert to float and divide by INT32_MAX + 1
						const v128 _f32 = gv_mulfs(gv_cvts32_tofs(_s32), 1.f / static_cast<f32>(0x80000000u));

						v128::storeu(gv_to_be32(_f32), &_output[i_out]);
					}

					for (; i_in <= au_size_s16 - 2; i_in += channel_num, i_out += 2)
					{
						const v128 s16be = gv_loadu32(&input_s16[i_in]);

						const v128 _s32 = std::endian::native == std::endian::little
							? gv_shuffle8(s16be, v128::normal_array_t<s8>{ -1, -1, 1, 0, -1, -1, 3, 2, -1, -1, 5, 4, -1, -1, 7, 6 })
							: gv_unpacklo16(s16be, gv_bcst16(0));

						const v128 _f32 = gv_mulfs(gv_cvts32_tofs(_s32), 1.f / static_cast<f32>(0x80000000u));

						std::memcpy(&_output[i_out], &gv_to_be32(_f32)._u64[0], sizeof(u64));
					}
					break;
				}
				case CELL_ADEC_BIT_LENGTH_20:
				{
					const s64 high_bytes_3_4_offset = lpcm_param->channelNumber == CELL_ADEC_LPCM_DVD_CH_MONO ? 5 : channel_num * 2;
					const s64 low_bits_1_2_offset = lpcm_param->channelNumber == CELL_ADEC_LPCM_DVD_CH_MONO ? 4 : channel_num * 4;
					const s64 low_bits_3_4_offset = channel_num * 4 + channel_num / 2 - !(channel_num & 1);
					const s64 next_samples_offset = channel_num * 5;

					// If channel_num is odd, the low bits of samples three and four are in different bytes
					alignas(alignof(v128)) static constexpr auto shuffle_ctrl_same_offset = std::endian::native == std::endian::little
						? v128::normal_array_t<s8>{ -1, 8, 1, 0, -1, 8, 3, 2, -1, 11, 5, 4, -1, 11, 7, 6 }
						: v128::normal_array_t<s8>{ 0, 1, 8, -1, 2, 3, 8, -1, 4, 5, 11, -1, 6, 7, 11, -1 };

					alignas(alignof(v128)) static constexpr auto shuffle_ctrl_different_offset = std::endian::native == std::endian::little
						? v128::normal_array_t<s8>{ -1, 8, 1, 0, -1, 8, 3, 2, -1, 10, 5, 4, -1, 11, 7, 6 }
						: v128::normal_array_t<s8>{ 0, 1, 8, -1, 2, 3, 8, -1, 4, 5, 10, -1, 6, 7, 11, -1 };

					const v128 shuffle_ctrl = channel_num & 1 ? v128::loadu(&shuffle_ctrl_different_offset) : v128::loadu(&shuffle_ctrl_same_offset);

					alignas(alignof(v128)) static constexpr auto low_bits_mask_same_offset = std::endian::native == std::endian::little
						? v128::normal_array_t<u8>{ 0x00, 0xf0, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0x00, 0xf0, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff }
						: v128::normal_array_t<u8>{ 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 0x0f, 0x00, 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 0x0f, 0x00 };

					alignas(alignof(v128)) static constexpr auto low_bits_mask_different_offset = std::endian::native == std::endian::little
						? v128::normal_array_t<u8>{ 0x00, 0xf0, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0x00, 0xf0, 0xff, 0xff }
						: v128::normal_array_t<u8>{ 0xff, 0xff, 0xf0, 0x00, 0xff, 0xff, 0x0f, 0x00, 0xff, 0xff, 0x0f, 0x00, 0xff, 0xff, 0xf0, 0x00 };

					const v128 low_bits_mask = channel_num & 1 ? v128::loadu(&low_bits_mask_different_offset) : v128::loadu(&low_bits_mask_same_offset);

					for (s64 i_in = 0, i_out = 0; i_in <= au_size_u8 - low_bits_3_4_offset - (channel_num & 1); i_in += next_samples_offset, i_out += 4)
					{
						// Load all the high and low bits of four samples
						const v128 tmp1 = gv_loadu32(&input_u8[i_in]);
						const v128 tmp2 = gv_loadu32(&input_u8[i_in + high_bytes_3_4_offset]);
						v128 s20be = gv_unpacklo32(tmp1, tmp2);
						s20be = gv_insert16<4>(s20be, read_from_ptr<u16>(&input_u8[i_in + low_bits_1_2_offset]));
						s20be = gv_insert16<5>(s20be, read_from_ptr<u16>(&input_u8[i_in + low_bits_3_4_offset]));

						// Reorder bytes to form four 32-bit integer samples
						v128 _s32 = gv_shuffle8(s20be, shuffle_ctrl);

						// Set low 12 bits to zero for each sample
						_s32 = _s32 & low_bits_mask;

						// LLE is missing a step: each byte that was ANDed with 0x0f would still need to be shifted left by 4

						// Convert to float and divide by INT32_MAX + 1
						const v128 _f32 = gv_mulfs(gv_cvts32_tofs(_s32), 1.f / static_cast<f32>(0x80000000u));

						v128::storeu(gv_to_be32(_f32), &_output[i_out]);
					}
					break;
				}
				case CELL_ADEC_BIT_LENGTH_24:
				{
					const s64 high_bytes_3_4_offset = lpcm_param->channelNumber == CELL_ADEC_LPCM_DVD_CH_MONO ? 6 : channel_num * 2;
					const s64 low_bytes_1_2_offset = lpcm_param->channelNumber == CELL_ADEC_LPCM_DVD_CH_MONO ? 4 : channel_num * 4;
					const s64 low_bytes_3_4_offset = channel_num * 5;
					const s64 next_samples_offset = channel_num * 6;

					for (s64 i_in = 0, i_out = 0; i_in <= au_size_u8 - low_bytes_3_4_offset - 2; i_in += next_samples_offset, i_out += 4)
					{
						// Load all the high and low bytes of four samples
						const v128 tmp1 = gv_loadu32(&input_u8[i_in]);
						const v128 tmp2 = gv_loadu32(&input_u8[i_in + high_bytes_3_4_offset]);
						v128 s24be = gv_unpacklo32(tmp1, tmp2);
						s24be = gv_insert16<4>(s24be, read_from_ptr<u16>(&input_u8[i_in + low_bytes_1_2_offset]));
						s24be = gv_insert16<5>(s24be, read_from_ptr<u16>(&input_u8[i_in + low_bytes_3_4_offset]));

						// Reorder bytes to form four 32-bit integer samples
						const v128 _s32 = std::endian::native == std::endian::little
							? gv_shuffle8(s24be, v128::normal_array_t<s8>{ -1, 8, 1, 0, -1, 9, 3, 2, -1, 10, 5, 4, -1, 11, 7, 6 })
							: gv_shuffle8(s24be, v128::normal_array_t<s8>{ 0, 1, 8, -1, 2, 3, 9, -1, 4, 5, 10, -1, 6, 7, 11, -1 });

						// Convert to float and divide by INT32_MAX + 1
						const v128 _f32 = gv_mulfs(gv_cvts32_tofs(_s32), 1.f / static_cast<f32>(0x80000000u));

						v128::storeu(gv_to_be32(_f32), &_output[i_out]);
					}
				}
				}
			}

			// Block savestate creation during callbacks
			std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

			if (!savestate_lock.owns_lock())
			{
				ppu.state += cpu_flag::again;
				return;
			}

			if (error_occurred)
			{
				notify_error.cbFunc(ppu, CELL_ADEC_ERROR_FATAL, notify_error.cbArg);
			}

			notify_au_done.cbFunc(ppu, cmd.pcm_handle, notify_au_done.cbArg);

			output_locked = true;
			error_occurred |= static_cast<u32>(sys_mutex_unlock(ppu, output_mutex) != CELL_OK);

			const vm::var<CellAdecLpcmInfo> bsi_info{{ lpcm_param->channelNumber, lpcm_param->sampleRate, static_cast<u32>(output_size) }};

			notify_pcm_out.cbFunc(ppu, cmd.pcm_handle, output, static_cast<u32>(output_size), notify_pcm_out.cbArg, vm::make_var<vm::bcptr<void>>(bsi_info), ADEC_CORRECT_PTS_VALUE_TYPE_LPCM_HDMV, error_occurred ? static_cast<s32>(CELL_ADEC_ERROR_FATAL) : CELL_OK);
			break;
		}
		default:
			fmt::throw_exception("Invalid command");
		}
	}
}

template <LpcmDecCmdType type>
error_code LpcmDecContext::send_command(ppu_thread& ppu, auto&&... args)
{
	ppu.state += cpu_flag::wait;

	if (error_code ret = sys_mutex_lock(ppu, queue_size_mutex, 0); ret != CELL_OK)
	{
		return ret;
	}

	if (cmd_queue.full())
	{
		ensure(sys_mutex_unlock(ppu, queue_size_mutex) == CELL_OK); // Error code isn't checked on LLE
		return CELL_ADEC_ERROR_BUSY;
	}

	// LLE copies the parameters directly into the context
	if constexpr (type == LpcmDecCmdType::start_seq)
	{
		*lpcm_param = { args... };
	}

	if (error_code ret = sys_mutex_lock(ppu, queue_mutex, 0); ret != CELL_OK)
	{
		ensure(sys_mutex_unlock(ppu, queue_size_mutex) == CELL_OK); // Error code isn't checked on LLE
		return ret;
	}

	cmd_queue.emplace(type, std::forward<decltype(args)>(args)...);

	if (error_code ret = sys_mutex_unlock(ppu, queue_mutex); ret != CELL_OK
		|| (ret = cmd_available.release(ppu)) != CELL_OK)
	{
		ensure(sys_mutex_unlock(ppu, queue_size_mutex) == CELL_OK); // Error code isn't checked on LLE
		return ret;
	}

	return sys_mutex_unlock(ppu, queue_size_mutex);
}

inline error_code LpcmDecContext::release_output(ppu_thread& ppu)
{
	if (error_code ret = sys_mutex_lock(ppu, output_mutex, 0); ret != CELL_OK)
	{
		return ret;
	}

	output_locked = false;

	if (error_code ret = sys_cond_signal(ppu, output_consumed); ret != CELL_OK)
	{
		return ret; // LLE doesn't unlock the mutex
	}

	return sys_mutex_unlock(ppu, output_mutex);
}

void lpcmDecEntry(ppu_thread& ppu, vm::ptr<LpcmDecContext> lpcm_dec)
{
	lpcm_dec->exec(ppu);

	if (ppu.state & cpu_flag::again)
	{
		// For savestates, save argument
		ppu.syscall_args[0] = lpcm_dec.addr();

		return;
	}

	ppu_execute<&sys_ppu_thread_exit>(ppu, CELL_OK);
}

error_code _CellAdecCoreOpGetMemSize_lpcm(vm::ptr<CellAdecAttr> attr)
{
	cellAdec.notice("_CellAdecCoreOpGetMemSize_lpcm(attr=*0x%x)", attr);

	constexpr u32 mem_size =
		utils::align(static_cast<u32>(sizeof(LpcmDecContext)), 0x80)
		+ utils::align(static_cast<u32>(sizeof(CellAdecParamLpcm)), 0x80)
		+ 0x100 // Command data for Spurs task
		+ LPCM_DEC_OUTPUT_BUFFER_SIZE
		+ 0x2900 // sizeof(CellSpurs) + sizeof(CellSpursTaskset)
		+ 0x3b400 // Spurs context
		+ 0x300 // (sizeof(CellSpursQueue) + 0x80 + queue buffer) * 2
		+ 0x855; // Unused

	static_assert(mem_size == 0x7ebd5);

	attr->workMemSize = mem_size;

	return CELL_OK;
}

error_code _CellAdecCoreOpOpenExt_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle, vm::ptr<AdecNotifyAuDone> notifyAuDone, vm::ptr<void> notifyAuDoneArg, vm::ptr<AdecNotifyPcmOut> notifyPcmOut, vm::ptr<void> notifyPcmOutArg,
	vm::ptr<AdecNotifyError> notifyError, vm::ptr<void> notifyErrorArg, vm::ptr<AdecNotifySeqDone> notifySeqDone, vm::ptr<void> notifySeqDoneArg, vm::cptr<CellAdecResource> res, vm::cptr<CellAdecResourceSpurs> spursRes)
{
	cellAdec.notice("_CellAdecCoreOpOpenExt_lpcm(handle=*0x%x, notifyAuDone=*0x%x, notifyAuDoneArg=0x%x, notifyPcmOut=*0x%x, notifyPcmOutArg=0x%x, notifyError=*0x%x, notifyErrorArg=0x%x, notifySeqDone=*0x%x, notifySeqDoneArg=0x%x, res=*0x%x, spursRes=*0x%x)",
		handle, notifyAuDone, notifyAuDoneArg, notifyPcmOut, notifyPcmOutArg, notifyError, notifyErrorArg, notifySeqDone, notifySeqDoneArg, res, spursRes);

	ensure(!!handle && !!res); // Not checked on LLE
	ensure(handle.aligned(0x80)); // LLE doesn't check the alignment or aligns the address itself
	ensure(!!notifyAuDone && !!notifyAuDoneArg && !!notifyPcmOut && !!notifyPcmOutArg && !!notifyError && !!notifyErrorArg && !!notifySeqDone && !!notifySeqDoneArg); // These should always be set

	const u32 end_of_context_addr = handle.addr() + utils::align(static_cast<u32>(sizeof(LpcmDecContext)), 0x80);

	handle->cmd_queue.front = 0;
	handle->cmd_queue.back = 0;
	handle->cmd_queue.size = 0;
	handle->run_thread = true;
	handle->notify_au_done = { notifyAuDone, notifyAuDoneArg };
	handle->notify_pcm_out = { notifyPcmOut, notifyPcmOutArg };
	handle->notify_error = { notifyError, notifyErrorArg };
	handle->notify_seq_done = { notifySeqDone, notifySeqDoneArg };
	handle->output = vm::bptr<f32>::make(end_of_context_addr + 0x180);
	handle->lpcm_param.set(end_of_context_addr);
	handle->error_occurred = false;

	const vm::var<sys_mutex_attribute_t> mutex_attr{{ SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem04"_u64 } }};
	const vm::var<sys_mutex_attribute_t> output_mutex_attr{{ SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem05"_u64 } }};
	const vm::var<sys_mutex_attribute_t> queue_mutex_attr{{ SYS_SYNC_PRIORITY, SYS_SYNC_NOT_RECURSIVE, SYS_SYNC_NOT_PROCESS_SHARED, SYS_SYNC_NOT_ADAPTIVE, 0, 0, 0, { "_adem06"_u64 } }};
	const vm::var<sys_cond_attribute_t> cond_attr{{ SYS_SYNC_NOT_PROCESS_SHARED, 0, 0, { "_adec03"_u64 } }};

	error_code ret = sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::queue_size_mutex), mutex_attr);
	ret = ret ? ret : sys_cond_create(ppu, handle.ptr(&LpcmDecContext::queue_size_cond), handle->queue_size_mutex, cond_attr);
	ret = ret ? ret : sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::unk_mutex), mutex_attr);
	ret = ret ? ret : sys_cond_create(ppu, handle.ptr(&LpcmDecContext::unk_cond), handle->unk_mutex, cond_attr);
	ret = ret ? ret : sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::output_mutex), output_mutex_attr);
	ret = ret ? ret : sys_cond_create(ppu, handle.ptr(&LpcmDecContext::output_consumed), handle->output_mutex, cond_attr);
	ret = ret ? ret : sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::queue_mutex), queue_mutex_attr);
	ret = ret ? ret : handle->release_output(ppu);
	ret = ret ? ret : handle->cmd_available.init(ppu, handle.ptr(&LpcmDecContext::cmd_available), 0);
	ret = ret ? ret : handle->reserved2.init(ppu, handle.ptr(&LpcmDecContext::reserved2), 0);

	if (ret != CELL_OK)
	{
		return ret;
	}

	// HLE exclusive
	handle->savestate = lpcm_dec_state::waiting_for_cmd_mutex_lock;
	handle->cmd_counter = 0;

	const vm::var<char[]> _name = vm::make_str("HLE LPCM decoder");
	const auto entry = g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(lpcmDecEntry));

	ret = ppu_execute<&sys_ppu_thread_create>(ppu, handle.ptr(&LpcmDecContext::thread_id), entry, handle.addr(), +res->ppuThreadPriority, +res->ppuThreadStackSize, SYS_PPU_THREAD_CREATE_JOINABLE, +_name);
	ret = ret ? ret : sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::spurs_queue_pop_mutex), mutex_attr);
	ret = ret ? ret : sys_mutex_create(ppu, handle.ptr(&LpcmDecContext::spurs_queue_push_mutex), mutex_attr);

	return ret;
}

error_code _CellAdecCoreOpOpen_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle, vm::ptr<AdecNotifyAuDone> notifyAuDone, vm::ptr<void> notifyAuDoneArg, vm::ptr<AdecNotifyPcmOut> notifyPcmOut, vm::ptr<void> notifyPcmOutArg,
	vm::ptr<AdecNotifyError> notifyError, vm::ptr<void> notifyErrorArg, vm::ptr<AdecNotifySeqDone> notifySeqDone, vm::ptr<void> notifySeqDoneArg, vm::cptr<CellAdecResource> res)
{
	cellAdec.notice("_CellAdecCoreOpOpen_lpcm(handle=*0x%x, notifyAuDone=*0x%x, notifyAuDoneArg=*0x%x, notifyPcmOut=*0x%x, notifyPcmOutArg=*0x%x, notifyError=*0x%x, notifyErrorArg=*0x%x, notifySeqDone=*0x%x, notifySeqDoneArg=*0x%x, res=*0x%x)",
		handle, notifyAuDone, notifyAuDoneArg, notifyPcmOut, notifyPcmOutArg, notifyError, notifyErrorArg, notifySeqDone, notifySeqDoneArg, res);

	return _CellAdecCoreOpOpenExt_lpcm(ppu, handle, notifyAuDone, notifyAuDoneArg, notifyPcmOut, notifyPcmOutArg, notifyError, notifyErrorArg, notifySeqDone, notifySeqDoneArg, res, vm::null);
}

error_code _CellAdecCoreOpClose_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle)
{
	ppu.state += cpu_flag::wait;

	cellAdec.notice("_CellAdecCoreOpClose_lpcm(handle=*0x%x)", handle);

	if (error_code ret = sys_mutex_lock(ppu, handle->queue_size_mutex, 0); ret != CELL_OK
		|| (ret = sys_mutex_lock(ppu, handle->queue_mutex, 0)) != CELL_OK)
	{
		return ret;
	}

	if (handle->cmd_queue.empty())
	{
		handle->cmd_queue.emplace(LpcmDecCmdType::close);

		if (error_code ret = sys_mutex_unlock(ppu, handle->queue_mutex); ret != CELL_OK)
		{
			return ret; // LLE doesn't unlock the queue size mutex
		}

		if (error_code ret = handle->cmd_available.release(ppu); ret != CELL_OK)
		{
			ensure(sys_mutex_unlock(ppu, handle->queue_size_mutex) == CELL_OK); // Error code isn't checked on LLE
			return ret;
		}
	}
	else
	{
		for (auto& cmd : handle->cmd_queue.elements)
		{
			cmd.type = LpcmDecCmdType::close;
		}

		if (error_code ret = sys_mutex_unlock(ppu, handle->queue_mutex); ret != CELL_OK)
		{
			return ret; // LLE doesn't unlock the queue size mutex
		}
	}

	error_code ret = sys_mutex_unlock(ppu, handle->queue_size_mutex);
	ret = ret ? ret : handle->release_output(ppu);

	vm::var<u64> thread_ret;
	ret = ret ? ret : sys_ppu_thread_join(ppu, static_cast<u32>(handle->thread_id), +thread_ret);

	ret = ret ? ret : sys_cond_destroy(ppu, handle->queue_size_cond);
	ret = ret ? ret : sys_cond_destroy(ppu, handle->unk_cond);
	ret = ret ? ret : sys_cond_destroy(ppu, handle->output_consumed);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->queue_mutex);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->queue_size_mutex);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->unk_mutex);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->output_mutex);
	ret = ret ? ret : handle->cmd_available.finalize(ppu);
	ret = ret ? ret : handle->reserved2.finalize(ppu);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->spurs_queue_pop_mutex);
	ret = ret ? ret : sys_mutex_destroy(ppu, handle->spurs_queue_push_mutex);

	return ret;
}

error_code _CellAdecCoreOpStartSeq_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle, vm::ptr<CellAdecParamLpcm> lpcmParam)
{
	cellAdec.notice("_CellAdecCoreOpStartSeq_lpcm(handle=*0x%x, lpcmParam=*0x%x)", handle, lpcmParam);

	ensure(!!handle && !!lpcmParam); // Not checked on LLE

	cellAdec.notice("_CellAdecCoreOpStartSeq_lpcm(): channelNumber=%d, sampleRate=%d, sizeOfWord=%d, audioPayloadSize=0x%x", lpcmParam->channelNumber, lpcmParam->sampleRate, lpcmParam->sizeOfWord, lpcmParam->audioPayloadSize);

	if (lpcmParam->channelNumber >= 0x20u || lpcmParam->sampleRate >= 0x20u || lpcmParam->sizeOfWord >= 0x20u || lpcmParam->audioPayloadSize == 0u)
	{
		return CELL_ADEC_ERROR_LPCM_ARG;
	}

	return handle->send_command<LpcmDecCmdType::start_seq>(ppu, *lpcmParam);
}

error_code _CellAdecCoreOpEndSeq_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle)
{
	cellAdec.notice("_CellAdecCoreOpEndSeq_lpcm(handle=*0x%x)", handle);

	ensure(!!handle); // Not checked on LLE

	return handle->send_command<LpcmDecCmdType::end_seq>(ppu);
}

error_code _CellAdecCoreOpDecodeAu_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle, s32 pcmHandle, vm::ptr<CellAdecAuInfo> auInfo)
{
	cellAdec.trace("_CellAdecCoreOpDecodeAu_lpcm(handle=*0x%x, pcmHandle=%d, auInfo=*0x%x)", handle, pcmHandle, auInfo);

	ensure(!!handle && !!auInfo); // Not checked on LLE

	cellAdec.trace("_CellAdecCoreOpDecodeAu_lpcm(): startAddr=*0x%x, size=0x%x, pts=0x%x, userData=0x%016x", auInfo->startAddr, auInfo->size, std::bit_cast<be_t<u64>>(auInfo->pts), auInfo->userData);

	return handle->send_command<LpcmDecCmdType::decode_au>(ppu, pcmHandle, *auInfo);
}

void _CellAdecCoreOpGetVersion_lpcm(vm::ptr<be_t<u32, 1>> version)
{
	cellAdec.notice("_CellAdecCoreOpGetVersion_lpcm(version=*0x%x)", version);

	*version = 0x20070323;
}

error_code _CellAdecCoreOpRealign_lpcm(vm::ptr<LpcmDecContext> handle, vm::ptr<void> outBuffer, vm::cptr<void> pcmStartAddr)
{
	cellAdec.trace("_CellAdecCoreOpRealign_lpcm(handle=*0x%x, outBuffer=*0x%x, pcmStartAddr=*0x%x)", handle, outBuffer, pcmStartAddr);

	if (!pcmStartAddr)
	{
		return CELL_ADEC_ERROR_LPCM_ARG;
	}

	if (outBuffer)
	{
		ensure(!!handle); // Not checked on LLE
		ensure(vm::check_addr(outBuffer.addr(), vm::page_info_t::page_writable, handle->output_size));

		std::memcpy(outBuffer.get_ptr(), pcmStartAddr.get_ptr(), handle->output_size);
	}

	return CELL_OK;
}

error_code _CellAdecCoreOpReleasePcm_lpcm(ppu_thread& ppu, vm::ptr<LpcmDecContext> handle, s32 pcmHandle, vm::ptr<void> outBuffer)
{
	ppu.state += cpu_flag::wait;

	cellAdec.trace("_CellAdecCoreOpReleasePcm_lpcm(handle=*0x%x, pcmHandle=%d, outBuffer=*0x%x)", handle, pcmHandle, outBuffer);

	ensure(!!handle); // Not checked on LLE

	return handle->release_output(ppu);
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
			case ADEC_CORRECT_PTS_VALUE_TYPE_LPCM_HDMV:      return 450;
			case ADEC_CORRECT_PTS_VALUE_TYPE_LPCM_DVD:       return 150;
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

	// Block savestate creation during ppu_thread::fast_call()
	std::unique_lock savestate_lock{g_fxo->get<hle_locks_t>(), std::try_to_lock};

	if (!savestate_lock.owns_lock())
	{
		ppu.state += cpu_flag::again;
		return {};
	}

	const s32 pcm_handle_num = core_ops->getPcmHandleNum(ppu);
	const u32 bitstream_info_size = core_ops->getBsiInfoSize(ppu);

	const auto _this = vm::ptr<AdecContext>::make(utils::align(+res->startAddr, 0x80));
	const auto frames = vm::ptr<AdecFrame>::make(_this.addr() + sizeof(AdecContext));
	const u32 bitstream_infos_addr = frames.addr() + pcm_handle_num * sizeof(AdecFrame);
	const auto core_handle = vm::ptr<void>::make(utils::align(bitstream_infos_addr + bitstream_info_size * pcm_handle_num, 0x80));

	if (type->audioCodecType == CELL_ADEC_TYPE_LPCM_DVD)
	{
		vm::static_ptr_cast<LpcmDecContext>(core_handle)->dvd_packing = true;
	}
	else if (type->audioCodecType == CELL_ADEC_TYPE_LPCM_PAMF || type->audioCodecType == CELL_ADEC_TYPE_LPCM_BLURAY)
	{
		vm::static_ptr_cast<LpcmDecContext>(core_handle)->dvd_packing = false;
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

error_code adecSetLpcmBlurayParams(vm::ptr<LpcmDecContext> handle, u64 userData)
{
	const u8 channel_number = static_cast<u8>(userData >> 32);
	const u8 sample_rate = static_cast<u8>(userData >> 40);
	const u8 size_of_word = static_cast<u8>(userData >> 48);
	const u8 unk = static_cast<u8>(userData >> 56);

	handle->lpcm_param->channelNumber = channel_number;
	handle->lpcm_param->sampleRate = sample_rate;
	handle->lpcm_param->sizeOfWord = size_of_word;

	u32 allocated_channels;

	switch (channel_number)
	{
	case CELL_ADEC_CH_MONO:
	case CELL_ADEC_CH_STEREO:
		allocated_channels = 2;
		break;

	case CELL_ADEC_CH_3_0:
	case CELL_ADEC_CH_2_1:
	case CELL_ADEC_CH_3_1:
	case CELL_ADEC_CH_2_2:
		allocated_channels = 4;
		break;

	case CELL_ADEC_CH_3_2:
	case CELL_ADEC_CH_3_2_LFE:
		allocated_channels = 6;
		break;

	case CELL_ADEC_CH_3_4:
	case CELL_ADEC_CH_3_4_LFE:
		allocated_channels = 8;
		break;

	default:
		return CELL_ADEC_ERROR_FATAL;
	}

	u32 samples_per_frame;

	switch (sample_rate)
	{
	case CELL_ADEC_FS_48kHz: samples_per_frame = 48000 / 200; break;
	case CELL_ADEC_FS_96kHz: samples_per_frame = 96000 / 200; break;
	case CELL_ADEC_FS_192kHz: samples_per_frame = 192000 / 200; break;
	default: return CELL_ADEC_ERROR_FATAL;
	}

	u32 allocated_bytes_per_sample;

	switch (size_of_word)
	{
	case CELL_ADEC_BIT_LENGTH_16: allocated_bytes_per_sample = 2; break;
	case CELL_ADEC_BIT_LENGTH_20: // Same as below
	case CELL_ADEC_BIT_LENGTH_24: allocated_bytes_per_sample = 3; break;
	default: return CELL_ADEC_ERROR_FATAL;
	}

	handle->lpcm_param->audioPayloadSize = allocated_bytes_per_sample * allocated_channels * samples_per_frame * unk;

	return CELL_OK;
}

error_code adecSetLpcmDvdParams(vm::ptr<LpcmDecContext> handle, u64 userData)
{
	const u8 channel_layout = static_cast<u8>(userData >> 32);
	const u8 sample_rate = static_cast<u8>(userData >> 40);
	const u8 size_of_word = static_cast<u8>(userData >> 48);
	const u8 unk = static_cast<u8>(userData >> 56);

	handle->lpcm_param->channelNumber = channel_layout;
	handle->lpcm_param->sampleRate = sample_rate;
	handle->lpcm_param->sizeOfWord = size_of_word;

	u32 samples_per_frame;

	switch (sample_rate)
	{
	case CELL_ADEC_FS_48kHz: samples_per_frame = 48000 / 600; break;
	case CELL_ADEC_FS_96kHz: samples_per_frame = 96000 / 600; break;
	default: return CELL_ADEC_ERROR_FATAL;
	}

	u32 bits_per_sample;

	switch (size_of_word)
	{
	case CELL_ADEC_BIT_LENGTH_16: bits_per_sample = 16; break;
	case CELL_ADEC_BIT_LENGTH_20: bits_per_sample = 20; break;
	case CELL_ADEC_BIT_LENGTH_24: bits_per_sample = 24; break;
	default: return CELL_ADEC_ERROR_FATAL;
	}

	u32 channel_number;

	switch (channel_layout)
	{
	case CELL_ADEC_LPCM_DVD_CH_MONO:
		channel_number = 1;
		break;

	case CELL_ADEC_LPCM_DVD_CH_STEREO:
		channel_number = 2;
		break;

	case 4:
		if (sample_rate == CELL_ADEC_FS_96kHz && size_of_word == CELL_ADEC_BIT_LENGTH_24)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 3;
		break;

	case 5:
		if (sample_rate == CELL_ADEC_FS_96kHz && size_of_word != CELL_ADEC_BIT_LENGTH_16)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 4;
		break;

	case CELL_ADEC_LPCM_DVD_CH_3_2:
		if (sample_rate == CELL_ADEC_FS_96kHz)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 5;
		break;

	case CELL_ADEC_LPCM_DVD_CH_3_2_LFE:
		if (sample_rate == CELL_ADEC_FS_96kHz || size_of_word == CELL_ADEC_BIT_LENGTH_24)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 6;
		break;

	case CELL_ADEC_LPCM_DVD_CH_3_4:
		if (sample_rate == CELL_ADEC_FS_96kHz || size_of_word != CELL_ADEC_BIT_LENGTH_16)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 7;
		break;

	case CELL_ADEC_LPCM_DVD_CH_3_4_LFE:
		if (sample_rate == CELL_ADEC_FS_96kHz || size_of_word != CELL_ADEC_BIT_LENGTH_16)
		{
			return CELL_ADEC_ERROR_FATAL;
		}

		channel_number = 8;
		break;

	default:
		return CELL_ADEC_ERROR_FATAL;
	}

	handle->lpcm_param->audioPayloadSize = bits_per_sample * channel_number * samples_per_frame / 8 * unk;

	return CELL_OK;
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
		if (adecSetLpcmBlurayParams(vm::static_ptr_cast<LpcmDecContext>(handle->core_handle), auInfo->userData) != CELL_OK)
		{
			return CELL_ADEC_ERROR_FATAL;
		}
	}
	else if (handle->type.audioCodecType == CELL_ADEC_TYPE_LPCM_DVD)
	{
		if (adecSetLpcmDvdParams(vm::static_ptr_cast<LpcmDecContext>(handle->core_handle), auInfo->userData) != CELL_OK)
		{
			return CELL_ADEC_ERROR_FATAL;
		}
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
		vm::static_ptr_cast<LpcmDecContext>(handle->core_handle)->output_size = pcm_item->size;
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

	REG_HIDDEN_FUNC(lpcmDecEntry);
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
