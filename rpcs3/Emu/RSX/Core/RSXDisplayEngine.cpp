#include "stdafx.h"
#include "RSXDisplayEngine.h"
#include "RSXDisplayOutput.h"

#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_time.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

#include "util/asm.hpp"

extern atomic_t<u32> g_lv2_preempts_taken;

LOG_CHANNEL(perf_log, "PERF");

namespace rsx
{
	void display_engine::init(thread* pump, display_output* output)
	{
		m_pump = pump;
		m_output = output;
	}

	void display_engine::post_vblank_event(u64 post_event_time)
	{
		vblank_count++;

		if (m_pump->isHLE)
		{
			if (auto ptr = m_pump->vblank_handler)
			{
				m_pump->intr_thread->cmd_list
				({
					{ ppu_cmd::set_args, 1 }, u64{1},
					{ ppu_cmd::lle_call, ptr },
					{ ppu_cmd::sleep, 0 }
				});

				m_pump->intr_thread->cmd_notify.store(1);
				m_pump->intr_thread->cmd_notify.notify_one();
			}
		}
		else
		{
			sys_rsx_context_attribute(0x55555555, 0xFED, 1, get_guest_system_time(post_event_time), 0, 0);
		}
	}

	void display_engine::start_vblank_thread()
	{
		g_fxo->get<vblank_thread>().set_thread(std::shared_ptr<named_thread<std::function<void()>>>(new named_thread<std::function<void()>>("VBlank Thread"sv, [this]() -> void
		{
#ifdef __linux__
			constexpr u32 host_min_quantum = 10;
#else
			constexpr u32 host_min_quantum = 500;
#endif
			u64 start_time = get_system_time();

			u64 vblank_rate = g_cfg.video.vblank_rate;
			u64 vblank_period = 1'000'000 + u64{g_cfg.video.vblank_ntsc.get()} * 1000;

			u64 local_vblank_count = 0;

			// TODO: exit condition
			while (!m_pump->is_stopped() && !m_pump->unsent_gcm_events && thread_ctrl::state() != thread_state::aborting)
			{
				// Get current time
				const u64 current = get_system_time();

				// Calculate the time at which we need to send a new VBLANK signal
				const u64 post_event_time = start_time + (local_vblank_count + 1) * vblank_period / vblank_rate;

				// Calculate time remaining to that time (0 if we passed it)
				const u64 wait_for = current >= post_event_time ? 0 : post_event_time - current;

#ifdef __linux__
				const u64 wait_sleep = wait_for;
#else
				// Substract host operating system min sleep quantom to get sleep time
				const u64 wait_sleep = wait_for - u64{wait_for >= host_min_quantum} * host_min_quantum;
#endif

				if (!wait_for)
				{
					{
						local_vblank_count++;

						if (local_vblank_count == vblank_rate)
						{
							// Advance start_time to the moment of the current VBLANK
							// Which is the last VBLANK event in this period
							// This is in order for multiplication by ratio above to use only small numbers
							start_time += vblank_period;
							local_vblank_count = 0;

							// We have a rare chance to update settings without losing precision whenever local_vblank_count is 0
							vblank_rate = g_cfg.video.vblank_rate;
							vblank_period = 1'000'000 + u64{g_cfg.video.vblank_ntsc.get()} * 1000;
						}

						post_vblank_event(post_event_time);
					}
				}
				else if (wait_sleep)
				{
					thread_ctrl::wait_for(wait_sleep);
				}
				else if (wait_for >= host_min_quantum / 3 * 2)
				{
					std::this_thread::yield();
				}

				if (Emu.IsPaused())
				{
					// Save the difference before pause
					start_time = get_system_time() - start_time;

					while (Emu.IsPaused() && !m_pump->is_stopped())
					{
						thread_ctrl::wait_for(5'000);
					}

					// Restore difference
					start_time = get_system_time() - start_time;
				}
			}
		})));
	}

	void display_engine::stop_vblank_thread()
	{
		g_fxo->get<vblank_thread>() = thread_state::finished;
	}

	f64 display_engine::cached_refresh_rate()
	{
		constexpr u64 uses_per_query = 512;

		f64 result = m_cached_display_rate;
		u64 count = m_display_rate_fetch_count++;

		while (true)
		{
			if (count % 512 == 0)
			{
				result = m_output->refresh_rate();
				m_cached_display_rate.store(result);
				m_display_rate_fetch_count += uses_per_query; // Notify users of the new value
				break;
			}

			const u64 new_count = m_display_rate_fetch_count;
			const f64 new_cached = m_cached_display_rate;

			if (result == new_cached && count / uses_per_query == new_count / uses_per_query)
			{
				break;
			}

			// An update might have gone through
			count = new_count;
			result = new_cached;
		}

		return result;
	}

	void display_engine::evaluate_cpu_usage_reduction_limits()
	{
		const u64 max_preempt_count = g_cfg.core.max_cpu_preempt_count_per_frame;

		if (!max_preempt_count)
		{
			frame_times.clear();
			lv2_obj::set_yield_frequency(0, 0);
			return;
		}

		const u64 current_time = get_system_time();
		const u64 current_tsc = utils::get_tsc();
		u64 preempt_count = 0;

		if (frame_times.size() >= 60)
		{
			u64 diffs = 0;

			for (usz i = 1; i < frame_times.size(); i++)
			{
				const u64 cur_diff = frame_times[i].timestamp - frame_times[i - 1].timestamp;
				diffs += cur_diff;
			}

			const usz avg_frame_time = diffs / 59;

			u32 lowered_delay = 0;
			u32 raised_delay = 0;
			bool can_reevaluate = true;
			u64 prev_preempt_count = umax;

			for (usz i = frame_times.size() - 30; i < frame_times.size(); i++)
			{
				if (prev_preempt_count == umax)
				{
					prev_preempt_count = frame_times[i].preempt_count;
					continue;
				}

				if (prev_preempt_count != frame_times[i].preempt_count)
				{
					if (prev_preempt_count > frame_times[i].preempt_count)
					{
						lowered_delay++;
					}
					else if (prev_preempt_count < frame_times[i].preempt_count)
					{
						raised_delay++;
					}

					if (i > frame_times.size() - 30)
					{
						// Slow preemption count increase
						can_reevaluate = false;
					}
				}

				prev_preempt_count = frame_times[i].preempt_count;
			}

			preempt_count = std::min<u64>(frame_times.back().preempt_count, max_preempt_count);

			u32 fails = 0;
			u32 hard_fails = 0;
			bool is_last_frame_a_fail = false;

			auto abs_dst = [](u64 a, u64 b)
			{
				return a >= b ? a - b : b - a;
			};

			for (u32 i = 1; i <= frame_times.size(); i++)
			{
				const u64 cur_diff = (i == frame_times.size() ? current_time : frame_times[i].timestamp) - frame_times[i - 1].timestamp;

				if (const u64 diff_of_diff = abs_dst(cur_diff, avg_frame_time);
					diff_of_diff >= avg_frame_time / 7)
				{
					if (diff_of_diff >= avg_frame_time / 3)
					{
						raised_delay++;
						hard_fails++;

						if (i == frame_times.size())
						{
							is_last_frame_a_fail = true;
						}
					}

					if (fails != umax)
					{
						fails++;
					}
				}
			}

			bool hard_measures_taken = false;
			const usz fps_10 = 10'000'000 / avg_frame_time;

			auto lower_preemption_count = [&]()
			{
				if (preempt_count >= 10)
				{
					preempt_count -= 10;
				}
				else
				{
					preempt_count = 0;
				}

				if ((hard_fails > 2 || fails > 20) && is_last_frame_a_fail)
				{
					hard_measures_taken = preempt_count > 1;
					preempt_count = preempt_count * 7 / 8;
					prevent_preempt_increase_tickets = 10;
				}
				else
				{
					prevent_preempt_increase_tickets = std::max<u32>(7, prevent_preempt_increase_tickets);
				}
			};

			const u64 vblank_rate_10 = g_cfg.video.vblank_rate * 10;

			if (can_reevaluate)
			{
				const bool is_avg_fps_ok = (abs_dst(fps_10, 300) < 3 || abs_dst(fps_10, 600) < 4 || abs_dst(fps_10, vblank_rate_10) < 4 || abs_dst(fps_10, vblank_rate_10 / 2) < 3);

				if (!hard_fails && fails < 6 && is_avg_fps_ok)
				{
					if (prevent_preempt_increase_tickets)
					{
						prevent_preempt_increase_tickets--;
					}
					else
					{
						preempt_count = std::min<u64>(preempt_count + 4, max_preempt_count);
					}
				}
				else
				{
					lower_preemption_count();
				}
			}
			// Sudden FPS drop detection
			else if ((fails > 13 || hard_fails > 2 || !(abs_dst(fps_10, 300) < 20 || abs_dst(fps_10, 600) < 30 || abs_dst(fps_10, g_cfg.video.vblank_rate * 10) < 30 || abs_dst(fps_10, g_cfg.video.vblank_rate * 10 / 2) < 20)) && lowered_delay < raised_delay && is_last_frame_a_fail)
			{
				lower_preemption_count();
			}

			perf_log.trace("CPU preemption control: reeval=%d, preempt_count=%llu, fails=%u, hard=%u, avg_frame_time=%llu, highered=%u, lowered=%u, taken=%u", can_reevaluate, preempt_count, fails, hard_fails, avg_frame_time, raised_delay, lowered_delay, ::g_lv2_preempts_taken.load());

			if (hard_measures_taken)
			{
				preempt_fail_old_preempt_count = std::max<u64>(preempt_fail_old_preempt_count, std::min<u64>(frame_times.back().preempt_count, max_preempt_count));
			}
			else if (preempt_fail_old_preempt_count)
			{
				perf_log.error("Lowering current preemption count significantly due to a performance drop, if this issue persists frequently consider lowering max preemptions count to 'new-count' or lower. (old-count=%llu, new-count=%llu)", preempt_fail_old_preempt_count, preempt_count);
				preempt_fail_old_preempt_count = 0;
			}

			const u64 tsc_diff = (current_tsc - frame_times.back().tsc);
			const u64 time_diff = (current_time - frame_times.back().timestamp);
			const u64 preempt_diff = tsc_diff * (1'000'000 / 30) / (time_diff * std::max<u64>(preempt_count, 1ull));

			if (!preempt_count)
			{
				lv2_obj::set_yield_frequency(0, 0);
			}
			else if (abs_dst(fps_10, 300) < 30)
			{
				// Set an upper limit so a backoff technique would be taken if there is a sudden performance drop
				// Allow 4% of no yield to reduce significantly the risk of stutter
				lv2_obj::set_yield_frequency(preempt_diff, current_tsc + (tsc_diff * (1'000'000 * 96 / (30 * 100)) / time_diff));
			}
			else if (abs_dst(fps_10, 600) < 40)
			{
				// 5% for 60fps
				lv2_obj::set_yield_frequency(preempt_diff, current_tsc + (tsc_diff * (1'000'000 * 94 / (60 * 100)) / time_diff));
			}
			else if (abs_dst(fps_10, vblank_rate_10) < 40)
			{
				lv2_obj::set_yield_frequency(preempt_diff, current_tsc + (tsc_diff * (1'000'000 * 94 / (vblank_rate_10 * 10)) / time_diff));
			}
			else if (abs_dst(fps_10, vblank_rate_10 / 2) < 30)
			{
				lv2_obj::set_yield_frequency(preempt_diff, current_tsc + (tsc_diff * (1'000'000 * 96 / ((vblank_rate_10 / 2) * 10)) / time_diff));
			}
			else
			{
				// Undetected case, last 12% is with no yield
				lv2_obj::set_yield_frequency(preempt_diff, current_tsc + (tsc_diff * 88 / 100));
			}

			frame_times.pop_front();
		}
		else
		{
			lv2_obj::set_yield_frequency(0, 0);
		}

		frame_times.push_back(frame_time_t{preempt_count, current_time, current_tsc});
	}

	void vblank_thread::set_thread(std::shared_ptr<named_thread<std::function<void()>>> thread)
	{
		std::swap(m_thread, thread);
	}

	vblank_thread& vblank_thread::operator=(thread_state state)
	{
		if (m_thread)
		{
			*m_thread = state;
		}

		return *this;
	}
}
