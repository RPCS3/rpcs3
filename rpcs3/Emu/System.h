#pragma once

#include "VFS.h"
#include "util/atomic.hpp"
#include "Utilities/Config.h"
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

enum class ppu_decoder_type
{
	precise,
	fast,
	llvm,
};

enum class spu_decoder_type
{
	precise,
	fast,
	asmjit,
	llvm,
};

enum class spu_block_size_type
{
	safe,
	mega,
	giga,
};

enum class lib_loading_type
{
	manual,
	hybrid,
	liblv2only,
	liblv2both,
	liblv2list,
};

enum sleep_timers_accuracy_level : u32
{
	_as_host = 0,
	_usleep,
	_all_timers,
};

enum class keyboard_handler
{
	null,
	basic,
};

enum class mouse_handler
{
	null,
	basic,
};

enum class pad_handler
{
	null,
	keyboard,
	ds3,
	ds4,
#ifdef _WIN32
	xinput,
	mm,
#endif
#ifdef HAVE_LIBEVDEV
	evdev,
#endif
};

enum class video_renderer
{
	null,
	opengl,
	vulkan,
};

enum class audio_renderer
{
	null,
#ifdef _WIN32
	xaudio,
#endif
#ifdef HAVE_ALSA
	alsa,
#endif
	openal,
#ifdef HAVE_PULSE
	pulse,
#endif
#ifdef HAVE_FAUDIO
	faudio,
#endif
};

enum class camera_handler
{
	null,
	fake,
};

enum class fake_camera_type
{
	unknown,
	eyetoy,
	eyetoy2,
	uvc1_1,
};

enum class move_handler
{
	null,
	fake,
	mouse,
};

enum class microphone_handler
{
	null,
	standard,
	singstar,
	real_singstar,
	rocksmith,
};

enum class video_resolution
{
	_1080,
	_720,
	_480,
	_576,
	_1600x1080,
	_1440x1080,
	_1280x1080,
	_960x1080,
};

enum class video_aspect
{
	_4_3,
	_16_9,
};

enum class frame_limit_type
{
	none,
	_59_94,
	_50,
	_60,
	_30,
	_auto,
};

enum class msaa_level
{
	none,
	_auto
};

enum class detail_level
{
	minimal,
	low,
	medium,
	high,
};

enum class screen_quadrant
{
	top_left,
	top_right,
	bottom_left,
	bottom_right
};

enum class tsx_usage
{
	disabled,
	enabled,
	forced,
};

enum enter_button_assign
{
	circle = 0, // CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CIRCLE
	cross  = 1  // CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS
};

enum CellNetCtlState : s32;
enum CellSysutilLang : s32;
enum CellKbMappingType : s32;

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

	bool BootGame(const std::string& path, const std::string& title_id = "", bool direct = false, bool add_only = false, bool force_global_config = false);
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

	void Load(const std::string& title_id = "", bool add_only = false, bool force_global_config = false);
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
};

extern Emulator Emu;

struct cfg_root : cfg::node
{
	struct node_core : cfg::node
	{
		static constexpr bool thread_scheduler_enabled_def =
#ifdef _WIN32
			true;
#else
			false;
#endif

		node_core(cfg::node* _this) : cfg::node(_this, "Core") {}

		cfg::_enum<ppu_decoder_type> ppu_decoder{this, "PPU Decoder", ppu_decoder_type::llvm};
		cfg::_int<1, 8> ppu_threads{this, "PPU Threads", 2}; // Amount of PPU threads running simultaneously (must be 2)
		cfg::_bool ppu_debug{this, "PPU Debug"};
		cfg::_bool llvm_logs{this, "Save LLVM logs"};
		cfg::string llvm_cpu{this, "Use LLVM CPU"};
		cfg::_int<0, INT32_MAX> llvm_threads{this, "Max LLVM Compile Threads", 0};
		cfg::_bool thread_scheduler_enabled{this, "Enable thread scheduler", thread_scheduler_enabled_def};
		cfg::_bool set_daz_and_ftz{this, "Set DAZ and FTZ", false};
		cfg::_enum<spu_decoder_type> spu_decoder{this, "SPU Decoder", spu_decoder_type::llvm};
		cfg::_bool lower_spu_priority{this, "Lower SPU thread priority"};
		cfg::_bool spu_debug{this, "SPU Debug"};
		cfg::_int<0, 6> preferred_spu_threads{this, "Preferred SPU Threads", 0, true}; //Numnber of hardware threads dedicated to heavy simultaneous spu tasks
		cfg::_int<0, 16> spu_delay_penalty{this, "SPU delay penalty", 3}; //Number of milliseconds to block a thread if a virtual 'core' isn't free
		cfg::_bool spu_loop_detection{this, "SPU loop detection", true}; //Try to detect wait loops and trigger thread yield
		cfg::_int<0, 6> max_spurs_threads{this, "Max SPURS Threads", 6}; // HACK. If less then 6, max number of running SPURS threads in each thread group.
		cfg::_enum<spu_block_size_type> spu_block_size{this, "SPU Block Size", spu_block_size_type::safe};
		cfg::_bool spu_accurate_getllar{this, "Accurate GETLLAR", false};
		cfg::_bool spu_accurate_putlluc{this, "Accurate PUTLLUC", false};
		cfg::_bool spu_verification{this, "SPU Verification", true}; // Should be enabled
		cfg::_bool spu_cache{this, "SPU Cache", true};
		cfg::_bool spu_prof{this, "SPU Profiler", false};
		cfg::_enum<tsx_usage> enable_TSX{this, "Enable TSX", tsx_usage::enabled}; // Enable TSX. Forcing this on Haswell/Broadwell CPUs should be used carefully
		cfg::_bool spu_accurate_xfloat{this, "Accurate xfloat", false};
		cfg::_bool spu_approx_xfloat{this, "Approximate xfloat", true};

