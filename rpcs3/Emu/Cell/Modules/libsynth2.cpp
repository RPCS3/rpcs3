#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "libsynth2.h"



logs::channel libsynth2("libsynth2");

s32 cellSoundSynth2Config(s16 param, s32 value)
{
	libsynth2.todo("cellSoundSynth2Config(param=%d, value=%d)", param, value);
	return CELL_OK;
}

s32 cellSoundSynth2Init(s16 flag)
{
	libsynth2.todo("cellSoundSynth2Init(flag=%d)", flag);
	return CELL_OK;
}

s32 cellSoundSynth2Exit()
{
	libsynth2.todo("cellSoundSynth2Exit()");
	return CELL_OK;
}

void cellSoundSynth2SetParam(u16 reg, u16 value)
{
	libsynth2.todo("cellSoundSynth2SetParam(register=0x%x, value=0x%x)", reg, value);
}

u16 cellSoundSynth2GetParam(u16 reg)
{
	libsynth2.todo("cellSoundSynth2GetParam(register=0x%x)", reg);
	fmt::throw_exception("Unimplemented" HERE);
}

void cellSoundSynth2SetSwitch(u16 reg, u32 value)
{
	libsynth2.todo("cellSoundSynth2SetSwitch(register=0x%x, value=0x%x)", reg, value);
}

u32 cellSoundSynth2GetSwitch(u16 reg)
{
	libsynth2.todo("cellSoundSynth2GetSwitch(register=0x%x)", reg);
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSoundSynth2SetAddr(u16 reg, u32 value)
{
	libsynth2.todo("cellSoundSynth2SetAddr(register=0x%x, value=0x%x)", reg, value);
	return CELL_OK;
}

u32 cellSoundSynth2GetAddr(u16 reg)
{
	libsynth2.todo("cellSoundSynth2GetAddr(register=0x%x)", reg);
	fmt::throw_exception("Unimplemented" HERE);
}

s32 cellSoundSynth2SetEffectAttr(s16 bus, vm::ptr<CellSoundSynth2EffectAttr> attr)
{
	libsynth2.todo("cellSoundSynth2SetEffectAttr(bus=%d, attr=*0x%x)", bus, attr);
	return CELL_OK;
}

s32 cellSoundSynth2SetEffectMode(s16 bus, vm::ptr<CellSoundSynth2EffectAttr> attr)
{
	libsynth2.todo("cellSoundSynth2SetEffectMode(bus=%d, attr=*0x%x)", bus, attr);
	return CELL_OK;
}

void cellSoundSynth2SetCoreAttr(u16 entry, u16 value)
{
	libsynth2.todo("cellSoundSynth2SetCoreAttr(entry=0x%x, value=0x%x)", entry, value);
}

s32 cellSoundSynth2Generate(u16 samples, vm::ptr<f32> Lout, vm::ptr<f32> Rout, vm::ptr<f32> Ls, vm::ptr<f32> Rs)
{
	libsynth2.todo("cellSoundSynth2Generate(samples=0x%x, Lout=*0x%x, Rout=*0x%x, Ls=*0x%x, Rs=*0x%x)", samples, Lout, Rout, Ls, Rs);
	return CELL_OK;
}

s32 cellSoundSynth2VoiceTrans(s16 channel, u16 mode, vm::ptr<u8> m_addr, u32 s_addr, u32 size)
{
	libsynth2.todo("cellSoundSynth2VoiceTrans(channel=%d, mode=0x%x, m_addr=*0x%x, s_addr=0x%x, size=0x%x)", channel, mode, m_addr, s_addr, size);
	return CELL_OK;
}

s32 cellSoundSynth2VoiceTransStatus(s16 channel, s16 flag)
{
	libsynth2.todo("cellSoundSynth2VoiceTransStatus(channel=%d, flag=%d)", channel, flag);
	return CELL_OK;
}

u16 cellSoundSynth2Note2Pitch(u16 center_note, u16 center_fine, u16 note, s16 fine)
{
	libsynth2.todo("cellSoundSynth2Note2Pitch(center_note=0x%x, center_fine=0x%x, note=0x%x, fine=%d)", center_note, center_fine, note, fine);
	fmt::throw_exception("Unimplemented" HERE);
}

u16 cellSoundSynth2Pitch2Note(u16 center_note, u16 center_fine, u16 pitch)
{
	libsynth2.todo("cellSoundSynth2Pitch2Note(center_note=0x%x, center_fine=0x%x, pitch=0x%x)", center_note, center_fine, pitch);
	fmt::throw_exception("Unimplemented" HERE);
}


DECLARE(ppu_module_manager::libsynth2)("libsynth2", []()
{
	REG_FUNC(libsynth2, cellSoundSynth2Config);
	REG_FUNC(libsynth2, cellSoundSynth2Init);
	REG_FUNC(libsynth2, cellSoundSynth2Exit);
	REG_FUNC(libsynth2, cellSoundSynth2SetParam);
	REG_FUNC(libsynth2, cellSoundSynth2GetParam);
	REG_FUNC(libsynth2, cellSoundSynth2SetSwitch);
	REG_FUNC(libsynth2, cellSoundSynth2GetSwitch);
	REG_FUNC(libsynth2, cellSoundSynth2SetAddr);
	REG_FUNC(libsynth2, cellSoundSynth2GetAddr);
	REG_FUNC(libsynth2, cellSoundSynth2SetEffectAttr);
	REG_FUNC(libsynth2, cellSoundSynth2SetEffectMode);
	REG_FUNC(libsynth2, cellSoundSynth2SetCoreAttr);
	REG_FUNC(libsynth2, cellSoundSynth2Generate);
	REG_FUNC(libsynth2, cellSoundSynth2VoiceTrans);
	REG_FUNC(libsynth2, cellSoundSynth2VoiceTransStatus);
	REG_FUNC(libsynth2, cellSoundSynth2Note2Pitch);
	REG_FUNC(libsynth2, cellSoundSynth2Pitch2Note);
});
