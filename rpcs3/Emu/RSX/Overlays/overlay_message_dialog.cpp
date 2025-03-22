#include "stdafx.h"
#include "overlay_manager.h"
#include "overlay_message_dialog.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/IdManager.h"
#include "Utilities/Thread.h"

#include <thread>

namespace rsx
{
	namespace overlays
	{
		message_dialog::message_dialog(bool allow_custom_background)
			: custom_background_allowed(allow_custom_background)
		{
			background.set_size(virtual_width, virtual_height);
			background.back_color.a = 0.85f;

			text_display.set_size(1100, 40);
			text_display.set_pos(90, 364);
			text_display.set_font("Arial", 16);
			text_display.align_text(overlay_element::text_align::center);
			text_display.set_wrap_text(true);
			text_display.back_color.a = 0.f;

			bottom_bar.back_color = color4f(1.f, 1.f, 1.f, 1.f);
			bottom_bar.set_size(1200, 2);
			bottom_bar.set_pos(40, 400);

			for (progress_bar& bar : progress_bars)
			{
				bar.set_size(800, 4);
				bar.back_color = color4f(0.25f, 0.f, 0.f, 0.85f);
			}

			btn_ok.set_text(localized_string_id::RSX_OVERLAYS_MSG_DIALOG_YES);
			btn_ok.set_size(140, 30);
			btn_ok.set_pos(545, 420);
			btn_ok.set_font("Arial", 16);

			btn_cancel.set_text(localized_string_id::RSX_OVERLAYS_MSG_DIALOG_NO);
			btn_cancel.set_size(140, 30);
			btn_cancel.set_pos(685, 420);
			btn_cancel.set_font("Arial", 16);

			if (g_cfg.sys.enter_button_assignment == enter_button_assign::circle)
			{
				btn_ok.set_image_resource(resource_config::standard_image_resource::circle);
				btn_cancel.set_image_resource(resource_config::standard_image_resource::cross);
			}
			else
			{
				btn_ok.set_image_resource(resource_config::standard_image_resource::cross);
				btn_cancel.set_image_resource(resource_config::standard_image_resource::circle);
			}

			fade_animation.duration_sec = 0.15f;

			update_custom_background();

			return_code = CELL_MSGDIALOG_BUTTON_NONE;
		}

		compiled_resource message_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			if (const auto [dirty, text] = text_guard.get_text(); dirty)
			{
				u16 text_w, text_h;
				text_display.set_pos(90, 364);
				text_display.set_text(text);
				text_display.measure_text(text_w, text_h);
				text_display.translate(0, -(text_h - 16));
			}

			for (u32 i = 0; i < progress_bars.size(); i++)
			{
				if (const auto [dirty, text] = ::at32(bar_text_guard, i).get_text(); dirty)
				{
					::at32(progress_bars, i).set_text(text);
				}
			}

			compiled_resource result;

			update_custom_background();

			if (background_image && background_image->get_data())
			{
				result.add(background_poster.get_compiled());

				if (background_overlay_image && background_overlay_image->get_data())
				{
					result.add(background_overlay_poster.get_compiled());
				}
			}

			result.add(background.get_compiled());
			result.add(text_display.get_compiled());

			if (num_progress_bars > 0)
			{
				result.add(::at32(progress_bars, 0).get_compiled());
			}

			if (num_progress_bars > 1)
			{
				result.add(::at32(progress_bars, 1).get_compiled());
			}

			if (interactive)
			{
				if (!num_progress_bars)
					result.add(bottom_bar.get_compiled());

				if (!cancel_only)
					result.add(btn_ok.get_compiled());

				if (!ok_only)
					result.add(btn_cancel.get_compiled());
			}

			fade_animation.apply(result);

			return result;
		}

		void message_dialog::on_button_pressed(pad_button button_press, bool /*is_auto_repeat*/)
		{
			if (fade_animation.active) return;

			switch (button_press)
			{
			case pad_button::cross:
			{
				if (ok_only)
				{
					return_code = CELL_MSGDIALOG_BUTTON_OK;
				}
				else if (cancel_only)
				{
					// Do not accept for cancel-only dialogs
					return;
				}
				else
				{
					return_code = CELL_MSGDIALOG_BUTTON_YES;
				}

				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
				break;
			}
			case pad_button::circle:
			{
				if (ok_only)
				{
					// Ignore cancel operation for Ok-only
					return;
				}

				if (cancel_only)
				{
					return_code = CELL_MSGDIALOG_BUTTON_ESCAPE;
				}
				else
				{
					return_code = CELL_MSGDIALOG_BUTTON_NO;
				}

				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				break;
			}
			default: return;
			}

			fade_animation.current = color4f(1.f);
			fade_animation.end = color4f(0.f);
			fade_animation.active = true;

			fade_animation.on_finish = [this]
			{
				close(true, true);
			};
		}

