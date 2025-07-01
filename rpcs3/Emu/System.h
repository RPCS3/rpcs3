#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/shared_ptr.hpp"
#include "Utilities/bit_set.h"
#include "config_mode.h"
#include "games_config.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>

void init_fxo_for_exec(utils::serial*, bool);

enum class localized_string_id;
enum class video_renderer;

class spu_thread;

template <typename T>
class named_thread;

namespace cfg
{
	class _base;
}

enum class system_state : u32
{
	stopped,
	loading,
	stopping,
	running,
	paused,
	frozen, // paused but cannot resume
	ready,
	starting,
};

enum class game_boot_result : u32
{
	no_errors,
	generic_error,
	nothing_to_boot,
	wrong_disc_location,
	invalid_file_or_folder,
	invalid_bdvd_folder,
	install_failed,
	decryption_error,
	file_creation_error,
	firmware_missing,
	firmware_version,
	unsupported_disc_type,
	savestate_corrupted,
	savestate_version_unsupported,
	still_running,
	already_added,
	currently_restricted,
};

constexpr bool is_error(game_boot_result res)
{
	return res != game_boot_result::no_errors;
}

struct EmuCallbacks
{
	std::function<void(std::function<void()>, atomic_t<u32>*)> call_from_main_thread;
	std::function<void(bool)> on_run; // (start_playtime) continuing or going ingame, so start the clock
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<void()> on_missing_fw;
	std::function<void(std::shared_ptr<atomic_t<bool>>, int)> on_emulation_stop_no_response;
	std::function<void(std::shared_ptr<atomic_t<bool>>, stx::shared_ptr<utils::serial>, stx::atomic_ptr<std::string>*, std::shared_ptr<void>)> on_save_state_progress;
	std::function<void(bool enabled)> enable_disc_eject;
	std::function<void(bool enabled)> enable_disc_insert;
	std::function<bool(bool, std::function<void()>)> try_to_quit; // (force_quit, on_exit) Try to close RPCS3
	std::function<void(s32, s32)> handle_taskbar_progress; // (type, value) type: 0 for reset, 1 for increment, 2 for set_limit, 3 for set_value
	std::function<void()> init_kb_handler;
	std::function<void()> init_mouse_handler;
	std::function<void(std::string_view title_id)> init_pad_handler;
	std::function<void()> update_emu_settings;
	std::function<void()> save_emu_settings;
	std::function<void()> close_gs_frame;
	std::function<std::unique_ptr<class GSFrameBase>()> get_gs_frame;
	std::function<std::shared_ptr<class camera_handler_base>()> get_camera_handler;
	std::function<std::shared_ptr<class music_handler_base>()> get_music_handler;
	std::function<void(utils::serial*)> init_gs_render;
	std::function<std::shared_ptr<class AudioBackend>()> get_audio;
	std::function<std::shared_ptr<class audio_device_enumerator>(u64)> get_audio_enumerator; // (audio_renderer)
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
	std::function<std::shared_ptr<class OskDialogBase>()> get_osk_dialog;
	std::function<std::unique_ptr<class SaveDialogBase>()> get_save_dialog;
	std::function<std::shared_ptr<class SendMessageDialogBase>()> get_sendmessage_dialog;
	std::function<std::shared_ptr<class RecvMessageDialogBase>()> get_recvmessage_dialog;
	std::function<std::unique_ptr<class TrophyNotificationBase>()> get_trophy_notification_dialog;
	std::function<std::string(localized_string_id, const char*)> get_localized_string;
	std::function<std::u32string(localized_string_id, const char*)> get_localized_u32string;
	std::function<std::string(const cfg::_base*, u32)> get_localized_setting;
	std::function<void(const std::string&)> play_sound;
	std::function<bool(const std::string&, std::string&, s32&, s32&, s32&)> get_image_info; // (filename, sub_type, width, height, CellSearchOrientation)
	std::function<bool(const std::string&, s32, s32, s32&, s32&, u8*, bool)> get_scaled_image; // (filename, target_width, target_height, width, height, dst, force_fit)
	std::string(*resolve_path)(std::string_view) = [](std::string_view arg){ return std::string{arg}; }; // Resolve path using Qt
	std::function<std::vector<std::string>()> get_font_dirs;
	std::function<bool(const std::vector<std::string>&)> on_install_pkgs;
	std::function<void(u32)> add_breakpoint;
	std::function<bool()> display_sleep_control_supported;
	std::function<void(bool)> enable_display_sleep;
	std::function<void()> check_microphone_permissions;
	std::function<std::unique_ptr<class video_source>()> make_video_source;
	std::function<void(bool)> enable_gamemode;
};

namespace utils
{
	struct serial;
};

class Emulator final
{
	atomic_t<system_state> m_state{system_state::stopped};

	EmuCallbacks m_cb;

	atomic_t<u64> m_pause_start_time{0}; // set when paused
	atomic_t<u64> m_pause_amend_time{0}; // increased when resumed
	atomic_t<u64> m_stop_ctr{1}; // Increments when emulation is stopped
	atomic_t<bool> m_emu_state_close_pending = false;
	atomic_t<u64> m_restrict_emu_state_change{0};

