#pragma once

#include "sys_sync.h"
#include "sys_event.h"
#include "Utilities/simple_ringbuf.h"
#include "Utilities/transactional_storage.h"
#include "Utilities/cond.h"
#include "Emu/system_config_types.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Audio/AudioDumper.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/Audio/audio_resampler.h"

#if defined(unix) || defined(__unix) || defined(__unix__)
// For BSD detection
#include <sys/param.h>
#endif

#ifdef _WIN32
#include <windows.h>
#elif defined(BSD) || defined(__APPLE__)
#include <sys/event.h>
#endif

enum : u32
{
	SYS_RSXAUDIO_SERIAL_STREAM_CNT        = 4,
	SYS_RSXAUDIO_STREAM_DATA_BLK_CNT      = 4,
	SYS_RSXAUDIO_DATA_BLK_SIZE            = 256,
	SYS_RSXAUDIO_STREAM_SIZE              = SYS_RSXAUDIO_DATA_BLK_SIZE * SYS_RSXAUDIO_STREAM_DATA_BLK_CNT,
	SYS_RSXAUDIO_CH_PER_STREAM            = 2,
	SYS_RSXAUDIO_SERIAL_MAX_CH            = 8,
	SYS_RSXAUDIO_SPDIF_MAX_CH             = 2,
	SYS_RSXAUDIO_STREAM_SAMPLE_CNT        = SYS_RSXAUDIO_STREAM_SIZE / SYS_RSXAUDIO_CH_PER_STREAM / sizeof(f32),

	SYS_RSXAUDIO_RINGBUF_BLK_SZ_SERIAL    = SYS_RSXAUDIO_STREAM_SIZE * SYS_RSXAUDIO_SERIAL_STREAM_CNT,
	SYS_RSXAUDIO_RINGBUF_BLK_SZ_SPDIF     = SYS_RSXAUDIO_STREAM_SIZE,

	SYS_RSXAUDIO_RINGBUF_SZ	              = 16,

	SYS_RSXAUDIO_AVPORT_CNT               = 5,

	SYS_RSXAUDIO_FREQ_BASE_384K           = 384000,
	SYS_RSXAUDIO_FREQ_BASE_352K           = 352800,

	SYS_RSXAUDIO_PORT_CNT                 = 3,

	SYS_RSXAUDIO_SPDIF_CNT                = 2,
};

enum class RsxaudioAvportIdx : u8
{
	HDMI_0  = 0,
	HDMI_1  = 1,
	AVMULTI = 2,
	SPDIF_0 = 3,
	SPDIF_1 = 4,
};

enum class RsxaudioPort : u8
{
	SERIAL  = 0,
	SPDIF_0 = 1,
	SPDIF_1 = 2,
	INVALID = 0xFF,
};

enum class RsxaudioSampleSize : u8
{
	_16BIT = 2,
	_32BIT = 4,
};

struct rsxaudio_shmem
{
	struct ringbuf_t
	{
		struct entry_t
		{
			be_t<u32> valid{};
			be_t<u32> unk1{};
			be_t<u64> audio_blk_idx{};
			be_t<u64> timestamp{};
			be_t<u32> buf_addr{};
			be_t<u32> dma_addr{};
		};

		be_t<u32> active{};
		be_t<u32> unk2{};
		be_t<s32> read_idx{};
		be_t<u32> write_idx{};
		be_t<s32> rw_max_idx{};
		be_t<s32> queue_notify_idx{};
		be_t<s32> queue_notify_step{};
		be_t<u32> unk6{};
		be_t<u32> dma_silence_addr{};
		be_t<u32> unk7{};
		be_t<u64> next_blk_idx{};

		entry_t entries[16]{};
	};

	struct uf_event_t
	{
		be_t<u64> unk1{};
		be_t<u32> uf_event_cnt{};
		u8 unk2[244]{};
	};

	struct ctrl_t
	{
		ringbuf_t ringbuf[SYS_RSXAUDIO_PORT_CNT]{};

