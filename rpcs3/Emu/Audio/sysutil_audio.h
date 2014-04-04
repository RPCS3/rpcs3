#pragma once

//error codes
enum AudioErrorCode
{
	CELL_AUDIO_OUT_SUCCEEDED						= 0,
	CELL_AUDIO_OUT_ERROR_NOT_IMPLEMENTED			= 0x8002b240,
	CELL_AUDIO_OUT_ERROR_ILLEGAL_CONFIGURATION		= 0x8002b241,
	CELL_AUDIO_OUT_ERROR_ILLEGAL_PARAMETER			= 0x8002b242,
	CELL_AUDIO_OUT_ERROR_PARAMETER_OUT_OF_RANGE		= 0x8002b243,
	CELL_AUDIO_OUT_ERROR_DEVICE_NOT_FOUND			= 0x8002b244,
	CELL_AUDIO_OUT_ERROR_UNSUPPORTED_AUDIO_OUT		= 0x8002b245,
	CELL_AUDIO_OUT_ERROR_UNSUPPORTED_SOUND_MODE		= 0x8002b246,
	CELL_AUDIO_OUT_ERROR_CONDITION_BUSY				= 0x8002b247,
	CELL_AUDIO_IN_ERROR_NOT_IMPLEMENTED				= 0x8002b260,
	CELL_AUDIO_IN_ERROR_ILLEGAL_CONFIGURATION		= 0x8002b261,
	CELL_AUDIO_IN_ERROR_ILLEGAL_PARAMETER			= 0x8002b262,
	CELL_AUDIO_IN_ERROR_PARAMETER_OUT_OF_RANGE		= 0x8002b263,
	CELL_AUDIO_IN_ERROR_DEVICE_NOT_FOUND			= 0x8002b264,
	CELL_AUDIO_IN_ERROR_UNSUPPORTED_AUDIO_IN		= 0x8002b265,
	CELL_AUDIO_IN_ERROR_UNSUPPORTED_SOUND_MODE		= 0x8002b266,
	CELL_AUDIO_IN_ERROR_CONDITION_BUSY				= 0x8002b267,
};

enum CellAudioOut
{
	CELL_AUDIO_OUT_PRIMARY   = 0,
	CELL_AUDIO_OUT_SECONDARY = 1,
};

enum CellAudioOutDownMixer
{
	CELL_AUDIO_OUT_DOWNMIXER_NONE   = 0,
	CELL_AUDIO_OUT_DOWNMIXER_TYPE_A = 1,
	CELL_AUDIO_OUT_DOWNMIXER_TYPE_B = 2,
};

enum
{
	CELL_AUDIO_OUT_SINGLE_DEVICE_MODE  = 0,
	CELL_AUDIO_OUT_MULTI_DEVICE_MODE   = 1,
	CELL_AUDIO_OUT_MULTI_DEVICE_MODE_2 = 2, 
};

enum CellAudioOutPortType
{
	CELL_AUDIO_OUT_PORT_HDMI      = 0,
	CELL_AUDIO_OUT_PORT_SPDIF     = 1,
	CELL_AUDIO_OUT_PORT_ANALOG    = 2,
	CELL_AUDIO_OUT_PORT_USB       = 3,
	CELL_AUDIO_OUT_PORT_BLUETOOTH = 4,
	CELL_AUDIO_OUT_PORT_NETWORK   = 5,
};

enum CellAudioOutDeviceState
{
	CELL_AUDIO_OUT_DEVICE_STATE_UNAVAILABLE = 0,
	CELL_AUDIO_OUT_DEVICE_STATE_AVAILABLE   = 1,
};

enum CellAudioOutOutputState
{
	CELL_AUDIO_OUT_OUTPUT_STATE_ENABLED   = 0,
	CELL_AUDIO_OUT_OUTPUT_STATE_DISABLED  = 1,
	CELL_AUDIO_OUT_OUTPUT_STATE_PREPARING = 2,
};

enum CellAudioOutCodingType
{
	CELL_AUDIO_OUT_CODING_TYPE_LPCM       = 0,
	CELL_AUDIO_OUT_CODING_TYPE_AC3        = 1,
	CELL_AUDIO_OUT_CODING_TYPE_MPEG1      = 2,
	CELL_AUDIO_OUT_CODING_TYPE_MP3        = 3,
	CELL_AUDIO_OUT_CODING_TYPE_MPEG2      = 4,
	CELL_AUDIO_OUT_CODING_TYPE_AAC        = 5,
	CELL_AUDIO_OUT_CODING_TYPE_DTS        = 6,
	CELL_AUDIO_OUT_CODING_TYPE_ATRAC      = 7,
	CELL_AUDIO_OUT_CODING_TYPE_BITSTREAM  = 0xff,
};

