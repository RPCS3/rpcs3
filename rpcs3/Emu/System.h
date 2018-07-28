#pragma once

#include "VFS.h"
#include "Utilities/Atomic.h"
#include "Utilities/Config.h"
#include <functional>
#include <memory>
#include <string>

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
	automatic,
	manual,
	both,
	liblv2only
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
	ds4,
#ifdef _MSC_VER
	xinput,
#endif
#ifdef _WIN32
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
#ifdef _MSC_VER
	dx12,
#endif
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
#ifdef HAVE_PULSE
	pulse,
#endif
	openal,
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
	_auto,
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

enum CellNetCtlState : s32;
enum CellSysutilLang : s32;

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_after;
	std::function<void()> on_run;
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<void()> exit;
	std::function<void(s32, s32)> handle_taskbar_progress; // (type, value) type: 0 for reset, 1 for increment, 2 for set_limit
	std::function<std::shared_ptr<class KeyboardHandlerBase>()> get_kb_handler;
	std::function<std::shared_ptr<class MouseHandlerBase>()> get_mouse_handler;
	std::function<std::shared_ptr<class pad_thread>()> get_pad_handler;
	std::function<std::unique_ptr<class GSFrameBase>()> get_gs_frame;
	std::function<std::shared_ptr<class GSRender>()> get_gs_render;
	std::function<std::shared_ptr<class AudioThread>()> get_audio;
	std::function<std::shared_ptr<class MsgDialogBase>()> get_msg_dialog;
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
	std::string m_cache_path;
	std::string m_title_id;
	std::string m_title;
	std::string m_cat;
	std::string m_dir;
	std::string m_usr{"00000001"};
	u32 m_usrid{1};

	bool m_force_boot = false;

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

	const std::string& GetCachePath() const
	{
		return m_cache_path;
	}

	const std::string& GetDir() const
	{
		return m_dir;
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

	u64 GetPauseTime()
	{
		return m_pause_amend_time;
	}

	bool BootGame(const std::string& path, bool direct = false, bool add_only = false);
	bool BootRsxCapture(const std::string& path);
	bool InstallPkg(const std::string& path);

private:
	static std::string GetEmuDir();
public:
	static std::string GetHddDir();

	void SetForceBoot(bool force_boot);

	void Load(bool add_only = false);
	void Run();
	bool Pause();
	void Resume();
	void Stop(bool restart = false);
	void Restart() { Stop(true); }

	bool IsRunning() const { return m_state == system_state::running; }
	bool IsPaused()  const { return m_state == system_state::paused; }
	bool IsStopped() const { return m_state == system_state::stopped; }
	bool IsReady()   const { return m_state == system_state::ready; }
	auto GetStatus() const { return m_state.load(); }
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
		cfg::_int<1, 16> ppu_threads{this, "PPU Threads", 2}; // Amount of PPU threads running simultaneously (must be 2)
		cfg::_bool ppu_debug{this, "PPU Debug"};
		cfg::_bool llvm_logs{this, "Save LLVM logs"};
		cfg::string llvm_cpu{this, "Use LLVM CPU"};
		cfg::_int<0, INT32_MAX> llvm_threads{this, "Max LLVM Compile Threads", 0};
		cfg::_bool thread_scheduler_enabled{this, "Enable thread scheduler", thread_scheduler_enabled_def};
		cfg::_bool set_daz_and_ftz{this, "Set DAZ and FTZ", false};
		cfg::_enum<spu_decoder_type> spu_decoder{this, "SPU Decoder", spu_decoder_type::asmjit};
		cfg::_bool lower_spu_priority{this, "Lower SPU thread priority"};
		cfg::_bool spu_debug{this, "SPU Debug"};
		cfg::_int<0, 6> preferred_spu_threads{this, "Preferred SPU Threads", 0}; //Numnber of hardware threads dedicated to heavy simultaneous spu tasks
		cfg::_int<0, 16> spu_delay_penalty{this, "SPU delay penalty", 3}; //Number of milliseconds to block a thread if a virtual 'core' isn't free
		cfg::_bool spu_loop_detection{this, "SPU loop detection", true}; //Try to detect wait loops and trigger thread yield
		cfg::_bool spu_shared_runtime{this, "SPU Shared Runtime", true}; // Share compiled SPU functions between all threads
		cfg::_enum<spu_block_size_type> spu_block_size{this, "SPU Block Size", spu_block_size_type::safe};
		cfg::_bool spu_accurate_getllar{this, "Accurate GETLLAR", false};
		cfg::_bool spu_accurate_putlluc{this, "Accurate PUTLLUC", false};
		cfg::_bool spu_verification{this, "SPU Verification", true}; // Should be enabled
		cfg::_bool spu_cache{this, "SPU Cache", true};
		cfg::_enum<tsx_usage> enable_TSX{this, "Enable TSX", tsx_usage::enabled}; // Enable TSX. Forcing this on Haswell/Broadwell CPUs should be used carefully

		cfg::_enum<lib_loading_type> lib_loading{this, "Lib Loader", lib_loading_type::liblv2only};
		cfg::_bool hook_functions{this, "Hook static functions"};
		cfg::set_entry load_libraries{this, "Load libraries"};

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

	} vfs{this};

	struct node_video : cfg::node
	{
		node_video(cfg::node* _this) : cfg::node(_this, "Video") {}

		cfg::_enum<video_renderer> renderer{this, "Renderer", video_renderer::opengl};

		cfg::_enum<video_resolution> resolution{this, "Resolution", video_resolution::_720};
		cfg::_enum<video_aspect> aspect_ratio{this, "Aspect ratio", video_aspect::_16_9};
		cfg::_enum<frame_limit_type> frame_limit{this, "Frame limit", frame_limit_type::none};

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
		cfg::_int<1, 8> consequtive_frames_to_draw{this, "Consecutive Frames To Draw", 1};
		cfg::_int<1, 8> consequtive_frames_to_skip{this, "Consecutive Frames To Skip", 1};
		cfg::_int<50, 800> resolution_scale_percent{this, "Resolution Scale", 100};
		cfg::_int<0, 16> anisotropic_level_override{this, "Anisotropic Filter Override", 0};
		cfg::_int<1, 1024> min_scalable_dimension{this, "Minimum Scalable Dimension", 16};
		cfg::_int<0, 30000000> driver_recovery_timeout{this, "Driver Recovery Timeout", 1000000};

		struct node_d3d12 : cfg::node
		{
			node_d3d12(cfg::node* _this) : cfg::node(_this, "D3D12") {}

			cfg::string adapter{this, "Adapter"};

		} d3d12{this};

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

			cfg::_bool perf_overlay_enabled{this, "Enabled", false};
			cfg::_enum<detail_level> level{this, "Detail level", detail_level::high};
			cfg::_int<30, 5000> update_interval{ this, "Metrics update interval (ms)", 350 };
			cfg::_int<4, 36> font_size{ this, "Font size (px)", 10 };
			cfg::_enum<screen_quadrant> position{this, "Position", screen_quadrant::top_left};
			cfg::string font{this, "Font", "n023055ms.ttf"};
			cfg::_int<0, 1280> margin_x{this, "Horizontal Margin (px)", 50}; // horizontal distance to the screen border relative to the screen_quadrant in px
			cfg::_int<0, 720> margin_y{this, "Vertical Margin (px)", 50}; // vertical distance to the screen border relative to the screen_quadrant in px
			cfg::_bool center_x{ this, "Center Horizontally", false };
			cfg::_bool center_y{ this, "Center Vertically", false };
			cfg::_int<0, 100> opacity{this, "Opacity (%)", 70};
			cfg::string color_body{ this, "Body Color (hex)", "#FFE138FF" };
			cfg::string background_body{ this, "Body Background (hex)", "#002339FF" };
			cfg::string color_title{ this, "Title Color (hex)", "#F26C24FF" };
			cfg::string background_title{ this, "Title Background (hex)", "#00000000" };

		} perf_overlay{this};

		struct node_shader_compilation_hint : cfg::node
		{
			node_shader_compilation_hint(cfg::node* _this) : cfg::node(_this, "Shader Compilation Hint") {}

			cfg::_int<0, 1280> pos_x{this, "Position X (px)", 20}; // horizontal position starting from the upper border in px
			cfg::_int<0, 720> pos_y{this, "Position Y (px)", 690}; // vertical position starting from the left border in px

		} shader_compilation_hint{this};

	} video{this};

	struct node_audio : cfg::node
	{
		node_audio(cfg::node* _this) : cfg::node(_this, "Audio") {}

		cfg::_enum<audio_renderer> renderer{this, "Renderer", static_cast<audio_renderer>(1)};

		cfg::_bool dump_to_file{this, "Dump to file"};
		cfg::_bool convert_to_u16{this, "Convert to 16 bit"};
		cfg::_bool downmix_to_2ch{this, "Downmix to Stereo", true};
		cfg::_int<2, 128> frames{this, "Buffer Count", 32};
		cfg::_int<1, 128> startt{this, "Start Threshold", 1};

	} audio{this};

	struct node_io : cfg::node
	{
		node_io(cfg::node* _this) : cfg::node(_this, "Input/Output") {}

		cfg::_enum<keyboard_handler> keyboard{this, "Keyboard", keyboard_handler::null};
		cfg::_enum<mouse_handler> mouse{this, "Mouse", mouse_handler::basic};
		cfg::_enum<pad_handler> pad{this, "Pad", pad_handler::keyboard};
		cfg::_enum<camera_handler> camera{this, "Camera", camera_handler::null};
		cfg::_enum<fake_camera_type> camera_type{this, "Camera type", fake_camera_type::unknown};
		cfg::_enum<move_handler> move{this, "Move", move_handler::null};

	} io{this};

	struct node_sys : cfg::node
	{
		node_sys(cfg::node* _this) : cfg::node(_this, "System") {}

		cfg::_enum<CellSysutilLang> language{this, "Language"};

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

		cfg::_bool autostart{this, "Automatically start games after boot", true};
		cfg::_bool autoexit{this, "Exit RPCS3 when process finishes"};
		cfg::_bool start_fullscreen{ this, "Start games in fullscreen mode" };
		cfg::_bool show_fps_in_title{ this, "Show FPS counter in window title", true};
		cfg::_bool show_trophy_popups{ this, "Show trophy popups", true};
		cfg::_bool show_shader_compilation_hint{ this, "Show shader compilation hint", true };
		cfg::_bool use_native_interface{ this, "Use native user interface", true };
		cfg::_int<1, 65535> gdb_server_port{this, "Port", 2345};

	} misc{this};

	cfg::log_entry log{this, "Log"};
};

extern cfg_root g_cfg;

extern bool g_use_rtm;
