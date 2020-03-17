#pragma once

#include "system_config_types.h"
#include "Utilities/Config.h"

enum CellNetCtlState : s32;
enum CellSysutilLang : s32;
enum CellKbMappingType : s32;

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

		cfg::_enum<ppu_decoder_type> ppu_decoder{ this, "PPU Decoder", ppu_decoder_type::llvm };
		cfg::_int<1, 8> ppu_threads{ this, "PPU Threads", 2 }; // Amount of PPU threads running simultaneously (must be 2)
		cfg::_bool ppu_debug{ this, "PPU Debug" };
		cfg::_bool llvm_logs{ this, "Save LLVM logs" };
		cfg::string llvm_cpu{ this, "Use LLVM CPU" };
		cfg::_int<0, INT32_MAX> llvm_threads{ this, "Max LLVM Compile Threads", 0 };
		cfg::_bool thread_scheduler_enabled{ this, "Enable thread scheduler", thread_scheduler_enabled_def };
		cfg::_bool set_daz_and_ftz{ this, "Set DAZ and FTZ", false };
		cfg::_enum<spu_decoder_type> spu_decoder{ this, "SPU Decoder", spu_decoder_type::llvm };
		cfg::_bool lower_spu_priority{ this, "Lower SPU thread priority" };
		cfg::_bool spu_debug{ this, "SPU Debug" };
		cfg::_int<0, 6> preferred_spu_threads{ this, "Preferred SPU Threads", 0, true }; // Number of hardware threads dedicated to heavy simultaneous spu tasks
		cfg::_int<0, 16> spu_delay_penalty{ this, "SPU delay penalty", 3 }; // Number of milliseconds to block a thread if a virtual 'core' isn't free
		cfg::_bool spu_loop_detection{ this, "SPU loop detection", true }; // Try to detect wait loops and trigger thread yield
		cfg::_int<0, 6> max_spurs_threads{ this, "Max SPURS Threads", 6 }; // HACK. If less then 6, max number of running SPURS threads in each thread group.
		cfg::_enum<spu_block_size_type> spu_block_size{ this, "SPU Block Size", spu_block_size_type::safe };
		cfg::_bool spu_accurate_getllar{ this, "Accurate GETLLAR", false };
		cfg::_bool spu_accurate_putlluc{ this, "Accurate PUTLLUC", false };
		cfg::_bool rsx_accurate_res_access{this, "Accurate RSX reservation access", false, true};
		cfg::_bool spu_verification{ this, "SPU Verification", true }; // Should be enabled
		cfg::_bool spu_cache{ this, "SPU Cache", true };
		cfg::_bool spu_prof{ this, "SPU Profiler", false };
		cfg::_enum<tsx_usage> enable_TSX{ this, "Enable TSX", tsx_usage::enabled }; // Enable TSX. Forcing this on Haswell/Broadwell CPUs should be used carefully
		cfg::_bool spu_accurate_xfloat{ this, "Accurate xfloat", false };
		cfg::_bool spu_approx_xfloat{ this, "Approximate xfloat", true };

		cfg::_bool debug_console_mode{ this, "Debug Console Mode", false }; // Debug console emulation, not recommended
		cfg::_enum<lib_loading_type> lib_loading{ this, "Lib Loader", lib_loading_type::liblv2only };
		cfg::_bool hook_functions{ this, "Hook static functions" };
		cfg::set_entry load_libraries{ this, "Load libraries" };
		cfg::_bool hle_lwmutex{ this, "HLE lwmutex" }; // Force alternative lwmutex/lwcond implementation

		cfg::_int<10, 3000> clocks_scale{ this, "Clocks scale", 100, true }; // Changing this from 100 (percentage) may affect game speed in unexpected ways
		cfg::_enum<sleep_timers_accuracy_level> sleep_timers_accuracy{ this, "Sleep Timers Accuracy",
#ifdef __linux__
			sleep_timers_accuracy_level::_as_host, true };
#else
			sleep_timers_accuracy_level::_usleep, true };
