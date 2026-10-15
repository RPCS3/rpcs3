#pragma once

#include <util/types.hpp>
#include <util/atomic.hpp>
#include <deque>

#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/gcm_enums.h"
#include "RSXDisplay.h"

namespace rsx
{
	class thread;
	class display_output;

	// Display management (flip, vblank and frame-pacing) state of RSX.
	class display_engine
	{
		thread* m_pump = nullptr;
		display_output* m_output = nullptr;

	public:
		enum class flip_request : u32
		{
			emu_requested = 1,
			native_ui = 2,

			any = emu_requested | native_ui
		};

		// Flip requests
		atomic_bitmask_t<flip_request> async_flip_requested{};
		u8 async_flip_buffer{ 0 };
		display_flip_info_t m_queued_flip{};

		// Guest-visible flip state
		u32 flip_status = CELL_GCM_DISPLAY_FLIP_STATUS_DONE;
		u64 int_flip_index = 0;
		u64 last_guest_flip_timestamp = 0;
		u64 last_host_flip_timestamp = 0;
		u64 target_rsx_flip_time = 0;

		// Vblank
		atomic_t<u64> vblank_count{0};
		u64 vblank_at_flip = umax;
		u64 flip_notification_count = 0;

		// Host display refresh rate cache
		atomic_t<u64> m_display_rate_fetch_count = 0;
		atomic_t<f64> m_cached_display_rate = 60.;

		// Frame pacing
		std::deque<frame_time_t> frame_times;
		u32 prevent_preempt_increase_tickets = 0;
		u64 preempt_fail_old_preempt_count = 0;

		// Savestates: pause after this many flips once loaded
		u32 m_pause_after_x_flips = 0;

		void init(thread* pump, display_output* output);

		// Guest flip. Flips immediately when called on the RSX thread (command buffer), otherwise queues it for the RSX thread (lv2 syscall).
		bool request_emu_flip(u32 buffer);
		void handle_emu_flip(u32 buffer);

		// Records the finished frame (buffer, stats, frame-skip flag) for the next guest flip
		void queue_frame(u32 buffer);

		// Presents a frame through the renderer. Guest flips and the native UI both end here.
		void flip(const display_flip_info_t& info);

		// Vblank.
		// NOTE: The vblank thread posts post_vblank_event at the configured vblank rate.
		void start_vblank_thread();
		void stop_vblank_thread();
		void post_vblank_event(u64 post_event_time);

		// Host display refresh rate
		f64 cached_refresh_rate();

		// Power saving stuff. Called when a guest flip is presented.
		void evaluate_cpu_usage_reduction_limits();
	};
}
