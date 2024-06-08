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
atomic_t<progress_dialog_string_t> g_progr{};
atomic_t<u32> g_progr_ftotal{0};
atomic_t<u32> g_progr_fdone{0};
atomic_t<u64> g_progr_ftotal_bits{0};
atomic_t<u64> g_progr_fknown_bits{0};
atomic_t<u32> g_progr_ptotal{0};
atomic_t<u32> g_progr_pdone{0};

// For Batch PPU Compilation
atomic_t<bool> g_system_progress_canceled{false};

// For showing feedback while stopping emulation
atomic_t<bool> g_system_progress_stopping{false};

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
	g_system_progress_stopping = false;

	const auto get_state = []()
	{
		auto whole_state = std::make_tuple(+g_progr.load(), +g_progr_ftotal, +g_progr_fdone, +g_progr_ftotal_bits, +g_progr_fknown_bits, +g_progr_ptotal, +g_progr_pdone);

		while (true)
		{
			auto new_state = std::make_tuple(+g_progr.load(), +g_progr_ftotal, +g_progr_fdone, +g_progr_ftotal_bits, +g_progr_fknown_bits, +g_progr_ptotal, +g_progr_pdone);

			if (new_state == whole_state)
			{
				// Only leave while it has a complete (atomic) state
				return whole_state;
			}

			whole_state = std::move(new_state);
		}

		return whole_state;
	};

	while (!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting)
	{
		// Wait for the start condition
		const char* text0 = g_progr.load();

		while (!text0)
		{
			if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
			{
				break;
			}

			if (g_progr_ftotal || g_progr_fdone || g_progr_ptotal || g_progr_pdone)
			{
				const auto& [text_new, ftotal, fdone, ftotal_bits, fknown_bits, ptotal, pdone] = get_state();

				if (text_new)
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
			text0 = g_progr.load();
		}

		if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		g_system_progress_canceled = false;

		// Initialize message dialog
		bool show_overlay_message = false; // Only show an overlay message after initial loading is done.
		std::shared_ptr<MsgDialogBase> dlg;

		if (const auto renderer = rsx::get_current_renderer())
		{
			// Some backends like OpenGL actually initialize a lot of driver objects in the "on_init" method.
			// Wait for init to complete within reasonable time. Abort just in case we have hardware/driver issues.
			renderer->is_initialized.wait(0, atomic_wait_timeout(5 * 1000000000ull));

			auto manager  = g_fxo->try_get<rsx::overlays::display_manager>();
			show_overlay_message = g_fxo->get<progress_dialog_workaround>().show_overlay_message_only;

			if (manager && !show_overlay_message)
			{
				MsgDialogType type{};
				type.se_mute_on         = true;
				type.se_normal          = true;
				type.bg_invisible       = true;
				type.disable_cancel     = true;
				type.progress_bar_count = 1;

				native_dlg = manager->create<rsx::overlays::progress_dialog>(true);
				native_dlg->show(false, text0, type, nullptr);
				native_dlg->progress_bar_set_message(0, "Please wait");
			}
		}

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
		const char* text1 = nullptr;

		const u64 start_time = get_system_time();
		u64 wait_no_update_count = 0;

		std::shared_ptr<atomic_t<u32>> ppu_cue_refs;

		// Update progress
		while (!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting)
		{
			const auto& [text_new, ftotal_new, fdone_new, ftotal_bits_new, fknown_bits_new, ptotal_new, pdone_new] = get_state();

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

				const bool text_changed = text_new && text_new != text1;

				if (text_new)
				{
					text1 = text_new;
				}

				if (!text1)
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

					thread_ctrl::wait_for(10000);
					continue;
				}

				// Compute new progress in percents
				// Assume not all programs were found if files were not compiled (as it may contain more)
				const bool use_bits = fknown_bits && ftotal_bits;
				const u64 known_files = use_bits ? fknown_bits : ftotal;
				const u64 total = utils::rational_mul<u64>(std::max<u64>(ptotal, 1), std::max<u64>(use_bits ? ftotal_bits : ftotal, 1), std::max<u64>(known_files, 1));
				const u64 done  = pdone;
				const u32 value = static_cast<u32>(done >= total ? 100 : done * 100 / total);

				std::string progr = "Progress:";

				if (ftotal || ptotal)
				{
					if (ftotal)
						fmt::append(progr, " file %u of %u%s", fdone, ftotal, ptotal ? "," : "");
					if (ptotal)
						fmt::append(progr, " module %u of %u", pdone, ptotal);

					const u32 of_1000 = static_cast<u32>(done >= total ? 1000 : done * 1000 / total);

					if (of_1000 >= 2)
					{
						const u64 passed = (get_system_time() - start_time);
						const u64 seconds_passed = passed / 1'000'000;
						const u64 seconds_total = (passed / 1'000'000 * 1000 / of_1000);
						const u64 seconds_remaining = seconds_total - seconds_passed;
						const u64 seconds = seconds_remaining % 60;
						const u64 minutes = (seconds_remaining / 60) % 60;
						const u64 hours = (seconds_remaining / 3600);

						if (seconds_passed < 4)
						{
							// Cannot rely on such small duration of time for estimation
						}
						else if (done >= total)
						{
							fmt::append(progr, " (done)", minutes, seconds);
						}
						else if (hours)
						{
							fmt::append(progr, " (%uh %2um remaining)", hours, minutes);
						}
						else if (minutes >= 2)
						{
							fmt::append(progr, " (%um remaining)", minutes);
						}
						else if (minutes == 0)
						{
							fmt::append(progr, " (%us remaining)", std::max<u64>(seconds, 1));
						}
						else
						{
							fmt::append(progr, " (%um %2us remaining)", minutes, seconds);
						}
					}
				}
				else
				{
					fmt::append(progr, " analyzing...");
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
			if (ftotal == fdone && ptotal == pdone && !text_new)
			{
				// Complete state, empty message: close dialog
				break;
			}

			thread_ctrl::wait_for(10'000);
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

	if (native_dlg && g_system_progress_stopping)
	{
		native_dlg->set_text("Stopping. Please wait...");
		native_dlg->refresh();
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
	g_progr.release(progress_dialog_string_t{});
}
