#pragma once

#include <util/types.hpp>
#include <util/atomic.hpp>
#include <deque>

#include "Emu/RSX/Common/bitfield.hpp"
#include "Emu/RSX/gcm_enums.h"
#include "RSXDisplay.h"

namespace rsx
{
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
	};
}