		be_t<u32> unk1{};
		be_t<u32> event_queue_1_id{};
		u8 unk2[16]{};
		be_t<u32> event_queue_2_id{};
		be_t<u32> spdif_ch0_channel_data_lo{};
		be_t<u32> spdif_ch0_channel_data_hi{};
		be_t<u32> spdif_ch0_channel_data_tx_cycles{};
		be_t<u32> unk3{};
		be_t<u32> event_queue_3_id{};
		be_t<u32> spdif_ch1_channel_data_lo{};
		be_t<u32> spdif_ch1_channel_data_hi{};
		be_t<u32> spdif_ch1_channel_data_tx_cycles{};
		be_t<u32> unk4{};
		be_t<u32> intr_thread_prio{};
		be_t<u32> unk5{};
		u8 unk6[248]{};
		uf_event_t channel_uf[SYS_RSXAUDIO_PORT_CNT]{};
		u8 pad[0x3530]{};
	};

	u8 dma_serial_region[0x10000]{};
	u8 dma_spdif_0_region[0x4000]{};
	u8 dma_spdif_1_region[0x4000]{};
	u8 dma_silence_region[0x4000]{};
	ctrl_t ctrl{};
};

static_assert(sizeof(rsxaudio_shmem::ringbuf_t) == 0x230U, "rsxAudioRingBufSizeTest");
static_assert(sizeof(rsxaudio_shmem::uf_event_t) == 0x100U, "rsxAudioUfEventTest");
static_assert(sizeof(rsxaudio_shmem::ctrl_t) == 0x4000U, "rsxAudioCtrlSizeTest");
static_assert(sizeof(rsxaudio_shmem) == 0x20000U, "rsxAudioShmemSizeTest");

enum rsxaudio_dma_flag : u32
{
	IO_BASE = 0,
	IO_ID = 1
};

struct lv2_rsxaudio final : lv2_obj
{
	static constexpr u32 id_base   = 0x60000000;
	static constexpr u64 dma_io_id = 1;
	static constexpr u32 dma_io_base = 0x30000000;

	shared_mutex mutex{};
	bool init = false;

	vm::addr_t shmem{};

	std::array<shared_ptr<lv2_event_queue>, SYS_RSXAUDIO_PORT_CNT> event_queue{};

	// lv2 uses port memory addresses for their names
	static constexpr std::array<u64, SYS_RSXAUDIO_PORT_CNT> event_port_name{ 0x8000000000400100, 0x8000000000400200, 0x8000000000400300 };

	lv2_rsxaudio() noexcept = default;
	lv2_rsxaudio(utils::serial& ar) noexcept;	void save(utils::serial& ar);

	void page_lock()
	{
		ensure(shmem && vm::page_protect(shmem, sizeof(rsxaudio_shmem), 0, 0, vm::page_readable | vm::page_writable | vm::page_executable));
	}

	void page_unlock()
	{
		ensure(shmem && vm::page_protect(shmem, sizeof(rsxaudio_shmem), 0, vm::page_readable | vm::page_writable));
	}

	rsxaudio_shmem* get_rw_shared_page() const
	{
		return reinterpret_cast<rsxaudio_shmem*>(vm::g_sudo_addr + u32{shmem});
	}
};

class rsxaudio_periodic_tmr
{
public:

	enum class wait_result
	{
		SUCCESS,
		INVALID_PARAM,
		TIMEOUT,
		TIMER_ERROR,
		TIMER_CANCELED,
	};

	rsxaudio_periodic_tmr();
	~rsxaudio_periodic_tmr();

	rsxaudio_periodic_tmr(const rsxaudio_periodic_tmr&) = delete;
	rsxaudio_periodic_tmr& operator=(const rsxaudio_periodic_tmr&) = delete;

	// Wait until timer fires and calls callback.
	wait_result wait(const std::function<void()> &callback);

	// Cancel wait() call
	void cancel_wait();

	// VTimer funtions

	void vtimer_access_sec(std::invocable<> auto func)
	{
		std::lock_guard lock(mutex);
		std::invoke(func);

		// Adjust timer expiration
		cancel_timer_unlocked();
		sched_timer();
	}

	void enable_vtimer(u32 vtimer_id, u32 rate, u64 crnt_time);

	void disable_vtimer(u32 vtimer_id);

