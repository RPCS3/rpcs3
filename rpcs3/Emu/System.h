#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>

#include "Emu/Cell/timers.hpp"

struct progress_dialog_workaround
{
	// WORKAROUND:
	// We don't want to show the native dialog during gameplay.
	// This can currently interfere with cell dialogs.
	atomic_t<bool> skip_the_progress_dialog = false;
};

enum class localized_string_id;
enum class video_renderer;

enum class system_state : u32
{
	stopped,
	running,
	paused,
	frozen, // paused but cannot resume
	ready,
};

enum class game_boot_result : u32
{
	no_errors,
	generic_error,
	nothing_to_boot,
	wrong_disc_location,
	invalid_file_or_folder,
	install_failed,
	decryption_error,
	file_creation_error,
	firmware_missing,
	unsupported_disc_type
};

enum class cfg_mode
{
	custom,           // Prefer regular custom config. Fall back to global config.
	custom_selection, // Use user-selected custom config. Fall back to global config.
	global,           // Use global config.
	config_override,  // Use config override. This does not use the global VFS settings! Fall back to global config.
	continuous,       // Use same config as on last boot. Fall back to global config.
	default_config    // Use the default values of the config entries.
};

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_from_main_thread;
	std::function<void(bool)> on_run; // (start_playtime) continuing or going ingame, so start the clock
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<bool()> on_missing_fw;
	std::function<bool(bool, std::function<void()>)> try_to_quit; // (force_quit, on_exit) Try to close RPCS3
	std::function<void(s32, s32)> handle_taskbar_progress; // (type, value) type: 0 for reset, 1 for increment, 2 for set_limit, 3 for set_value
	std::function<void()> init_kb_handler;
	std::function<void()> init_mouse_handler;
	std::function<void(std::string_view title_id)> init_pad_handler;
	std::function<std::unique_ptr<class GSFrameBase>()> get_gs_frame;
	std::function<void()> init_gs_render;
	std::function<std::shared_ptr<class camera_handler_base>()> get_camera_handler;
	std::function<std::shared_ptr<class music_handler_base>()> get_music_handler;
	std::function<std::shared_ptr<class AudioBackend>()> get_audio;
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::shared_ptr<class OskDialogBase>()> get_osk_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
	std::function<std::shared_ptr<class SendMessageDialogBase>()> get_sendmessage_dialog;
	std::function<std::shared_ptr<class RecvMessageDialogBase>()> get_recvmessage_dialog;
	std::function<std::unique_ptr<class TrophyNotificationBase>()> get_trophy_notification_dialog;
	std::function<std::string(localized_string_id, const char*)> get_localized_string;
	std::function<std::u32string(localized_string_id, const char*)> get_localized_u32string;
	std::function<void(const std::string&)> play_sound;
	std::string(*resolve_path)(std::string_view) = nullptr; // Resolve path using Qt
};

class Emulator final
{
	atomic_t<system_state> m_state{system_state::stopped};

	EmuCallbacks m_cb;

	atomic_t<u64> m_pause_start_time{0}; // set when paused
	atomic_t<u64> m_pause_amend_time{0}; // increased when resumed
	atomic_t<u64> m_stop_ctr{0}; // Increments when emulation is stopped

	video_renderer m_default_renderer;
	std::string m_default_graphics_adapter;

	cfg_mode m_config_mode = cfg_mode::custom;
	std::string m_config_path;
	std::string m_path;
	std::string m_path_old;
	std::string m_title_id;
	std::string m_title;
	std::string m_app_version;
	std::string m_cat;
	std::string m_dir;
	std::string m_sfo_dir;
	std::string m_game_dir{"PS3_GAME"};
	std::string m_usr{"00000001"};
	u32 m_usrid{1};

	// This flag should be adjusted before each Kill() or each BootGame() and similar because:
	// 1. It forces an application to boot immediately by calling Run() in Load().
	// 2. It signifies that we don't want to exit on Kill(), for example if we want to transition to another application.
	bool m_force_boot = false;

	bool m_has_gui = true;

public:
	Emulator() = default;

	void SetCallbacks(EmuCallbacks&& cb)
	{
		m_cb = std::move(cb);
	}

	const auto& GetCallbacks() const
	{
		return m_cb;
	}

