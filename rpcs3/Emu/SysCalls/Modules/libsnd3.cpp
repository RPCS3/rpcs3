#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void libsnd3_init();
Module libsnd3("libsnd3", libsnd3_init);

#include "libsnd3.h"

s32 cellSnd3Init() //u32 maxVoice, u32 samples, CellSnd3RequestQueueCtx *queue
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3Exit()
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SetOutputMode() //u32 mode
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3Synthesis() //float *pOutL, float *pOutR
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SynthesisEx() //float *pOutL, float *pOutR, float *pOutRL, float *pOutRR
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3VoiceSetReserveMode() //u32 voiceNum, u32 reserveMode
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3BindSoundData() //CellSnd3DataCtx *snd3Ctx, void *hd3, u32 synthMemOffset
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3UnbindSoundData(u32 hd3ID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOnByTone() //u32 hd3ID, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3KeyOnByTone() //u32 hd3ID, u32 toneIndex,  u32 pitch,u32 keyOnID,CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3VoiceNoteOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 note, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3VoiceKeyOnByTone() //u32 hd3ID, u32 voiceNum, u32 toneIndex, u32 pitch, u32 keyOnID, CellSnd3KeyOnParam *keyOnParam
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
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

s32 cellSnd3VoiceGetStatus()  //u32 voiceNum
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

u32 cellSnd3KeyOffByID(u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3GetVoice() //u32 midiChannel, u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3GetVoiceByID() //u32 keyOnID, CellSnd3VoiceBitCtx *voiceBit
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOn() //u32 hd3ID, u32 midiChannel, u32 midiProgram, u32 midiNote, u32 sustain,CellSnd3KeyOnParam *keyOnParam, u32 keyOnID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3NoteOff(u32 midiChannel, u32 midiNote, u32 keyOnID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SetSustainHold(u32 midiChannel, u32 sustainHold, u32 ID)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SetEffectType(u16 effectType, s16 returnVol, u16 delay, u16 feedback)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

u16 cellSnd3Note2Pitch() //u16 center_note, u16 center_fine, u16 note, s16 fine
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

u16 cellSnd3Pitch2Note() //u16 center_note, u16 center_fine, u16 pitch
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFBind() //CellSnd3SmfCtx *smfCtx, void *smf, u32 hd3ID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFUnbind() //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
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

s32 cellSnd3SMFGetTempo() //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFSetPlayVelocity(u32 smfID, u32 playVelocity)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayVelocity()  //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
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

s32 cellSnd3SMFGetPlayPanpot() //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFGetPlayPanpotEx() //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFGetPlayStatus()  //u32 smfID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK; //it's NOT real value
	//TODO
}

s32 cellSnd3SMFSetPlayChannel(u32 smfID, u32 playChannelBit)
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetPlayChannel() //u32 smfID, u32 *playChannelBit
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

s32 cellSnd3SMFGetKeyOnID() //u32 smfID, u32 midiChannel, u32 *keyOnID
{
	UNIMPLEMENTED_FUNC(libsnd3);
	return CELL_OK;
}

void libsnd3_init()
{

}