	bool is_vtimer_behind(u32 vtimer_id, u64 crnt_time) const;

	void vtimer_skip_periods(u32 vtimer_id, u64 crnt_time);

	void vtimer_incr(u32 vtimer_id, u64 crnt_time);

	bool is_vtimer_active(u32 vtimer_id) const;

	u64 vtimer_get_sched_time(u32 vtimer_id) const;

private:

	static constexpr u64 MAX_BURST_PERIODS = SYS_RSXAUDIO_RINGBUF_SZ;
	static constexpr u32 VTIMER_MAX = 4;

	struct vtimer
	{
		u64 blk_cnt = 0;
		f64 blk_time = 0.0;
		bool active = false;
	};

	std::array<vtimer, VTIMER_MAX> vtmr_pool{};

	shared_mutex mutex{};
	bool in_wait = false;
	bool zero_period = false;

#if defined(_WIN32)
	HANDLE cancel_event{};
	HANDLE timer_handle{};
#elif defined(__linux__)
	int cancel_event{};
	int timer_handle{};
	int epoll_fd{};
#elif defined(BSD) || defined(__APPLE__)
	static constexpr u64 TIMER_ID = 0;
	static constexpr u64 CANCEL_ID = 1;

	int kq{};
	struct kevent handle[2]{};
#else
#error "Implement"
#endif

	void sched_timer();
	void cancel_timer_unlocked();
	void reset_cancel_flag();

	bool is_vtimer_behind(const vtimer& vtimer, u64 crnt_time) const;

	u64 get_crnt_blk(u64 crnt_time, f64 blk_time) const;
	f64 get_blk_time(u32 data_rate) const;

	u64 get_rel_next_time();
};

struct rsxaudio_hw_param_t
{
	struct serial_param_t
	{
		bool dma_en = false;
		bool buf_empty_en = false;
		bool muted = true;
		bool en = false;
		u8   freq_div = 8;
		RsxaudioSampleSize depth = RsxaudioSampleSize::_16BIT;
	};

	struct spdif_param_t
	{
		bool dma_en = false;
		bool buf_empty_en = false;
		bool muted = true;
		bool en = false;
		bool use_serial_buf = true;
		u8   freq_div = 8;
		RsxaudioSampleSize depth = RsxaudioSampleSize::_16BIT;
		std::array<u8, 6> cs_data = { 0x00, 0x90, 0x00, 0x40, 0x80, 0x00 }; // HW supports only 6 bytes (uart pkt has 8)
	};

	struct hdmi_param_t
	{
		struct hdmi_ch_cfg_t
		{
			std::array<u8, SYS_RSXAUDIO_SERIAL_MAX_CH> map{};
			AudioChannelCnt total_ch_cnt = AudioChannelCnt::STEREO;
		};

		static constexpr u8 MAP_SILENT_CH = umax;

		bool                init = false;
		hdmi_ch_cfg_t       ch_cfg{};
		std::array<u8, 5>   info_frame{}; // TODO: check chstat and info_frame for info on audio layout, add default values
		std::array<u8, 5>   chstat{};

		bool                muted = true;
		bool                force_mute = true;
		bool                use_spdif_1 = false; // TODO: unused for now
	};

	u32 serial_freq_base = SYS_RSXAUDIO_FREQ_BASE_384K;
	u32 spdif_freq_base = SYS_RSXAUDIO_FREQ_BASE_352K;

	bool avmulti_av_muted = true;

	serial_param_t serial{};
	spdif_param_t spdif[2]{};
	hdmi_param_t hdmi[2]{};

	std::array<RsxaudioPort, SYS_RSXAUDIO_AVPORT_CNT> avport_src =
	{
		RsxaudioPort::INVALID,
		RsxaudioPort::INVALID,
		RsxaudioPort::INVALID,
		RsxaudioPort::INVALID,
		RsxaudioPort::INVALID
	};
};

// 16-bit PCM converted into float, so buffer must be twice as big
using ra_stream_blk_t = std::array<f32, SYS_RSXAUDIO_STREAM_SAMPLE_CNT * 2>;

class rsxaudio_data_container
{
public:

