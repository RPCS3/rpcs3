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
	libsnd3.todo("cellSnd3Init(maxVoice=%d, samples=%d, queue=*0x%x)", maxVoice, samples, queue);
	return CELL_OK;
}

error_code cellSnd3Exit()
{
	libsnd3.todo("cellSnd3Exit()");
	return CELL_OK;
}

u16 cellSnd3Note2Pitch(u16 center_note, u16 center_fine, u16 note, s16 fine)
{
	libsnd3.todo("cellSnd3Note2Pitch(center_note=%d, center_fine=%d, note=%d, fine=%d)", center_note, center_fine, note, fine);
	return 0;
}

u16 cellSnd3Pitch2Note(u16 center_note, u16 center_fine, u16 pitch)
{
	libsnd3.todo("cellSnd3Pitch2Note(center_note=%d, center_fine=%d, pitch=%d)", center_note, center_fine, pitch);
	return 0;
}

error_code cellSnd3SetOutputMode(u32 mode)
{
	libsnd3.todo("cellSnd3SetOutputMode(mode=%d)", mode);
	return CELL_OK;
}

error_code cellSnd3Synthesis(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR)
{
	libsnd3.todo("cellSnd3Synthesis(pOutL=*0x%x, pOutR=*0x%x)", pOutL, pOutR);
	return CELL_OK;
}

error_code cellSnd3SynthesisEx(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR, vm::ptr<f32> pOutRL, vm::ptr<f32> pOutRR)
{
	libsnd3.todo("cellSnd3SynthesisEx(pOutL=*0x%x, pOutR=*0x%x, pOutRL=*0x%x, pOutRR=*0x%x)", pOutL, pOutR, pOutRL, pOutRR);
	return CELL_OK;
}

error_code cellSnd3BindSoundData(vm::ptr<CellSnd3DataCtx> snd3Ctx, vm::ptr<void> hd3, u32 synthMemOffset)
{
	libsnd3.todo("cellSnd3BindSoundData(snd3Ctx=*0x%x, hd3=*0x%x, synthMemOffset=0x%x)", snd3Ctx, hd3, synthMemOffset);
	return CELL_OK;
}

error_code cellSnd3UnbindSoundData(u32 hd3ID)
{
	libsnd3.todo("cellSnd3UnbindSoundData(hd3ID=0x%x)", hd3ID);
	return CELL_OK;
}

error_code cellSnd3NoteOnByTone(u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3NoteOnByTone(hd3ID=0x%x, toneIndex=%d, note=%d, keyOnID=0x%x, keyOnParam=*0x%x)", hd3ID, toneIndex, note, keyOnID, keyOnParam);
	return CELL_OK;
}

error_code cellSnd3KeyOnByTone(u32 hd3ID, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3KeyOnByTone(hd3ID=0x%x, toneIndex=%d, pitch=%d, keyOnID=0x%x, keyOnParam=*0x%x)", hd3ID, toneIndex, pitch, keyOnID, keyOnParam);
	return CELL_OK;
}

error_code cellSnd3VoiceNoteOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3VoiceNoteOnByTone(hd3ID=0x%x, voiceNum=%d, toneIndex=%d, note=%d, keyOnID=0x%x, keyOnParam=*0x%x)", hd3ID, voiceNum, toneIndex, note, keyOnID, keyOnParam);
	return CELL_OK;
}

error_code cellSnd3VoiceKeyOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	libsnd3.todo("cellSnd3VoiceKeyOnByTone(hd3ID=0x%x, voiceNum=%d, toneIndex=%d, pitch=%d, keyOnID=0x%x, keyOnParam=*0x%x)", hd3ID, voiceNum, toneIndex, pitch, keyOnID, keyOnParam);
	return CELL_OK;
}

error_code cellSnd3VoiceSetReserveMode(u32 voiceNum, u32 reserveMode)
{
	libsnd3.todo("cellSnd3VoiceSetReserveMode(voiceNum=%d, reserveMode=%d)", voiceNum, reserveMode);
	return CELL_OK;
}

error_code cellSnd3VoiceSetSustainHold(u32 voiceNum, u32 sustainHold)
{
	libsnd3.todo("cellSnd3VoiceSetSustainHold(voiceNum=%d, sustainHold=%d)", voiceNum, sustainHold);
	return CELL_OK;
}

