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
		case audio_renderer::cubeb: return "Cubeb";
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
		case detail_level::none: return "None";
		case detail_level::minimal: return "Minimal";
		case detail_level::low: return "Low";
		case detail_level::medium: return "Medium";
		case detail_level::high: return "High";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<perf_graph_detail_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](perf_graph_detail_level value)
	{
		switch (value)
		{
		case perf_graph_detail_level::minimal: return "Minimal";
		case perf_graph_detail_level::show_min_max: return "Show min and max";
		case perf_graph_detail_level::show_one_percent_avg: return "Show 1% and average";
		case perf_graph_detail_level::show_all: return "All";
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
		case np_psn_status::psn_fake: return "Simulated";
		case np_psn_status::psn_rpcn: return "RPCN";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_decoder_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_decoder_type type)
	{
		switch (type)
		{
		case spu_decoder_type::_static: return "Interpreter (static)";
		case spu_decoder_type::dynamic: return "Interpreter (dynamic)";
		case spu_decoder_type::asmjit: return "Recompiler (ASMJIT)";
		case spu_decoder_type::llvm: return "Recompiler (LLVM)";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_block_size_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_block_size_type type)
	{
		switch (type)
		{
		case spu_block_size_type::safe: return "Safe";
		case spu_block_size_type::mega: return "Mega";
		case spu_block_size_type::giga: return "Giga";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<frame_limit_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](frame_limit_type value)
	{
		switch (value)
		{
		case frame_limit_type::none: return "Off";
		case frame_limit_type::_59_94: return "59.94";
		case frame_limit_type::_50: return "50";
		case frame_limit_type::_60: return "60";
		case frame_limit_type::_30: return "30";
		case frame_limit_type::_auto: return "Auto";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<microphone_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case microphone_handler::null: return "Null";
		case microphone_handler::standard: return "Standard";
		case microphone_handler::singstar: return "SingStar";
		case microphone_handler::real_singstar: return "Real SingStar";
		case microphone_handler::rocksmith: return "Rocksmith";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<camera_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case camera_handler::null: return "Null";
		case camera_handler::fake: return "Fake";
		case camera_handler::qt: return "Qt";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<music_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case music_handler::null: return "Null";
		case music_handler::qt: return "Qt";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<fake_camera_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case fake_camera_type::unknown: return "Unknown";
		case fake_camera_type::eyetoy: return "EyeToy";
		case fake_camera_type::eyetoy2: return "PS Eye";
		case fake_camera_type::uvc1_1: return "UVC 1.1";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<camera_flip>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case camera_flip::none: return "None";
		case camera_flip::horizontal: return "Horizontal";
		case camera_flip::vertical: return "Vertical";
		case camera_flip::both: return "Both";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<move_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case move_handler::null: return "Null";
		case move_handler::fake: return "Fake";
		case move_handler::mouse: return "Mouse";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<pad_handler_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case pad_handler_mode::single_threaded: return "Single-threaded";
		case pad_handler_mode::multi_threaded: return "Multi-threaded";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<buzz_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case buzz_handler::null: return "Null";
		case buzz_handler::one_controller: return "1 controller";
		case buzz_handler::two_controllers: return "2 controllers";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<turntable_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case turntable_handler::null: return "Null";
		case turntable_handler::one_controller: return "1 controller";
		case turntable_handler::two_controllers: return "2 controllers";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<ghltar_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto value)
	{
		switch (value)
		{
		case ghltar_handler::null: return "Null";
		case ghltar_handler::one_controller: return "1 controller";
		case ghltar_handler::two_controllers: return "2 controllers";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<ppu_decoder_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](ppu_decoder_type type)
	{
		switch (type)
		{
		case ppu_decoder_type::_static: return "Interpreter (static)";
		case ppu_decoder_type::dynamic: return "Interpreter (dynamic)";
		case ppu_decoder_type::llvm: return "Recompiler (LLVM)";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<shader_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](shader_mode value)
	{
		switch (value)
		{
		case shader_mode::recompiler: return "Shader Recompiler";
		case shader_mode::async_recompiler: return "Async Shader Recompiler";
		case shader_mode::async_with_interpreter: return "Async with Shader Interpreter";
		case shader_mode::interpreter_only: return "Shader Interpreter only";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_provider>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_provider value)
	{
		switch (value)
		{
		case audio_provider::none: return "None";
		case audio_provider::cell_audio: return "CellAudio";
		case audio_provider::rsxaudio: return "RSXAudio";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_downmix>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_downmix value)
	{
		switch (value)
		{
		case audio_downmix::no_downmix: return "No downmix";
		case audio_downmix::downmix_to_stereo: return "Downmix to Stereo";
		case audio_downmix::downmix_to_5_1: return "Downmix to 5.1";
		case audio_downmix::use_application_settings: return "Use application settings";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<vk_gpu_scheduler_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](vk_gpu_scheduler_mode value)
	{
		switch (value)
		{
		case vk_gpu_scheduler_mode::safe: return "Safe";
		case vk_gpu_scheduler_mode::fast: return "Fast";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<vk_metal_semaphore_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](vk_metal_semaphore_mode value)
	{
		switch (value)
		{
		case vk_metal_semaphore_mode::software: return "Software emulation";
		case vk_metal_semaphore_mode::mtlevent_preferred: return "MTLEvent preferred";
		case vk_metal_semaphore_mode::mtlevent: return "MTLEvent";
		case vk_metal_semaphore_mode::mtlfence: return "MTLFence";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<thread_scheduler_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](thread_scheduler_mode value)
	{
		switch (value)
		{
		case thread_scheduler_mode::old: return "RPCS3 Scheduler";
		case thread_scheduler_mode::alt: return "RPCS3 Alternative Scheduler";
		case thread_scheduler_mode::os: return "Operating System";
		}

		return unknown;
	});
}