	games_config m_games_config;

	video_renderer m_default_renderer;
	std::string m_default_graphics_adapter;

	cfg_mode m_config_mode = cfg_mode::custom;
	std::string m_config_path;
	std::string m_path;
	std::string m_path_old;
	std::string m_path_original;
	std::string m_title_id;
	std::string m_title;
	std::string m_localized_title;
	std::string m_app_version;
	std::string m_hash;
	std::string m_cat;
	std::string m_dir;
	std::string m_sfo_dir;
	std::string m_game_dir{"PS3_GAME"};
	std::string m_usr{"00000001"};
	u32 m_usrid{1};
	std::shared_ptr<utils::serial> m_ar;

	// This flag should be adjusted before each Kill() or each BootGame() and similar because:
	// 1. It forces an application to boot immediately by calling Run() in Load().
	// 2. It signifies that we don't want to exit on Kill(), for example if we want to transition to another application.
	bool m_force_boot = false;

	bool m_continuous_mode = false;
	bool m_has_gui = true;

	bool m_state_inspection_savestate = false;

	usz m_tty_file_init_pos = umax;

	std::vector<std::shared_ptr<atomic_t<u32>>> m_pause_msgs_refs;

	std::vector<std::function<void()>> m_postponed_init_code;

	void ExecPostponedInitCode()
	{
		for (auto&& func : ::as_rvalue(std::move(m_postponed_init_code)))
		{
			func();
		}
	}

	enum class SaveStateExtentionFlags1 : u8
	{
		SupportsMenuOpenResume,
		ShouldCloseMenu,

		__bitset_enum_max,
	};

	bs_t<SaveStateExtentionFlags1> m_savestate_extension_flags1{};

public:
	static constexpr std::string_view game_id_boot_prefix = "%RPCS3_GAMEID%:";
	static constexpr std::string_view vfs_boot_prefix = "%RPCS3_VFS%:";

	Emulator() noexcept = default;
	~Emulator() noexcept = default;

	void SetCallbacks(EmuCallbacks&& cb)
	{
		m_cb = std::move(cb);
	}

	const auto& GetCallbacks() const
	{
		return m_cb;
	}

	// Call from the GUI thread
	void CallFromMainThread(std::function<void()>&& func, atomic_t<u32>* wake_up = nullptr, bool track_emu_state = true, u64 stop_ctr = umax,
		std::source_location src_loc = std::source_location::current()) const;

	// Blocking call from the GUI thread
	void BlockingCallFromMainThread(std::function<void()>&& func, bool track_emu_state = true, std::source_location src_loc = std::source_location::current()) const;

	enum class stop_counter_t : u64{};

	// Returns a different value each time we start a new emulation.
	stop_counter_t GetEmulationIdentifier(bool subtract_one = false) const
	{
		if (subtract_one)
		{
			return stop_counter_t{m_stop_ctr - 1};
		}

		return stop_counter_t{+m_stop_ctr};
	}

	void CallFromMainThread(std::function<void()>&& func, stop_counter_t counter,
		std::source_location src_loc = std::source_location::current()) const
	{
		CallFromMainThread(std::move(func), nullptr, true, static_cast<u64>(counter), src_loc);
	}

