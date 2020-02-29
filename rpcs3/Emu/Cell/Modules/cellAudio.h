#pragma once

#include "Emu/Memory/vm_ptr.h"
#include "Utilities/Thread.h"
#include "Emu/Memory/vm.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/AudioDumper.h"

// Error codes
enum CellAudioError : u32
{
	CELL_AUDIO_ERROR_ALREADY_INIT               = 0x80310701,
	CELL_AUDIO_ERROR_AUDIOSYSTEM                = 0x80310702,
	CELL_AUDIO_ERROR_NOT_INIT                   = 0x80310703,
	CELL_AUDIO_ERROR_PARAM                      = 0x80310704,
	CELL_AUDIO_ERROR_PORT_FULL                  = 0x80310705,
	CELL_AUDIO_ERROR_PORT_ALREADY_RUN           = 0x80310706,
	CELL_AUDIO_ERROR_PORT_NOT_OPEN              = 0x80310707,
	CELL_AUDIO_ERROR_PORT_NOT_RUN               = 0x80310708,
	CELL_AUDIO_ERROR_TRANS_EVENT                = 0x80310709,
	CELL_AUDIO_ERROR_PORT_OPEN                  = 0x8031070a,
	CELL_AUDIO_ERROR_SHAREDMEMORY               = 0x8031070b,
	CELL_AUDIO_ERROR_MUTEX                      = 0x8031070c,
	CELL_AUDIO_ERROR_EVENT_QUEUE                = 0x8031070d,
	CELL_AUDIO_ERROR_AUDIOSYSTEM_NOT_FOUND      = 0x8031070e,
	CELL_AUDIO_ERROR_TAG_NOT_FOUND              = 0x8031070f,
};

// constants
enum
{
	CELL_AUDIO_BLOCK_16                = 16,
	CELL_AUDIO_BLOCK_8                 = 8,
	CELL_AUDIO_BLOCK_SAMPLES           = 256,
	CELL_AUDIO_CREATEEVENTFLAG_SPU     = 0x00000001,
	CELL_AUDIO_EVENT_HEADPHONE         = 1,
	CELL_AUDIO_EVENT_MIX               = 0,
	CELL_AUDIO_EVENTFLAG_BEFOREMIX     = 0x80000000,
	CELL_AUDIO_EVENTFLAG_DECIMATE_2    = 0x08000000,
	CELL_AUDIO_EVENTFLAG_DECIMATE_4    = 0x10000000,
	CELL_AUDIO_EVENTFLAG_HEADPHONE     = 0x20000000,
	CELL_AUDIO_EVENTFLAG_NOMIX         = 0x40000000,
	CELL_AUDIO_MAX_PORT                = 4,
	CELL_AUDIO_MAX_PORT_2              = 8,
	CELL_AUDIO_MISC_ACCVOL_ALLDEVICE   = 0x0000ffffUL,
	CELL_AUDIO_PERSONAL_DEVICE_PRIMARY = 0x8000,
	CELL_AUDIO_PORT_2CH                = 2,
	CELL_AUDIO_PORT_8CH                = 8,
	CELL_AUDIO_PORTATTR_BGM            = 0x0000000000000010ULL,
	CELL_AUDIO_PORTATTR_INITLEVEL      = 0x0000000000001000ULL,
	CELL_AUDIO_PORTATTR_OUT_NO_ROUTE   = 0x0000000000100000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_0 = 0x0000000001000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_1 = 0x0000000002000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_2 = 0x0000000004000000ULL,
	CELL_AUDIO_PORTATTR_OUT_PERSONAL_3 = 0x0000000008000000ULL,
	CELL_AUDIO_PORTATTR_OUT_SECONDARY  = 0x0000000000000001ULL,
	CELL_AUDIO_STATUS_CLOSE            = 0x1010,
	CELL_AUDIO_STATUS_READY            = 1,
	CELL_AUDIO_STATUS_RUN              = 2,
};

//libaudio datatypes
struct CellAudioPortParam
{
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u64> attr;
	be_t<float> level;
};

struct CellAudioPortConfig
{
	vm::bptr<u64> readIndexAddr;
	be_t<u32> status;
	be_t<u64> nChannel;
	be_t<u64> nBlock;
	be_t<u32> portSize;
	be_t<u32> portAddr;
};

enum : u32
{
	AUDIO_PORT_COUNT = 8,
	AUDIO_MAX_BLOCK_COUNT = 32,
	AUDIO_MAX_CHANNELS_COUNT = 8,

	AUDIO_PORT_OFFSET = AUDIO_BUFFER_SAMPLES * AUDIO_MAX_BLOCK_COUNT * AUDIO_MAX_CHANNELS_COUNT * sizeof(f32),
	EXTRA_AUDIO_BUFFERS = 8,
	MAX_AUDIO_EVENT_QUEUES = 64,

