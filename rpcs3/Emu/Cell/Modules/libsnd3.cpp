#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsnd3.h"

LOG_CHANNEL(libsnd3);

template<>
void fmt_class_string<CellSnd3Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_SND3_ERROR_PARAM);
			STR_CASE(CELL_SND3_ERROR_CREATE_MUTEX);
			STR_CASE(CELL_SND3_ERROR_SYNTH);
			STR_CASE(CELL_SND3_ERROR_ALREADY);
			STR_CASE(CELL_SND3_ERROR_NOTINIT);
			STR_CASE(CELL_SND3_ERROR_SMFFULL);
			STR_CASE(CELL_SND3_ERROR_HD3ID);
			STR_CASE(CELL_SND3_ERROR_SMF);
			STR_CASE(CELL_SND3_ERROR_SMFCTX);
			STR_CASE(CELL_SND3_ERROR_FORMAT);
			STR_CASE(CELL_SND3_ERROR_SMFID);
			STR_CASE(CELL_SND3_ERROR_SOUNDDATAFULL);
			STR_CASE(CELL_SND3_ERROR_VOICENUM);
			STR_CASE(CELL_SND3_ERROR_RESERVEDVOICE);
			STR_CASE(CELL_SND3_ERROR_REQUESTQUEFULL);
			STR_CASE(CELL_SND3_ERROR_OUTPUTMODE);
		}

		return unknown;
	});
}

error_code cellSnd3Init(u32 maxVoice, u32 samples, vm::ptr<CellSnd3RequestQueueCtx> queue)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3Exit()
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

u16 cellSnd3Note2Pitch(u16 center_note, u16 center_fine, u16 note, s16 fine)
{
	libsnd3.todo("cellSnd3Note2Pitch()");
	return 0;
}

u16 cellSnd3Pitch2Note(u16 center_note, u16 center_fine, u16 pitch)
{
	libsnd3.todo("cellSnd3Pitch2Note()");
	return 0;
}

error_code cellSnd3SetOutputMode(u32 mode)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3Synthesis(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SynthesisEx(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR, vm::ptr<f32> pOutRL, vm::ptr<f32> pOutRR)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3BindSoundData(vm::ptr<CellSnd3DataCtx> snd3Ctx, vm::ptr<void> hd3, u32 synthMemOffset)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3UnbindSoundData(u32 hd3ID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3NoteOnByTone(u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3NoteOnByTone()");
	return CELL_OK;
}

error_code cellSnd3KeyOnByTone(u32 hd3ID, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3KeyOnByTone()");
	return CELL_OK;
}

error_code cellSnd3VoiceNoteOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3VoiceNoteOnByTone()");
	return CELL_OK;
}

error_code cellSnd3VoiceKeyOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3VoiceKeyOnByTone()");
	return CELL_OK;
}