	struct buf_t
	{
		std::array<ra_stream_blk_t, SYS_RSXAUDIO_SERIAL_MAX_CH> serial{};
		std::array<ra_stream_blk_t, SYS_RSXAUDIO_SPDIF_MAX_CH> spdif[SYS_RSXAUDIO_SPDIF_CNT]{};
	};

	using data_blk_t = std::array<f32, SYS_RSXAUDIO_STREAM_SAMPLE_CNT * SYS_RSXAUDIO_SERIAL_MAX_CH * 2>;

	rsxaudio_data_container(const rsxaudio_hw_param_t& hw_param, const buf_t& buf, bool serial_rdy, bool spdif_0_rdy, bool spdif_1_rdy);
	u32 get_data_size(RsxaudioAvportIdx avport);
	void get_data(RsxaudioAvportIdx avport, data_blk_t& data_out);
	bool data_was_used();

private:

	const rsxaudio_hw_param_t& hwp;
	const buf_t& out_buf;

	std::array<bool, 5> avport_data_avail{};
	u8 hdmi_stream_cnt[2]{};
	bool data_was_written = false;

	rsxaudio_data_container(const rsxaudio_data_container&) = delete;
	rsxaudio_data_container& operator=(const rsxaudio_data_container&) = delete;

	rsxaudio_data_container(rsxaudio_data_container&&) = delete;
	rsxaudio_data_container& operator=(rsxaudio_data_container&&) = delete;

	// Mix individual channels into final PCM stream. Channels in channel map that are > input_ch_cnt treated as silent.
	template<usz output_ch_cnt, usz input_ch_cnt>
	requires (output_ch_cnt > 0 && output_ch_cnt <= 8 && input_ch_cnt > 0)
	constexpr void mix(const std::array<u8, 8> &ch_map, RsxaudioSampleSize sample_size, const std::array<ra_stream_blk_t, input_ch_cnt> &input_channels, data_blk_t& data_out)
	{
		const ra_stream_blk_t silent_channel{};

		// Build final map
		std::array<const ra_stream_blk_t*, output_ch_cnt> real_input_ch = {};
		for (u64 ch_idx = 0; ch_idx < output_ch_cnt; ch_idx++)
		{
			if (ch_map[ch_idx] >= input_ch_cnt)
			{
				real_input_ch[ch_idx] = &silent_channel;
			}
			else
			{
				real_input_ch[ch_idx] = &input_channels[ch_map[ch_idx]];
			}
		}

		const u32 samples_in_buf = sample_size == RsxaudioSampleSize::_16BIT ? SYS_RSXAUDIO_STREAM_SAMPLE_CNT * 2 : SYS_RSXAUDIO_STREAM_SAMPLE_CNT;

		for (u32 sample_idx = 0; sample_idx < samples_in_buf * output_ch_cnt; sample_idx += output_ch_cnt)
		{
			const u32 src_sample_idx = sample_idx / output_ch_cnt;

			if constexpr (output_ch_cnt >= 1) data_out[sample_idx + 0] = (*real_input_ch[0])[src_sample_idx];
			if constexpr (output_ch_cnt >= 2) data_out[sample_idx + 1] = (*real_input_ch[1])[src_sample_idx];
			if constexpr (output_ch_cnt >= 3) data_out[sample_idx + 2] = (*real_input_ch[2])[src_sample_idx];
			if constexpr (output_ch_cnt >= 4) data_out[sample_idx + 3] = (*real_input_ch[3])[src_sample_idx];
			if constexpr (output_ch_cnt >= 5) data_out[sample_idx + 4] = (*real_input_ch[4])[src_sample_idx];
			if constexpr (output_ch_cnt >= 6) data_out[sample_idx + 5] = (*real_input_ch[5])[src_sample_idx];
			if constexpr (output_ch_cnt >= 7) data_out[sample_idx + 6] = (*real_input_ch[6])[src_sample_idx];
			if constexpr (output_ch_cnt >= 8) data_out[sample_idx + 7] = (*real_input_ch[7])[src_sample_idx];
		}
	}
};

namespace audio
{
	void configure_rsxaudio();
}

