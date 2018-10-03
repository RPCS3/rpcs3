#pragma once

#include "Utilities/Thread.h"

// Error Codes
enum
{
	CELL_MIC_ERROR_ALREADY_INIT = 0x80140101,
	CELL_MIC_ERROR_SYSTEM = 0x80140102,
	CELL_MIC_ERROR_NOT_INIT = 0x80140103,
	CELL_MIC_ERROR_PARAM = 0x80140104,
	CELL_MIC_ERROR_PORT_FULL = 0x80140105,
	CELL_MIC_ERROR_ALREADY_OPEN = 0x80140106,
	CELL_MIC_ERROR_NOT_OPEN = 0x80140107,
	CELL_MIC_ERROR_NOT_RUN = 0x80140108,
	CELL_MIC_ERROR_TRANS_EVENT = 0x80140109,
	CELL_MIC_ERROR_OPEN = 0x8014010a,
	CELL_MIC_ERROR_SHAREDMEMORY = 0x8014010b,
	CELL_MIC_ERROR_MUTEX = 0x8014010c,
	CELL_MIC_ERROR_EVENT_QUEUE = 0x8014010d,
	CELL_MIC_ERROR_DEVICE_NOT_FOUND = 0x8014010e,
	CELL_MIC_ERROR_SYSTEM_NOT_FOUND = 0x8014010e,
	CELL_MIC_ERROR_FATAL = 0x8014010f,
	CELL_MIC_ERROR_DEVICE_NOT_SUPPORT = 0x80140110,
};

struct CellMicInputFormat
{
	u8    channelNum;
	u8    subframeSize;
	u8    bitResolution;
	u8    dataType;
	be_t<u32>   sampleRate;
};

enum CellMicSignalState
{
	CELL_MIC_SIGSTATE_LOCTALK = 0,
	CELL_MIC_SIGSTATE_FARTALK = 1,
	CELL_MIC_SIGSTATE_NSR     = 3,
	CELL_MIC_SIGSTATE_AGC     = 4,
	CELL_MIC_SIGSTATE_MICENG  = 5,
	CELL_MIC_SIGSTATE_SPKENG  = 6,
};

enum CellMicCommand
{
	CELL_MIC_ATTACH = 2,
	CELL_MIC_DATA = 5,
};

// TODO: generate this from input from an actual microphone
const u32 bufferSize = 1;

class mic_context
{
public:
	void operator()();

	// Default value of 48000 for no particular reason
	u32 DspFrequency = 48000; // DSP is the default type
	u32 rawFrequency = 48000;
	u32 AuxFrequency = 48000;
	u8 bitResolution = 32;
	bool micOpened = false;
	bool micStarted = false;
	u64 eventQueueKey = 0;

	u32 signalStateLocalTalk = 9; // value is in range 0-10. 10 indicates talking, 0 indicating none.
	u32 signalStateFarTalk = 0; // value is in range 0-10. 10 indicates talking from far away, 0 indicating none.
	f32 signalStateNoiseSupression; // value is in decibels
	f32 signalStateGainControl;
	f32 signalStateMicSignalLevel; // value is in decibels
	f32 signalStateSpeakerSignalLevel; // value is in decibels
};

using mic_thread = named_thread<mic_context>;