error_code cellSnd3VoiceSetReserveMode(u32 voiceNum, u32 reserveMode)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetSustainHold(u32 voiceNum, u32 sustainHold)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceKeyOff(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPitch(u32 voiceNum, s32 addPitch)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetVelocity(u32 voiceNum, u32 velocity)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPanpot(u32 voiceNum, u32 panpot)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPanpotEx(u32 voiceNum, u32 panpotEx)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPitchBend(u32 voiceNum, u32 bendValue)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceAllKeyOff()
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceGetEnvelope(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3VoiceGetStatus(u32 voiceNum)
{
	libsnd3.todo("cellSnd3VoiceGetStatus()");
	return CELL_OK;
}

u32 cellSnd3KeyOffByID(u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3GetVoice(u32 midiChannel, u32 keyOnID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3GetVoiceByID(u32 ID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3NoteOn(u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain, vm::ptr<CellSnd3KeyOnParam> keyOnParam, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFBind(vm::ptr<CellSnd3SmfCtx> smfCtx, vm::ptr<void> smf, u32 hd3ID)
{
	libsnd3.todo("cellSnd3SMFBind()");
	return CELL_OK;
}

error_code cellSnd3SMFUnbind(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFUnbind()");
	return CELL_OK;
}

error_code cellSnd3SMFPlay(u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFPlayEx(u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFPause(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFResume(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFStop(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFAddTempo(u32 smfID, s32 addTempo)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFGetTempo(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetTempo()");
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayVelocity(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayVelocity()");
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayPanpot(u32 smfID, u32 playPanpot)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayPanpotEx(u32 smfID, u32 playPanpotEx)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayPanpot(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayPanpot()");
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayPanpotEx(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayPanpotEx()");
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayStatus(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayStatus()");
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayChannel(u32 smfID, vm::ptr<u32> playChannelBit)
{
	libsnd3.todo("cellSnd3SMFGetPlayChannel()");
	return CELL_OK;
}

error_code cellSnd3SMFGetKeyOnID(u32 smfID, u32 midiChannel, vm::ptr<u32> keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}


DECLARE(ppu_module_manager::libsnd3)("libsnd3", []()
{
	REG_FUNC(libsnd3, cellSnd3Init);
	REG_FUNC(libsnd3, cellSnd3Exit);
	REG_FUNC(libsnd3, cellSnd3Note2Pitch);
	REG_FUNC(libsnd3, cellSnd3Pitch2Note);
	REG_FUNC(libsnd3, cellSnd3SetOutputMode);
	REG_FUNC(libsnd3, cellSnd3Synthesis);
	REG_FUNC(libsnd3, cellSnd3SynthesisEx);
	REG_FUNC(libsnd3, cellSnd3BindSoundData);
	REG_FUNC(libsnd3, cellSnd3UnbindSoundData);
	REG_FUNC(libsnd3, cellSnd3NoteOnByTone);
	REG_FUNC(libsnd3, cellSnd3KeyOnByTone);
	REG_FUNC(libsnd3, cellSnd3VoiceNoteOnByTone);
	REG_FUNC(libsnd3, cellSnd3VoiceKeyOnByTone);
	REG_FUNC(libsnd3, cellSnd3VoiceSetReserveMode);
	REG_FUNC(libsnd3, cellSnd3VoiceSetSustainHold);
	REG_FUNC(libsnd3, cellSnd3VoiceKeyOff);
	REG_FUNC(libsnd3, cellSnd3VoiceSetPitch);
	REG_FUNC(libsnd3, cellSnd3VoiceSetVelocity);
	REG_FUNC(libsnd3, cellSnd3VoiceSetPanpot);
	REG_FUNC(libsnd3, cellSnd3VoiceSetPanpotEx);
	REG_FUNC(libsnd3, cellSnd3VoiceSetPitchBend);
	REG_FUNC(libsnd3, cellSnd3VoiceAllKeyOff);
	REG_FUNC(libsnd3, cellSnd3VoiceGetEnvelope);
	REG_FUNC(libsnd3, cellSnd3VoiceGetStatus);
	REG_FUNC(libsnd3, cellSnd3KeyOffByID);
	REG_FUNC(libsnd3, cellSnd3GetVoice);
	REG_FUNC(libsnd3, cellSnd3GetVoiceByID);
	REG_FUNC(libsnd3, cellSnd3NoteOn);
	REG_FUNC(libsnd3, cellSnd3NoteOff);
	REG_FUNC(libsnd3, cellSnd3SetSustainHold);
	REG_FUNC(libsnd3, cellSnd3SetEffectType);
	REG_FUNC(libsnd3, cellSnd3SMFBind);
	REG_FUNC(libsnd3, cellSnd3SMFUnbind);
	REG_FUNC(libsnd3, cellSnd3SMFPlay);
	REG_FUNC(libsnd3, cellSnd3SMFPlayEx);
	REG_FUNC(libsnd3, cellSnd3SMFPause);
	REG_FUNC(libsnd3, cellSnd3SMFResume);
	REG_FUNC(libsnd3, cellSnd3SMFStop);
	REG_FUNC(libsnd3, cellSnd3SMFAddTempo);
	REG_FUNC(libsnd3, cellSnd3SMFGetTempo);
	REG_FUNC(libsnd3, cellSnd3SMFSetPlayVelocity);
	REG_FUNC(libsnd3, cellSnd3SMFGetPlayVelocity);
	REG_FUNC(libsnd3, cellSnd3SMFSetPlayPanpot);
	REG_FUNC(libsnd3, cellSnd3SMFSetPlayPanpotEx);
	REG_FUNC(libsnd3, cellSnd3SMFGetPlayPanpot);
	REG_FUNC(libsnd3, cellSnd3SMFGetPlayPanpotEx);
	REG_FUNC(libsnd3, cellSnd3SMFGetPlayStatus);
	REG_FUNC(libsnd3, cellSnd3SMFSetPlayChannel);
	REG_FUNC(libsnd3, cellSnd3SMFGetPlayChannel);
	REG_FUNC(libsnd3, cellSnd3SMFGetKeyOnID);
});