#endif
	} core{ this };

	struct node_vfs : cfg::node
	{
		node_vfs(cfg::node* _this) : cfg::node(_this, "VFS") {}

		std::string get(const cfg::string&, const char*) const;

		cfg::string emulator_dir{ this, "$(EmulatorDir)" }; // Default (empty): taken from fs::get_config_dir()
		cfg::string dev_hdd0{ this, "/dev_hdd0/", "$(EmulatorDir)dev_hdd0/" };
		cfg::string dev_hdd1{ this, "/dev_hdd1/", "$(EmulatorDir)dev_hdd1/" };
		cfg::string dev_flash{ this, "/dev_flash/", "$(EmulatorDir)dev_flash/" };
		cfg::string dev_usb000{ this, "/dev_usb000/", "$(EmulatorDir)dev_usb000/" };
		cfg::string dev_bdvd{ this, "/dev_bdvd/" }; // Not mounted
		cfg::string app_home{ this, "/app_home/" }; // Not mounted
	
		std::string get_dev_flash() const
		{
			return get(dev_flash, "dev_flash/");
		}
	
		cfg::_bool host_root{ this, "Enable /host_root/" };
		cfg::_bool init_dirs{ this, "Initialize Directories", true };

		cfg::_bool limit_cache_size{ this, "Limit disk cache size", false };
		cfg::_int<0, 10240> cache_max_size{ this, "Disk cache maximum size (MB)", 5120 };

	} vfs{ this };

	struct node_video : cfg::node
	{
		node_video(cfg::node* _this) : cfg::node(_this, "Video") {}

		cfg::_enum<video_renderer> renderer{ this, "Renderer", video_renderer::opengl };

		cfg::_enum<video_resolution> resolution{ this, "Resolution", video_resolution::_720 };
		cfg::_enum<video_aspect> aspect_ratio{ this, "Aspect ratio", video_aspect::_16_9 };
		cfg::_enum<frame_limit_type> frame_limit{ this, "Frame limit", frame_limit_type::none, true };
		cfg::_enum<msaa_level> antialiasing_level{ this, "MSAA", msaa_level::_auto };

		cfg::_bool write_color_buffers{ this, "Write Color Buffers" };
		cfg::_bool write_depth_buffer{ this, "Write Depth Buffer" };
		cfg::_bool read_color_buffers{ this, "Read Color Buffers" };
		cfg::_bool read_depth_buffer{ this, "Read Depth Buffer" };
		cfg::_bool log_programs{ this, "Log shader programs" };
		cfg::_bool vsync{ this, "VSync" };
		cfg::_bool debug_output{ this, "Debug output" };
		cfg::_bool overlay{ this, "Debug overlay" };
		cfg::_bool gl_legacy_buffers{ this, "Use Legacy OpenGL Buffers" };
		cfg::_bool use_gpu_texture_scaling{ this, "Use GPU texture scaling", false };
		cfg::_bool stretch_to_display_area{ this, "Stretch To Display Area", false, true };
		cfg::_bool force_high_precision_z_buffer{ this, "Force High Precision Z buffer" };
		cfg::_bool strict_rendering_mode{ this, "Strict Rendering Mode" };
		cfg::_bool disable_zcull_queries{ this, "Disable ZCull Occlusion Queries", false };
		cfg::_bool disable_vertex_cache{ this, "Disable Vertex Cache", false };
		cfg::_bool disable_FIFO_reordering{ this, "Disable FIFO Reordering", false };
		cfg::_bool frame_skip_enabled{ this, "Enable Frame Skip", false };
		cfg::_bool force_cpu_blit_processing{ this, "Force CPU Blit", false, true }; // Debugging option
		cfg::_bool disable_on_disk_shader_cache{ this, "Disable On-Disk Shader Cache", false };
		cfg::_bool disable_vulkan_mem_allocator{ this, "Disable Vulkan Memory Allocator", false };
		cfg::_bool full_rgb_range_output{ this, "Use full RGB output range", true }; // Video out dynamic range
		cfg::_bool disable_asynchronous_shader_compiler{ this, "Disable Asynchronous Shader Compiler", false };
		cfg::_bool strict_texture_flushing{ this, "Strict Texture Flushing", false };
		cfg::_bool disable_native_float16{ this, "Disable native float16 support", false };
		cfg::_bool multithreaded_rsx{ this, "Multithreaded RSX", false };
		cfg::_bool relaxed_zcull_sync{ this, "Relaxed ZCULL Sync", false };
		cfg::_bool enable_3d{ this, "Enable 3D", false };
		cfg::_int<1, 8> consequtive_frames_to_draw{ this, "Consecutive Frames To Draw", 1 };
		cfg::_int<1, 8> consequtive_frames_to_skip{ this, "Consecutive Frames To Skip", 1 };
		cfg::_int<50, 800> resolution_scale_percent{ this, "Resolution Scale", 100 };
		cfg::_int<0, 16> anisotropic_level_override{ this, "Anisotropic Filter Override", 0 };
		cfg::_int<1, 1024> min_scalable_dimension{ this, "Minimum Scalable Dimension", 16 };
		cfg::_int<0, 30000000> driver_recovery_timeout{ this, "Driver Recovery Timeout", 1000000, true };
		cfg::_int<0, 16667> driver_wakeup_delay{ this, "Driver Wake-Up Delay", 1, true };
		cfg::_int<1, 1800> vblank_rate{ this, "Vblank Rate", 60, true }; // Changing this from 60 may affect game speed in unexpected ways

		struct node_vk : cfg::node
		{
			node_vk(cfg::node* _this) : cfg::node(_this, "Vulkan") {}

			cfg::string adapter{ this, "Adapter" };
			cfg::_bool force_fifo{ this, "Force FIFO present mode" };
			cfg::_bool force_primitive_restart{ this, "Force primitive restart flag" };

		} vk{ this };

		struct node_perf_overlay : cfg::node
		{
			node_perf_overlay(cfg::node* _this) : cfg::node(_this, "Performance Overlay") {}

			cfg::_bool perf_overlay_enabled{ this, "Enabled", false, true };
			cfg::_bool framerate_graph_enabled{ this, "Enable Framerate Graph", false, true };
			cfg::_bool frametime_graph_enabled{ this, "Enable Frametime Graph", false, true };
			cfg::_enum<detail_level> level{ this, "Detail level", detail_level::medium, true };
			cfg::_int<1, 5000> update_interval{ this, "Metrics update interval (ms)", 350, true };
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

		} perf_overlay{ this };

		struct node_shader_compilation_hint : cfg::node
		{
			node_shader_compilation_hint(cfg::node* _this) : cfg::node(_this, "Shader Compilation Hint") {}

			cfg::_int<0, 1280> pos_x{ this, "Position X (px)", 20, true }; // horizontal position starting from the upper border in px
			cfg::_int<0, 720> pos_y{ this, "Position Y (px)", 690, true }; // vertical position starting from the left border in px

		} shader_compilation_hint{ this };

		struct node_shader_preloading_dialog : cfg::node
		{
			node_shader_preloading_dialog(cfg::node* _this) : cfg::node(_this, "Shader Loading Dialog") {}

			cfg::_bool use_custom_background{ this, "Allow custom background", true, true };
			cfg::_int<0, 100> darkening_strength{ this, "Darkening effect strength", 30, true };
			cfg::_int<0, 100> blur_strength{ this, "Blur effect strength", 0, true };

		} shader_preloading_dialog{ this };

	} video{ this };

	struct node_audio : cfg::node
	{
		node_audio(cfg::node* _this) : cfg::node(_this, "Audio") {}

		cfg::_enum<audio_renderer> renderer{ this, "Renderer",
#ifdef _WIN32
			audio_renderer::xaudio };
#elif HAVE_FAUDIO
			audio_renderer::faudio };
