#include "stdafx.h"
#include "system_progress.hpp"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/RSX/Overlays/overlay_message_dialog.h"
#include "Emu/RSX/Overlays/overlay_message.h"
#include "Emu/RSX/Overlays/overlay_compile_notification.h"
#include "Emu/System.h"

#include "util/asm.hpp"

LOG_CHANNEL(sys_log, "SYS");

// Progress display server synchronization variables
lf_array<atomic_ptr<std::string>> g_progr_text_queue;
progress_dialog_string_t g_progr_text{};
atomic_t<u32> g_progr_ftotal{0};
atomic_t<u32> g_progr_fdone{0};
atomic_t<u64> g_progr_ftotal_bits{0};
atomic_t<u64> g_progr_fknown_bits{0};
atomic_t<u32> g_progr_ptotal{0};
atomic_t<u32> g_progr_pdone{0};

// For Batch PPU Compilation
atomic_t<bool> g_system_progress_canceled{false};

// For showing feedback while stopping emulation
atomic_t<system_progress_stop_state> g_system_progress_stopping{system_progress_stop_state::stop_state_disabled};

namespace rsx::overlays
{
	class progress_dialog : public message_dialog
	{
	public:
		using message_dialog::message_dialog;
	};
} // namespace rsx::overlays

void progress_dialog_server::operator()()
{
	std::shared_ptr<rsx::overlays::progress_dialog> native_dlg;
	g_system_progress_stopping = system_progress_stop_state::stop_state_disabled;
	g_system_progress_canceled = false;

	const auto get_state = []()
	{
		auto whole_state = std::make_tuple(g_progr_text.operator std::string(), +g_progr_ftotal, +g_progr_fdone, +g_progr_ftotal_bits, +g_progr_fknown_bits, +g_progr_ptotal, +g_progr_pdone);

		while (true)
		{
			auto new_state = std::make_tuple(g_progr_text.operator std::string(), +g_progr_ftotal, +g_progr_fdone, +g_progr_ftotal_bits, +g_progr_fknown_bits, +g_progr_ptotal, +g_progr_pdone);

			if (new_state == whole_state)
			{
				// Only leave while it has a complete (atomic) state
				return whole_state;
			}

			whole_state = std::move(new_state);
		}

		return whole_state;
	};

	const auto create_native_dialog = [&native_dlg](const std::string& text, bool* show_overlay_message)
	{
		if (const auto renderer = rsx::get_current_renderer())
		{
			// Some backends like OpenGL actually initialize a lot of driver objects in the "on_init" method.
			// Wait for init to complete within reasonable time. Abort just in case we have hardware/driver issues.
			renderer->is_initialized.wait(0, atomic_wait_timeout(5 * 1000000000ull));

			auto manager = g_fxo->try_get<rsx::overlays::display_manager>();

			if (show_overlay_message)
			{
				*show_overlay_message = g_fxo->get<progress_dialog_workaround>().show_overlay_message_only;
				if (*show_overlay_message)
				{
					return;
				}
			}

			if (manager)
			{
				MsgDialogType type{};
				type.se_mute_on         = true;
				type.se_normal          = true;
				type.bg_invisible       = true;
				type.disable_cancel     = true;
				type.progress_bar_count = 1;

				native_dlg = manager->create<rsx::overlays::progress_dialog>(true);
				native_dlg->show(false, text, type, msg_dialog_source::sys_progress, nullptr);
				native_dlg->progress_bar_set_message(0, get_localized_string(localized_string_id::PROGRESS_DIALOG_PLEASE_WAIT));
			}
		}
	};

	while (!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting)
	{
		// Wait for the start condition
		std::string text0 = g_progr_text;

		while (text0.empty())
		{
			if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
			{
				break;
			}

			if (g_progr_ftotal || g_progr_fdone || g_progr_ptotal || g_progr_pdone)
			{
				const auto [text_new, ftotal, fdone, ftotal_bits, fknown_bits, ptotal, pdone] = get_state();

				if (!text_new.empty())
				{
					text0 = text_new;
					break;
				}

				if ((ftotal || ptotal) && ftotal == fdone && ptotal == pdone)
				{
					// Cleanup (missed message but do not cry over spilt milk)
					g_progr_fdone -= fdone;
					g_progr_pdone -= pdone;
					g_progr_ftotal_bits -= ftotal_bits;
					g_progr_fknown_bits -= fknown_bits;
					g_progr_ftotal -= ftotal;
					g_progr_ptotal -= ptotal;
					g_progr_ptotal.notify_all();
				}
			}

			thread_ctrl::wait_for(5000);
			text0 = g_progr_text;
		}

		if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		g_system_progress_canceled = false;

		// Initialize message dialog
		bool show_overlay_message = false; // Only show an overlay message after initial loading is done.
		std::shared_ptr<MsgDialogBase> dlg;

		create_native_dialog(text0, &show_overlay_message);

		if (!show_overlay_message && !native_dlg && (dlg = Emu.GetCallbacks().get_msg_dialog()))
		{
			dlg->type.se_normal          = true;
			dlg->type.bg_invisible       = true;
			dlg->type.progress_bar_count = 1;
			dlg->on_close = [](s32 /*status*/)
			{
				Emu.CallFromMainThread([]()
				{
					// Abort everything
					sys_log.notice("Aborted progress dialog");
					Emu.GracefulShutdown(false, true);
				});

				g_system_progress_canceled = true;
			};

			Emu.CallFromMainThread([dlg, text0]()
			{
				dlg->Create(text0, text0);
			});
		}

		u32 ftotal = 0;
		u32 fdone = 0;
		u64 fknown_bits = 0;
		u64 ftotal_bits = 0;
		u32 ptotal = 0;
		u32 pdone = 0;
		std::string text1;

		const u64 start_time = get_system_time();
		u64 wait_no_update_count = 0;

		std::shared_ptr<atomic_t<u32>> ppu_cue_refs;

		std::vector<std::pair<u64, u64>> time_left_queue(1024);
		usz time_left_queue_idx = 0;

		// Update progress
		for (u64 sleep_until = get_system_time(), sleep_for = 500;
			!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting;
			thread_ctrl::wait_until(&sleep_until, std::exchange(sleep_for, 500)))
		{
			const auto [text_new, ftotal_new, fdone_new, ftotal_bits_new, fknown_bits_new, ptotal_new, pdone_new] = get_state();

			// Force-update every 20 seconds to update remaining time
			if (wait_no_update_count == 100u * 20 || ftotal != ftotal_new || fdone != fdone_new || fknown_bits != fknown_bits_new
				|| ftotal_bits != ftotal_bits_new || ptotal != ptotal_new || pdone != pdone_new || text_new != text1)
			{
				wait_no_update_count = 0;
				ftotal = ftotal_new;
				fdone  = fdone_new;
				ftotal_bits = ftotal_bits_new;
				fknown_bits = fknown_bits_new;
				ptotal = ptotal_new;
				pdone  = pdone_new;

				const bool text_changed = !text_new.empty() && text_new != text1;

				if (!text_new.empty())
				{
					text1 = text_new;
				}

				if (text1.empty())
				{
					// Cannot do anything
					continue;
				}

				if (show_overlay_message)
				{
					// Show a message instead (if compilation period is estimated to be lengthy)
					if (pdone < ptotal && g_cfg.misc.show_ppu_compilation_hint)
					{
						const u64 passed_usec = (get_system_time() - start_time);
						const u64 remaining_usec = pdone ? utils::rational_mul<u64>(passed_usec, static_cast<u64>(ptotal) - pdone, pdone) : (passed_usec * ptotal);

						// Only show compile notification if we estimate at least 100ms
						if (remaining_usec >= 100'000ULL)
						{
							if (!ppu_cue_refs || !*ppu_cue_refs)
							{
								ppu_cue_refs = rsx::overlays::show_ppu_compile_notification();
							}

							// Make sure to update any pending messages. PPU compilation may freeze the image.
							rsx::overlays::refresh_message_queue();
						}
					}

					if (pdone >= ptotal)
					{
						if (ppu_cue_refs)
						{
							*ppu_cue_refs = 0;
							ppu_cue_refs.reset();

							rsx::overlays::refresh_message_queue();
						}
					}

					sleep_for = 10000;
					continue;
				}

				// Compute new progress in percents
				// Assume not all programs were found if files were not compiled (as it may contain more)
				const bool use_bits = fknown_bits && ftotal_bits;
				const u64 known_files = use_bits ? fknown_bits : ftotal;
				const u64 total = utils::rational_mul<u64>(std::max<u64>(ptotal, 1), std::max<u64>(use_bits ? ftotal_bits : ftotal, 1), std::max<u64>(known_files, 1));
				const u64 done  = pdone;
				const u32 value = static_cast<u32>(done >= total ? 100 : done * 100 / total);

				std::string progr;

				if (ftotal || ptotal)
				{
					progr = get_localized_string(localized_string_id::PROGRESS_DIALOG_PROGRESS);

					if (ftotal)
						fmt::append(progr, " %s %u %s %u%s", get_localized_string(localized_string_id::PROGRESS_DIALOG_FILE), fdone, get_localized_string(localized_string_id::PROGRESS_DIALOG_OF), ftotal, ptotal ? "," : "");
					if (ptotal)
						fmt::append(progr, " %s %u %s %u", get_localized_string(localized_string_id::PROGRESS_DIALOG_MODULE), pdone, get_localized_string(localized_string_id::PROGRESS_DIALOG_OF), ptotal);

					const u32 of_1000 = static_cast<u32>(done >= total ? 1000 : done * 1000 / total);

					if (of_1000 >= 2)
					{
						const u64 passed = (get_system_time() - start_time);
						const u64 total = utils::rational_mul<u64>(passed, 1000, of_1000);
						const u64 remaining = total - passed;

						// Stabilize the result by using the maximum one from the recent history
						// This is a very simple approach yet appears to solve most inconsistencies
						u64 max_remaining = remaining;

						for (usz i = 0; i < time_left_queue.size(); i++)
						{
							const auto& sample = time_left_queue[(time_left_queue.size() + time_left_queue_idx - i) % time_left_queue.size()];

							const u64 sample_age = passed - sample.first;

							if (passed - sample.first >= 4'000'000)
							{
								// Ignore old samples
								break;
							}

							max_remaining = std::max<u64>(max_remaining, sample.second >= sample_age ? sample.second - sample_age : 0);
						}

						if (auto new_val = std::make_pair(passed, remaining); time_left_queue[time_left_queue_idx] != new_val)
						{
							time_left_queue_idx = (time_left_queue_idx + 1) % time_left_queue.size();
							time_left_queue[time_left_queue_idx] = new_val;
						}

						const u64 max_seconds_remaining = max_remaining / 1'000'000;
						const u64 seconds = max_seconds_remaining % 60;
						const u64 minutes = (max_seconds_remaining / 60) % 60;
						const u64 hours = (max_seconds_remaining / 3600);

						if (passed < 4'000'000)
						{
							// Cannot rely on such small duration of time for estimation
						}
						else if (done >= total)
						{
							fmt::append(progr, " (%s)", get_localized_string(localized_string_id::PROGRESS_DIALOG_DONE));
						}
						else if (hours)
						{
							fmt::append(progr, " (%uh %2um %s)", hours, minutes, get_localized_string(localized_string_id::PROGRESS_DIALOG_REMAINING));
						}
						else if (minutes >= 2)
						{
							fmt::append(progr, " (%um %s)", minutes, get_localized_string(localized_string_id::PROGRESS_DIALOG_REMAINING));
						}
						else if (minutes == 0)
						{
							fmt::append(progr, " (%us %s)", std::max<u64>(seconds, 1), get_localized_string(localized_string_id::PROGRESS_DIALOG_REMAINING));
						}
						else
						{
							fmt::append(progr, " (%um %2us %s)", minutes, seconds, get_localized_string(localized_string_id::PROGRESS_DIALOG_REMAINING));
						}
					}
				}
				else
				{
					progr = get_localized_string(localized_string_id::PROGRESS_DIALOG_PROGRESS_ANALYZING);
				}

				// Changes detected, send update
				if (native_dlg)
				{
					if (text_changed)
					{
						native_dlg->set_text(text1);
					}

					native_dlg->progress_bar_set_message(0, std::move(progr));
					native_dlg->progress_bar_set_value(0, static_cast<f32>(value));
				}
				else if (dlg)
				{
					Emu.CallFromMainThread([=]()
					{
						if (text_changed)
						{
							dlg->SetMsg(text1);
						}

						dlg->ProgressBarSetMsg(0, progr);
						dlg->ProgressBarSetValue(0, value);
					});
				}
			}

			if (show_overlay_message)
			{
				// Make sure to update any pending messages. PPU compilation may freeze the image.
				rsx::overlays::refresh_message_queue();
			}

			// Leave only if total count is equal to done count
			if (ftotal == fdone && ptotal == pdone && text_new.empty())
			{
				// Complete state, empty message: close dialog
				break;
			}

			sleep_for = 10'000;
			wait_no_update_count++;
		}

		if (ppu_cue_refs)
		{
			*ppu_cue_refs = 0;
		}

		if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		if (show_overlay_message)
		{
			// Do nothing
		}
		else if (native_dlg)
		{
			native_dlg->close(false, false);
			native_dlg.reset();
		}
		else if (dlg)
		{
			Emu.CallFromMainThread([=]()
			{
				dlg->Close(true);
			});
		}

		// Cleanup
		g_progr_fdone -= fdone;
		g_progr_pdone -= pdone;
		g_progr_ftotal_bits -= ftotal_bits;
		g_progr_fknown_bits -= fknown_bits;
		g_progr_ftotal -= ftotal;
		g_progr_ptotal -= ptotal;
		g_progr_ptotal.notify_all();
	}

	if (g_system_progress_stopping)
	{
		const std::string text = get_localized_string(
			g_system_progress_stopping == system_progress_stop_state::stop_state_continuous_savestate
				? localized_string_id::PROGRESS_DIALOG_SAVESTATE_PLEASE_WAIT
				: localized_string_id::PROGRESS_DIALOG_STOPPING_PLEASE_WAIT
		);
		if (native_dlg)
		{
			native_dlg->set_text(text);
		}
		else
		{
			create_native_dialog(text, nullptr);
		}
		if (native_dlg)
		{
			native_dlg->refresh();
		}
	}

	if (g_progr_ptotal.exchange(0))
	{
		g_progr_ptotal.notify_all();
	}
}

progress_dialog_server::~progress_dialog_server()
{
	g_progr_ftotal.release(0);
	g_progr_fdone.release(0);
	g_progr_ftotal_bits.release(0);
	g_progr_fknown_bits.release(0);
	g_progr_ptotal.release(0);
	g_progr_pdone.release(0);
	g_progr_text.data.release(std::common_type_t<progress_dialog_string_t::data_t>{});
}