	// Call from the GUI thread
	void CallFromMainThread(std::function<void()>&& func, bool track_emu_state = true, u64 stop_ctr = umax) const
	{
		if (!track_emu_state)
		{
			return m_cb.call_from_main_thread(std::move(func));
		}

		std::function<void()> final_func = [this, before = IsStopped(), count = (stop_ctr == umax ? +m_stop_ctr : stop_ctr), func = std::move(func)]
		{
			if (count == m_stop_ctr && before == IsStopped())
			{
				func();
			}
		};

		return m_cb.call_from_main_thread(std::move(final_func));
	}

	enum class stop_counter_t : u64{};

	stop_counter_t ProcureCurrentEmulationCourseInformation() const
	{
		return stop_counter_t{+m_stop_ctr};
	}

	void CallFromMainThread(std::function<void()>&& func, stop_counter_t counter) const
	{
		CallFromMainThread(std::move(func), true, static_cast<u64>(counter));
	}

	/** Set emulator mode to running unconditionnaly.
	 * Required to execute various part (PPUInterpreter, memory manager...) outside of rpcs3.
	 */
	void SetTestMode()
	{
		m_state = system_state::running;
	}

	void Init(bool add_only = false);

	std::vector<std::string> argv;
	std::vector<std::string> envp;
	std::vector<u8> data;
	std::vector<u128> klic;
	std::string disc;
	std::string hdd1;

	u32 m_boot_source_type = 0; // CELL_GAME_GAMETYPE_SYS

	const u32& GetBootSourceType() const
	{
		return m_boot_source_type;
	}

	const std::string& GetBoot() const
	{
		return m_path;
	}

	const std::string& GetTitleID() const
	{
		return m_title_id;
	}

	const std::string& GetTitle() const
	{
		return m_title;
	}

	const std::string GetTitleAndTitleID() const
	{
		return m_title + (m_title_id.empty() ? "" : " [" + m_title_id + "]");
	}

	const std::string& GetAppVersion() const
	{
		return m_app_version;
	}

	const std::string& GetCat() const
	{
		return m_cat;
	}

	const std::string& GetFakeCat() const;

	const std::string& GetDir() const
	{
		return m_dir;
	}

	const std::string& GetSfoDir() const
	{
		return m_sfo_dir;
	}

	// String for GUI dialogs.
	const std::string& GetUsr() const
	{
		return m_usr;
	}

	// u32 for cell.
	u32 GetUsrId() const
	{
		return m_usrid;
	}

	void SetUsr(const std::string& user);

	std::string GetBackgroundPicturePath() const;

	u64 GetPauseTime() const
	{
		return m_pause_amend_time;
	}

	const std::string& GetUsedConfig() const
	{
		return m_config_path;
	}

	game_boot_result BootGame(const std::string& path, const std::string& title_id = "", bool direct = false, bool add_only = false, cfg_mode config_mode = cfg_mode::custom, const std::string& config_path = "");
	bool BootRsxCapture(const std::string& path);

	void SetForceBoot(bool force_boot);

	game_boot_result Load(const std::string& title_id = "", bool add_only = false, bool is_disc_patch = false);
	void Run(bool start_playtime);
	bool Pause(bool freeze_emulation = false);
	void Resume();
	void GracefulShutdown(bool allow_autoexit = true, bool async_op = false);
	void Kill(bool allow_autoexit = true);
	game_boot_result Restart();
	bool Quit(bool force_quit);
	static void CleanUp();

	bool IsRunning() const { return m_state == system_state::running; }
	bool IsPaused()  const { return m_state >= system_state::paused; } // ready is also considered paused by this function
	bool IsStopped() const { return m_state == system_state::stopped; }
	bool IsReady()   const { return m_state == system_state::ready; }
	auto GetStatus() const { system_state state = m_state; return state == system_state::frozen ? system_state::paused : state; }

	bool HasGui() const { return m_has_gui; }
	void SetHasGui(bool has_gui) { m_has_gui = has_gui; }

	void SetDefaultRenderer(video_renderer renderer) { m_default_renderer = renderer; }
	void SetDefaultGraphicsAdapter(std::string adapter) { m_default_graphics_adapter = std::move(adapter); }

	std::string GetFormattedTitle(double fps) const;

	void ConfigurePPUCache() const;

	std::set<std::string> GetGameDirs() const;

	// Check if path is inside the specified directory
	bool IsPathInsideDir(std::string_view path, std::string_view dir) const;
};

extern Emulator Emu;

extern bool g_use_rtm;
extern u64 g_rtm_tx_limit1;
extern u64 g_rtm_tx_limit2;
