#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "libsnd3.h"

s32 cellSnd3Init(u32 maxVoice, u32 samples, vm::ptr<CellSnd3RequestQueueCtx> queue)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3Exit()
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

u16 cellSnd3Note2Pitch(u16 center_note, u16 center_fine, u16 note, s16 fine)
{
	throw EXCEPTION("");
}

u16 cellSnd3Pitch2Note(u16 center_note, u16 center_fine, u16 pitch)
{
	throw EXCEPTION("");
}

s32 cellSnd3SetOutputMode(u32 mode)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3Synthesis(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SynthesisEx(vm::ptr<f32> pOutL, vm::ptr<f32> pOutR, vm::ptr<f32> pOutRL, vm::ptr<f32> pOutRR)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3BindSoundData(vm::ptr<CellSnd3DataCtx> snd3Ctx, vm::ptr<void> hd3, u32 synthMemOffset)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3UnbindSoundData(u32 hd3ID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOnByTone(u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	throw EXCEPTION("");
}

s32 cellSnd3KeyOnByTone(u32 hd3ID, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	throw EXCEPTION("");
}

s32 cellSnd3VoiceNoteOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	throw EXCEPTION("");
}

s32 cellSnd3VoiceKeyOnByTone(u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, vm::ptr<CellSnd3KeyOnParam> keyOnParam)
{
	throw EXCEPTION("");
}

s32 cellSnd3VoiceSetReserveMode(u32 voiceNum, u32 reserveMode)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetSustainHold(u32 voiceNum, u32 sustainHold)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceKeyOff(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPitch(u32 voiceNum, s32 addPitch)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetVelocity(u32 voiceNum, u32 velocity)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPanpot(u32 voiceNum, u32 panpot)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPanpotEx(u32 voiceNum, u32 panpotEx)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetPitchBend(u32 voiceNum, u32 bendValue)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceAllKeyOff()
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceGetEnvelope(u32 voiceNum)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceGetStatus(u32 voiceNum)
{
	throw EXCEPTION("");
}

u32 cellSnd3KeyOffByID(u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3GetVoice(u32 midiChannel, u32 keyOnID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3GetVoiceByID(u32 ID, vm::ptr<CellSnd3VoiceBitCtx> voiceBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOn(u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain, vm::ptr<CellSnd3KeyOnParam> keyOnParam, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFBind(vm::ptr<CellSnd3SmfCtx> smfCtx, vm::ptr<void> smf, u32 hd3ID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFUnbind(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFPlay(u32 smfID, u32 playVelocity, u32 playPan, u32 playCount)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFPlayEx(u32 smfID, u32 playVelocity, u32 playPan, u32 playPanEx, u32 playCount)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFPause(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFResume(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFStop(u32 smfID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFAddTempo(u32 smfID, s32 addTempo)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetTempo(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayVelocity(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFSetPlayPanpot(u32 smfID, u32 playPanpot)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFSetPlayPanpotEx(u32 smfID, u32 playPanpotEx)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayPanpot(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFGetPlayPanpotEx(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFGetPlayStatus(u32 smfID)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayChannel(u32 smfID, vm::ptr<u32> playChannelBit)
{
	throw EXCEPTION("");
}

s32 cellSnd3SMFGetKeyOnID(u32 smfID, u32 midiChannel, vm::ptr<u32> keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}


Module libsnd3("libsnd3", []()
{
	REG_SUB(libsnd3,, cellSnd3Init);
	REG_SUB(libsnd3,, cellSnd3Exit);
	REG_SUB(libsnd3,, cellSnd3Note2Pitch);
	REG_SUB(libsnd3,, cellSnd3Pitch2Note);
	REG_SUB(libsnd3,, cellSnd3SetOutputMode);
	REG_SUB(libsnd3,, cellSnd3Synthesis);
	REG_SUB(libsnd3,, cellSnd3SynthesisEx);
	REG_SUB(libsnd3,, cellSnd3BindSoundData);
	REG_SUB(libsnd3,, cellSnd3UnbindSoundData);
	REG_SUB(libsnd3,, cellSnd3NoteOnByTone);
	REG_SUB(libsnd3,, cellSnd3KeyOnByTone);
	REG_SUB(libsnd3,, cellSnd3VoiceNoteOnByTone);
	REG_SUB(libsnd3,, cellSnd3VoiceKeyOnByTone);
	REG_SUB(libsnd3,, cellSnd3VoiceSetReserveMode);
	REG_SUB(libsnd3,, cellSnd3VoiceSetSustainHold);
	REG_SUB(libsnd3,, cellSnd3VoiceKeyOff);
	REG_SUB(libsnd3,, cellSnd3VoiceSetPitch);
	REG_SUB(libsnd3,, cellSnd3VoiceSetVelocity);
	REG_SUB(libsnd3,, cellSnd3VoiceSetPanpot);
	REG_SUB(libsnd3,, cellSnd3VoiceSetPanpotEx);
	REG_SUB(libsnd3,, cellSnd3VoiceSetPitchBend);
	REG_SUB(libsnd3,, cellSnd3VoiceAllKeyOff);
	REG_SUB(libsnd3,, cellSnd3VoiceGetEnvelope);
	REG_SUB(libsnd3,, cellSnd3VoiceGetStatus);
	REG_SUB(libsnd3,, cellSnd3KeyOffByID);
	REG_SUB(libsnd3,, cellSnd3GetVoice);
	REG_SUB(libsnd3,, cellSnd3GetVoiceByID);
	REG_SUB(libsnd3,, cellSnd3NoteOn);
	REG_SUB(libsnd3,, cellSnd3NoteOff);
	REG_SUB(libsnd3,, cellSnd3SetSustainHold);
	REG_SUB(libsnd3,, cellSnd3SetEffectType);
	REG_SUB(libsnd3,, cellSnd3SMFBind);
	REG_SUB(libsnd3,, cellSnd3SMFUnbind);
	REG_SUB(libsnd3,, cellSnd3SMFPlay);
	REG_SUB(libsnd3,, cellSnd3SMFPlayEx);
	REG_SUB(libsnd3,, cellSnd3SMFPause);
	REG_SUB(libsnd3,, cellSnd3SMFResume);
	REG_SUB(libsnd3,, cellSnd3SMFStop);
	REG_SUB(libsnd3,, cellSnd3SMFAddTempo);
	REG_SUB(libsnd3,, cellSnd3SMFGetTempo);
	REG_SUB(libsnd3,, cellSnd3SMFSetPlayVelocity);
	REG_SUB(libsnd3,, cellSnd3SMFGetPlayVelocity);
	REG_SUB(libsnd3,, cellSnd3SMFSetPlayPanpot);
	REG_SUB(libsnd3,, cellSnd3SMFSetPlayPanpotEx);
	REG_SUB(libsnd3,, cellSnd3SMFGetPlayPanpot);
	REG_SUB(libsnd3,, cellSnd3SMFGetPlayPanpotEx);
	REG_SUB(libsnd3,, cellSnd3SMFGetPlayStatus);
	REG_SUB(libsnd3,, cellSnd3SMFSetPlayChannel);
	REG_SUB(libsnd3,, cellSnd3SMFGetPlayChannel);
	REG_SUB(libsnd3,, cellSnd3SMFGetKeyOnID);
});
