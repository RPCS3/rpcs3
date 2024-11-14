#include "stdafx.h"
#include "overlay_home_menu_settings.h"
#include "overlay_home_menu_components.h"
#include "Emu/system_config.h"

// TODO: Localization of the dropdown values

namespace rsx
{
	namespace overlays
	{
		home_menu_settings::home_menu_settings(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS))
		{
			add_page(std::make_shared<home_menu_settings_audio>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_video>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_input>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_advanced>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_overlays>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_performance_overlay>(x, y, width, height, use_separators, this));
			add_page(std::make_shared<home_menu_settings_debug>(x, y, width, height, use_separators, this));

			apply_layout();
		}

		home_menu_settings_audio::home_menu_settings_audio(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_AUDIO))
		{
			add_signed_slider(&g_cfg.audio.volume, "Master Volume", localized_string_id::HOME_MENU_SETTINGS_AUDIO_MASTER_VOLUME, " %", 1);
			add_dropdown(&g_cfg.audio.renderer, "Audio Backend", localized_string_id::HOME_MENU_SETTINGS_AUDIO_BACKEND);

			add_checkbox(&g_cfg.audio.enable_buffering, "Enable Buffering", localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFERING);
			add_signed_slider(&g_cfg.audio.desired_buffer_duration, "Desired Audio Buffer Duration", localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFER_DURATION, " ms", 1);

			add_checkbox(&g_cfg.audio.enable_time_stretching, "Enable Time Stretching", localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING);
			add_signed_slider(&g_cfg.audio.time_stretching_threshold, "Time Stretching Threshold", localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING_THRESHOLD, " %", 1);

			apply_layout();
		}

		home_menu_settings_video::home_menu_settings_video(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_VIDEO))
		{
			add_dropdown(&g_cfg.video.frame_limit, "Frame Limit", localized_string_id::HOME_MENU_SETTINGS_VIDEO_FRAME_LIMIT);
			add_unsigned_slider(&g_cfg.video.anisotropic_level_override, "Anisotropic Filter Override", localized_string_id::HOME_MENU_SETTINGS_VIDEO_ANISOTROPIC_OVERRIDE, "x", 2, {{0, "Auto"}}, {14});

			add_dropdown(&g_cfg.video.output_scaling, "Output Scaling", localized_string_id::HOME_MENU_SETTINGS_VIDEO_OUTPUT_SCALING);
			if (g_cfg.video.renderer == video_renderer::vulkan && g_cfg.video.output_scaling == output_scaling_mode::fsr)
			{
				add_unsigned_slider(&g_cfg.video.vk.rcas_sharpening_intensity, "FidelityFX CAS Sharpening Intensity", localized_string_id::HOME_MENU_SETTINGS_VIDEO_RCAS_SHARPENING, " %", 1);
			}

			add_checkbox(&g_cfg.video.stretch_to_display_area, "Stretch To Display Area", localized_string_id::HOME_MENU_SETTINGS_VIDEO_STRETCH_TO_DISPLAY);

			apply_layout();
		}

		home_menu_settings_advanced::home_menu_settings_advanced(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_ADVANCED))
		{
			add_signed_slider(&g_cfg.core.preferred_spu_threads, "Preferred SPU Threads", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_PREFERRED_SPU_THREADS, "", 1);
			add_unsigned_slider(&g_cfg.core.max_cpu_preempt_count_per_frame, "Max Power Saving CPU-Preemptions", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_CPU_PREEMPTIONS, "", 1);
			add_checkbox(&g_cfg.core.rsx_accurate_res_access, "Accurate RSX reservation access", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_ACCURATE_RSX_RESERVATION_ACCESS);
			add_dropdown(&g_cfg.core.sleep_timers_accuracy, "Sleep Timers Accuracy", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_SLEEP_TIMERS_ACCURACY);
			add_signed_slider(&g_cfg.core.max_spurs_threads, "Max SPURS Threads", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_SPURS_THREADS, "", 1);

			add_unsigned_slider(&g_cfg.video.driver_wakeup_delay, "Driver Wake-Up Delay", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_DRIVER_WAKE_UP_DELAY, " µs", 20, {}, {}, g_cfg.video.driver_wakeup_delay.min, 800);
			add_signed_slider(&g_cfg.video.vblank_rate, "VBlank Frequency", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_FREQUENCY, " Hz", 30);
			add_checkbox(&g_cfg.video.vblank_ntsc, "VBlank NTSC Fixup", localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_NTSC);

			apply_layout();
		}

		home_menu_settings_input::home_menu_settings_input(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_INPUT))
		{
			add_checkbox(&g_cfg.io.background_input_enabled, "Background Input Enabled", localized_string_id::HOME_MENU_SETTINGS_INPUT_BACKGROUND_INPUT);
			add_checkbox(&g_cfg.io.keep_pads_connected, "Keep Pads Connected", localized_string_id::HOME_MENU_SETTINGS_INPUT_KEEP_PADS_CONNECTED);
			add_checkbox(&g_cfg.io.show_move_cursor, "Show PS Move Cursor", localized_string_id::HOME_MENU_SETTINGS_INPUT_SHOW_PS_MOVE_CURSOR);

			if (g_cfg.io.camera == camera_handler::qt)
			{
				add_dropdown(&g_cfg.io.camera_flip_option, "Camera Flip", localized_string_id::HOME_MENU_SETTINGS_INPUT_CAMERA_FLIP);
			}

			add_dropdown(&g_cfg.io.pad_mode, "Pad Handler Mode", localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_MODE);
			add_unsigned_slider(&g_cfg.io.pad_sleep, "Pad Handler Sleep", localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_SLEEP, " µs", 100);

			apply_layout();
		}

		home_menu_settings_overlays::home_menu_settings_overlays(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_OVERLAYS))
		{
			add_checkbox(&g_cfg.misc.show_trophy_popups, "Show Trophy Popups", localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_TROPHY_POPUPS);
			add_checkbox(&g_cfg.misc.show_rpcn_popups, "Show RPCN Popups", localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_RPCN_POPUPS);
			add_checkbox(&g_cfg.misc.show_shader_compilation_hint, "Show Shader Compilation Hint", localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_SHADER_COMPILATION_HINT);
			add_checkbox(&g_cfg.misc.show_ppu_compilation_hint, "Show PPU Compilation Hint", localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_PPU_COMPILATION_HINT);
			add_checkbox(&g_cfg.misc.show_autosave_autoload_hint, "Show Autosave/Autoload Hint", localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_AUTO_SAVE_LOAD_HINT);

			apply_layout();
		}

		home_menu_settings_performance_overlay::home_menu_settings_performance_overlay(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY))
		{
			add_checkbox(&g_cfg.video.perf_overlay.perf_overlay_enabled, "Enable Performance Overlay", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE);
			add_checkbox(&g_cfg.video.perf_overlay.framerate_graph_enabled, "Enable Framerate Graph", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMERATE_GRAPH);
			add_checkbox(&g_cfg.video.perf_overlay.frametime_graph_enabled, "Enable Frametime Graph", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMETIME_GRAPH);

			add_dropdown(&g_cfg.video.perf_overlay.level, "Detail level", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_DETAIL_LEVEL);
			add_dropdown(&g_cfg.video.perf_overlay.framerate_graph_detail_level, "Framerate Graph Detail Level", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DETAIL_LEVEL);
			add_dropdown(&g_cfg.video.perf_overlay.frametime_graph_detail_level, "Frametime Graph Detail Level", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DETAIL_LEVEL);

			add_unsigned_slider(&g_cfg.video.perf_overlay.framerate_datapoint_count, "Framerate Datapoints", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DATAPOINT_COUNT, "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.frametime_datapoint_count, "Frametime Datapoints", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DATAPOINT_COUNT, "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.update_interval, "Metrics Update Interval", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_UPDATE_INTERVAL, " ms", 1);

			add_dropdown(&g_cfg.video.perf_overlay.position, "Position", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_POSITION);
			add_checkbox(&g_cfg.video.perf_overlay.center_x, "Center Horizontally", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_X);
			add_checkbox(&g_cfg.video.perf_overlay.center_y, "Center Vertically", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_Y);
			add_unsigned_slider(&g_cfg.video.perf_overlay.margin_x, "Horizontal Margin", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_X, " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.margin_y, "Vertical Margin", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_Y, " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.font_size, "Font Size", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FONT_SIZE, " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.opacity, "Opacity", localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_OPACITY, " %", 1);

			apply_layout();
		}

		home_menu_settings_debug::home_menu_settings_debug(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_DEBUG))
		{
			add_checkbox(&g_cfg.video.overlay, "Debug Overlay", localized_string_id::HOME_MENU_SETTINGS_DEBUG_OVERLAY);
			add_checkbox(&g_cfg.video.disable_video_output, "Disable Video Output", localized_string_id::HOME_MENU_SETTINGS_DEBUG_DISABLE_VIDEO_OUTPUT);
			add_float_slider(&g_cfg.video.texture_lod_bias, "Texture LOD Bias Addend", localized_string_id::HOME_MENU_SETTINGS_DEBUG_TEXTURE_LOD_BIAS, "", 0.25f);

			apply_layout();
		}
	}
}
