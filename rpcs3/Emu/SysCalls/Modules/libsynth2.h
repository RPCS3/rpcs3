#pragma once

enum
{
	//libsynt2 Error Codes
	CELL_SOUND_SYNTH2_ERROR_FATAL = 0x80310201,
	CELL_SOUND_SYNTH2_ERROR_INVALID_PARAMETER = 0x80310202,
	CELL_SOUND_SYNTH2_ERROR_ALREADY_INITIALIZED = 0x80310203,
};

struct CellSoundSynth2EffectAttr
{
	be_t<u16> core;
	be_t<u16> mode;
	be_t<s16> depth_L;
	be_t<s16> depth_R;
	be_t<u16> delay;
	be_t<u16> feedback;
};