class rsxaudio_backend_thread
{
public:

	struct port_config
	{
		AudioFreq freq = AudioFreq::FREQ_48K;
		AudioChannelCnt ch_cnt = AudioChannelCnt::STEREO;

		auto operator<=>(const port_config&) const = default;
	};

	struct avport_bit
	{
		bool hdmi_0  : 1;
		bool hdmi_1  : 1;
		bool avmulti : 1;
		bool spdif_0 : 1;
		bool spdif_1 : 1;
	};

	rsxaudio_backend_thread();
	~rsxaudio_backend_thread();

	void operator()();
	rsxaudio_backend_thread& operator=(thread_state state);

	void set_new_stream_param(const std::array<port_config, SYS_RSXAUDIO_AVPORT_CNT> &cfg, avport_bit muted_avports);
	void set_mute_state(avport_bit muted_avports);
	void add_data(rsxaudio_data_container& cont);

	void update_emu_cfg();

	u32 get_sample_rate() const;
	u8 get_channel_count() const;

	static constexpr auto thread_name = "RsxAudio Backend Thread"sv;

	SAVESTATE_INIT_POS(8.91); // Depends on audio_out_configuration

private:

	struct emu_audio_cfg
	{
		std::string audio_device{};
		s64 desired_buffer_duration = 0;
		f64 time_stretching_threshold = 0;
		bool buffering_enabled = false;
		bool convert_to_s16 = false;
		bool enable_time_stretching = false;
		bool dump_to_file = false;
		AudioChannelCnt channels = AudioChannelCnt::STEREO;
		audio_channel_layout channel_layout = audio_channel_layout::automatic;
		audio_renderer renderer = audio_renderer::null;
		audio_provider provider = audio_provider::none;
		RsxaudioAvportIdx avport = RsxaudioAvportIdx::HDMI_0;

		auto operator<=>(const emu_audio_cfg&) const = default;
	};

	struct rsxaudio_state
	{
		std::array<port_config, SYS_RSXAUDIO_AVPORT_CNT> port{};
	};

	struct alignas(16) callback_config
	{
		static constexpr u16 VOL_NOMINAL = 10000;
		static constexpr f32 VOL_NOMINAL_INV = 1.0f / VOL_NOMINAL;

		u32 freq : 20            = 48000;

		u16 target_volume        = 10000;
		u16 initial_volume       = 10000;
		u16 current_volume       = 10000;

		RsxaudioAvportIdx avport_idx = RsxaudioAvportIdx::HDMI_0;
		u8 mute_state : SYS_RSXAUDIO_AVPORT_CNT = 0b11111;

		u8 input_ch_cnt          : 4 = 2;
		u8 output_channel_layout : 4 = static_cast<u8>(audio_channel_layout::stereo);

		bool ready : 1           = false;
		bool convert_to_s16 : 1  = false;
		bool cfg_changed : 1     = false;
		bool callback_active : 1 = false;
	};

	static_assert(sizeof(callback_config) <= 16);

	struct backend_config
	{
		port_config cfg{};
		RsxaudioAvportIdx avport = RsxaudioAvportIdx::HDMI_0;
	};

	static constexpr u64 ERROR_SERVICE_PERIOD = 500'000;
	static constexpr u64 SERVICE_PERIOD = 10'000;
	static constexpr f64 SERVICE_PERIOD_SEC = SERVICE_PERIOD / 1'000'000.0;
	static constexpr u64 SERVICE_THRESHOLD = 1'500;

	static constexpr f64 TIME_STRETCHING_STEP = 0.1f;

	u64 start_time = get_system_time();
	u64 time_period_idx = 1;

	emu_audio_cfg new_emu_cfg{};
	bool emu_cfg_changed = true;

	rsxaudio_state new_ra_state{};
	bool ra_state_changed = true;

	shared_mutex  state_update_m{};
	cond_variable state_update_c{};

	simple_ringbuf ringbuf{};
	simple_ringbuf aux_ringbuf{};
	std::vector<u8> thread_tmp_buf{};
	std::vector<f32> callback_tmp_buf{};
	bool use_aux_ringbuf = false;
	shared_mutex ringbuf_mutex{};

