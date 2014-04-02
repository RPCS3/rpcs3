#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void libsynth2_init();
Module libsynth2("libsynth2", libsynth2_init);

#include "libsynth2.h"

int cellSoundSynth2Config(s16 param, int value)
{
	libsynth2.Error("cellSoundSynth2Config(param=%d, value=%d)", param, value);
	return CELL_OK;
}

int cellSoundSynth2Init(s16 flag)
{
	libsynth2.Error("cellSoundSynth2Init(flag=%d)", flag);
	return CELL_OK;
}

int cellSoundSynth2Exit()
{
	libsynth2.Error("cellSoundSynth2Exit()");
	return CELL_OK;
}

void cellSoundSynth2SetParam(u16 reg, u16 value)
{
	libsynth2.Error("cellSoundSynth2SetParam(register=0x%x, value=0x%x)", reg, value);
}

u16 cellSoundSynth2GetParam(u16 reg)
{
	libsynth2.Error("cellSoundSynth2GetParam(register=0x%x) -> 0", reg);
	return 0;
}

void cellSoundSynth2SetSwitch(u16 reg, u32 value)
{
	libsynth2.Error("cellSoundSynth2SetSwitch(register=0x%x, value=0x%x)", reg, value);
}

u32 cellSoundSynth2GetSwitch(u16 reg)
{
	libsynth2.Error("cellSoundSynth2GetSwitch(register=0x%x) -> 0", reg);
	return 0;
}

int cellSoundSynth2SetAddr(u16 reg, u32 value)
{
	libsynth2.Error("cellSoundSynth2SetAddr(register=0x%x, value=0x%x)", reg, value);
	return CELL_OK;
}

u32 cellSoundSynth2GetAddr(u16 reg)
{
	libsynth2.Error("cellSoundSynth2GetAddr(register=0x%x) -> 0", reg);
	return 0;
}

int cellSoundSynth2SetEffectAttr(s16 bus, mem_ptr_t<CellSoundSynth2EffectAttr> attr)
{
	libsynth2.Error("cellSoundSynth2SetEffectAttr(bus=%d, attr_addr=0x%x)", bus, attr.GetAddr());
	return CELL_OK;
}

int cellSoundSynth2SetEffectMode(s16 bus, mem_ptr_t<CellSoundSynth2EffectAttr> attr)
{
	libsynth2.Error("cellSoundSynth2SetEffectMode(bus=%d, attr_addr=0x%x)", bus, attr.GetAddr());
	return CELL_OK;
}

void cellSoundSynth2SetCoreAttr(u16 entry, u16 value)
{
	libsynth2.Error("cellSoundSynth2SetCoreAttr(entry=0x%x, value=0x%x)", entry, value);
}

int cellSoundSynth2Generate(u16 samples, u32 L_addr, u32 R_addr, u32 Lr_addr, u32 Rr_addr)
{
	libsynth2.Error("cellSoundSynth2Generate(samples=0x%x, left=0x%x, right=0x%x, left_rear=0x%x, right_rear=0x%x)",
		samples, L_addr, R_addr, Lr_addr, Rr_addr);
	return CELL_OK;
}

int cellSoundSynth2VoiceTrans(s16 channel, u16 mode, u32 mem_side_addr, u32 lib_side_addr, u32 size)
{
	libsynth2.Error("cellSoundSynth2VoiceTrans(channel=%d, mode=0x%x, m_addr=0x%x, s_addr=0x%x, size=0x%x)",
		channel, mode, mem_side_addr, lib_side_addr, size);
	return CELL_OK;
}

int cellSoundSynth2VoiceTransStatus(s16 channel, s16 flag)
{
	libsynth2.Error("cellSoundSynth2VoiceTransStatus(channel=%d, flag=%d)", channel, flag);
	return CELL_OK;
}

u16 cellSoundSynth2Note2Pitch(u16 center_note, u16 center_fine, u16 note, s16 fine)
{
	libsynth2.Error("cellSoundSynth2Note2Pitch(center_note=0x%x, center_fine=0x%x, note=0x%x, fine=%d) -> 0",
		center_note, center_fine, note, fine);
	return 0;
}

u16 cellSoundSynth2Pitch2Note(u16 center_note, u16 center_fine, u16 pitch)
{
	libsynth2.Error("cellSoundSynth2Pitch2Note(center_note=0x%x, center_fine=0x%x, pitch=0x%x) -> 0",
		center_note, center_fine, pitch);
	return 0;
}

void libsynth2_init()
{
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Init,
		/*
		0xffffffff7d800026,
		0xfffffffff821ff41,
		0xfffffffffb610098,
		0xff0000008362001c, // lwz
		0xfffffffffb8100a0,
		0xffffffff3f9b0008,
		0xfffffffffba100a8,
		0xffffffff3fa08031,
		0xfffffffffbe100b8,
		0xfffffffffb010080,
		0xfffffffffb210088,
		0xfffffffffb410090,
		0xfffffffffbc100b0,
		0xffffffff7c7f1b78,
		0xffffffff63bd0203,
		0xffffffff918100c8,
		0xffffffff7c0802a6,
		0xfffffffff80100d0,
		0xffffffff897c7688,
		0xffffffff2f8b0000,
		0xffffff00409e01fc, // bne
		0xffffffff38000002,
		0xffffffff39200020,
		0xffffffff3ba00000,
		*/
	);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Exit);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Config);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2GetAddr);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2GetParam);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2GetSwitch);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetAddr);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetParam);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetSwitch);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetEffectMode);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetEffectAttr);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Note2Pitch);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Pitch2Note);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2VoiceTrans);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2VoiceTransStatus);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2SetCoreAttr);
	REG_SUB(libsynth2, "synth2", cellSoundSynth2Generate);
}