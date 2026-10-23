#pragma once

#include <util/types.hpp>
#include <util/atomic.hpp>
#include <deque>

#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/gcm_enums.h"
#include "RSXDisplay.h"

namespace utils
{
	struct serial;
}

namespace rsx
{
	class thread;
	class display_output;
	struct context;

	// Display management (flip, vblank and frame-pacing) state of RSX.
	class display_engine
	{
	public:
		enum class flip_request : u32
		{
			emu_requested = 1,
			native_ui = 2,

			any = emu_requested | native_ui
		};

		struct async_flip_request_t
		{
			atomic_bitmask_t<flip_request> requested{};
			u32 context_id = 0;
			u8 buffer = 0;
		};

	private:
		thread* m_pump = nullptr;
		display_output* m_output = nullptr;

		// Flip requests
		async_flip_request_t m_async_flip_request{};
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

		void handle_emu_flip(u32 buffer);

	public:
		void init(thread* pump, display_output* output);

		// Savestates
		void save(utils::serial& ar);

		// Called when the RSX thread starts, before the vblank thread
		void on_thread_start();

		// Savestates: pause the emulator after this many flips
		void pause_after_flips(u32 count) { m_pause_after_x_flips = count; }

		// Guest flip of the given context. Flips immediately when called on the RSX thread enqueues it otherwise.
		bool request_emu_flip(u32 ctx_id, u32 buffer);

		// RSX thread: runs the guest flip queued by request_emu_flip if requested, once its context is bound
		void handle_pending_flip();

		// The context id a queued guest flip waits for, or umax if there is none
		u32 pending_flip_context() const { return m_async_flip_request.requested.test(flip_request::emu_requested) ? m_async_flip_request.context_id : umax; }

		// Drops a queued guest flip whose context got nuked
		void discard_pending_flip() { m_async_flip_request.requested.clear(flip_request::emu_requested); }

		void request_native_ui_flip() { m_async_flip_request.requested |= flip_request::native_ui; }
		bool has_flip_request(flip_request request) const { return m_async_flip_request.requested & request; }

		// A flip command reached the FIFO. A vsync-held flip still pending sends one more flip notification when it presents.
		void on_flip_queued()
		{
			if (vblank_at_flip != umax)
			{
				flip_notification_count++;
			}
		}

		// Guest-visible flip state (cellGcmSys, capture replay)
		u32 get_flip_status() const { return flip_status; }
		void set_flip_status(u32 status) { flip_status = status; }
		u64 get_flip_index() const { return int_flip_index; }
		u64 get_last_guest_flip_timestamp() const { return last_guest_flip_timestamp; }
		u64 get_last_host_flip_timestamp() const { return last_host_flip_timestamp; }
		u64 get_vblank_count() const { return vblank_count; }

		// Records the finished frame of a context for the next guest flip
		void queue_frame(const context* ctx, u32 buffer);

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
