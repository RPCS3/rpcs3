#pragma once

enum SceVoicePortType : s32
{
	SCEVOICE_PORTTYPE_NULL = -1,
	SCEVOICE_PORTTYPE_IN_DEVICE = 0,
	SCEVOICE_PORTTYPE_IN_PCMAUDIO = 1,
	SCEVOICE_PORTTYPE_IN_VOICE = 2,
	SCEVOICE_PORTTYPE_OUT_PCMAUDIO = 3,
	SCEVOICE_PORTTYPE_OUT_VOICE = 4,
	SCEVOICE_PORTTYPE_OUT_DEVICE = 5
};

enum SceVoicePortState : s32
{
	SCEVOICE_PORTSTATE_NULL = -1,
	SCEVOICE_PORTSTATE_IDLE = 0,
	SCEVOICE_PORTSTATE_READY = 1,
	SCEVOICE_PORTSTATE_BUFFERING = 2,
	SCEVOICE_PORTSTATE_RUNNING = 3
};

enum SceVoiceBitRate : s32
{
	SCEVOICE_BITRATE_NULL = -1,
	SCEVOICE_BITRATE_3850 = 3850,
	SCEVOICE_BITRATE_4650 = 4650,
	SCEVOICE_BITRATE_5700 = 5700,
	SCEVOICE_BITRATE_7300 = 7300
};

enum SceVoiceSamplingRate : s32
{
	SCEVOICE_SAMPLINGRATE_NULL = -1,
	SCEVOICE_SAMPLINGRATE_16000 = 16000
};

enum SceVoicePcmDataType : s32
{
	SCEVOICE_PCM_NULL = -1,
	SCEVOICE_PCM_SHORT_LITTLE_ENDIAN = 0
};

enum SceVoiceVersion : s32
{
	SCEVOICE_VERSION_100 = 100
};

enum SceVoiceAppType : s32
{
	SCEVOICE_APPTYPE_GAME = 1 << 29
};

struct SceVoicePCMFormat
{
	le_t<s32> dataType; // SceVoicePcmDataType
	le_t<s32> sampleRate; // SceVoiceSamplingRate
};

struct SceVoiceResourceInfo
{
	le_t<u16> maxInVoicePort;
	le_t<u16> maxOutVoicePort;
	le_t<u16> maxInDevicePort;
	le_t<u16> maxOutDevicePort;
	le_t<u16> maxTotalPort;
};

struct SceVoiceBasePortInfo
{
	le_t<s32> portType; // SceVoicePortType
	le_t<s32> state; // SceVoicePortState
	vm::lptr<u32> pEdge;
	le_t<u32> numByte;
	le_t<u32> frameSize;
	le_t<u16> numEdge;
	le_t<u16> reserved;
};

struct SceVoicePortParam
{
	// aux structs

	struct _voice_t
	{
		le_t<s32> bitrate; // SceVoiceBitRate
	};

	struct _pcmaudio_t
	{
		using _format_t = SceVoicePCMFormat;

		le_t<u32> bufSize;
		_format_t format;
	};

	struct _device_t
	{
		le_t<u32> playerId;
	};

	// struct members

	le_t<s32> portType; // SceVoicePortType
	le_t<u16> threshold;
	le_t<u16> bMute;
	le_t<float> volume;

	union
	{
		_pcmaudio_t pcmaudio;
		_device_t device;
		_voice_t voice;
	};
};

using SceVoiceEventCallback = void(vm::ptr<void> event);

struct SceVoiceInitParam
{
	le_t<s32> appType;
	vm::lptr<SceVoiceEventCallback> onEvent;
	u8 reserved[24];
};

struct SceVoiceStartParam
{
	le_t<s32> container;
	u8 reserved[28];
};

extern psv_log_base sceVoice;