	void PostponeInitCode(std::function<void()>&& func)
	{
		m_postponed_init_code.emplace_back(std::move(func));
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
	std::vector<u128> klic;
	std::string disc;
	std::string hdd1;
	std::function<void(u32)> init_mem_containers;
	std::function<void()> after_kill_callback;

	u32 m_boot_source_type = 0; // CELL_GAME_GAMETYPE_SYS

	const u32& GetBootSourceType() const
	{
		return m_boot_source_type;
	}

	const std::string& GetBoot() const
	{
		return m_path;
	}

	const std::string& GetLastBoot() const
	{
		return m_path_original;
	}

	const std::string& GetTitleID() const
	{
		return m_title_id;
	}

	const std::string& GetTitle() const
	{
		return m_title;
	}

	const std::string& GetLocalizedTitle() const
	{
		return m_localized_title;
	}

	const std::string GetTitleAndTitleID() const
	{
		return m_title + (m_title_id.empty() ? "" : " [" + m_title_id + "]");
	}

	const std::string& GetAppVersion() const
	{
		return m_app_version;
	}

	const std::string& GetExecutableHash() const
	{
		return m_hash;
	}

	void SetExecutableHash(std::string hash) { m_hash = std::move(hash); }

	const std::string& GetCat() const
	{
		return m_cat;
	}

	const std::string& GetFakeCat() const;

	const std::string& GetDir() const
	{
		return m_dir;
	}

	const std::string GetSfoDir(bool prefer_disc_sfo) const;

	// String for GUI dialogs.
	const std::string& GetUsr() const
	{
		return m_usr;
	}

	const games_config& GetGamesConfig() const
	{
		return m_games_config;
	}

	// Get deserialization manager
	utils::serial* DeserialManager() const;

	// u32 for cell.
	u32 GetUsrId() const
	{
		return m_usrid;
	}

	void SetUsr(const std::string& user);

	u64 GetPauseTime() const
	{
		return m_pause_amend_time;
	}

	const std::string& GetUsedConfig() const
	{
		return m_config_path;
	}

	bool IsChildProcess() const
	{
		return m_config_mode == cfg_mode::continuous;
	}

	bool ContinuousModeEnabled(bool reset)
	{
		if (reset)
		{
			return std::exchange(m_continuous_mode, false);
		}
		return m_continuous_mode;
	}

	class emulation_state_guard_t
	{
		class Emulator* _this = nullptr;
		bool active = true;

	public:
		explicit emulation_state_guard_t(Emulator* this0) noexcept
			: _this(this0)
		{
			_this->m_restrict_emu_state_change++;
		}

		~emulation_state_guard_t() noexcept
		{
			if (active)
			{
				_this->m_restrict_emu_state_change--;
			}
		}

		emulation_state_guard_t(emulation_state_guard_t&& rhs) noexcept
		{
			_this = rhs._this;
			active = std::exchange(rhs.active, false);
		}

		emulation_state_guard_t& operator=(const emulation_state_guard_t&) = delete;
		emulation_state_guard_t(const emulation_state_guard_t&) = delete;
	};

	emulation_state_guard_t MakeEmulationStateGuard()
	{
		return emulation_state_guard_t{this};
	}

	game_boot_result BootGame(const std::string& path, const std::string& title_id = "", bool direct = false, cfg_mode config_mode = cfg_mode::custom, const std::string& config_path = "");
	bool BootRsxCapture(const std::string& path);

	void SetForceBoot(bool force_boot);
	void SetContinuousMode(bool continuous_mode);

	game_boot_result Load(const std::string& title_id = "", bool is_disc_patch = false, usz recursion_count = 0);
	void Run(bool start_playtime);
	void RunPPU();
	void FixGuestTime();
	void FinalizeRunRequest();

	bool IsBootingRestricted() const
	{
		return m_restrict_emu_state_change != 0;
	}

private:
	struct savestate_stage
	{
		bool prepared = false;
		std::vector<std::pair<shared_ptr<named_thread<spu_thread>>, u32>> paused_spus;
	};
public:

	bool Pause(bool freeze_emulation = false, bool show_resume_message = true);
	void Resume();
	void GracefulShutdown(bool allow_autoexit = true, bool async_op = false, bool savestate = false, bool continuous_mode = false);
	void Kill(bool allow_autoexit = true, bool savestate = false, savestate_stage* stage = nullptr);
	game_boot_result Restart(bool graceful = true);
	bool Quit(bool force_quit);
	static void CleanUp();

	bool IsRunning() const { return m_state == system_state::running; }
	bool IsPaused() const { system_state state = m_state; return state >= system_state::paused && state <= system_state::frozen; }
	bool IsPausedOrReady() const { return m_state >= system_state::paused; }
	bool IsStopped(bool test_fully = false) const { return test_fully ? m_state == system_state::stopped : m_state <= system_state::stopping; }
	bool IsReady()   const { return m_state == system_state::ready; }
	bool IsStarting() const { return m_state == system_state::starting; }
	auto GetStatus(bool fixup = true) const { system_state state = m_state; return fixup && state == system_state::frozen ? system_state::paused : fixup && state == system_state::stopping ? system_state::stopped : state; }

	bool HasGui() const { return m_has_gui; }
	void SetHasGui(bool has_gui) { m_has_gui = has_gui; }

	void SetDefaultRenderer(video_renderer renderer) { m_default_renderer = renderer; }
	void SetDefaultGraphicsAdapter(std::string adapter) { m_default_graphics_adapter = std::move(adapter); }

	std::string GetFormattedTitle(double fps) const;

	void ConfigurePPUCache() const;

	std::set<std::string> GetGameDirs() const;
	u32 AddGamesFromDir(const std::string& path);
	game_boot_result AddGame(const std::string& path);
	game_boot_result AddGameToYml(const std::string& path);
	u32 RemoveGamesFromDir(const std::string& games_dir, const std::vector<std::string>& serials_to_remove_from_yml = {}, bool save_on_disk = true);
	u32 RemoveGames(const std::vector<std::string>& title_id_list, bool save_on_disk = true);
	game_boot_result RemoveGameFromYml(const std::string& title_id);

	// Check if path is inside the specified directory
	bool IsPathInsideDir(std::string_view path, std::string_view dir) const;
	game_boot_result VerifyPathCasing(std::string_view path, std::string_view dir, bool from_dir) const;

	void EjectDisc();
	game_boot_result InsertDisc(const std::string& path);

	static game_boot_result GetElfPathFromDir(std::string& elf_path, const std::string& path);
	static void GetBdvdDir(std::string& bdvd_dir, std::string& sfb_dir, std::string& game_dir, const std::string& elf_dir);
	friend void init_fxo_for_exec(utils::serial*, bool);

	static bool IsVsh();
	static bool IsValidSfb(const std::string& path);

	static void SaveSettings(const std::string& settings, const std::string& title_id);
};

extern Emulator Emu;

extern bool g_use_rtm;
extern u64 g_rtm_tx_limit1;
extern u64 g_rtm_tx_limit2;
