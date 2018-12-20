#pragma once

#include "Utilities/Thread.h"
#include "Emu/Memory/vm.h"
#include "Emu/Audio/AudioThread.h"
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

	PORT_BUFFER_TAG_COUNT = 4,

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
	f32 tag_backup[AUDIO_MAX_BLOCK_COUNT][PORT_BUFFER_TAG_COUNT] = { 0 };

	constexpr static bool is_tag(float val);
	void tag(s32 offset = 0);
	void apply_tag_backups(s32 offset = 0);
};

class audio_ringbuffer
{
private:
	const std::shared_ptr<AudioThread> backend;

	const u32 num_allocated_buffers;
	const u32 buf_sz;
	const u32 audio_sampling_rate;
	const u32 channels;

	std::unique_ptr<AudioDumper> m_dump;

	std::unique_ptr<float[]> buffer[MAX_AUDIO_BUFFERS];
	const float silence_buffer[8 * AUDIO_BUFFER_SAMPLES] = { 0 };

	bool backend_open = false;
	bool playing = false;
	bool emu_paused = false;

	u64 update_timestamp = 0;
	u64 play_timestamp = 0;

	u64 last_remainder = 0;
	u64 enqueued_samples = 0;

	u32 next_buf = 0;

public:
	audio_ringbuffer(u32 num_buffers, u32 audio_sampling_rate, u32 channels);
	~audio_ringbuffer();

	void play();
	void enqueue(const float* in_buffer = nullptr);
	void flush();
	u64 update();
	void enqueue_silence(u32 buf_count = 1);

	float* get_buffer(u32 num) const
	{
		AUDIT(num < num_allocated_buffers);
		AUDIT(buffer[num].get() != nullptr);
		return buffer[num].get();
	}

	u32 get_buf_sz() const
	{
		return buf_sz;
	}

	u64 get_timestamp() const
	{
		return get_system_time() - Emu.GetPauseTime();
	}

	float* get_current_buffer() const
	{
		return get_buffer(next_buf);
	}

	u64 get_enqueued_samples() const
	{
		return enqueued_samples;
	}

	bool is_playing() const
	{
		return playing;
	}
};


class cell_audio_thread
{
	vm::ptr<char> m_buffer;
	vm::ptr<u64> m_indexes;

	std::unique_ptr<audio_ringbuffer> ringbuffer;

	void reset_ports(s32 offset = 0);
	void advance(u64 timestamp, bool reset = true);
	std::tuple<u32, u32, u32, u32> count_port_buffer_tags();
	template<bool downmix_to_2ch> void mix(float *out_buffer, s32 offset = 0);
	void finish_port_volume_stepping();

	constexpr static u64 get_thread_wait_delay(u64 time_left)
	{
		return (time_left > 1000) ? time_left - 750 : 100;
	}

public:
	const s64 period_comparison_margin = 100; // When comparing the current period time with the desired period, if it is below this number of usecs we do not wait any longer

	const u32 audio_channels = AudioThread::get_channels();
	const u32 audio_sampling_rate = AudioThread::get_sampling_rate();
	const u64 audio_block_period = AUDIO_BUFFER_SAMPLES * 1000000 / audio_sampling_rate;
	const u64 desired_buffer_duration = g_cfg.audio.enable_buffering ? g_cfg.audio.desired_buffer_duration : 0;
	const bool buffering_enabled = g_cfg.audio.enable_buffering && (desired_buffer_duration >= audio_block_period);

	const u64 minimum_block_period = audio_block_period / 2; // the block period will not be dynamically lowered below this value (usecs)
	const u64 maximum_block_period = audio_block_period + (audio_block_period - minimum_block_period); // the block period will not be dynamically increased above this value (usecs)

	const u32 desired_full_buffers = buffering_enabled ? static_cast<u32>(desired_buffer_duration / audio_block_period) + 1 : 1;
	const u32 num_allocated_buffers = desired_full_buffers + EXTRA_AUDIO_BUFFERS; // number of ringbuffer buffers

	std::vector<u64> keys;
	std::array<audio_port, AUDIO_PORT_COUNT> ports;

	u64 m_last_period_end = 0;
	u64 m_counter = 0;
	u64 m_start_time = 0;
	u64 m_dynamic_period = 0;

	void operator()();

	cell_audio_thread(vm::ptr<char> buf, vm::ptr<u64> ind)
		: m_buffer(buf)
		, m_indexes(ind)
	{
		for (u32 i = 0; i < AUDIO_PORT_COUNT; i++)
		{
			ports[i].number = i;
			ports[i].addr   = m_buffer + AUDIO_PORT_OFFSET * i;
			ports[i].index  = m_indexes + i;
		}
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
};

using cell_audio = named_thread<cell_audio_thread>;
