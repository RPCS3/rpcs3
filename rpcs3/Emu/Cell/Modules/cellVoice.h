#pragma once

// libvoice = 0x80310801 - 0x803108ff
// libvoice version 100

// Error Codes
enum CellVoiceError : u32
{
	CELL_VOICE_ERROR_ADDRESS_INVALID       = 0x8031080a,
	CELL_VOICE_ERROR_ARGUMENT_INVALID      = 0x80310805,
	CELL_VOICE_ERROR_CONTAINER_INVALID     = 0x80310806,
	CELL_VOICE_ERROR_DEVICE_NOT_PRESENT    = 0x80310812,
	CELL_VOICE_ERROR_EVENT_DISPATCH        = 0x80310811,
	CELL_VOICE_ERROR_EVENT_QUEUE           = 0x8031080f,
	CELL_VOICE_ERROR_GENERAL               = 0x80310803,
	CELL_VOICE_ERROR_LIBVOICE_INITIALIZED  = 0x80310802,
	CELL_VOICE_ERROR_LIBVOICE_NOT_INIT     = 0x80310801,
	CELL_VOICE_ERROR_NOT_IMPLEMENTED       = 0x80310809,
	CELL_VOICE_ERROR_PORT_INVALID          = 0x80310804,
	CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT = 0x80310808,
	CELL_VOICE_ERROR_SERVICE_ATTACHED      = 0x8031080c,
	CELL_VOICE_ERROR_SERVICE_DETACHED      = 0x8031080b,
	CELL_VOICE_ERROR_SERVICE_HANDLE        = 0x80310810,
	CELL_VOICE_ERROR_SERVICE_NOT_FOUND     = 0x8031080d,
	CELL_VOICE_ERROR_SHAREDMEMORY          = 0x8031080e,
	CELL_VOICE_ERROR_TOPOLOGY              = 0x80310807,
};

enum CellVoiceAppType : u32
{
	CELLVOICE_APPTYPE_GAME_1MB = 1 << 29
};

enum CellVoiceBitRate : u32
{
	CELLVOICE_BITRATE_NULL  = ~0u,
	CELLVOICE_BITRATE_3850  = 3850,
	CELLVOICE_BITRATE_4650  = 4650,
	CELLVOICE_BITRATE_5700  = 5700,
	CELLVOICE_BITRATE_7300  = 7300,
	CELLVOICE_BITRATE_14400 = 14400,
	CELLVOICE_BITRATE_16000 = 16000,
	CELLVOICE_BITRATE_22533 = 22533,
};

enum CellVoiceEventType : u32
{
	CELLVOICE_EVENT_DATA_ERROR         = 1 << 0,
	CELLVOICE_EVENT_PORT_ATTACHED      = 1 << 1,
	CELLVOICE_EVENT_PORT_DETACHED      = 1 << 2,
	CELLVOICE_EVENT_SERVICE_ATTACHED   = 1 << 3,
	CELLVOICE_EVENT_SERVICE_DETACHED   = 1 << 4,
	CELLVOICE_EVENT_PORT_WEAK_ATTACHED = 1 << 5,
	CELLVOICE_EVENT_PORT_WEAK_DETACHED = 1 << 6,
};

enum CellVoicePcmDataType : u32
{
	CELLVOICE_PCM_NULL                  = ~0u,
	CELLVOICE_PCM_FLOAT                 = 0,
	CELLVOICE_PCM_FLOAT_LITTLE_ENDIAN   = 1,
	CELLVOICE_PCM_SHORT                 = 2,
	CELLVOICE_PCM_SHORT_LITTLE_ENDIAN   = 3,
	CELLVOICE_PCM_INTEGER               = 4,
	CELLVOICE_PCM_INTEGER_LITTLE_ENDIAN = 5,
};

enum CellVoicePortAttr : u32
{
	CELLVOICE_ATTR_ENERGY_LEVEL      = 1000,
	CELLVOICE_ATTR_VAD               = 1001,
	CELLVOICE_ATTR_DTX               = 1002,
	CELLVOICE_ATTR_AUTO_RESAMPLE     = 1003,
	CELLVOICE_ATTR_LATENCY           = 1004,
	CELLVOICE_ATTR_SILENCE_THRESHOLD = 1005,
};