		void message_dialog::close(bool use_callback, bool stop_pad_interception)
		{
			if (num_progress_bars > 0)
			{
				Emu.GetCallbacks().handle_taskbar_progress(0, 1);
			}

			user_interface::close(use_callback, stop_pad_interception);
		}

		void message_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
				fade_animation.update(timestamp_us);
		}

		error_code message_dialog::show(bool is_blocking, const std::string& text, const MsgDialogType& type, msg_dialog_source source, std::function<void(s32 status)> on_close)
		{
			visible = false;
			m_source = source;

			num_progress_bars = type.progress_bar_count;
			if (num_progress_bars)
			{
				s16 offset = 58;
				::at32(progress_bars, 0).set_pos(240, 412);

				if (num_progress_bars > 1)
				{
					::at32(progress_bars, 1).set_pos(240, 462);
					offset = 98;
				}

				// Push the other stuff down
				bottom_bar.translate(0, offset);
				btn_ok.translate(0, offset);
				btn_cancel.translate(0, offset);
			}
			else
			{
				fade_animation.current = color4f(0.f);
				fade_animation.end = color4f(1.f);
				fade_animation.active = true;
			}

			if (!type.se_mute_on)
			{
				if (type.se_normal)
					Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_system_ok.wav");
				else
					Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_system_ng.wav");
			}

			set_text(text);

			switch (type.button_type.unshifted())
			{
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
				interactive = !type.disable_cancel;
				if (interactive)
				{
					btn_cancel.set_pos(585, btn_cancel.y);
					btn_cancel.set_text(localized_string_id::RSX_OVERLAYS_MSG_DIALOG_CANCEL);
					cancel_only = true;
				}
				break;
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
				btn_ok.set_pos(600, btn_ok.y);
				btn_ok.set_text(localized_string_id::RSX_OVERLAYS_MSG_DIALOG_OK);
				interactive = true;
				ok_only     = true;
				break;
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO:
				interactive = true;
				break;
			default:
				break;
			}

			this->on_close = std::move(on_close);
			visible = true;

			if (is_blocking)
			{
				if (interactive)
				{
					if (const auto error = run_input_loop())
					{
						if (error != selection_code::canceled)
						{
							rsx_log.error("Message dialog input loop exited with error code=%d", error);
						}
						return error;
					}
				}
				else
				{
					while (!m_stop_input_loop)
					{
						refresh();

						// Only update the screen at about 60fps since updating it everytime slows down the process
						std::this_thread::sleep_for(16ms);
					}
				}
			}
			else
			{
				if (!m_stop_input_loop)
				{
					const auto notify = std::make_shared<atomic_t<u32>>(0);
					auto& overlayman = g_fxo->get<display_manager>();

					if (interactive)
					{
						overlayman.attach_thread_input(
							uid, "Message dialog",
							[notify]() { *notify = true; notify->notify_one(); }
						);
					}
					else
					{
						overlayman.attach_thread_input(
							uid, "Message dialog",
							[notify]() { *notify = true; notify->notify_one(); },
							nullptr,
							[&]()
							{
								while (!m_stop_input_loop && thread_ctrl::state() != thread_state::aborting)
								{
									refresh();

									// Only update the screen at about 60fps since updating it everytime slows down the process
									std::this_thread::sleep_for(16ms);

									if (!g_fxo->is_init<display_manager>())
									{
										rsx_log.fatal("display_manager was improperly destroyed");
										break;
									}
								}

								return 0;
							}
						);
					}

					while (!Emu.IsStopped() && !*notify)
					{
						notify->wait(false, atomic_wait_timeout{1'000'000});
					}
				}
			}

			return CELL_OK;
		}

		void message_dialog::set_text(std::string text)
		{
			text_guard.set_text(std::move(text));
		}

		void message_dialog::update_custom_background()
		{
			if (custom_background_allowed && g_cfg.video.shader_preloading_dialog.use_custom_background)
			{
				bool dirty = std::exchange(background_blur_strength, g_cfg.video.shader_preloading_dialog.blur_strength.get()) != background_blur_strength;
				dirty     |= std::exchange(background_darkening_strength, g_cfg.video.shader_preloading_dialog.darkening_strength.get()) != background_darkening_strength;

				if (!background_image)
				{
					// Search for any useable background picture in the given order
					game_content_type content_type = game_content_type::background_picture;

					for (game_content_type type : { game_content_type::background_picture, game_content_type::overlay_picture, game_content_type::content_icon })
					{
						if (const std::string picture_path = rpcs3::utils::get_game_content_path(type); !picture_path.empty())
						{
							content_type = type;
							background_image = std::make_unique<image_info>(picture_path);
							dirty |= !!background_image->get_data();
							break;
						}
					}

					// Search for an overlay picture in the same dir in case we found a real background picture
					if (background_image && !background_overlay_image && content_type == game_content_type::background_picture)
					{
						if (const std::string picture_path = rpcs3::utils::get_game_content_path(game_content_type::overlay_picture); !picture_path.empty())
						{
							background_overlay_image = std::make_unique<image_info>(picture_path);
							dirty |= !!background_overlay_image->get_data();
						}
					}
				}

				if (dirty && background_image && background_image->get_data())
				{
					const f32 color              = (100 - background_darkening_strength) / 100.f;
					background_poster.fore_color = color4f(color, color, color, 1.);
					background.back_color.a      = 0.f;

					background_poster.set_size(virtual_width, virtual_height);
					background_poster.set_raw_image(background_image.get());
					background_poster.set_blur_strength(static_cast<u8>(background_blur_strength));

					ensure(background_image->w > 0);
					ensure(background_image->h > 0);
					ensure(background_poster.h > 0);

					// Set padding in order to keep the aspect ratio
					if ((background_image->w / static_cast<double>(background_image->h)) > (background_poster.w / static_cast<double>(background_poster.h)))
					{
						const int padding = (background_poster.h - static_cast<int>(background_image->h * (background_poster.w / static_cast<double>(background_image->w)))) / 2;
						background_poster.set_padding(0, 0, padding, padding);
					}
					else
					{
						const int padding = (background_poster.w - static_cast<int>(background_image->w * (background_poster.h / static_cast<double>(background_image->h)))) / 2;
						background_poster.set_padding(padding, padding, 0, 0);
					}

					if (background_overlay_image && background_overlay_image->get_data())
					{
						constexpr f32 reference_factor = 2.0f / 3.0f;
						const f32 image_aspect = background_overlay_image->w / static_cast<f32>(background_overlay_image->h);
						const f32 overlay_width = background_overlay_image->w * reference_factor;
						const f32 overlay_height = overlay_width / image_aspect;
						const u16 overlay_x = static_cast<u16>(std::min(virtual_width - overlay_width, (virtual_width * reference_factor) - (overlay_width / 2.0f)));
						const u16 overlay_y = static_cast<u16>(std::min(virtual_height - overlay_height, (virtual_height * reference_factor) - (overlay_height / 2.0f)));
						const f32 color = (100 - background_darkening_strength) / 100.f;

						background_overlay_poster.fore_color = color4f(color, color, color, 1.);
						background_overlay_poster.set_size(static_cast<u16>(overlay_width), static_cast<u16>(overlay_height));
						background_overlay_poster.set_pos(overlay_x, overlay_y);
						background_overlay_poster.set_raw_image(background_overlay_image.get());
						background_overlay_poster.set_blur_strength(static_cast<u8>(background_blur_strength));
					}
				}
			}
			else
			{
				if (background_image)
				{
					background_poster.clear_image();
					background_image.reset();
				}

				if (background_overlay_image)
				{
					background_overlay_poster.clear_image();
					background_overlay_image.reset();
				}

				background.back_color.a = 0.85f;
			}
		}

		u32 message_dialog::progress_bar_count() const
		{
			return num_progress_bars;
		}

		void message_dialog::progress_bar_set_taskbar_index(s32 index)
		{
			taskbar_index = index;
		}

		error_code message_dialog::progress_bar_set_message(u32 index, std::string msg)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			::at32(bar_text_guard, index).set_text(std::move(msg));

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_increment(u32 index, f32 value)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			::at32(progress_bars, index).inc(value);

			if (index == static_cast<u32>(taskbar_index) || taskbar_index == -1)
				Emu.GetCallbacks().handle_taskbar_progress(1, static_cast<s32>(value));

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_set_value(u32 index, f32 value)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			::at32(progress_bars, index).set_value(value);

			if (index == static_cast<u32>(taskbar_index) || taskbar_index == -1)
				Emu.GetCallbacks().handle_taskbar_progress(3, static_cast<s32>(value));

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_reset(u32 index)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			::at32(progress_bars, index).set_value(0.f);

			Emu.GetCallbacks().handle_taskbar_progress(0, 0);

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_set_limit(u32 index, u32 limit)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			::at32(progress_bars, index).set_limit(static_cast<f32>(limit));

			if (index == static_cast<u32>(taskbar_index))
			{
				taskbar_limit = limit;
				Emu.GetCallbacks().handle_taskbar_progress(2, taskbar_limit);
			}
			else if (taskbar_index == -1)
			{
				taskbar_limit += limit;
				Emu.GetCallbacks().handle_taskbar_progress(2, taskbar_limit);
			}

			return CELL_OK;
		}
	} // namespace overlays
} // namespace rsx
