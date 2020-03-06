#pragma once

#include "stdafx.h"
#include <functional>
#include <memory>
#include <string>

u64 get_system_time();
u64 get_guest_system_time();

enum class system_state
{
	running,
	paused,
	stopped,
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
};

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_after;
	std::function<void(bool)> on_run; // (start_playtime) continuing or going ingame, so start the clock
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<void(bool)> exit; // (force_quit) close RPCS3
	std::function<void(const std::string&)> reset_pads;
	std::function<void(bool)> enable_pads;
	std::function<void(s32, s32)> handle_taskbar_progress; // (type, value) type: 0 for reset, 1 for increment, 2 for set_limit
	std::function<void()> init_kb_handler;
	std::function<void()> init_mouse_handler;
	std::function<void(std::string_view title_id)> init_pad_handler;
	std::function<std::unique_ptr<class GSFrameBase>()> get_gs_frame;
	std::function<void()> init_gs_render;
	std::function<std::shared_ptr<class AudioBackend>()> get_audio;
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::shared_ptr<class OskDialogBase>()> get_osk_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
	std::function<std::unique_ptr<class TrophyNotificationBase>()> get_trophy_notification_dialog;
};

class Emulator final
{
	atomic_t<system_state> m_state{system_state::stopped};

	EmuCallbacks m_cb;

	atomic_t<u64> m_pause_start_time; // set when paused
	atomic_t<u64> m_pause_amend_time; // increased when resumed

	std::string m_path;
	std::string m_path_old;
	std::string m_title_id;
	std::string m_title;
	std::string m_cat;
	std::string m_dir;
	std::string m_sfo_dir;
	std::string m_game_dir{"PS3_GAME"};
	std::string m_usr{"00000001"};
	u32 m_usrid{1};

	bool m_force_global_config = false;
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
	void CallAfter(std::function<void()>&& func) const
	{
		return m_cb.call_after(std::move(func));
	}

	/** Set emulator mode to running unconditionnaly.
	 * Required to execute various part (PPUInterpreter, memory manager...) outside of rpcs3.
	 */
	void SetTestMode()
	{
		m_state = system_state::running;
	}

	void Init();

	std::vector<std::string> argv;
	std::vector<std::string> envp;
	std::vector<u8> data;
	std::vector<u8> klic;
	std::string disc;
	std::string hdd1;

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

	const std::string& GetCat() const
	{
		return m_cat;
	}

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
	const u32 GetUsrId() const
	{
		return m_usrid;
	}

	const bool SetUsr(const std::string& user);

	const std::string GetBackgroundPicturePath() const;

	u64 GetPauseTime()
	{
		return m_pause_amend_time;
	}

	std::string PPUCache() const;

	game_boot_result BootGame(const std::string& path, const std::string& title_id = "", bool direct = false, bool add_only = false, bool force_global_config = false);
	bool BootRsxCapture(const std::string& path);
	bool InstallPkg(const std::string& path);

private:
	void LimitCacheSize();

public:
	static std::string GetEmuDir();
	static std::string GetHddDir();
	static std::string GetHdd1Dir();
	static std::string GetSfoDirFromGamePath(const std::string& game_path, const std::string& user, const std::string& title_id = "");

	static std::string GetCustomConfigDir();
	static std::string GetCustomConfigPath(const std::string& title_id, bool get_deprecated_path = false);
	static std::string GetCustomInputConfigDir(const std::string& title_id);
	static std::string GetCustomInputConfigPath(const std::string& title_id);

	void SetForceBoot(bool force_boot);

	game_boot_result Load(const std::string& title_id = "", bool add_only = false, bool force_global_config = false, bool is_disc_patch = false);
	void Run(bool start_playtime);
	bool Pause();
	void Resume();
	void Stop(bool restart = false);
	void Restart() { Stop(true); }

	bool IsRunning() const { return m_state == system_state::running; }
	bool IsPaused()  const { return m_state == system_state::paused; }
	bool IsStopped() const { return m_state == system_state::stopped; }
	bool IsReady()   const { return m_state == system_state::ready; }
	auto GetStatus() const { return m_state.load(); }

	bool HasGui() const { return m_has_gui; }
	void SetHasGui(bool has_gui) { m_has_gui = has_gui; }

	std::string GetFormattedTitle(double fps) const;

	u32 GetMaxThreads() const;
};

extern Emulator Emu;

extern bool g_use_rtm;