error_code cellSnd3VoiceKeyOff(u32 voiceNum)
{
	libsnd3.todo("cellSnd3VoiceKeyOff(voiceNum=%d)", voiceNum);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPitch(u32 voiceNum, s32 addPitch)
{
	libsnd3.todo("cellSnd3VoiceSetPitch(voiceNum=%d, addPitch=%d)", voiceNum, addPitch);
	return CELL_OK;
}

error_code cellSnd3VoiceSetVelocity(u32 voiceNum, u32 velocity)
{
	libsnd3.todo("cellSnd3VoiceSetVelocity(voiceNum=%d, velocity=%d)", voiceNum, velocity);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPanpot(u32 voiceNum, u32 panpot)
{
	libsnd3.todo("cellSnd3VoiceSetPanpot(voiceNum=%d, panpot=%d)", voiceNum, panpot);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPanpotEx(u32 voiceNum, u32 panpotEx)
{
	libsnd3.todo("cellSnd3VoiceSetPanpotEx(voiceNum=%d, panpotEx=%d)", voiceNum, panpotEx);
	return CELL_OK;
}

error_code cellSnd3VoiceSetPitchBend(u32 voiceNum, u32 bendValue)
{
	libsnd3.todo("cellSnd3VoiceSetPitchBend(voiceNum=%d, bendValue=%d)", voiceNum, bendValue);
	return CELL_OK;
}

error_code cellSnd3VoiceAllKeyOff()
{
	libsnd3.todo("cellSnd3VoiceAllKeyOff()");
	return CELL_OK;
}

error_code cellSnd3VoiceGetEnvelope(u32 voiceNum)
{
	libsnd3.todo("cellSnd3VoiceGetEnvelope(voiceNum=%d)", voiceNum);
	return CELL_OK;
}

error_code cellSnd3VoiceGetStatus(u32 voiceNum)
{
	libsnd3.todo("cellSnd3VoiceGetStatus(voiceNum=%d)", voiceNum);
	return CELL_OK;
}

u32 cellSnd3KeyOffByID(u32 keyOnID)
{
	libsnd3.todo("cellSnd3KeyOffByID(keyOnID=%d)", keyOnID);
	return CELL_OK;
}

error_code cellSnd3GetVoice(u32 midiChannel, u32 keyOnID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	libsnd3.todo("cellSnd3GetVoice(midiChannel=%d, keyOnID=%d, voiceBit=*0x%x)", midiChannel, keyOnID, voiceBit);
	return CELL_OK;
}

error_code cellSnd3GetVoiceByID(u32 ID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	libsnd3.todo("cellSnd3GetVoiceByID(ID=%d, voiceBit=*0x%x)", ID, voiceBit);
	return CELL_OK;
}

error_code cellSnd3NoteOn(u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain, vm::ptr<CellSnd3KeyOnParam> keyOnParam, u32 keyOnID)
{
	libsnd3.todo("cellSnd3NoteOn(hd3ID=%d, midiChannel=%d, midiProgram=%d, midiNote=%d, sustain=%d, keyOnParam=*0x%x, keyOnID=%d)", hd3ID, midiChannel, midiProgram, midiNote, sustain, keyOnParam, keyOnID);
	return CELL_OK;
}

error_code cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
{
	libsnd3.todo("cellSnd3NoteOff(midiChannel=%d, midiNote=%d, keyOnID=%d)", midiChannel, midiNote, keyOnID);
	return CELL_OK;
}

error_code cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 keyOnID)
{
	libsnd3.todo("cellSnd3SetSustainHold(midiChannel=%d, sustainHold=%d, keyOnID=%d)", midiChannel, sustainHold, keyOnID);
	return CELL_OK;
}

error_code cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
{
	libsnd3.todo("cellSnd3SetEffectType(effectType=%d, returnVol=%d, delay=%d, feedback=%d)", effectType, returnVol, delay, feedback);
	return CELL_OK;
}

error_code cellSnd3SMFBind(vm::ptr<CellSnd3SmfCtx> smfCtx, vm::ptr<void> smf, u32 hd3ID)
{
	libsnd3.todo("cellSnd3SMFBind(smfCtx=*0x%x, delay=*0x%x, hd3ID=%d)", smfCtx, smf, hd3ID);
	return CELL_OK;
}

error_code cellSnd3SMFUnbind(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFUnbind(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFPlay(u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
{
	libsnd3.todo("cellSnd3SMFPlay(smfID=%d, playVelocity=%d, playPan=%d, playCount=%d)", smfID, playVelocity, playPan, playCount);
	return CELL_OK;
}

error_code cellSnd3SMFPlayEx(u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
{
	libsnd3.todo("cellSnd3SMFPlayEx(smfID=%d, playVelocity=%d, playPan=%d, playPanEx=%d, playCount=%d)", smfID, playVelocity, playPan, playPanEx, playCount);
	return CELL_OK;
}

error_code cellSnd3SMFPause(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFPause(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFResume(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFResume(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFStop(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFStop(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFAddTempo(u32 smfID, s32 addTempo)
{
	libsnd3.todo("cellSnd3SMFAddTempo(smfID=%d, addTempo=%d)", smfID, addTempo);
	return CELL_OK;
}

error_code cellSnd3SMFGetTempo(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetTempo(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
{
	libsnd3.todo("cellSnd3SMFSetPlayVelocity(smfID=%d, playVelocity=%d)", smfID, playVelocity);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayVelocity(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayVelocity(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayPanpot(u32 smfID, u32 playPanpot)
{
	libsnd3.todo("cellSnd3SMFSetPlayPanpot(smfID=%d, playPanpot=%d)", smfID, playPanpot);
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayPanpotEx(u32 smfID, u32 playPanpotEx)
{
	libsnd3.todo("cellSnd3SMFSetPlayPanpotEx(smfID=%d, playPanpotEx=%d)", smfID, playPanpotEx);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayPanpot(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayPanpot(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayPanpotEx(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayPanpotEx(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayStatus(u32 smfID)
{
	libsnd3.todo("cellSnd3SMFGetPlayStatus(smfID=%d)", smfID);
	return CELL_OK;
}

error_code cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
{
	libsnd3.todo("cellSnd3SMFSetPlayChannel(smfID=%d, playChannelBit=%d)", smfID, playChannelBit);
	return CELL_OK;
}

error_code cellSnd3SMFGetPlayChannel(u32 smfID, vm::ptr<u32> playChannelBit)
{
	libsnd3.todo("cellSnd3SMFGetPlayChannel(smfID=%d, playChannelBit=*0x%x)", smfID, playChannelBit);
	return CELL_OK;
}

error_code cellSnd3SMFGetKeyOnID(u32 smfID, u32 midiChannel, vm::ptr<u32> keyOnID)
{
	libsnd3.todo("cellSnd3SMFAddTempo(smfID=%d, midiChannel=%d, keyOnID=*0x%x)", smfID, midiChannel, keyOnID);
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