#else
			audio_renderer::openal };
#endif

		cfg::_bool dump_to_file{ this, "Dump to file" };
		cfg::_bool convert_to_u16{ this, "Convert to 16 bit" };
		cfg::_bool downmix_to_2ch{ this, "Downmix to Stereo", true };
		cfg::_int<1, 128> startt{ this, "Start Threshold", 1 }; // TODO: used only by ALSA, should probably be removed once ALSA is upgraded
		cfg::_int<0, 200> volume{ this, "Master Volume", 100 };
		cfg::_bool enable_buffering{ this, "Enable Buffering", true };
		cfg::_int <20, 250> desired_buffer_duration{ this, "Desired Audio Buffer Duration", 100 };
		cfg::_int<1, 1000> sampling_period_multiplier{ this, "Sampling Period Multiplier", 100 };
		cfg::_bool enable_time_stretching{ this, "Enable Time Stretching", false };
		cfg::_int<0, 100> time_stretching_threshold{ this, "Time Stretching Threshold", 75 };
		cfg::_enum<microphone_handler> microphone_type{ this, "Microphone Type", microphone_handler::null };
		cfg::string microphone_devices{ this, "Microphone Devices", ";;;;" };
	} audio{ this };

	struct node_io : cfg::node
	{
		node_io(cfg::node* _this) : cfg::node(_this, "Input/Output") {}

		cfg::_enum<keyboard_handler> keyboard{ this, "Keyboard", keyboard_handler::null };
		cfg::_enum<mouse_handler> mouse{ this, "Mouse", mouse_handler::basic };
		cfg::_enum<camera_handler> camera{ this, "Camera", camera_handler::null };
		cfg::_enum<fake_camera_type> camera_type{ this, "Camera type", fake_camera_type::unknown };
		cfg::_enum<move_handler> move{ this, "Move", move_handler::null };
	} io{ this };

	struct node_sys : cfg::node
	{
		node_sys(cfg::node* _this) : cfg::node(_this, "System") {}

		cfg::_enum<CellSysutilLang> language{ this, "Language", CellSysutilLang{1} }; // CELL_SYSUTIL_LANG_ENGLISH_US
		cfg::_enum<CellKbMappingType> keyboard_type{ this, "Keyboard Type", CellKbMappingType{0} }; // CELL_KB_MAPPING_101 = US
		cfg::_enum<enter_button_assign> enter_button_assignment{ this, "Enter button assignment", enter_button_assign::cross };

	} sys{ this };

	struct node_net : cfg::node
	{
		node_net(cfg::node* _this) : cfg::node(_this, "Net") {}

		cfg::_enum<np_internet_status> net_active{this, "Internet enabled", np_internet_status::disabled};
		cfg::string ip_address{this, "IP address", "0.0.0.0"};
		cfg::string dns{this, "DNS address", "8.8.8.8"};
		cfg::string swap_list{this, "IP swap list", ""};

		cfg::_enum<np_psn_status> psn_status{this, "PSN status", np_psn_status::disabled};
		cfg::string psn_npid{this, "NPID", ""};
	} net{this};
	
	struct node_misc : cfg::node
	{
		node_misc(cfg::node* _this) : cfg::node(_this, "Miscellaneous") {}

		cfg::_bool autostart{ this, "Automatically start games after boot", true, true };
		cfg::_bool autoexit{ this, "Exit RPCS3 when process finishes", false, true };
		cfg::_bool start_fullscreen{ this, "Start games in fullscreen mode", false, true };
		cfg::_bool prevent_display_sleep{ this, "Prevent display sleep while running games", true };
		cfg::_bool show_trophy_popups{ this, "Show trophy popups", true, true };
		cfg::_bool show_shader_compilation_hint{ this, "Show shader compilation hint", true, true };
		cfg::_bool use_native_interface{ this, "Use native user interface", true };
		cfg::string gdb_server{ this, "GDB Server", "127.0.0.1:2345" };
		cfg::_bool silence_all_logs{ this, "Silence All Logs", false, false };
		cfg::string title_format{ this, "Window Title Format", "FPS: %F | %R | %V | %T [%t]", true };
	
	} misc{ this };
	
	cfg::log_entry log{ this, "Log" };
	
	std::string name;
};

extern cfg_root g_cfg;
