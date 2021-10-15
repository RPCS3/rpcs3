#include "stdafx.h"
#include "system_progress.hpp"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/RSX/Overlays/overlay_message_dialog.h"
#include "Emu/System.h"

// Progress display server synchronization variables
atomic_t<const char*> g_progr{nullptr};
atomic_t<u32> g_progr_ftotal{0};
atomic_t<u32> g_progr_fdone{0};
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

	while (!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting)
	{
		// Wait for the start condition
		auto text0 = +g_progr;

		while (!text0)
		{
			if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
			{
				break;
			}

			std::this_thread::sleep_for(5ms);
			text0 = +g_progr;
		}

		if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		g_system_progress_canceled = false;

		// Initialize message dialog
		bool skip_this_one = false; // Workaround: do not open a progress dialog if there is already a cell message dialog open.
		std::shared_ptr<MsgDialogBase> dlg;

		if (const auto renderer = rsx::get_current_renderer();
		    renderer && renderer->is_inited)
		{
			auto manager  = g_fxo->try_get<rsx::overlays::display_manager>();
			skip_this_one = g_fxo->get<progress_dialog_workaround>().skip_the_progress_dialog || (manager && manager->get<rsx::overlays::message_dialog>());

			if (manager && !skip_this_one)
			{
				MsgDialogType type{};
				type.se_normal          = true;
				type.bg_invisible       = true;
				type.disable_cancel     = true;
				type.progress_bar_count = 1;

				native_dlg = manager->create<rsx::overlays::progress_dialog>(true);
				native_dlg->show(false, text0, type, nullptr);
				native_dlg->progress_bar_set_message(0, "Please wait");
			}
		}

		if (!skip_this_one && !native_dlg)
		{
			dlg                          = Emu.GetCallbacks().get_msg_dialog();
			dlg->type.se_normal          = true;
			dlg->type.bg_invisible       = true;
			dlg->type.progress_bar_count = 1;
			dlg->on_close = [](s32 /*status*/)
			{
				Emu.CallAfter([]()
				{
					// Abort everything
					Emu.Stop();
				});

				g_system_progress_canceled = true;
			};

			Emu.CallAfter([dlg, text0]()
			{
				dlg->Create(text0, text0);
			});
		}

		u32 ftotal = 0;
		u32 fdone  = 0;
		u32 ptotal = 0;
		u32 pdone  = 0;
		auto text1 = text0;

		// Update progress
		while (!g_system_progress_stopping && thread_ctrl::state() != thread_state::aborting)
		{
			const auto text_new = g_progr.load();

			const u32 ftotal_new = g_progr_ftotal;
			const u32 fdone_new  = g_progr_fdone;
			const u32 ptotal_new = g_progr_ptotal;
			const u32 pdone_new  = g_progr_pdone;

			if (ftotal != ftotal_new || fdone != fdone_new || ptotal != ptotal_new || pdone != pdone_new || text_new != text1)
			{
				ftotal = ftotal_new;
				fdone  = fdone_new;
				ptotal = ptotal_new;
				pdone  = pdone_new;
				text1  = text_new;

				if (!text_new)
				{
					// Close dialog
					break;
				}

				if (skip_this_one)
				{
					// Do nothing
					std::this_thread::sleep_for(10ms);
					continue;
				}

				// Compute new progress in percents
				// Assume not all programs were found if files were not compiled (as it may contain more)
				const u64 total = std::max<u64>(ptotal, 1) * std::max<u64>(ftotal, 1);
				const u64 done  = pdone * std::max<u64>(fdone, 1);
				const f32 value = static_cast<f32>(std::fmin(done * 100. / total, 100.f));

				std::string progr = "Progress:";

				if (ftotal)
					fmt::append(progr, " file %u of %u%s", fdone, ftotal, ptotal ? "," : "");
				if (ptotal)
					fmt::append(progr, " module %u of %u", pdone, ptotal);

				// Changes detected, send update
				if (native_dlg)
				{
					native_dlg->set_text(text_new);
					native_dlg->progress_bar_set_message(0, progr);
					native_dlg->progress_bar_set_value(0, std::floor(value));
				}
				else if (dlg)
				{
					Emu.CallAfter([=]()
					{
						dlg->SetMsg(text_new);
						dlg->ProgressBarSetMsg(0, progr);
						dlg->ProgressBarSetValue(0, static_cast<u32>(std::floor(value)));
					});
				}
			}

			std::this_thread::sleep_for(10ms);
		}

		if (g_system_progress_stopping || thread_ctrl::state() == thread_state::aborting)
		{
			break;
		}

		if (skip_this_one)
		{
			// Do nothing
		}
		else if (native_dlg)
		{
			native_dlg->close(false, false);
		}
		else if (dlg)
		{
			Emu.CallAfter([=]()
			{
				dlg->Close(true);
			});
		}

		// Cleanup
		g_progr_fdone -= fdone;
		g_progr_pdone -= pdone;
		g_progr_ftotal -= ftotal;
		g_progr_ptotal -= ptotal;
		g_progr_ptotal.notify_all();
	}

	if (native_dlg && g_system_progress_stopping)
	{
		native_dlg->set_text("Stopping. Please wait...");
		native_dlg->refresh();
	}
}

progress_dialog_server::~progress_dialog_server()
{
	g_progr_ftotal.release(0);
	g_progr_fdone.release(0);
	g_progr_ptotal.release(0);
	g_progr_pdone.release(0);
	g_progr.release(nullptr);
}
