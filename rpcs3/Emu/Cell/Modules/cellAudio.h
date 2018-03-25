#pragma once

#include "Utilities/Thread.h"



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
	BUFFER_NUM = 32,
	BUFFER_SIZE = 256,
	AUDIO_PORT_COUNT = 8,
	AUDIO_PORT_OFFSET = 256 * 1024,
	AUDIO_SAMPLES = CELL_AUDIO_BLOCK_SAMPLES,
};

extern u64 get_system_time();

enum class audio_port_state : u32
{
	closed,
	opened,
	started,
};

struct audio_port
{
	atomic_t<audio_port_state> state{ audio_port_state::closed };

	u32 number;
	vm::ptr<char> addr{};
	vm::ptr<u64> index{};

	u32 channel;
	u32 block;
	u64 attr;
	u64 tag;
	u64 counter; // copy of global counter
	u32 size;

	struct alignas(8) level_set_t
	{
		float value;
		float inc;
	};

	float level;
	atomic_t<level_set_t> level_set;
};

class audio_config final : public named_thread
{
	void on_task() override;

	std::string get_name() const override { return "Audio Thread"; }

	vm::ptr<char> m_buffer = vm::null;
	vm::ptr<u64> m_indexes = vm::null;

	u64 m_counter{};

public:
	void on_init(const std::shared_ptr<void>&) override;

	const u64 start_time = get_system_time();

	std::array<audio_port, AUDIO_PORT_COUNT> ports;

	std::vector<u64> keys;

	semaphore<> mutex;

	audio_config() = default;

	~audio_config()
	{
		vm::dealloc_verbose_nothrow(m_buffer.addr());
		vm::dealloc_verbose_nothrow(m_indexes.addr());
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