enum CellVoicePortState : u32
{
	CELLVOICE_PORTSTATE_NULL      = ~0u,
	CELLVOICE_PORTSTATE_IDLE      = 0,
	CELLVOICE_PORTSTATE_READY     = 1,
	CELLVOICE_PORTSTATE_BUFFERING = 2,
	CELLVOICE_PORTSTATE_RUNNING   = 3,
};

enum CellVoicePortType : u32
{
	CELLVOICE_PORTTYPE_NULL          = ~0u,
	CELLVOICE_PORTTYPE_IN_MIC        = 0,
	CELLVOICE_PORTTYPE_IN_PCMAUDIO   = 1,
	CELLVOICE_PORTTYPE_IN_VOICE      = 2,
	CELLVOICE_PORTTYPE_OUT_PCMAUDIO  = 3,
	CELLVOICE_PORTTYPE_OUT_VOICE     = 4,
	CELLVOICE_PORTTYPE_OUT_SECONDARY = 5,
};

enum CellVoiceSamplingRate : u32
{
	CELLVOICE_SAMPLINGRATE_NULL  = ~0u,
	CELLVOICE_SAMPLINGRATE_16000 = 16000,
};

enum CellVoiceVersionCheck : u32
{
	CELLVOICE_VERSION_100 = 100
};

// Definitions
enum
{
	CELLVOICE_MAX_IN_VOICE_PORT           = 32,
	CELLVOICE_MAX_OUT_VOICE_PORT          = 4,
	CELLVOICE_GAME_1MB_MAX_IN_VOICE_PORT  = 8,
	CELLVOICE_GAME_1MB_MAX_OUT_VOICE_PORT = 2,
	CELLVOICE_MAX_PORT                    = 128,
	CELLVOICE_INVALID_PORT_ID             = 0xff,
};

struct CellVoiceBasePortInfo // aligned(64)
{
	be_t<CellVoicePortType> portType;
	be_t<CellVoicePortState> state;
	be_t<u16> numEdge;
	vm::bptr<u32> pEdge;
	be_t<u32> numByte;
	be_t<u32> frameSize;
};

struct CellVoiceInitParam // aligned(32)
{
	be_t<CellVoiceEventType> eventMask;
	be_t<CellVoiceVersionCheck> version;
	be_t<s32> appType;
	u8 reserved[32 - sizeof(s32) * 3];
};

struct CellVoicePCMFormat
{
	u8 numChannels;
	u8 sampleAlignment;
	be_t<CellVoicePcmDataType> dataType;
	be_t<CellVoiceSamplingRate> sampleRate;
};

struct CellVoicePortParam // aligned(64)
{
	be_t<CellVoicePortType> portType;
	be_t<u16> threshold;
	be_t<u16> bMute;
	be_t<f32> volume;
	union
	{
		struct
		{
			be_t<CellVoiceBitRate> bitrate;
		} voice;
		struct
		{
			be_t<u32> bufSize;
			CellVoicePCMFormat format;
		} pcmaudio;
		struct
		{
			be_t<u32> playerId;
		} device;
	};
};

struct CellVoiceStartParam // aligned(32)
{
	be_t<u32> container;
	u8 reserved[32 - sizeof(s32) * 1];
};

struct voice_manager
{
	struct port_t
	{
		s32 state = CELLVOICE_PORTSTATE_NULL;
		CellVoicePortParam info;

		ENABLE_BITWISE_SERIALIZATION;
	};

	// See cellVoiceCreatePort
	u8 id_ctr = 0;

	// For cellVoiceSetNotifyEventQueue
	u32 port_source = 0;

	std::unordered_map<u16, port_t> ports;
	std::unordered_map<u64, std::deque<u64>> queue_keys;
	bool voice_service_started = false;

	port_t* access_port(u32 id)
	{
		// Upper 16 bits are ignored
		auto pos = ports.find(static_cast<u16>(id));

		if (pos == ports.end())
		{
			return nullptr;
		}

		return &pos->second;
	}

	void reset();
	shared_mutex mtx;
	atomic_t<bool> is_init{ false };

	SAVESTATE_INIT_POS(17);

	voice_manager() = default;
	voice_manager(utils::serial& ar) { save(ar); }
	void save(utils::serial& ar);
};
