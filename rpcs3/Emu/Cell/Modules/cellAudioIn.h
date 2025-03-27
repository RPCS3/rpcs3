#pragma once

#include "util/types.hpp"

// Error codes
enum CellAudioInError : u32
{
	CELL_AUDIO_IN_ERROR_NOT_IMPLEMENTED        = 0x8002b260,
	CELL_AUDIO_IN_ERROR_ILLEGAL_CONFIGURATION  = 0x8002b261,
	CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER      = 0x8002b262,
	CELL_AUDIO_IN_ERROR_PARAMETER_OUT_OF_RANGE = 0x8002b263,
	CELL_AUDIO_IN_ERROR_DEVICE_NOT_FOUND       = 0x8002b264,
	CELL_AUDIO_IN_ERROR_UNSUPPORTED_AUDIO_IN   = 0x8002b265,
	CELL_AUDIO_IN_ERROR_UNSUPPORTED_SOUND_MODE = 0x8002b266,
	CELL_AUDIO_IN_ERROR_CONDITION_BUSY         = 0x8002b267,
};

enum CellAudioInDeviceMode
{
	CELL_AUDIO_IN_SINGLE_DEVICE_MODE   = 0,
	CELL_AUDIO_IN_MULTI_DEVICE_MODE    = 1,
	CELL_AUDIO_IN_MULTI_DEVICE_MODE_2  = 2,
	CELL_AUDIO_IN_MULTI_DEVICE_MODE_10 = 10,
};

enum CellAudioInPortType
{
	CELL_AUDIO_IN_PORT_USB       = 3,
	CELL_AUDIO_IN_PORT_BLUETOOTH = 4,
};

enum CellAudioInDeviceState
{
	CELL_AUDIO_IN_DEVICE_STATE_UNAVAILABLE = 0,
	CELL_AUDIO_IN_DEVICE_STATE_AVAILABLE   = 1,
};

enum CellAudioInCodingType
{
	CELL_AUDIO_IN_CODING_TYPE_LPCM = 0,
};

enum CellAudioInChnum
{
	CELL_AUDIO_IN_CHNUM_NONE = 0,
	CELL_AUDIO_IN_CHNUM_1    = 1,
	CELL_AUDIO_IN_CHNUM_2    = 2,
};

enum CellAudioInFs
{
	CELL_AUDIO_IN_FS_UNDEFINED = 0x00,
	CELL_AUDIO_IN_FS_8KHZ      = 0x01,
	CELL_AUDIO_IN_FS_12KHZ     = 0x02,
	CELL_AUDIO_IN_FS_16KHZ     = 0x04,
	CELL_AUDIO_IN_FS_24KHZ     = 0x08,
	CELL_AUDIO_IN_FS_32KHZ     = 0x10,
	CELL_AUDIO_IN_FS_48KHZ     = 0x20,
};

struct CellAudioInSoundMode
{
	u8 type;
	u8 channel;
	be_t<u16> fs;
	u8 reserved[4];
};

struct CellAudioInDeviceInfo
{
	u8 portType;
	u8 availableModeCount;
	u8 state;
	u8 deviceNumber;
	u8 reserved[12];
	be_t<u64> deviceId;
	be_t<u64> type;
	char name[64]; // Not necessarily null terminated!
	CellAudioInSoundMode availableModes[16];
};

struct CellAudioInRegistrationOption
{
	be_t<u32> reserved;
};

struct CellAudioInDeviceConfiguration
{
	u8 volume;
	u8 reserved[31];
};