enum CellAudioOutChnum
{
	CELL_AUDIO_OUT_CHNUM_2  = 2,
	CELL_AUDIO_OUT_CHNUM_4  = 4,
	CELL_AUDIO_OUT_CHNUM_6  = 6,
	CELL_AUDIO_OUT_CHNUM_8  = 8,
};

enum CellAudioOutFs
{
	CELL_AUDIO_OUT_FS_32KHZ  = 0x01,
	CELL_AUDIO_OUT_FS_44KHZ  = 0x02,
	CELL_AUDIO_OUT_FS_48KHZ  = 0x04,
	CELL_AUDIO_OUT_FS_88KHZ  = 0x08,
	CELL_AUDIO_OUT_FS_96KHZ  = 0x10, 
	CELL_AUDIO_OUT_FS_176KHZ = 0x20,
	CELL_AUDIO_OUT_FS_192KHZ = 0x40,
};

enum
{
	CELL_AUDIO_OUT_SPEAKER_LAYOUT_2CH          = 0x00000001,
	CELL_AUDIO_OUT_SPEAKER_LAYOUT_6CH_LREClr   = 0x00010000,
	CELL_AUDIO_OUT_SPEAKER_LAYOUT_8CH_LREClrxy = 0x40000000,
};

enum
{
	CELL_AUDIO_OUT_COPY_CONTROL_COPY_FREE  = 0,
	CELL_AUDIO_OUT_COPY_CONTROL_COPY_ONCE  = 1,
	CELL_AUDIO_OUT_COPY_CONTROL_COPY_NEVER = 2,
};

enum
{
	CELL_AUDIO_IN_SINGLE_DEVICE_MODE  = 0,
	CELL_AUDIO_IN_MULTI_DEVICE_MODE   = 1,
	CELL_AUDIO_IN_MULTI_DEVICE_MODE_2 = 2,
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

struct CellAudioOutConfiguration
{ 
	u8 channel; 
	u8 encoder; 
	u8 reserved[10];
	be_t<u32> downMixer;
};

struct CellAudioOutSoundMode
{ 
	u8 type; 
	u8 channel; 
	u8 fs; 
	u8 reserved; 
	be_t<u32> layout; 
};

struct CellAudioOutDeviceInfo
{ 
	u8 portType; 
	u8 availableModeCount; 
	u8 state;
	u8 reserved[3];
	be_t<u16> latency;
	CellAudioOutSoundMode availableModes[16];
};

struct CellAudioOutState
{ 
	u8 state; 
	u8 encoder; 
	u8 reserved[6]; 
	be_t<u32> downMixer; 
	CellAudioOutSoundMode soundMode; 
};

struct CellAudioOutSoundMode2
{ 
	u8 type; 
	u8 channel; 
	be_t<u16> fs; 
	u8 reserved[4]; 
};

struct CellAudioOutDeviceInfo2
{ 
	u8 portType; 
	u8 availableModeCount; 
	u8 state; 
	u8 deviceNumber; 
	u8 reserved[12]; 
	be_t<u64> deviceId; 
	be_t<u64> type; 
	char name[64]; 
	CellAudioOutSoundMode2 availableModes2[16];
};

struct CellAudioOutOption
{ 
	//(Omitted)
};

struct CellAudioOutRegistrationOption
{ 
	//(Omitted)
};

struct CellAudioOutDeviceConfiguration
{ 
	//(Omitted)
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
	char name[64]; 
	CellAudioInSoundMode availableModes[16];
};

struct CellAudioInRegistrationOption
{ 
	//(Omitted)
};

struct CellAudioInDeviceConfiguration
{ 
	u8 volume; 
	u8 reserved[31];
};

enum CellBgmPlaybackStatusState
{
	CELL_BGMPLAYBACK_STATUS_PLAY = 0,
	CELL_BGMPLAYBACK_STATUS_STOP = 1
};

enum CellBgmPlaybackStatusEnabled
{
	CELL_BGMPLAYBACK_STATUS_ENABLE = 0,
	CELL_BGMPLAYBACK_STATUS_DISABLE = 1
};

struct CellBgmPlaybackStatus
{
	u8 playbackState;
	u8 enabled;
	char contentId[16];
	u8 fadeRatio;
	char reserved[13];
};