	AUDIO_BLOCK_SIZE_2CH = 2 * AUDIO_BUFFER_SAMPLES,
	AUDIO_BLOCK_SIZE_8CH = 8 * AUDIO_BUFFER_SAMPLES,

	PORT_BUFFER_TAG_COUNT = 6,

	PORT_BUFFER_TAG_LAST_2CH = AUDIO_BLOCK_SIZE_2CH - 1,
	PORT_BUFFER_TAG_DELTA_2CH = PORT_BUFFER_TAG_LAST_2CH / (PORT_BUFFER_TAG_COUNT - 1),
	PORT_BUFFER_TAG_FIRST_2CH = PORT_BUFFER_TAG_LAST_2CH % (PORT_BUFFER_TAG_COUNT - 1),

	PORT_BUFFER_TAG_LAST_8CH = AUDIO_BLOCK_SIZE_8CH - 1,
	PORT_BUFFER_TAG_DELTA_8CH = PORT_BUFFER_TAG_LAST_8CH / (PORT_BUFFER_TAG_COUNT - 1),
	PORT_BUFFER_TAG_FIRST_8CH = PORT_BUFFER_TAG_LAST_8CH % (PORT_BUFFER_TAG_COUNT - 1),
};

enum class audio_port_state : u32
{
	closed,
	opened,
	started,
};

struct audio_port
{
	atomic_t<audio_port_state> state = audio_port_state::closed;

	u32 number;
	vm::ptr<char> addr{};
	vm::ptr<u64> index{};

	u32 num_channels;
	u32 num_blocks;
	u64 attr;
	u64 cur_pos;
	u64 global_counter; // copy of global counter
	u64 active_counter;
	u32 size;
	u64 timestamp; // copy of global timestamp

	struct alignas(8) level_set_t
	{
		float value;
		float inc;
	};

	float level;
	atomic_t<level_set_t> level_set;

	u32 block_size() const
	{
		return num_channels * AUDIO_BUFFER_SAMPLES;
	}

	u32 buf_size() const
	{
		return block_size() * sizeof(float);
	}

	u32 position(s32 offset = 0) const
	{
		s32 ofs = (offset % num_blocks) + num_blocks;
		return (cur_pos + ofs) % num_blocks;
	}

	u32 buf_addr(s32 offset = 0) const
	{
		return addr.addr() + position(offset) * buf_size();
	}

	to_be_t<float>* get_vm_ptr(s32 offset = 0) const
	{
		return vm::_ptr<f32>(buf_addr(offset));
	}


	// Tags
	u32 prev_touched_tag_nr;
	f32 last_tag_value[PORT_BUFFER_TAG_COUNT] = { 0 };

	void tag(s32 offset = 0);
};

struct cell_audio_config
{
	const std::shared_ptr<AudioBackend> backend = Emu.GetCallbacks().get_audio();

	const u32 audio_channels = AudioBackend::get_channels();
	const u32 audio_sampling_rate = AudioBackend::get_sampling_rate();
	const u32 audio_block_period = AUDIO_BUFFER_SAMPLES * 1000000 / audio_sampling_rate;

	const u32 audio_buffer_length = AUDIO_BUFFER_SAMPLES * audio_channels;
	const u32 audio_buffer_size = audio_buffer_length * AudioBackend::get_sample_size();

	/*
	 * Buffering
	 */
	const u64 desired_buffer_duration = g_cfg.audio.desired_buffer_duration * 1000llu;
private:
	const bool raw_buffering_enabled = static_cast<bool>(g_cfg.audio.enable_buffering);
public:
	// We need a non-blocking backend (implementing play/pause/flush) to be able to do buffering correctly
	// We also need to be able to query the current playing state
	const bool buffering_enabled = raw_buffering_enabled && backend->has_capability(AudioBackend::PLAY_PAUSE_FLUSH | AudioBackend::IS_PLAYING);

	const u64 minimum_block_period = audio_block_period / 2; // the block period will not be dynamically lowered below this value (usecs)
	const u64 maximum_block_period = (6 * audio_block_period) / 5; // the block period will not be dynamically increased above this value (usecs)

	const u32 desired_full_buffers = buffering_enabled ? static_cast<u32>(desired_buffer_duration / audio_block_period) + 3 : 2;
	const u32 num_allocated_buffers = desired_full_buffers + EXTRA_AUDIO_BUFFERS; // number of ringbuffer buffers

	const f32 period_average_alpha = 0.02f; // alpha factor for the m_average_period rolling average

	const s64 period_comparison_margin = 250; // when comparing the current period time with the desired period, if it is below this number of usecs we do not wait any longer

	const u64 fully_untouched_timeout = 2 * audio_block_period; // timeout if the game has not touched any audio buffer yet
	const u64 partially_untouched_timeout = 4 * audio_block_period; // timeout if the game has not touched all audio buffers yet

