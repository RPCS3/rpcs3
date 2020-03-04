#include "stdafx.h"
#include "system_config_types.h"

template <>
void fmt_class_string<mouse_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mouse_handler value)
	{
		switch (value)
		{
		case mouse_handler::null: return "Null";
		case mouse_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_renderer value)
	{
		switch (value)
		{
		case video_renderer::null: return "Null";
		case video_renderer::opengl: return "OpenGL";
		case video_renderer::vulkan: return "Vulkan";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_resolution>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_resolution value)
	{
		switch (value)
		{
		case video_resolution::_1080: return "1920x1080";
		case video_resolution::_720: return "1280x720";
		case video_resolution::_480: return "720x480";
		case video_resolution::_576: return "720x576";
		case video_resolution::_1600x1080: return "1600x1080";
		case video_resolution::_1440x1080: return "1440x1080";
		case video_resolution::_1280x1080: return "1280x1080";
		case video_resolution::_960x1080: return "960x1080";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_aspect>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_aspect value)
	{
		switch (value)
		{
		case video_aspect::_4_3: return "4:3";
		case video_aspect::_16_9: return "16:9";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<msaa_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](msaa_level value)
	{
		switch (value)
		{
		case msaa_level::none: return "Disabled";
		case msaa_level::_auto: return "Auto";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<keyboard_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](keyboard_handler value)
	{
		switch (value)
		{
		case keyboard_handler::null: return "Null";
		case keyboard_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_renderer value)
	{
		switch (value)
		{
		case audio_renderer::null: return "Null";
#ifdef _WIN32
		case audio_renderer::xaudio: return "XAudio2";
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return "ALSA";
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return "PulseAudio";
#endif
		case audio_renderer::openal: return "OpenAL";
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return "FAudio";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<detail_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](detail_level value)
	{
		switch (value)
		{
		case detail_level::minimal: return "Minimal";
		case detail_level::low: return "Low";
		case detail_level::medium: return "Medium";
		case detail_level::high: return "High";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<screen_quadrant>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](screen_quadrant value)
	{
		switch (value)
		{
		case screen_quadrant::top_left: return "Top Left";
		case screen_quadrant::top_right: return "Top Right";
		case screen_quadrant::bottom_left: return "Bottom Left";
		case screen_quadrant::bottom_right: return "Bottom Right";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<tsx_usage>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](tsx_usage value)
	{
		switch (value)
		{
		case tsx_usage::disabled: return "Disabled";
		case tsx_usage::enabled: return "Enabled";
		case tsx_usage::forced: return "Forced";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<sleep_timers_accuracy_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](sleep_timers_accuracy_level value)
	{
		switch (value)
		{
		case sleep_timers_accuracy_level::_as_host: return "As Host";
		case sleep_timers_accuracy_level::_usleep: return "Usleep Only";
		case sleep_timers_accuracy_level::_all_timers: return "All Timers";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<enter_button_assign>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](enter_button_assign value)
	{
		switch (value)
		{
		case enter_button_assign::circle: return "Enter with circle";
		case enter_button_assign::cross: return "Enter with cross";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<np_internet_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](np_internet_status value)
	{
		switch (value)
		{
		case np_internet_status::disabled: return "Disconnected";
		case np_internet_status::enabled: return "Connected";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<np_psn_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](np_psn_status value)
	{
		switch (value)
		{
		case np_psn_status::disabled: return "Disconnected";
		case np_psn_status::fake: return "Simulated";
		}

		return unknown;
	});
}

