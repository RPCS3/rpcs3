#pragma once

enum class ppu_decoder_type : unsigned
{
	_static,
	llvm,
};

enum class spu_decoder_type : unsigned
{
	_static,
	dynamic,
	asmjit,
	llvm,
};

enum class spu_block_size_type
{
	safe,
	mega,
	giga,
};

enum class sleep_timers_accuracy_level
{
	_as_host,
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
	raw
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
	cubeb,
#ifdef HAVE_FAUDIO
	faudio,
#endif
};

enum class audio_provider
{
	none,
	cell_audio,
	rsxaudio
};

enum class audio_avport
{
	hdmi_0,
	hdmi_1,
	avmulti,
	spdif_0,
	spdif_1
};

enum class audio_format
{
	stereo,
	surround_5_1,
	surround_7_1,
	automatic,
	manual,
};

enum class audio_format_flag : unsigned
{
	lpcm_2_48khz   = 0x00000000, // Linear PCM 2 Ch. 48 kHz (always available)
	lpcm_5_1_48khz = 0x00000001, // Linear PCM 5.1 Ch. 48 kHz
	lpcm_7_1_48khz = 0x00000002, // Linear PCM 7.1 Ch. 48 kHz
	ac3            = 0x00000004, // Dolby Digital 5.1 Ch.
	dts            = 0x00000008, // DTS 5.1 Ch.
};

enum class audio_channel_layout
{
	automatic,
	mono,
	stereo,
	stereo_lfe,
	quadraphonic,
	quadraphonic_lfe,
	surround_5_1,
	surround_7_1,
};

enum class music_handler
{
	null,
	qt
};

enum class camera_handler
{
	null,
	fake,
	qt
};

enum class camera_flip
{
	none,
	horizontal,
	vertical,
	both
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
	real,
	fake,
	mouse,
	raw_mouse,
#ifdef HAVE_LIBEVDEV
	gun
#endif
};

enum class buzz_handler
{
	null,
	one_controller,
	two_controllers,
};

enum class turntable_handler
{
	null,
	one_controller,
	two_controllers,
};

enum class ghltar_handler
{
	null,
	one_controller,
	two_controllers,
};

enum class microphone_handler
{
	null,
	standard,
	singstar,
	real_singstar,
	rocksmith,
};

enum class pad_handler_mode
{
	single_threaded, // All pad handlers run on the same thread sequentially.
	multi_threaded   // Each pad handler has its own thread.
};

enum class video_resolution
{
	_1080p,
	_1080i,
	_720p,
	_480p,
	_480i,
	_576p,
	_576i,
	_1600x1080p,
	_1440x1080p,
	_1280x1080p,
	_960x1080p,
};

enum class video_aspect
{
	_4_3,
	_16_9,
};

enum class frame_limit_type
{
	none,
	_30,
	_50,
	_60,
	_120,
	display_rate,
	_auto,
	_ps3,
	infinite,
};

enum class msaa_level
{
	none,
	_auto
};

enum class detail_level
{
	none,
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

enum class rsx_fifo_mode : unsigned
{
	fast,
	atomic,
	atomic_ordered,
	as_ps3,
};

enum class tsx_usage
{
	disabled,
	enabled,
	forced,
};

enum class enter_button_assign
{
	circle, // CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CIRCLE
	cross   // CELL_SYSUTIL_ENTER_BUTTON_ASSIGN_CROSS
};

enum class np_internet_status
{
	disabled,
	enabled,
};

enum class np_psn_status
{
	disabled,
	psn_fake,
	psn_rpcn,
};

enum class shader_mode
{
	recompiler,
	async_recompiler,
	async_with_interpreter,
	interpreter_only
};

enum class vk_exclusive_fs_mode
{
	unspecified,
	disable,
	enable
};

enum class vk_gpu_scheduler_mode
{
	safe,
	fast
};

enum class thread_scheduler_mode
{
	os,
	old,
	alt
};

enum class perf_graph_detail_level
{
	minimal,
	show_min_max,
	show_one_percent_avg,
	show_all
};

enum class zcull_precision_level
{
	precise,
	approximate,
	relaxed,
	undefined
};

enum class gpu_preset_level
{
	ultra,
	high,
	low,
	_auto
};

enum class output_scaling_mode
{
	nearest,
	bilinear,
	fsr
};

enum class stereo_render_mode_options
{
	disabled,
	side_by_side,
	over_under,
	interlaced,
	anaglyph_red_green,
	anaglyph_red_blue,
	anaglyph_red_cyan,
	anaglyph_magenta_cyan,
	anaglyph_trioscopic,
	anaglyph_amber_blue,
};

enum class xfloat_accuracy
{
	accurate,
	approximate,
	relaxed, // Approximate accuracy for only the "FCGT", "FNMS", "FREST" AND "FRSQEST" instructions
	inaccurate
};