	/*
	 * Time Stretching
	 */
private:
	const bool raw_time_stretching_enabled = buffering_enabled && g_cfg.audio.enable_time_stretching && (g_cfg.audio.time_stretching_threshold > 0);
public:
	// We need to be able to set a dynamic frequency ratio to be able to do time stretching
	const bool time_stretching_enabled = raw_time_stretching_enabled && backend->has_capability(AudioBackend::SET_FREQUENCY_RATIO);

	const f32 time_stretching_threshold = g_cfg.audio.time_stretching_threshold / 100.0f; // we only apply time stretching below this buffer fill rate (adjusted for average period)
	const f32 time_stretching_step = 0.1f; // will only reduce/increase the frequency ratio in steps of at least this value
	const f32 time_stretching_scale = 0.9f;

	/*
	 * Constructor
	 */
	cell_audio_config();
};

class audio_ringbuffer
{
private:
	const std::shared_ptr<AudioBackend> backend;

	const cell_audio_config& cfg;

	const u32 buf_sz;

	std::unique_ptr<AudioDumper> m_dump;

	std::unique_ptr<float[]> buffer[MAX_AUDIO_BUFFERS];
	const float silence_buffer[u32{AUDIO_MAX_CHANNELS_COUNT} * u32{AUDIO_BUFFER_SAMPLES}] = { 0 };

	bool backend_open = false;
	bool playing = false;
	bool emu_paused = false;

	u64 update_timestamp = 0;
	u64 play_timestamp = 0;

	u64 last_remainder = 0;
	u64 enqueued_samples = 0;

	f32 frequency_ratio = 1.0f;

	u32 cur_pos = 0;

	bool get_backend_playing() const
	{
		return has_capability(AudioBackend::PLAY_PAUSE_FLUSH | AudioBackend::IS_PLAYING) ? backend->IsPlaying() : playing;
	}

public:
	audio_ringbuffer(cell_audio_config &cfg);
	~audio_ringbuffer();

	void play();
	void enqueue(const float* in_buffer = nullptr);
	void flush();
	u64 update();
	void enqueue_silence(u32 buf_count = 1);
	f32 set_frequency_ratio(f32 new_ratio);

	float* get_buffer(u32 num) const
	{
		AUDIT(num < cfg.num_allocated_buffers);
		AUDIT(buffer[num].get() != nullptr);
		return buffer[num].get();
	}

	u64 get_timestamp() const
	{
		return get_system_time() - Emu.GetPauseTime();
	}

	float* get_current_buffer() const
	{
		return get_buffer(cur_pos);
	}

	u64 get_enqueued_samples() const
	{
		AUDIT(cfg.buffering_enabled);
		return enqueued_samples;
	}

	u64 get_enqueued_playtime(bool raw = false) const
	{
		AUDIT(cfg.buffering_enabled);
		u64 sampling_rate = raw ? cfg.audio_sampling_rate : static_cast<u64>(cfg.audio_sampling_rate * frequency_ratio);
		return enqueued_samples * 1'000'000 / sampling_rate;
	}

	bool is_playing() const
	{
		return playing;
	}

	f32 get_frequency_ratio() const
	{
		return frequency_ratio;
	}

	u32 has_capability(u32 cap) const
	{
		return backend->has_capability(cap);
	}

	const char* get_backend_name() const
	{
		return backend->GetName();
	}
};


class cell_audio_thread
{
	std::unique_ptr<audio_ringbuffer> ringbuffer;

	void reset_ports(s32 offset = 0);
	void advance(u64 timestamp, bool reset = true);
	std::tuple<u32, u32, u32, u32> count_port_buffer_tags();
	template <bool DownmixToStereo>
	void mix(float *out_buffer, s32 offset = 0);
	void finish_port_volume_stepping();

	constexpr static u64 get_thread_wait_delay(u64 time_left)
	{
		return (time_left > 350) ? time_left - 250 : 100;
	}

public:
	cell_audio_config cfg;

	shared_mutex mutex;
	atomic_t<u32> init = 0;

	u32 key_count = 0;
	u8 event_period = 0;

	struct key_info
	{
		u8 start_period; // Starting event_period
		u32 flags; // iFlags
		u64 source; // Event source
		u64 key; // Key
	};

	std::vector<key_info> keys;
	std::array<audio_port, AUDIO_PORT_COUNT> ports;

	u64 m_last_period_end = 0;
	u64 m_counter = 0;
	u64 m_start_time = 0;
	u64 m_dynamic_period = 0;
	f32 m_average_playtime;

	void operator()();

	cell_audio_thread()
	{
	}

	audio_port* open_port()
	{
		for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
		{
			if (ports[i].state.compare_and_swap_test(audio_port_state::closed, audio_port_state::opened))
			{
				return &ports[i];
			}
		}

		return nullptr;
	}

	bool has_capability(u32 cap) const
	{
		return ringbuffer->has_capability(cap);
	}

	static constexpr auto thread_name = "cellAudio Thread"sv;
};

using cell_audio = named_thread<cell_audio_thread>;
