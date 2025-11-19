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
		case mouse_handler::raw: return "Raw";
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
		case video_resolution::_1080p: return "1920x1080";
		case video_resolution::_1080i: return "1920x1080i";
		case video_resolution::_720p: return "1280x720";
		case video_resolution::_480p: return "720x480";
		case video_resolution::_480i: return "720x480i";
		case video_resolution::_576p: return "720x576";
		case video_resolution::_576i: return "720x576i";
		case video_resolution::_1600x1080p: return "1600x1080";
		case video_resolution::_1440x1080p: return "1440x1080";
		case video_resolution::_1280x1080p: return "1280x1080";
		case video_resolution::_960x1080p: return "960x1080";
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
void fmt_class_string<audio_channel_layout>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_channel_layout value)
	{
		switch (value)
		{
		case audio_channel_layout::automatic: return "Automatic";
		case audio_channel_layout::mono: return "Mono";
		case audio_channel_layout::stereo: return "Stereo";
		case audio_channel_layout::stereo_lfe: return "Stereo LFE";
		case audio_channel_layout::quadraphonic: return "Quadraphonic";
		case audio_channel_layout::quadraphonic_lfe: return "Quadraphonic LFE";
		case audio_channel_layout::surround_5_1: return "Surround 5.1";
		case audio_channel_layout::surround_7_1: return "Surround 7.1";
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
void fmt_class_string<rsx_fifo_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](rsx_fifo_mode value)
	{
		switch (value)
		{
		case rsx_fifo_mode::fast: return "Fast";
		case rsx_fifo_mode::atomic: return "Atomic";
		case rsx_fifo_mode::atomic_ordered: return "Ordered & Atomic";
		case rsx_fifo_mode::as_ps3: return "PS3";
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
		case frame_limit_type::_30: return "30";
		case frame_limit_type::_50: return "50";
		case frame_limit_type::_60: return "60";
		case frame_limit_type::_120: return "120";
		case frame_limit_type::display_rate: return "Display";
		case frame_limit_type::_auto: return "Auto";
		case frame_limit_type::_ps3: return "PS3 Native";
		case frame_limit_type::infinite: return "Infinite";
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
		case move_handler::real: return "Real";
		case move_handler::fake: return "Fake";
		case move_handler::mouse: return "Mouse";
		case move_handler::raw_mouse: return "Raw Mouse";
#ifdef HAVE_LIBEVDEV
		case move_handler::gun: return "Gun";
#endif
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
void fmt_class_string<audio_avport>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_avport value)
	{
		switch (value)
		{
		case audio_avport::hdmi_0: return "HDMI 0";
		case audio_avport::hdmi_1: return "HDMI 1";
		case audio_avport::avmulti: return "AV multiout";
		case audio_avport::spdif_0: return "SPDIF 0";
		case audio_avport::spdif_1: return "SPDIF 1";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_format value)
	{
		switch (value)
		{
		case audio_format::stereo: return "Stereo";
		case audio_format::surround_5_1: return "Surround 5.1";
		case audio_format::surround_7_1: return "Surround 7.1";
		case audio_format::automatic: return "Automatic";
		case audio_format::manual: return "Manual";
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

template <>
void fmt_class_string<gpu_preset_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](gpu_preset_level value)
	{
		switch (value)
		{
		case gpu_preset_level::_auto: return "Auto";
		case gpu_preset_level::ultra: return "Ultra";
		case gpu_preset_level::high: return "High";
		case gpu_preset_level::low: return "Low";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<vk_exclusive_fs_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](vk_exclusive_fs_mode value)
		{
			switch (value)
			{
			case vk_exclusive_fs_mode::unspecified: return "Automatic";
			case vk_exclusive_fs_mode::disable: return "Disable";
			case vk_exclusive_fs_mode::enable: return "Enable";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<stereo_render_mode_options>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](stereo_render_mode_options value)
		{
			switch (value)
			{
			case stereo_render_mode_options::disabled: return "Disabled";
			case stereo_render_mode_options::side_by_side: return "Side-by-Side";
			case stereo_render_mode_options::over_under: return "Over-Under";
			case stereo_render_mode_options::interlaced: return "Interlaced";
			case stereo_render_mode_options::anaglyph_red_green: return "Anaglyph Red-Green";
			case stereo_render_mode_options::anaglyph_red_blue: return "Anaglyph Red-Blue";
			case stereo_render_mode_options::anaglyph_red_cyan: return "Anaglyph Red-Cyan";
			case stereo_render_mode_options::anaglyph_magenta_cyan: return "Anaglyph Magenta-Cyan";
			case stereo_render_mode_options::anaglyph_trioscopic: return "Anaglyph Trioscopic";
			case stereo_render_mode_options::anaglyph_amber_blue: return "Anaglyph Amber-Blue";
			}

			return unknown;
		});
}

template <>
void fmt_class_string<output_scaling_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](output_scaling_mode value)
	{
		switch (value)
		{
		case output_scaling_mode::nearest: return "Nearest";
		case output_scaling_mode::bilinear: return "Bilinear";
		case output_scaling_mode::fsr: return "FidelityFX Super Resolution";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<xfloat_accuracy>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](xfloat_accuracy value)
	{
		switch (value)
		{
		case xfloat_accuracy::accurate: return "Accurate";
		case xfloat_accuracy::approximate: return "Approximate";
		case xfloat_accuracy::relaxed: return "Relaxed";
		case xfloat_accuracy::inaccurate: return "Inaccurate";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<date_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](date_format value)
	{
		switch (value)
		{
		case date_format::yyyymmdd: return "yyyymmdd";
		case date_format::ddmmyyyy: return "ddmmyyyy";
		case date_format::mmddyyyy: return "mmddyyyy";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<time_format>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](time_format value)
	{
		switch (value)
		{
		case time_format::clock12: return "clock12";
		case time_format::clock24: return "clock24";
		}

		return unknown;
	});
}
