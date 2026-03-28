#include "stdafx.h"
#include "overlay_home_menu_settings.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_settings::home_menu_settings(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS))
		{
			m_tabs = std::make_unique<tabbed_container>(width, height, 300);
			m_tabs->set_pos(x, y);
			m_tabs->set_headers_background_color({ 0.25f, 0.25f, 0.25f, 0.95f });

			add_page(home_menu::fa_icon::audio, std::make_shared<home_menu_settings_audio>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::video, std::make_shared<home_menu_settings_video>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::gamepad, std::make_shared<home_menu_settings_input>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::settings, std::make_shared<home_menu_settings_advanced>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::settings_sliders, std::make_shared<home_menu_settings_overlays>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::settings_gauge, std::make_shared<home_menu_settings_performance_overlay>(x, y, width, height, use_separators, nullptr));
			add_page(home_menu::fa_icon::bug, std::make_shared<home_menu_settings_debug>(x, y, width, height, use_separators, nullptr));

			// Select the first item
			m_tabs->set_selected_tab(0);
		}

		void home_menu_settings::add_page(home_menu::fa_icon icon, std::shared_ptr<home_menu_page> page)
		{
			auto panel = std::static_pointer_cast<overlay_element>(page);
			page->on_deactivate();
			page->hide_prompt_buttons();

			if (icon == home_menu::fa_icon::none)
			{
				m_tabs->add_tab(page->title, panel);
				return;
			}

			// Custom tab header. Matches the main menu style
			std::unique_ptr<overlay_element> label_widget = std::make_unique<label>(page->title);
			label_widget->set_size(300, 60);
			label_widget->set_font("Arial", 16);
			label_widget->back_color.a = 0.f;
			label_widget->set_padding(16, 4, 16, 4);

			auto icon_info = ensure(home_menu::get_icon(icon));
			auto icon_view = std::make_unique<image_view>();
			icon_view->set_raw_image(icon_info);
			icon_view->set_size(42, 60);
			icon_view->set_padding(18, 0, 18, 18);

			auto box = std::make_unique<horizontal_layout>();
			box->set_size(0, 16);
			box->set_padding(1);
			box->add_element(icon_view);
			box->add_element(label_widget);

			m_tabs->add_tab(box, panel);
		}

		page_navigation home_menu_settings::handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms)
		{
			if (!m_tabs->is_in_selection_mode())
			{
				auto page = ensure(dynamic_cast<home_menu_page*>(m_tabs->get_selected()));
				const auto nav_action = page->handle_button_press(button_press, is_auto_repeat, auto_repeat_interval_ms);
				switch (nav_action)
				{
				case page_navigation::exit:
					m_tabs->toggle_selection_mode();
					page->on_deactivate();
					return page_navigation::stay;
				default:
					return nav_action;
				}
			}

			const auto prev_tab = m_tabs->get_selected_idx();
			auto next_tab = prev_tab;

			page_navigation action = page_navigation::stay;
			sound_effect sound = sound_effect::cursor;
			bool do_play_sound = !is_auto_repeat || auto_repeat_interval_ms >= user_interface::m_auto_repeat_ms_interval_default;

			switch (button_press)
			{
			case pad_button::dpad_up:
			case pad_button::ls_up:
				next_tab = (prev_tab > 0)
					? (prev_tab - 1)
					: (m_tabs->tab_count() - 1);
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				next_tab = ((prev_tab + 1) >= m_tabs->tab_count())
					? 0
					: prev_tab + 1;
				break;
			case pad_button::cross:
				sound = sound_effect::accept;
				m_tabs->toggle_selection_mode();
				ensure(dynamic_cast<home_menu_page*>(m_tabs->get_selected()))->on_activate();
				break;
			case pad_button::circle:
				set_current_page(this->parent);
				action = this->parent ? page_navigation::back : page_navigation::exit;
				sound = sound_effect::cancel;
				break;
			default:
				do_play_sound = false;
				break;
			}

			if (prev_tab != next_tab)
			{
				m_tabs->set_selected_tab(next_tab);
			}

			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			if (do_play_sound)
			{
				play_sound(sound);
			}

			return action;
		}

		compiled_resource& home_menu_settings::get_compiled()
		{
			// TODO: Caching
			compiled_resources = home_menu_page::get_compiled();
			compiled_resources.add(m_tabs->get_compiled());
			return compiled_resources;
		}

		home_menu_settings_audio::home_menu_settings_audio(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_AUDIO))
		{
			add_signed_slider(&g_cfg.audio.volume, localized_string_id::HOME_MENU_SETTINGS_AUDIO_MASTER_VOLUME, " %", 1);
			add_dropdown(&g_cfg.audio.renderer, localized_string_id::HOME_MENU_SETTINGS_AUDIO_BACKEND);

			add_checkbox(&g_cfg.audio.enable_buffering, localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFERING);
			add_signed_slider(&g_cfg.audio.desired_buffer_duration, localized_string_id::HOME_MENU_SETTINGS_AUDIO_BUFFER_DURATION, " ms", 1);

			add_checkbox(&g_cfg.audio.enable_time_stretching, localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING);
			add_signed_slider(&g_cfg.audio.time_stretching_threshold, localized_string_id::HOME_MENU_SETTINGS_AUDIO_TIME_STRETCHING_THRESHOLD, " %", 1);

			apply_layout();
		}

		home_menu_settings_video::home_menu_settings_video(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_VIDEO))
		{
			add_unsigned_slider(&g_cfg.video.resolution_scale_percent, localized_string_id::HOME_MENU_SETTINGS_VIDEO_RESOLUTION_SCALE_PERCENT, "%", 25);
			add_unsigned_slider(&g_cfg.video.min_scalable_dimension, localized_string_id::HOME_MENU_SETTINGS_VIDEO_RESOLUTION_SCALE_THRESHOLD, "px", 1);

			add_dropdown(&g_cfg.video.vsync, localized_string_id::HOME_MENU_SETTINGS_VIDEO_VSYNC);

			add_dropdown(&g_cfg.video.frame_limit, localized_string_id::HOME_MENU_SETTINGS_VIDEO_FRAME_LIMIT);
			add_unsigned_slider(&g_cfg.video.anisotropic_level_override, localized_string_id::HOME_MENU_SETTINGS_VIDEO_ANISOTROPIC_OVERRIDE, "x", 2, {{0, "Auto"}}, {14});

			add_dropdown(&g_cfg.video.output_scaling, localized_string_id::HOME_MENU_SETTINGS_VIDEO_OUTPUT_SCALING);
			add_unsigned_slider(&g_cfg.video.rcas_sharpening_intensity, localized_string_id::HOME_MENU_SETTINGS_VIDEO_RCAS_SHARPENING, " %", 1);

			add_checkbox(&g_cfg.video.stretch_to_display_area, localized_string_id::HOME_MENU_SETTINGS_VIDEO_STRETCH_TO_DISPLAY);

			if (g_cfg.video.stereo_enabled)
			{
				add_dropdown(&g_cfg.video.stereo_render_mode, localized_string_id::HOME_MENU_SETTINGS_VIDEO_STEREO_MODE);
			}

			apply_layout();
		}

		home_menu_settings_advanced::home_menu_settings_advanced(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_ADVANCED))
		{
			add_signed_slider(&g_cfg.core.preferred_spu_threads, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_PREFERRED_SPU_THREADS, "", 1);
			add_unsigned_slider(&g_cfg.core.max_cpu_preempt_count_per_frame, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_CPU_PREEMPTIONS, "", 1);
			add_checkbox(&g_cfg.core.rsx_accurate_res_access, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_ACCURATE_RSX_RESERVATION_ACCESS);
			add_dropdown(&g_cfg.core.sleep_timers_accuracy, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_SLEEP_TIMERS_ACCURACY);
			add_signed_slider(&g_cfg.core.max_spurs_threads, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_MAX_SPURS_THREADS, "", 1);

			add_unsigned_slider(&g_cfg.video.driver_wakeup_delay, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_DRIVER_WAKE_UP_DELAY, " µs", 20, {}, {}, g_cfg.video.driver_wakeup_delay.min, 800);
			add_signed_slider(&g_cfg.video.vblank_rate, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_FREQUENCY, " Hz", 30, {}, 30);
			add_checkbox(&g_cfg.video.vblank_ntsc, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_VBLANK_NTSC);
			add_checkbox(&g_cfg.video.handle_tiled_memory, localized_string_id::HOME_MENU_SETTINGS_ADVANCED_RSX_MEMORY_TILING);

			apply_layout();
		}

		home_menu_settings_input::home_menu_settings_input(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_INPUT))
		{
			add_checkbox(&g_cfg.io.background_input_enabled, localized_string_id::HOME_MENU_SETTINGS_INPUT_BACKGROUND_INPUT);
			add_checkbox(&g_cfg.io.keep_pads_connected, localized_string_id::HOME_MENU_SETTINGS_INPUT_KEEP_PADS_CONNECTED);
			add_checkbox(&g_cfg.io.show_move_cursor, localized_string_id::HOME_MENU_SETTINGS_INPUT_SHOW_PS_MOVE_CURSOR);

			switch (g_cfg.io.camera)
			{
	#ifdef HAVE_SDL3
			case camera_handler::sdl:
	#endif
			case camera_handler::qt:
				add_dropdown(&g_cfg.io.camera_flip_option, localized_string_id::HOME_MENU_SETTINGS_INPUT_CAMERA_FLIP);
				break;
			case camera_handler::fake:
			case camera_handler::null:
				break;
			}

			add_dropdown(&g_cfg.io.pad_mode, localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_MODE);
			add_unsigned_slider(&g_cfg.io.pad_sleep, localized_string_id::HOME_MENU_SETTINGS_INPUT_PAD_SLEEP, " µs", 100);
			add_unsigned_slider(&g_cfg.io.fake_move_rotation_cone_h, localized_string_id::HOME_MENU_SETTINGS_INPUT_FAKE_MOVE_ROTATION_CONE_H, "°", 1);
			add_unsigned_slider(&g_cfg.io.fake_move_rotation_cone_v, localized_string_id::HOME_MENU_SETTINGS_INPUT_FAKE_MOVE_ROTATION_CONE_V, "°", 1);

			apply_layout();
		}

		home_menu_settings_overlays::home_menu_settings_overlays(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_OVERLAYS))
		{
			add_checkbox(&g_cfg.misc.show_trophy_popups, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_TROPHY_POPUPS);
			add_checkbox(&g_cfg.misc.show_rpcn_popups, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_RPCN_POPUPS);
			add_checkbox(&g_cfg.misc.show_shader_compilation_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_SHADER_COMPILATION_HINT);
			add_checkbox(&g_cfg.misc.show_ppu_compilation_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_PPU_COMPILATION_HINT);
			add_checkbox(&g_cfg.misc.show_autosave_autoload_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_AUTO_SAVE_LOAD_HINT);
			add_checkbox(&g_cfg.misc.show_pressure_intensity_toggle_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_PRESSURE_INTENSITY_TOGGLE_HINT);
			add_checkbox(&g_cfg.misc.show_analog_limiter_toggle_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_ANALOG_LIMITER_TOGGLE_HINT);
			add_checkbox(&g_cfg.misc.show_mouse_and_keyboard_toggle_hint, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_SHOW_MOUSE_AND_KB_TOGGLE_HINT);
			add_checkbox(&g_cfg.video.record_with_overlays, localized_string_id::HOME_MENU_SETTINGS_OVERLAYS_RECORD_WITH_OVERLAYS);

			apply_layout();
		}

		home_menu_settings_performance_overlay::home_menu_settings_performance_overlay(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY))
		{
			add_checkbox(&g_cfg.video.perf_overlay.perf_overlay_enabled, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE);
			add_checkbox(&g_cfg.video.perf_overlay.framerate_graph_enabled, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMERATE_GRAPH);
			add_checkbox(&g_cfg.video.perf_overlay.frametime_graph_enabled, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_ENABLE_FRAMETIME_GRAPH);

			add_dropdown(&g_cfg.video.perf_overlay.level, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_DETAIL_LEVEL);
			add_dropdown(&g_cfg.video.perf_overlay.framerate_graph_detail_level, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DETAIL_LEVEL);
			add_dropdown(&g_cfg.video.perf_overlay.frametime_graph_detail_level, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DETAIL_LEVEL);

			add_unsigned_slider(&g_cfg.video.perf_overlay.framerate_datapoint_count, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMERATE_DATAPOINT_COUNT, "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.frametime_datapoint_count, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FRAMETIME_DATAPOINT_COUNT, "", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.update_interval, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_UPDATE_INTERVAL, " ms", 1);

			add_dropdown(&g_cfg.video.perf_overlay.position, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_POSITION);
			add_checkbox(&g_cfg.video.perf_overlay.center_x, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_X);
			add_checkbox(&g_cfg.video.perf_overlay.center_y, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_CENTER_Y);
			add_float_slider(&g_cfg.video.perf_overlay.margin_x, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_X, " %", 0.25f);
			add_float_slider(&g_cfg.video.perf_overlay.margin_y, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_MARGIN_Y, " %", 0.25f);
			add_unsigned_slider(&g_cfg.video.perf_overlay.font_size, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_FONT_SIZE, " px", 1);
			add_unsigned_slider(&g_cfg.video.perf_overlay.opacity, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_OPACITY, " %", 1);
			add_checkbox(&g_cfg.video.perf_overlay.perf_overlay_use_window_space, localized_string_id::HOME_MENU_SETTINGS_PERFORMANCE_OVERLAY_USE_WINDOW_SPACE);

			apply_layout();
		}

		home_menu_settings_debug::home_menu_settings_debug(s16 x, s16 y, u16 width, u16 height, bool use_separators, home_menu_page* parent)
			: home_menu_settings_page(x, y, width, height, use_separators, parent, get_localized_string(localized_string_id::HOME_MENU_SETTINGS_DEBUG))
		{
			add_checkbox(&g_cfg.video.debug_overlay, localized_string_id::HOME_MENU_SETTINGS_DEBUG_OVERLAY);
			add_checkbox(&g_cfg.io.pad_debug_overlay, localized_string_id::HOME_MENU_SETTINGS_DEBUG_INPUT_OVERLAY);
			add_checkbox(&g_cfg.io.mouse_debug_overlay, localized_string_id::HOME_MENU_SETTINGS_MOUSE_DEBUG_INPUT_OVERLAY);
			add_checkbox(&g_cfg.video.disable_video_output, localized_string_id::HOME_MENU_SETTINGS_DEBUG_DISABLE_VIDEO_OUTPUT);
			add_float_slider(&g_cfg.video.texture_lod_bias, localized_string_id::HOME_MENU_SETTINGS_DEBUG_TEXTURE_LOD_BIAS, "", 0.25f);

			apply_layout();
		}
	}
}