	std::shared_ptr<AudioBackend> backend{};
	backend_config backend_current_cfg{ {}, new_emu_cfg.avport };
	atomic_t<callback_config> callback_cfg{};
	bool backend_error_occured = false;
	bool backend_device_changed = false;

	AudioDumper dumper{};
	audio_resampler resampler{};

	// Backend
	void backend_init(const rsxaudio_state& ra_state, const emu_audio_cfg& emu_cfg, bool reset_backend = true);
	void backend_start();
	void backend_stop();
	bool backend_playing();
	u32 write_data_callback(u32 bytes, void* buf);
	void state_changed_callback(AudioStateEvent event);

	// Time management
	u64 get_time_until_service();
	void update_service_time();
	void reset_service_time();

	// Helpers
	static emu_audio_cfg get_emu_cfg();
	static u8 gen_mute_state(avport_bit avports);
	static RsxaudioAvportIdx convert_avport(audio_avport avport);
};

class rsxaudio_data_thread
{
public:

	// Prevent creation of multiple rsxaudio contexts
	atomic_t<bool> rsxaudio_ctx_allocated = false;

	shared_mutex rsxaudio_obj_upd_m{};
	shared_ptr<lv2_rsxaudio> rsxaudio_obj_ptr{};

	void operator()();
	rsxaudio_data_thread& operator=(thread_state state);

	rsxaudio_data_thread();

	void update_hw_param(std::function<void(rsxaudio_hw_param_t&)> update_callback);
	void update_mute_state(RsxaudioPort port, bool muted);
	void update_av_mute_state(RsxaudioAvportIdx avport, bool muted, bool force_mute, bool set = true);
	void reset_hw();

	static constexpr auto thread_name = "RsxAudioData Thread"sv;

private:

	rsxaudio_data_container::buf_t output_buf{};

	transactional_storage<rsxaudio_hw_param_t> hw_param_ts{std::make_shared<universal_pool>(), std::make_shared<rsxaudio_hw_param_t>()};
	rsxaudio_periodic_tmr timer{};

	void advance_all_timers();
	void extract_audio_data();
	static std::pair<bool /*data_present*/, void* /*addr*/> get_ringbuf_addr(RsxaudioPort dst, const lv2_rsxaudio& rsxaudio_obj);

	static f32 pcm_to_float(s32 sample);
	static f32 pcm_to_float(s16 sample);
	static void pcm_serial_process_channel(RsxaudioSampleSize word_bits, ra_stream_blk_t& buf_out_l, ra_stream_blk_t& buf_out_r, const void* buf_in, u8 src_stream);
	static void pcm_spdif_process_channel(RsxaudioSampleSize word_bits, ra_stream_blk_t& buf_out_l, ra_stream_blk_t& buf_out_r, const void* buf_in);
	bool enqueue_data(RsxaudioPort dst, bool silence, const void* src_addr, const rsxaudio_hw_param_t& hwp);

	static rsxaudio_backend_thread::avport_bit calc_avport_mute_state(const rsxaudio_hw_param_t& hwp);
	static bool calc_port_active_state(RsxaudioPort port, const rsxaudio_hw_param_t& hwp);
};

using rsx_audio_backend = named_thread<rsxaudio_backend_thread>;
using rsx_audio_data = named_thread<rsxaudio_data_thread>;

// SysCalls

error_code sys_rsxaudio_initialize(vm::ptr<u32> handle);
error_code sys_rsxaudio_finalize(u32 handle);
error_code sys_rsxaudio_import_shared_memory(u32 handle, vm::ptr<u64> addr);
error_code sys_rsxaudio_unimport_shared_memory(u32 handle, vm::ptr<u64> addr);
error_code sys_rsxaudio_create_connection(u32 handle);
error_code sys_rsxaudio_close_connection(u32 handle);
error_code sys_rsxaudio_prepare_process(u32 handle);
error_code sys_rsxaudio_start_process(u32 handle);
error_code sys_rsxaudio_stop_process(u32 handle);
error_code sys_rsxaudio_get_dma_param(u32 handle, u32 flag, vm::ptr<u64> out);
