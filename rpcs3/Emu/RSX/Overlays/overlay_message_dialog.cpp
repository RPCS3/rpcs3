#include "stdafx.h"
#include "overlay_message_dialog.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/ErrorCodes.h"

namespace rsx
{
	namespace overlays
	{
		message_dialog::message_dialog(bool use_custom_background)
		{
			background.set_size(1280, 720);
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

			progress_1.set_size(800, 4);
			progress_2.set_size(800, 4);
			progress_1.back_color = color4f(0.25f, 0.f, 0.f, 0.85f);
			progress_2.back_color = color4f(0.25f, 0.f, 0.f, 0.85f);

			btn_ok.set_text("Yes");
			btn_ok.set_size(140, 30);
			btn_ok.set_pos(545, 420);
			btn_ok.set_font("Arial", 16);

			btn_cancel.set_text("No");
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

			if (use_custom_background)
			{
				const auto picture_path = Emu.GetBackgroundPicturePath();

				if (fs::exists(picture_path))
				{
					background_image = std::make_unique<image_info>(picture_path.c_str());

					if (background_image->data)
					{
						const f32 color              = (100 - g_cfg.video.shader_preloading_dialog.darkening_strength) / 100.f;
						background_poster.fore_color = color4f(color, color, color, 1.);
						background.back_color.a      = 0.f;

						background_poster.set_size(1280, 720);
						background_poster.set_raw_image(background_image.get());
						background_poster.set_blur_strength(static_cast<u8>(g_cfg.video.shader_preloading_dialog.blur_strength));
					}
				}
			}

			return_code = CELL_MSGDIALOG_BUTTON_NONE;
		}

		compiled_resource message_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;

			if (background_image && background_image->data)
			{
				result.add(background_poster.get_compiled());
			}

			result.add(background.get_compiled());
			result.add(text_display.get_compiled());

			if (num_progress_bars > 0)
			{
				result.add(progress_1.get_compiled());
			}

			if (num_progress_bars > 1)
			{
				result.add(progress_2.get_compiled());
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

			return result;
		}

		void message_dialog::on_button_pressed(pad_button button_press)
		{
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

				break;
			}
			case pad_button::circle:
			{
				if (ok_only)
				{
					// Ignore cancel operation for Ok-only
					return;
				}
				else if (cancel_only)
				{
					return_code = CELL_MSGDIALOG_BUTTON_ESCAPE;
				}
				else
				{
					return_code = CELL_MSGDIALOG_BUTTON_NO;
				}

				break;
			}
			default: return;
			}

			close(true, true);
		}

		error_code message_dialog::show(bool is_blocking, const std::string& text, const MsgDialogType& type, std::function<void(s32 status)> on_close)
		{
			visible = false;

			num_progress_bars = type.progress_bar_count;
			if (num_progress_bars)
			{
				u16 offset = 58;
				progress_1.set_pos(240, 412);

				if (num_progress_bars > 1)
				{
					progress_2.set_pos(240, 462);
					offset = 98;
				}

				// Push the other stuff down
				bottom_bar.translate(0, offset);
				btn_ok.translate(0, offset);
				btn_cancel.translate(0, offset);
			}

			text_display.set_text(text);

			u16 text_w, text_h;
			text_display.measure_text(text_w, text_h);
			text_display.translate(0, -(text_h - 16));

			switch (type.button_type.unshifted())
			{
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE:
				interactive = !type.disable_cancel;
				if (interactive)
				{
					btn_cancel.set_pos(585, btn_cancel.y);
					btn_cancel.set_text("Cancel");
					cancel_only = true;
				}
				break;
			case CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK:
				btn_ok.set_pos(600, btn_ok.y);
				btn_ok.set_text("OK");
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
					if (auto error = run_input_loop())
					{
						rsx_log.error("Dialog input loop exited with error code=%d", error);
						return error;
					}
				}
				else
				{
					while (!exit)
					{
						refresh();

						// Only update the screen at about 60fps since updating it everytime slows down the process
						std::this_thread::sleep_for(16ms);
					}
				}
			}
			else
			{
				if (!exit)
				{
					g_fxo->init<named_thread>("MsgDialog Thread", [&, tbit = alloc_thread_bit()]()
					{
						g_thread_bit = tbit;

						if (interactive)
						{
							auto ref = g_fxo->get<display_manager>()->get(uid);

							if (auto error = run_input_loop())
							{
								rsx_log.error("Dialog input loop exited with error code=%d", error);
							}
						}
						else
						{
							while (!exit && thread_ctrl::state() != thread_state::aborting)
							{
								refresh();

								// Only update the screen at about 60fps since updating it everytime slows down the process
								std::this_thread::sleep_for(16ms);

								if (!g_fxo->get<display_manager>())
								{
									rsx_log.fatal("display_manager was improperly destroyed");
									break;
								}
							}
						}

						thread_bits &= ~tbit;
						thread_bits.notify_all();
					});
				}
			}

			return CELL_OK;
		}

		u32 message_dialog::progress_bar_count()
		{
			return num_progress_bars;
		}

		void message_dialog::progress_bar_set_taskbar_index(s32 index)
		{
			taskbar_index = index;
		}

		error_code message_dialog::progress_bar_set_message(u32 index, const std::string& msg)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			if (index == 0)
				progress_1.set_text(msg);
			else
				progress_2.set_text(msg);

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_increment(u32 index, f32 value)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			if (index == 0)
				progress_1.inc(value);
			else
				progress_2.inc(value);

			if (index == static_cast<u32>(taskbar_index) || taskbar_index == -1)
				Emu.GetCallbacks().handle_taskbar_progress(1, static_cast<s32>(value));

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_reset(u32 index)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			if (index == 0)
				progress_1.set_value(0.f);
			else
				progress_2.set_value(0.f);

			Emu.GetCallbacks().handle_taskbar_progress(0, 0);

			return CELL_OK;
		}

		error_code message_dialog::progress_bar_set_limit(u32 index, u32 limit)
		{
			if (index >= num_progress_bars)
				return CELL_MSGDIALOG_ERROR_PARAM;

			if (index == 0)
				progress_1.set_limit(static_cast<f32>(limit));
			else
				progress_2.set_limit(static_cast<f32>(limit));

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
