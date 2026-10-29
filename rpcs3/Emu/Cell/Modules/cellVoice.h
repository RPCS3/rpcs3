#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Emu/savestate_utils.hpp"
#include "Utilities/Thread.h"
#include "Utilities/mutex.h"
#include "cellVoice_firmware.h"

#include <deque>
#include <set>

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

// The voice service (cellVoice.cpp): the ports' data, their connections, the microphone, the headset output and the
// PPU worker that runs them every 20 ms, as sys_audio's voice process does on the PS3.
struct voice_service;
struct voice_manager;

enum class voice_run_state : u32
{
	stopped,
	starting,
	running,
	stopping,
	failed,
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
	voice_run_state run_state = voice_run_state::stopped;
	u64 service_generation = 0;
	u32 worker_id = 0;
	u32 stop_caller = 0;
	u32 last_error = 0;

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

	// Sends an event to every queue registered with cellVoiceSetNotifyEventQueue
	void notify(u64 event, u64 data2 = 0);

	// The service (created with its thread on first use)
	voice_service& service();

	void reset();
	shared_mutex mtx;
	atomic_t<bool> is_init{ false };

	std::unique_ptr<voice_service> m_service;

	SAVESTATE_INIT_POS(17);

	voice_manager();
	voice_manager(utils::serial& ar);
	voice_manager(const voice_manager&) = delete;
	voice_manager& operator=(const voice_manager&) = delete;
	~voice_manager();
	void save(utils::serial& ar);
};

class AudioBackend;
class microphone_device;
namespace utils { class audio_decoder; }

// On the PS3, libvoice is a client of sys_audio's voice process, which mixes each output port's connected inputs every
// frame: IN_MIC (the headset microphone), IN_PCMAUDIO and IN_VOICE (data the game writes) into OUT_PCMAUDIO and OUT_VOICE
// (data the game reads) and OUT_SECONDARY (the headset). Voice ports carry MPEG-4 CELP frames, whole
// frames back to back without headers; the service runs at 16 kHz and CELP at 8 kHz. Games such as Modern Warfare 3,
// Black Ops and Black Ops II connect IN_MIC to an OUT_VOICE port at 7300 bps (19-byte frames of 20 ms), send what they
// read from it, write what they receive to an IN_VOICE port per talker, and connect those to OUT_SECONDARY.
struct voice_service
{
	static constexpr u32 rate = 16000;          // CELLVOICE_SAMPLINGRATE_16000
	static constexpr u32 tick_samples = 320;    // the service's period: 20 ms
	static constexpr u64 tick_usec = 20'000;

	// 2:1 between 16 and 8 kHz: a 31-tap windowed-sinc low-pass at 3.8 kHz
	struct halfband_filter
	{
		std::array<f32, 31> x{};

		f32 push(f32 v);
		void down(const f32* in, u32 count, std::vector<f32>& out); // 16 kHz to 8 kHz (count even)
		void up(const f32* in, u32 count, std::vector<f32>& out);   // 8 kHz to 16 kHz
	};

	struct port_data
	{
		std::vector<u8> bytes;                   // IN_VOICE / IN_PCMAUDIO: written by the game; OUT_VOICE / OUT_PCMAUDIO: for it to read
		std::vector<f32> pcm;                    // IN_VOICE: decoded 16 kHz samples not played yet; OUT_VOICE: 8 kHz samples not encoded
		std::array<f32, tick_samples> signal{};  // an input's samples this period (16 kHz, 16-bit scale)
		u32 codec_bitrate = 0;
		u64 codec_generation = 0;
		bool codec_encode = false;
		u32 codec_pending = 0;
		bool codec_failed = false;
		halfband_filter filter;
		bool playing = false;                    // IN_VOICE: past its pre-buffering
		u32 waited = 0;
		u32 hangover = 0;                        // OUT_VOICE: voice activity
		f32 noise_db = 30.0f;
		bool attached = false;                   // IN_MIC: PORT_ATTACHED sent
		bool paused = false;
		bool logged = false;
		bool read_logged = false;
	};

	std::unordered_map<u16, port_data> data;
	std::set<std::pair<u16, u16>> edges; // (input, output)
	bool mute_all = false;

	// The headset microphone (IN_MIC): the device configured for cellMic, or RPCS3_CELLVOICE_MIC_FILE (mic_file)
	std::unique_ptr<microphone_device> mic;
	bool mic_failed = false;
	bool mic_file_loaded = false;
	std::vector<f32> mic_file;
	usz mic_file_pos = 0;
	std::vector<f32> mic_fifo;
	std::vector<u8> mic_raw;
	bool mic_primed = false;
	f64 mic_resample_pos = 0.0;
	f32 mic_last = 0.0f;
	std::unique_ptr<utils::audio_decoder> mic_decoder;
	u64 mic_decoder_pos = 0;
	u32 mic_config_type = ~0u;
	std::string mic_config_device;
	std::string mic_config_file;
	bool mic_config_test = false;
	bool mic_source_changed = false;
	bool mic_transmitting = true;
	u64 mic_talk_generation = 0;

	// The headset output (OUT_SECONDARY)
	std::shared_ptr<AudioBackend> headset;
	bool headset_failed = false;
	shared_mutex headset_mtx;
	std::deque<f32> headset_fifo;
	bool headset_primed = false;
	f64 headset_pos = 1.0;
	f64 headset_step = 1.0 / 3;
	f32 headset_prev = 0.0f, headset_cur = 0.0f, headset_volume = 1.0f;
	u32 headset_channels = 2;
	bool headset_s16 = false;

	// Firmware ownership is used only by the PPU worker. Requests contain copied inputs.
	std::unique_ptr<voice_firmware::backend> firmware;
	std::vector<voice_firmware::request> requests;
	u64 next_codec_generation = 1;

	explicit voice_service(voice_manager& manager, bool run = true);
	voice_service(const voice_service&) = delete;
	voice_service& operator=(const voice_service&) = delete;
	~voice_service();

	bool mic_present() const;

	// One period: the microphone, the inputs, each output's mix (manager.mtx held)
	void tick(voice_manager& manager);

	// Devices closed, IN_MIC ports detached, data dropped
	void stop(voice_manager& manager);
	void remove_port(u16 id);
	void clear();
	void commit(voice_manager& manager, const voice_firmware::result& result);
	std::vector<voice_firmware::key> codec_keys() const;
	void save(utils::serial& ar);

	// cellVoiceWriteToIPort / cellVoiceReadFromOPort: the bytes taken or given
	u32 write(const voice_manager::port_t& port, u16 id, const u8* src, u32 size);
	u32 read(voice_manager& manager, const voice_manager::port_t& port, u16 id, u8* dst, u32 capacity);
	u32 bytes_queued(u16 id) const;

private:
	bool ensure_codec(port_data& d, u32 bitrate, bool encode, u16 id);
	void decode_voice(const voice_manager::port_t& port, port_data& d, u16 id);
	void encode_voice(const voice_manager::port_t& port, port_data& d, u16 id, const f32* mix);
	void read_pcm(const voice_manager::port_t& port, port_data& d);
	void write_pcm(const voice_manager::port_t& port, port_data& d, const f32* mix);

	void open_mic();
	void close_mic();
	void pull_mic();
	void take_mic(f32* out);
	bool update_mic_gate(voice_manager& manager);
	void discard_mic_outputs(voice_manager& manager);

	void open_headset();
	void close_headset();
	void play_headset(const f32* mix);
	u32 headset_callback(u32 bytes, void* buffer);
};
