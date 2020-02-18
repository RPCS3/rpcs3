#pragma once

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

enum np_psn_status
{
	disabled,
	fake,
};
