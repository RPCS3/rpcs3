#include "headless_application.h"

#include "Emu/System.h"
#include "Emu/RSX/Null/NullGSRender.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/Cell/Modules/cellSaveData.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"
#include "Emu/Io/Null/null_camera_handler.h"
#include "Emu/Io/Null/null_music_handler.h"
#include "util/video_source.h"

#include <clocale>

LOG_CHANNEL(sys_log, "SYS");

[[noreturn]] void report_fatal_error(std::string_view text, bool is_html = false, bool include_help_text = true);

// For now, a trivial constructor/destructor. May add command line usage later.
headless_application::headless_application(int& argc, char** argv) : QCoreApplication(argc, argv)
{
}

bool headless_application::Init()
{
	// Force init the emulator
	InitializeEmulator(m_active_user.empty() ? "00000001" : m_active_user, false);

	// Create callbacks from the emulator, which reference the handlers.
	InitializeCallbacks();

	// Create connects to propagate events throughout Gui.
	InitializeConnects();

	// As per Qt recommendations to avoid conflicts for POSIX functions
	std::setlocale(LC_NUMERIC, "C");

	return true;
}

void headless_application::InitializeConnects() const
{
	qRegisterMetaType<std::function<void()>>("std::function<void()>");
	connect(this, &headless_application::RequestCallFromMainThread, this, &headless_application::CallFromMainThread);
}

/** RPCS3 emulator has functions it desires to call from the GUI at times. Initialize them in here. */
void headless_application::InitializeCallbacks()
{
	EmuCallbacks callbacks = CreateCallbacks();

	callbacks.try_to_quit = [this](bool force_quit, std::function<void()> on_exit) -> bool
	{
		if (force_quit)
		{
			if (on_exit)
			{
				on_exit();
			}

			sys_log.notice("Quitting headless application");
			quit();
			return true;
		}

		return false;
	};
	callbacks.call_from_main_thread = [this](std::function<void()> func, atomic_t<u32>* wake_up)
	{
		RequestCallFromMainThread(std::move(func), wake_up);
	};

	callbacks.init_gs_render = [](utils::serial* ar)
	{
		switch (const video_renderer type = g_cfg.video.renderer)
		{
		case video_renderer::null:
		{
			g_fxo->init<rsx::thread, named_thread<NullGSRender>>(ar);
			break;
		}
		case video_renderer::opengl:
		case video_renderer::vulkan:
		{
			fmt::throw_exception("Headless mode can only be used with the %s video renderer. Current renderer: %s", video_renderer::null, type);
			[[fallthrough]];
		}
		default:
		{
			fmt::throw_exception("Invalid video renderer: %s", type);
		}
		}
	};

	callbacks.get_camera_handler = []() -> std::shared_ptr<camera_handler_base>
	{
		switch (g_cfg.io.camera.get())
		{
		case camera_handler::null:
		case camera_handler::fake:
		{
			return std::make_shared<null_camera_handler>();
		}
		case camera_handler::qt:
		{
			fmt::throw_exception("Headless mode can not be used with this camera handler. Current handler: %s", g_cfg.io.camera.get());
		}
		}
		return nullptr;
	};

	callbacks.get_music_handler = []() -> std::shared_ptr<music_handler_base>
	{
		switch (g_cfg.audio.music.get())
		{
		case music_handler::null:
		{
			return std::make_shared<null_music_handler>();
		}
		case music_handler::qt:
		{
			fmt::throw_exception("Headless mode can not be used with this music handler. Current handler: %s", g_cfg.audio.music.get());
		}
		}
		return nullptr;
	};

	callbacks.close_gs_frame = [](){};
	callbacks.get_gs_frame = []() -> std::unique_ptr<GSFrameBase>
	{
		if (g_cfg.video.renderer != video_renderer::null)
		{
			fmt::throw_exception("Headless mode can only be used with the %s video renderer. Current renderer: %s", video_renderer::null, g_cfg.video.renderer.get());
		}
		return std::unique_ptr<GSFrameBase>();
	};

	callbacks.get_msg_dialog                 = []() -> std::shared_ptr<MsgDialogBase> { return std::shared_ptr<MsgDialogBase>(); };
	callbacks.get_osk_dialog                 = []() -> std::shared_ptr<OskDialogBase> { return std::shared_ptr<OskDialogBase>(); };
	callbacks.get_save_dialog                = []() -> std::unique_ptr<SaveDialogBase> { return std::unique_ptr<SaveDialogBase>(); };
	callbacks.get_trophy_notification_dialog = []() -> std::unique_ptr<TrophyNotificationBase> { return std::unique_ptr<TrophyNotificationBase>(); };

	callbacks.on_run    = [](bool /*start_playtime*/) {};
	callbacks.on_pause  = []() {};
	callbacks.on_resume = []() {};
	callbacks.on_stop   = []() {};
	callbacks.on_ready  = []() {};
	callbacks.on_emulation_stop_no_response = [](std::shared_ptr<atomic_t<bool>> closed_successfully, int /*seconds_waiting_already*/)
	{
		if (!closed_successfully || !*closed_successfully)
		{
			report_fatal_error(tr("Stopping emulator took too long."
						"\nSome thread has probably deadlocked. Aborting.").toStdString());
		}
	};

	callbacks.on_save_state_progress = [](std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>)
	{
	};

	callbacks.enable_disc_eject  = [](bool) {};
	callbacks.enable_disc_insert = [](bool) {};

	callbacks.on_missing_fw = []() {};

	callbacks.handle_taskbar_progress = [](s32, s32) {};

	callbacks.get_localized_string    = [](localized_string_id, const char*) -> std::string { return {}; };
	callbacks.get_localized_u32string = [](localized_string_id, const char*) -> std::u32string { return {}; };
	callbacks.get_localized_setting   = [](const cfg::_base*, u32) -> std::string { return {}; };

	callbacks.play_sound = [](const std::string&){};
	callbacks.add_breakpoint = [](u32 /*addr*/){};

	callbacks.display_sleep_control_supported = [](){ return false; };
	callbacks.enable_display_sleep = [](bool /*enabled*/){};

	callbacks.check_microphone_permissions = [](){};

	callbacks.make_video_source = [](){ return nullptr; };

	Emu.SetCallbacks(std::move(callbacks));
}

/**
 * Using connects avoids timers being unable to be used in a non-qt thread. So, even if this looks stupid to just call func, it's succinct.
 */
void headless_application::CallFromMainThread(const std::function<void()>& func, atomic_t<u32>* wake_up)
{
	func();

	if (wake_up)
	{
		*wake_up = true;
		wake_up->notify_one();
	}
}
