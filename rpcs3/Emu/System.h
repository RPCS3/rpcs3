#pragma once

#include "VFS.h"
#include "Utilities/Atomic.h"
#include "Utilities/Config.h"
#include <functional>
#include <memory>
#include <string>

enum class system_type
{
	ps3,
	psv, // Experimental
	//psp, // Hypothetical
};

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

enum CellNetCtlState : s32;
enum CellSysutilLang : s32;

// Current process type
extern system_type g_system;

struct EmuCallbacks
{
	std::function<void(std::function<void()>)> call_after;
	std::function<void()> process_events;
	std::function<void()> on_run;
	std::function<void()> on_pause;
	std::function<void()> on_resume;
	std::function<void()> on_stop;
	std::function<void()> on_ready;
	std::function<void()> exit;
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

	const std::string& GetCachePath() const
	{
		return m_cache_path;
	}

	u64 GetPauseTime()
	{
		return m_pause_amend_time;
	}

	bool BootGame(const std::string& path, bool direct = false, bool add_only = false);
	bool InstallPkg(const std::string& path);

	static std::string GetHddDir();
	static std::string GetLibDir();

	void SetForceBoot(bool force_boot);

	void Load(bool add_only = false);
	void Run();
	bool Pause();
	void Resume();
	void Stop();

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
		node_core(cfg::node* _this) : cfg::node(_this, "Core") {}

		cfg::_enum<ppu_decoder_type> ppu_decoder{this, "PPU Decoder", ppu_decoder_type::llvm};
		cfg::_int<1, 16> ppu_threads{this, "PPU Threads", 2}; // Amount of PPU threads running simultaneously (must be 2)
		cfg::_bool ppu_debug{this, "PPU Debug"};
		cfg::_bool llvm_logs{this, "Save LLVM logs"};
		cfg::string llvm_cpu{this, "Use LLVM CPU"};

		cfg::_enum<spu_decoder_type> spu_decoder{this, "SPU Decoder", spu_decoder_type::asmjit};
		cfg::_bool bind_spu_cores{this, "Bind SPU threads to secondary cores"};
		cfg::_bool lower_spu_priority{this, "Lower SPU thread priority"};
		cfg::_bool spu_debug{this, "SPU Debug"};
		cfg::_int<0, 16384> max_spu_immediate_write_size{this, "Maximum immediate DMA write size", 16384}; // Maximum size that an SPU thread can write directly without posting to MFC
		cfg::_int<0, 6> preferred_spu_threads{this, "Preferred SPU Threads", 0}; //Numnber of hardware threads dedicated to heavy simultaneous spu tasks
		cfg::_int<0, 16> spu_delay_penalty{this, "SPU delay penalty", 3}; //Number of milliseconds to block a thread if a virtual 'core' isn't free
		cfg::_bool spu_loop_detection{this, "SPU loop detection", true}; //Try to detect wait loops and trigger thread yield

		cfg::_enum<lib_loading_type> lib_loading{this, "Lib Loader", lib_loading_type::liblv2only};
		cfg::_bool hook_functions{this, "Hook static functions"};
		cfg::set_entry load_libraries{this, "Load libraries"};

	} core{this};

	struct node_vfs : cfg::node
	{
		node_vfs(cfg::node* _this) : cfg::node(_this, "VFS") {}

		cfg::string emulator_dir{this, "$(EmulatorDir)"}; // Default (empty): taken from fs::get_config_dir()
		cfg::string dev_hdd0{this, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/"};
		cfg::string dev_hdd1{this, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/"};
		cfg::string dev_flash{this, "/dev_flash/", "$(EmulatorDir)dev_flash/"};
		cfg::string dev_usb000{this, "/dev_usb000/", "$(EmulatorDir)dev_usb000/"};
		cfg::string dev_bdvd{this, "/dev_bdvd/"}; // Not mounted
		cfg::string app_home{this, "/app_home/"}; // Not mounted

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
		cfg::_bool use_gpu_texture_scaling{this, "Use GPU texture scaling", true};
		cfg::_bool stretch_to_display_area{this, "Stretch To Display Area"};
		cfg::_bool force_high_precision_z_buffer{this, "Force High Precision Z buffer"};
		cfg::_bool strict_rendering_mode{this, "Strict Rendering Mode"};
		cfg::_bool disable_zcull_queries{this, "Disable ZCull Occlusion Queries", false};
		cfg::_bool disable_vertex_cache{this, "Disable Vertex Cache", false};
		cfg::_bool frame_skip_enabled{this, "Enable Frame Skip", false};
		cfg::_bool force_cpu_blit_processing{this, "Force CPU Blit", false}; //Debugging option
		cfg::_int<1, 8> consequtive_frames_to_draw{this, "Consecutive Frames To Draw", 1};
		cfg::_int<1, 8> consequtive_frames_to_skip{this, "Consecutive Frames To Skip", 1};
		cfg::_int<50, 800> resolution_scale_percent{this, "Resolution Scale", 100};
		cfg::_int<0, 16> anisotropic_level_override{this, "Anisotropic Filter Override", 0};
		cfg::_int<1, 1024> min_scalable_dimension{this, "Minimum Scalable Dimension", 16};

		struct node_d3d12 : cfg::node
		{
			node_d3d12(cfg::node* _this) : cfg::node(_this, "D3D12") {}

			cfg::string adapter{this, "Adapter"};

		} d3d12{this};

		struct node_vk : cfg::node
		{
			node_vk(cfg::node* _this) : cfg::node(_this, "Vulkan") {}

			cfg::string adapter{this, "Adapter"};

		} vk{this};

	} video{this};

	struct node_audio : cfg::node
	{
		node_audio(cfg::node* _this) : cfg::node(_this, "Audio") {}

		cfg::_enum<audio_renderer> renderer{this, "Renderer", static_cast<audio_renderer>(1)};

		cfg::_bool dump_to_file{this, "Dump to file"};
		cfg::_bool convert_to_u16{this, "Convert to 16 bit"};
		cfg::_bool downmix_to_2ch{this, "Downmix to Stereo", true};
		cfg::_int<2, 128> frames{this, "Buffer Count", 32};

	} audio{this};

	struct node_io : cfg::node
	{
		node_io(cfg::node* _this) : cfg::node(_this, "Input/Output") {}

		cfg::_enum<keyboard_handler> keyboard{this, "Keyboard", keyboard_handler::null};
		cfg::_enum<mouse_handler> mouse{this, "Mouse", mouse_handler::basic};
		cfg::_enum<pad_handler> pad{this, "Pad", pad_handler::keyboard};
		cfg::_enum<camera_handler> camera{this, "Camera", camera_handler::null};
		cfg::_enum<fake_camera_type> camera_type{this, "Camera type", fake_camera_type::unknown};

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
		cfg::_int<1, 65535> gdb_server_port{this, "Port", 2345};

	} misc{this};

	cfg::log_entry log{this, "Log"};
};

extern cfg_root g_cfg;