		cfg::_bool debug_console_mode{this, "Debug Console Mode", false}; // Debug console emulation, not recommended
		cfg::_enum<lib_loading_type> lib_loading{this, "Lib Loader", lib_loading_type::liblv2only};
		cfg::_bool hook_functions{this, "Hook static functions"};
		cfg::set_entry load_libraries{this, "Load libraries"};
		cfg::_bool hle_lwmutex{this, "HLE lwmutex"}; // Force alternative lwmutex/lwcond implementation

		cfg::_int<10, 3000> clocks_scale{this, "Clocks scale", 100, true}; // Changing this from 100 (percentage) may affect game speed in unexpected ways
		cfg::_enum<sleep_timers_accuracy_level> sleep_timers_accuracy{this, "Sleep Timers Accuracy",
#ifdef __linux__
		sleep_timers_accuracy_level::_as_host, true};
#else
		sleep_timers_accuracy_level::_usleep, true};
#endif
	} core{this};

	struct node_vfs : cfg::node
	{
		node_vfs(cfg::node* _this) : cfg::node(_this, "VFS") {}

		std::string get(const cfg::string&, const char*) const;

		cfg::string emulator_dir{this, "$(EmulatorDir)"}; // Default (empty): taken from fs::get_config_dir()
		cfg::string dev_hdd0{this, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/"};
		cfg::string dev_hdd1{this, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/"};
		cfg::string dev_flash{this, "/dev_flash/"};
		cfg::string dev_usb000{this, "/dev_usb000/", "$(EmulatorDir)dev_usb000/"};
		cfg::string dev_bdvd{this, "/dev_bdvd/"}; // Not mounted
		cfg::string app_home{this, "/app_home/"}; // Not mounted

		std::string get_dev_flash() const
		{
			return get(dev_flash, "dev_flash/");
		}

		cfg::_bool host_root{this, "Enable /host_root/"};
		cfg::_bool init_dirs{this, "Initialize Directories", true};

		cfg::_bool limit_cache_size{this, "Limit disk cache size", false};
		cfg::_int<0, 10240> cache_max_size{this, "Disk cache maximum size (MB)", 5120};

	} vfs{this};

	struct node_video : cfg::node
	{
		node_video(cfg::node* _this) : cfg::node(_this, "Video") {}

		cfg::_enum<video_renderer> renderer{this, "Renderer", video_renderer::opengl};

		cfg::_enum<video_resolution> resolution{this, "Resolution", video_resolution::_720};
		cfg::_enum<video_aspect> aspect_ratio{this, "Aspect ratio", video_aspect::_16_9};
		cfg::_enum<frame_limit_type> frame_limit{this, "Frame limit", frame_limit_type::none};
		cfg::_enum<msaa_level> antialiasing_level{this, "MSAA", msaa_level::_auto};

		cfg::_bool write_color_buffers{this, "Write Color Buffers"};
		cfg::_bool write_depth_buffer{this, "Write Depth Buffer"};
		cfg::_bool read_color_buffers{this, "Read Color Buffers"};
		cfg::_bool read_depth_buffer{this, "Read Depth Buffer"};
		cfg::_bool log_programs{this, "Log shader programs"};
		cfg::_bool vsync{this, "VSync"};
		cfg::_bool debug_output{this, "Debug output"};
		cfg::_bool overlay{this, "Debug overlay"};
		cfg::_bool gl_legacy_buffers{this, "Use Legacy OpenGL Buffers"};
		cfg::_bool use_gpu_texture_scaling{this, "Use GPU texture scaling", false};
		cfg::_bool stretch_to_display_area{this, "Stretch To Display Area"};
		cfg::_bool force_high_precision_z_buffer{this, "Force High Precision Z buffer"};
		cfg::_bool strict_rendering_mode{this, "Strict Rendering Mode"};
		cfg::_bool disable_zcull_queries{this, "Disable ZCull Occlusion Queries", false};
		cfg::_bool disable_vertex_cache{this, "Disable Vertex Cache", false};
		cfg::_bool disable_FIFO_reordering{this, "Disable FIFO Reordering", false};
		cfg::_bool frame_skip_enabled{this, "Enable Frame Skip", false};
		cfg::_bool force_cpu_blit_processing{this, "Force CPU Blit", false}; // Debugging option
		cfg::_bool disable_on_disk_shader_cache{this, "Disable On-Disk Shader Cache", false};
		cfg::_bool disable_vulkan_mem_allocator{this, "Disable Vulkan Memory Allocator", false};
		cfg::_bool full_rgb_range_output{this, "Use full RGB output range", true}; // Video out dynamic range
		cfg::_bool disable_asynchronous_shader_compiler{this, "Disable Asynchronous Shader Compiler", false};
		cfg::_bool strict_texture_flushing{this, "Strict Texture Flushing", false};
		cfg::_bool disable_native_float16{this, "Disable native float16 support", false};
		cfg::_bool multithreaded_rsx{this, "Multithreaded RSX", false};
		cfg::_bool relaxed_zcull_sync{this, "Relaxed ZCULL Sync", false};
		cfg::_int<1, 8> consequtive_frames_to_draw{this, "Consecutive Frames To Draw", 1};
		cfg::_int<1, 8> consequtive_frames_to_skip{this, "Consecutive Frames To Skip", 1};
		cfg::_int<50, 800> resolution_scale_percent{this, "Resolution Scale", 100};
		cfg::_int<0, 16> anisotropic_level_override{this, "Anisotropic Filter Override", 0};
		cfg::_int<1, 1024> min_scalable_dimension{this, "Minimum Scalable Dimension", 16};
		cfg::_int<0, 30000000> driver_recovery_timeout{this, "Driver Recovery Timeout", 1000000, true};
		cfg::_int<0, 16667> driver_wakeup_delay{this, "Driver Wake-Up Delay", 1, true};
		cfg::_int<1, 1800> vblank_rate{this, "Vblank Rate", 60, true}; // Changing this from 60 may affect game speed in unexpected ways

		struct node_vk : cfg::node
		{
			node_vk(cfg::node* _this) : cfg::node(_this, "Vulkan") {}

			cfg::string adapter{this, "Adapter"};
			cfg::_bool force_fifo{this, "Force FIFO present mode"};
			cfg::_bool force_primitive_restart{this, "Force primitive restart flag"};

		} vk{this};

		struct node_perf_overlay : cfg::node
		{
			node_perf_overlay(cfg::node* _this) : cfg::node(_this, "Performance Overlay") {}

			cfg::_bool perf_overlay_enabled{ this, "Enabled", false, true };
			cfg::_bool framerate_graph_enabled{ this, "Enable Framerate Graph", false, true };
			cfg::_bool frametime_graph_enabled{ this, "Enable Frametime Graph", false, true };
			cfg::_enum<detail_level> level{ this, "Detail level", detail_level::medium, true };
			cfg::_int<30, 5000> update_interval{ this, "Metrics update interval (ms)", 350, true };
			cfg::_int<4, 36> font_size{ this, "Font size (px)", 10, true };
			cfg::_enum<screen_quadrant> position{ this, "Position", screen_quadrant::top_left, true };
			cfg::string font{ this, "Font", "n023055ms.ttf", true };
			cfg::_int<0, 1280> margin_x{ this, "Horizontal Margin (px)", 50, true }; // horizontal distance to the screen border relative to the screen_quadrant in px
			cfg::_int<0, 720> margin_y{ this, "Vertical Margin (px)", 50, true }; // vertical distance to the screen border relative to the screen_quadrant in px
			cfg::_bool center_x{ this, "Center Horizontally", false, true };
			cfg::_bool center_y{ this, "Center Vertically", false, true };
			cfg::_int<0, 100> opacity{ this, "Opacity (%)", 70, true };
			cfg::string color_body{ this, "Body Color (hex)", "#FFE138FF", true };
			cfg::string background_body{ this, "Body Background (hex)", "#002339FF", true };
			cfg::string color_title{ this, "Title Color (hex)", "#F26C24FF", true };
			cfg::string background_title{ this, "Title Background (hex)", "#00000000", true };

		} perf_overlay{this};

		struct node_shader_compilation_hint : cfg::node
		{
			node_shader_compilation_hint(cfg::node* _this) : cfg::node(_this, "Shader Compilation Hint") {}

			cfg::_int<0, 1280> pos_x{ this, "Position X (px)", 20, true }; // horizontal position starting from the upper border in px
			cfg::_int<0, 720> pos_y{ this, "Position Y (px)", 690, true }; // vertical position starting from the left border in px

		} shader_compilation_hint{this};

		struct node_shader_preloading_dialog : cfg::node
		{
			node_shader_preloading_dialog(cfg::node* _this) : cfg::node(_this, "Shader Loading Dialog"){}

			cfg::_bool use_custom_background{ this, "Allow custom background", true, true };
			cfg::_int<0, 100> darkening_strength{ this, "Darkening effect strength", 30, true };
			cfg::_int<0, 100> blur_strength{ this, "Blur effect strength", 0, true };

		} shader_preloading_dialog{this};

	} video{this};

	struct node_audio : cfg::node
	{
		node_audio(cfg::node* _this) : cfg::node(_this, "Audio") {}

		cfg::_enum<audio_renderer> renderer{this, "Renderer", static_cast<audio_renderer>(1)};

		cfg::_bool dump_to_file{this, "Dump to file"};
		cfg::_bool convert_to_u16{this, "Convert to 16 bit"};
		cfg::_bool downmix_to_2ch{this, "Downmix to Stereo", true};
		cfg::_int<1, 128> startt{this, "Start Threshold", 1}; // TODO: used only by ALSA, should probably be removed once ALSA is upgraded
		cfg::_int<0, 200> volume{this, "Master Volume", 100};
		cfg::_bool enable_buffering{this, "Enable Buffering", true};
		cfg::_int <20, 250> desired_buffer_duration{this, "Desired Audio Buffer Duration", 100};
		cfg::_int<1, 1000> sampling_period_multiplier{this, "Sampling Period Multiplier", 100};
		cfg::_bool enable_time_stretching{this, "Enable Time Stretching", false};
		cfg::_int<0, 100> time_stretching_threshold{this, "Time Stretching Threshold", 75};
		cfg::_enum<microphone_handler> microphone_type{ this, "Microphone Type", microphone_handler::null };
		cfg::string microphone_devices{ this, "Microphone Devices", ";;;;" };
	} audio{this};

	struct node_io : cfg::node
	{
		node_io(cfg::node* _this) : cfg::node(_this, "Input/Output") {}

		cfg::_enum<keyboard_handler> keyboard{this, "Keyboard", keyboard_handler::null};
		cfg::_enum<mouse_handler> mouse{this, "Mouse", mouse_handler::basic};
		cfg::_enum<camera_handler> camera{this, "Camera", camera_handler::null};
		cfg::_enum<fake_camera_type> camera_type{this, "Camera type", fake_camera_type::unknown};
		cfg::_enum<move_handler> move{this, "Move", move_handler::null};
	} io{this};

	struct node_sys : cfg::node
	{
		node_sys(cfg::node* _this) : cfg::node(_this, "System") {}

		cfg::_enum<CellSysutilLang> language{this, "Language", CellSysutilLang{1}}; // CELL_SYSUTIL_LANG_ENGLISH_US
		cfg::_enum<CellKbMappingType> keyboard_type{this, "Keyboard Type", CellKbMappingType{0}}; // CELL_KB_MAPPING_101 = US
		cfg::_enum<enter_button_assign> enter_button_assignment{this, "Enter button assignment", enter_button_assign::cross};

	} sys{this};

	struct node_net : cfg::node
	{
		node_net(cfg::node* _this) : cfg::node(_this, "Net") {}

		cfg::_enum<CellNetCtlState> net_status{this, "Connection status"};
		cfg::string ip_address{this, "IP address", "192.168.1.1"};

	} net{this};

	struct node_misc : cfg::node
	{
		node_misc(cfg::node* _this) : cfg::node(_this, "Miscellaneous") {}

		cfg::_bool autostart{ this, "Automatically start games after boot", true, true };
		cfg::_bool autoexit{ this, "Exit RPCS3 when process finishes", false, true };
		cfg::_bool start_fullscreen{ this, "Start games in fullscreen mode", false, true };
		cfg::_bool prevent_display_sleep{ this, "Prevent display sleep while running games", true};
		cfg::_bool show_fps_in_title{ this, "Show FPS counter in window title", true, true };
		cfg::_bool show_trophy_popups{ this, "Show trophy popups", true, true };
		cfg::_bool show_shader_compilation_hint{ this, "Show shader compilation hint", true, true };
		cfg::_bool use_native_interface{ this, "Use native user interface", true };
		cfg::string gdb_server{this, "GDB Server", "127.0.0.1:2345"};
		cfg::_bool silence_all_logs{this, "Silence All Logs", false, false};

	} misc{this};

	cfg::log_entry log{this, "Log"};

	std::string name;
};

extern cfg_root g_cfg;

extern bool g_use_rtm;
