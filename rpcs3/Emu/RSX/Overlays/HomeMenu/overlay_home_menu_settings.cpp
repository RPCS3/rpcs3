#include "stdafx.h"
#include "overlay_home_menu_settings.h"
#include "overlay_home_menu_components.h"
#include "Emu/system_config.h"

// TODO: Localization of the setting names
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
			add_signed_slider(&g_cfg.audio.volume, "Master Volume", " %", 1);
			add_dropdown(&g_cfg.audio.renderer, "Audio Backend");

			add_checkbox(&g_cfg.audio.enable_buffering, "Enable Buffering");
			add_signed_slider(&g_cfg.audio.desired_buffer_duration, "Desired Audio Buffer Duration", " ms", 1);

			add_checkbox(&g_cfg.audio.enable_time_stretching, "Enable Time Stretching");
			add_signed_slider(&g_cfg.audio.time_stretching_threshold, "Time Stretching Threshold", " %", 1);

			apply_layout();
		}

		home_menu_settings_video::home_menu_settings_video(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_VIDEO))
		{
			add_dropdown(&g_cfg.video.frame_limit, "Frame Limit");
			add_unsigned_slider(&g_cfg.video.anisotropic_level_override, "Anisotropic Filter Override", "x", 2, {{0, "Auto"}}, {14});

			add_dropdown(&g_cfg.video.output_scaling, "Output Scaling");
			if (g_cfg.video.renderer == video_renderer::vulkan && g_cfg.video.output_scaling == output_scaling_mode::fsr)
			{
				add_unsigned_slider(&g_cfg.video.vk.rcas_sharpening_intensity, "FidelityFX CAS Sharpening Intensity", " %", 1);
			}

			add_checkbox(&g_cfg.video.stretch_to_display_area, "Stretch To Display Area");

			apply_layout();
		}

		home_menu_settings_advanced::home_menu_settings_advanced(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_ADVANCED))
		{
			add_signed_slider(&g_cfg.core.preferred_spu_threads, "Preferred SPU Threads", "", 1);
			add_unsigned_slider(&g_cfg.core.max_cpu_preempt_count_per_frame, "Max Power Saving CPU-Preemptions", "", 1);
			add_checkbox(&g_cfg.core.rsx_accurate_res_access, "Accurate RSX reservation access");
			add_dropdown(&g_cfg.core.sleep_timers_accuracy, "Sleep Timers Accuracy");
			add_signed_slider(&g_cfg.core.max_spurs_threads, "Max SPURS Threads", "", 1);

			add_unsigned_slider(&g_cfg.video.driver_wakeup_delay, "Driver Wake-Up Delay", " µs", 20, {}, {}, g_cfg.video.driver_wakeup_delay.min, 800);
			add_signed_slider(&g_cfg.video.vblank_rate, "VBlank Frequency", " Hz", 30);
			add_checkbox(&g_cfg.video.vblank_ntsc, "VBlank NTSC Fixup");

			apply_layout();
		}

		home_menu_settings_input::home_menu_settings_input(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_INPUT))
		{
			add_checkbox(&g_cfg.io.background_input_enabled, "Background Input Enabled");
			add_checkbox(&g_cfg.io.keep_pads_connected, "Keep Pads Connected");
			add_checkbox(&g_cfg.io.show_move_cursor, "Show PS Move Cursor");

			if (g_cfg.io.camera == camera_handler::qt)
			{
				add_dropdown(&g_cfg.io.camera_flip_option, "Camera Flip");
			}

			add_dropdown(&g_cfg.io.pad_mode, "Pad Handler Mode");
			add_unsigned_slider(&g_cfg.io.pad_sleep, "Pad Handler Sleep", " µs", 100);

			apply_layout();
		}

		home_menu_settings_overlays::home_menu_settings_overlays(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_OVERLAYS))
		{
			add_checkbox(&g_cfg.misc.show_trophy_popups, "Show Trophy Popups");
			add_checkbox(&g_cfg.misc.show_shader_compilation_hint, "Show Shader Compilation Hint");
			add_checkbox(&g_cfg.misc.show_ppu_compilation_hint, "Show PPU Compilation Hint");

			apply_layout();
		}

		home_menu_settings_performance_overlay::home_menu_settings_performance_overlay(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY))
		{
			add_checkbox(&g_cfg.video.perf_overlay.perf_overlay_enabled, "Enable Performance Overlay");
			add_checkbox(&g_cfg.video.perf_overlay.framerate_graph_enabled, "Enable Framerate Graph");
			add_checkbox(&g_cfg.video.perf_overlay.frametime_graph_enabled, "Enable Frametime Graph");

			add_dropdown(&g_cfg.video.perf_overlay.level, "Detail level");
			add_dropdown(&g_cfg.video.perf_overlay.framerate_graph_detail_level, "Framerate Graph Detail Level");
			add_dropdown(&g_cfg.video.perf_overlay.frametime_graph_detail_level, "Frametime Graph Detail Level");

			add_unsigned_slider(&g_cfg.video.perf_overlay.framerate_datapoint_count, "Framerate Datapoints", "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.frametime_datapoint_count, "Frametime Datapoints", "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.update_interval, "Metrics Update Interval", " ms", 1);

			add_dropdown(&g_cfg.video.perf_overlay.position, "Position");
			add_checkbox(&g_cfg.video.perf_overlay.center_x, "Center Horizontally");
			add_checkbox(&g_cfg.video.perf_overlay.center_y, "Center Vertically");
			add_unsigned_slider(&g_cfg.video.perf_overlay.margin_x, "Horizontal Margin", " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.margin_y, "Vertical Margin", " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.font_size, "Font Size", " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.opacity, "Opacity", " %", 1);

			apply_layout();
		}

		home_menu_settings_debug::home_menu_settings_debug(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_DEBUG))
		{
			add_checkbox(&g_cfg.video.overlay, "Debug Overlay");
			add_checkbox(&g_cfg.video.disable_video_output, "Disable Video Output");
			add_float_slider(&g_cfg.video.texture_lod_bias, "Texture LOD Bias Addend", "", 0.25f);

			apply_layout();
		}
	}
}
