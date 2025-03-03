#include "stdafx.h"
#include "overlay_manager.h"
#include "overlay_osk.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/Cell/timers.hpp"

LOG_CHANNEL(osk, "OSK");

namespace rsx
{
	namespace overlays
	{
		osk_dialog::osk_dialog()
		{
			m_auto_repeat_buttons.insert(pad_button::L1);
			m_auto_repeat_buttons.insert(pad_button::R1);
			m_auto_repeat_buttons.insert(pad_button::cross);
			m_auto_repeat_buttons.insert(pad_button::triangle);
			m_auto_repeat_buttons.insert(pad_button::square);

			m_keyboard_input_enabled = true;
		}

		void osk_dialog::Close(s32 status)
		{
			osk.notice("Closing osk (status=%d)", status);

			if (status == FAKE_CELL_OSKDIALOG_CLOSE_TERMINATE)
			{
				close(false, true);
				return;
			}

			if (status != FAKE_CELL_OSKDIALOG_CLOSE_ABORT && m_use_separate_windows && continuous_mode == CELL_OSKDIALOG_CONTINUOUS_MODE_REMAIN_OPEN)
			{
				// Just call the on_osk_close and don't actually close the dialog.
				if (on_osk_close)
				{
					Emu.CallFromMainThread([this, status]()
					{
						on_osk_close(status);
					});
				}
				return;
			}

			fade_animation.current = color4f(1.f);
			fade_animation.end = color4f(0.f);
			fade_animation.duration_sec = 0.5f;

			fade_animation.on_finish = [this, status]
			{
				if (on_osk_close)
				{
					Emu.CallFromMainThread([this, status]()
					{
						on_osk_close(status);
					});
				}

				set_visible(false);

				// Only really close the dialog if we aren't in the continuous separate window mode
				if (!m_use_separate_windows || continuous_mode == CELL_OSKDIALOG_CONTINUOUS_MODE_NONE)
				{
					close(true, true);
					return;
				}
			};

			fade_animation.active = true;
		}

		void osk_dialog::Clear(bool clear_all_data)
		{
			// Try to lock. Clear might be called recursively.
			const bool locked = m_preview_mutex.try_lock();

			osk.notice("Clearing osk (clear_all_data=%d)", clear_all_data);

			m_preview.caret_position = 0;
			m_preview.set_text({});

			if (clear_all_data)
			{
				on_text_changed();
			}

			if (locked)
			{
				m_preview_mutex.unlock();
			}

			m_update = true;
		}

		void osk_dialog::SetText(const std::u16string& text)
		{
			// Try to lock. Insert might be called recursively.
			const bool locked = m_preview_mutex.try_lock();

			const std::u16string new_str = text.length() <= char_limit ? text : text.substr(0, char_limit);

			osk.notice("Setting osk text (text='%s', new_str='%s', char_limit=%d)", utf16_to_ascii8(text), utf16_to_ascii8(new_str), char_limit);

			m_preview.caret_position = new_str.length();
			m_preview.set_unicode_text(utf16_to_u32string(new_str));

			on_text_changed();

			if (locked)
			{
				m_preview_mutex.unlock();
			}

			m_update = true;
		}

		void osk_dialog::Insert(const std::u16string& text)
		{
			// Try to lock. Insert might be called recursively.
			const bool locked = m_preview_mutex.try_lock();

			osk.notice("Inserting into osk at position %d (text='%s', char_limit=%d)", m_preview.caret_position, utf16_to_ascii8(text), char_limit);

			// Append to output text
			if (m_preview.value.empty())
			{
				const std::u16string new_str = text.length() <= char_limit ? text : text.substr(0, char_limit);

				m_preview.caret_position = new_str.length();
				m_preview.set_unicode_text(utf16_to_u32string(new_str));
			}
			else if ((m_preview.value.length() + text.length()) <= char_limit)
			{
				m_preview.insert_text(utf16_to_u32string(text));
			}
			else
			{
				osk.notice("Can't insert into osk: Character limit reached.");
			}

			on_text_changed();

			if (locked)
			{
				m_preview_mutex.unlock();
			}

			m_update = true;
		}

		void osk_dialog::add_panel(const osk_panel& panel)
		{
			// On PS3 apparently only 7 panels are added, the rest is ignored
			if (m_panels.size() < 7)
			{
				// Don't add this panel if there already exists one with the same panel mode
				if (std::none_of(m_panels.begin(), m_panels.end(), [&panel](const osk_panel& existing) { return existing.osk_panel_mode == panel.osk_panel_mode; }))
				{
					m_panels.push_back(panel);
				}
			}
		}

		void osk_dialog::step_panel(bool next_panel)
		{
			const usz num_panels = m_panels.size();

			if (num_panels > 0)
			{
				if (next_panel)
				{
					m_panel_index = (m_panel_index + 1) % num_panels;
				}
				else if (m_panel_index > 0)
				{
					m_panel_index = (m_panel_index - 1) % num_panels;
				}
				else
				{
					m_panel_index = num_panels - 1;
				}
			}

			update_panel();
		}

		void osk_dialog::update_panel()
		{
			ensure(m_panel_index < m_panels.size());

			const auto& panel = m_panels[m_panel_index];

			num_rows = panel.num_rows;
			num_columns = panel.num_columns;
			cell_size_x = get_scaled(panel.cell_size_x);
			cell_size_y = get_scaled(panel.cell_size_y);

			update_layout();

			const u32 cell_count = num_rows * num_columns;

			m_grid.resize(cell_count);
			num_shift_layers_by_charset.clear();

			const position2u grid_origin(m_panel_frame.x, m_panel_frame.y);

			const u32 old_index = (selected_y * num_columns) + selected_x;

			u32 index = 0;

			for (const auto& props : panel.layout)
			{
				for (u32 c = 0; c < props.num_cell_hz; ++c)
				{
					const auto row = (index / num_columns);
					const auto col = (index % num_columns);
					ensure(row < num_rows && col < num_columns);

					auto& _cell = m_grid[index++];
					_cell.button_flag = props.type_flags;
					_cell.pos = { grid_origin.x + col * cell_size_x, grid_origin.y + row * cell_size_y };
					_cell.backcolor = props.color;
					_cell.callback = props.callback;
					_cell.outputs = props.outputs;
					_cell.selected = false;

					// Add shift layers
					for (u32 layer = 0; layer < _cell.outputs.size(); ++layer)
					{
						// Only add a shift layer if at least one default button has content in a layer

						if (props.type_flags != button_flags::_default)
						{
							continue;
						}

						usz cell_shift_layers = 0;

						for (usz i = 0; i < _cell.outputs[layer].size(); ++i)
						{
							if (_cell.outputs[layer][i].empty() == false)
							{
								cell_shift_layers = i + 1;
							}
						}

						if (layer >= num_shift_layers_by_charset.size())
						{
							num_shift_layers_by_charset.push_back(static_cast<u32>(cell_shift_layers));
						}
						else
						{
							num_shift_layers_by_charset[layer] = std::max(num_shift_layers_by_charset[layer], static_cast<u32>(cell_shift_layers));
						}
					}

					switch (props.type_flags)
					{
					default:
					case button_flags::_default:
						_cell.enabled = true;
						break;
					case button_flags::_space:
						_cell.enabled = !(flags & CELL_OSKDIALOG_NO_SPACE);
						break;
					case button_flags::_return:
						_cell.enabled = !(flags & CELL_OSKDIALOG_NO_RETURN);
						break;
					case button_flags::_shift:
						_cell.enabled |= !_cell.outputs.empty();
						break;
					case button_flags::_layer:
						_cell.enabled |= !num_shift_layers_by_charset.empty();
						break;
					}

					if (props.num_cell_hz == 1) [[likely]]
					{
						_cell.flags = border_flags::default_cell;
					}
					else if (c == 0)
					{
						// Leading cell
						_cell.flags = border_flags::start_cell;
					}
					else if (c == (props.num_cell_hz - 1))
					{
						// Last cell
						_cell.flags = border_flags::end_cell;
					}
					else
					{
						// Middle cell
						_cell.flags = border_flags::middle_cell;
					}
				}
			}

			ensure(num_shift_layers_by_charset.size());

			for (u32 layer = 0; layer < num_shift_layers_by_charset.size(); ++layer)
			{
				ensure(num_shift_layers_by_charset[layer]);
			}

			// Reset to first shift layer in the first charset, because the panel changed and we don't know if the layers are similar between panels.
			m_selected_charset = 0;
			selected_z = 0;

			// Enable/Disable the control buttons based on the current layout.
			update_controls();

			// Roughly keep x and y selection in grid if possible. Jumping to (0,0) would be annoying. Needs to be done after updating the control buttons.
			update_selection_by_index(old_index);

			m_update = true;
		}

		void osk_dialog::update_layout()
		{
			const bool show_panel = m_show_panel || !m_use_separate_windows;

			// The title is omitted in separate window mode
			const u16 title_height = m_use_separate_windows ? 0 : get_scaled(30);
			const u16 preview_height = get_scaled((flags & CELL_OSKDIALOG_NO_RETURN) ? 40 : 90);

			// Place elements with absolute positioning
			const u16 button_margin = get_scaled(30);
			const u16 button_height = get_scaled(30);
			const u16 panel_w = show_panel ? (num_columns * cell_size_x) : 0;
			const u16 panel_h = show_panel ? (num_rows * cell_size_y) : 0;
			const u16 input_w = m_use_separate_windows ? m_input_field_window_width : panel_w;
			const u16 input_h = title_height + preview_height;
			const u16 button_h = show_panel ? (button_height + button_margin) : 0;
			const u16 total_w = std::max(input_w, panel_w);
			const u16 total_h = input_h + panel_h + button_h;

			// The cellOskDialog's origin is at the center of the screen with positive values increasing toward the right and upper directions.
			// The layout mode tells us which corner of the dialog we should use for positioning.
			// With CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT and CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP and a point (0,0) the dialog's top left corner would be in the center of the screen.
			// With CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER and CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_CENTER and a point (0,0) the dialog would be exactly in the center of the screen.

			// TODO: Make sure separate windows don't overlap.

			// Calculate initial position and analog movement range.
			constexpr f32 margin = 50.0f; // Let's add a minimal margin on all sides
			const s16 x_min = static_cast<s16>(margin);
			const s16 x_max = static_cast<s16>(static_cast<f32>(virtual_width - total_w) - margin);
			const s16 y_min = static_cast<s16>(margin);
			const s16 y_max = static_cast<s16>(static_cast<f32>(virtual_height - total_h) - margin);
			s16 input_x = 0;
			s16 input_y = 0;
			s16 panel_x = 0;
			s16 panel_y = 0;

			// x pos should only be 0 the first time, because we always add a margin
			if (m_x_input_pos == 0)
			{
				const auto get_x = [](const osk_window_layout& layout, const u16& width) -> f32
				{
					constexpr f32 origin_x = virtual_width / 2.0f;
					const f32 x = origin_x + layout.x_offset;

					switch (layout.x_align)
					{
					case CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_RIGHT:
						return x - width;
					case CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER:
						return x - (width / 2.0f);
					case CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_LEFT:
					default:
						return x;
					}
				};

				const auto get_y = [](const osk_window_layout& layout, const u16& height) -> f32
				{
					constexpr f32 origin_y = virtual_height / 2.0f;
					const f32 y = origin_y - layout.y_offset; // Negative because we increase y towards the bottom and cellOsk increases y towards the top.

					switch (layout.y_align)
					{
					case CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_BOTTOM:
						return y - height;
					case CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_CENTER:
						return y - (height / 2.0f);
					case CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP:
					default:
						return y;
					}
				};

				if (m_use_separate_windows)
				{
					input_x = m_x_input_pos = static_cast<s16>(std::clamp<f32>(get_x(m_input_layout, input_w), x_min, x_max));
					input_y = m_y_input_pos = static_cast<s16>(std::clamp<f32>(get_y(m_input_layout, input_h), y_min, y_max));
					panel_x = m_x_panel_pos = static_cast<s16>(std::clamp<f32>(get_x(m_panel_layout, panel_w), x_min, x_max));
					panel_y = m_y_panel_pos = static_cast<s16>(std::clamp<f32>(get_y(m_panel_layout, panel_h), static_cast<f32>(y_min + input_h), static_cast<f32>(y_max + input_h)));
				}
				else
				{
					input_x = panel_x = m_x_input_pos = m_x_panel_pos = static_cast<u16>(std::clamp<f32>(get_x(m_layout, total_w), x_min, x_max));
					input_y = m_y_input_pos = static_cast<s16>(std::clamp<f32>(get_y(m_layout, total_h), y_min, y_max));
					panel_y = m_y_panel_pos = input_y + input_h;
				}
			}
			else if (m_use_separate_windows)
			{
				input_x = m_x_input_pos = std::clamp(m_x_input_pos, x_min, x_max);
				input_y = m_y_input_pos = std::clamp(m_y_input_pos, y_min, y_max);
				panel_x = m_x_panel_pos = std::clamp(m_x_panel_pos, x_min, x_max);
				panel_y = m_y_panel_pos = std::clamp<s16>(m_y_panel_pos, y_min + input_h, y_max + input_h);
			}
			else
			{
				input_x = panel_x = m_x_input_pos = m_x_panel_pos = std::clamp(m_x_input_pos, x_min, x_max);
				input_y = m_y_input_pos = std::clamp(m_y_input_pos, y_min, y_max);
				panel_y = m_y_panel_pos = input_y + input_h;
			}

			m_input_frame.set_pos(input_x, input_y);
			m_input_frame.set_size(input_w, input_h);

			m_panel_frame.set_pos(panel_x, panel_y);
			m_panel_frame.set_size(panel_w, panel_h);

			m_title.set_pos(input_x, input_y);
			m_title.set_size(input_w, title_height);
			m_title.set_padding(get_scaled(15), 0, get_scaled(5), 0);

			m_preview.set_pos(input_x, input_y + title_height);
			m_preview.set_size(input_w, preview_height);
			m_preview.set_padding(get_scaled(15), get_scaled(15), get_scaled(10), 0);

			const s16 button_y = panel_y + panel_h + button_margin;

			m_btn_cancel.set_pos(panel_x, button_y);
			m_btn_cancel.set_size(get_scaled(140), button_height);
			m_btn_cancel.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_CANCEL);
			m_btn_cancel.set_text_vertical_adjust(get_scaled(5));

			m_btn_space.set_pos(panel_x + get_scaled(100), button_y);
			m_btn_space.set_size(get_scaled(100), button_height);
			m_btn_space.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SPACE);
			m_btn_space.set_text_vertical_adjust(get_scaled(5));

			m_btn_delete.set_pos(panel_x + get_scaled(200), button_y);
			m_btn_delete.set_size(get_scaled(100), button_height);
			m_btn_delete.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_BACKSPACE);
			m_btn_delete.set_text_vertical_adjust(get_scaled(5));

			m_btn_shift.set_pos(panel_x + get_scaled(320), button_y);
			m_btn_shift.set_size(get_scaled(80), button_height);
			m_btn_shift.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SHIFT);
			m_btn_shift.set_text_vertical_adjust(get_scaled(5));

			m_btn_accept.set_pos(panel_x + get_scaled(400), button_y);
			m_btn_accept.set_size(get_scaled(100), button_height);
			m_btn_accept.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ACCEPT);
			m_btn_accept.set_text_vertical_adjust(get_scaled(5));

			m_update = true;
		}

		void osk_dialog::initialize_layout(const std::u32string& title, const std::u32string& initial_text)
		{
			const auto scale_font = [this](overlay_element& elem)
			{
				if (const font* fnt = elem.get_font())
				{
					elem.set_font(fnt->get_name().data(), get_scaled(fnt->get_size_pt()));
				}
			};

			m_pointer.set_color(color4f{ 1.f, 1.f, 1.f, 1.f });

			m_background.set_size(virtual_width, virtual_height);

			m_title.set_unicode_text(title);
			scale_font(m_title);

			m_preview.password_mode = m_password_mode;
			m_preview.set_placeholder(get_placeholder());
			m_preview.set_unicode_text(initial_text);
			scale_font(m_preview);

			if (m_preview.value.empty())
			{
				m_preview.caret_position = 0;
				m_preview.fore_color.a = 0.5f; // Muted contrast for hint text
			}
			else
			{
				m_preview.caret_position = m_preview.value.length();
				m_preview.fore_color.a = 1.f;
			}

			scale_font(m_btn_shift);
			scale_font(m_btn_accept);
			scale_font(m_btn_space);
			scale_font(m_btn_delete);
			scale_font(m_btn_cancel);

			m_btn_shift.text_horizontal_offset = get_scaled(m_btn_shift.text_horizontal_offset);
			m_btn_accept.text_horizontal_offset = get_scaled(m_btn_accept.text_horizontal_offset);
			m_btn_space.text_horizontal_offset = get_scaled(m_btn_space.text_horizontal_offset);
			m_btn_delete.text_horizontal_offset = get_scaled(m_btn_delete.text_horizontal_offset);
			m_btn_cancel.text_horizontal_offset = get_scaled(m_btn_cancel.text_horizontal_offset);

			m_btn_shift.set_image_resource(resource_config::standard_image_resource::select);
			m_btn_accept.set_image_resource(resource_config::standard_image_resource::start);
			m_btn_space.set_image_resource(resource_config::standard_image_resource::triangle);
			m_btn_delete.set_image_resource(resource_config::standard_image_resource::square);

			if (g_cfg.sys.enter_button_assignment == enter_button_assign::circle)
			{
				m_btn_cancel.set_image_resource(resource_config::standard_image_resource::cross);
			}
			else
			{
				m_btn_cancel.set_image_resource(resource_config::standard_image_resource::circle);
			}

			m_update = true;
			set_visible(continuous_mode != CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE);

			m_stop_input_loop = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.duration_sec = 0.5f;
			fade_animation.active = true;
		}

		void osk_dialog::update_controls()
		{
			const bool shift_enabled = num_shift_layers_by_charset[m_selected_charset] > 1;
			const bool layer_enabled = num_shift_layers_by_charset.size() > 1;

			for (auto& cell : m_grid)
			{
				switch (cell.button_flag)
				{
				case button_flags::_shift:
					cell.enabled = shift_enabled;
					break;
				case button_flags::_layer:
					cell.enabled = layer_enabled;
					break;
				default:
					break;
				}
			}

			m_update = true;
		}

		std::pair<u32, u32> osk_dialog::get_cell_geometry(u32 index)
		{
			const u32 grid_size = num_columns * num_rows;
			u32 start_index = index;
			u32 count = 0;

			while (start_index >= grid_size && start_index >= num_columns)
			{
				// Try one row above
				start_index -= num_columns;
			}

			// Find first cell
			while (!(m_grid[start_index].flags & border_flags::left) && start_index)
			{
				--start_index;
			}

			// Find last cell
			while (true)
			{
				const u32 current_index = (start_index + count);
				ensure(current_index < grid_size);
				++count;

				if (m_grid[current_index].flags & border_flags::right)
				{
					break;
				}
			}

			return std::make_pair(start_index, count);
		}

		void osk_dialog::update_selection_by_index(u32 index)
		{
			auto select_cell = [&](u32 i, bool state)
			{
				const auto info = get_cell_geometry(i);

				// Tag all in range
				for (u32 _index = info.first, _ctr = 0; _ctr < info.second; ++_index, ++_ctr)
				{
					m_grid[_index].selected = state;
				}
			};

			// 1. Deselect current
			const auto current_index = (selected_y * num_columns) + selected_x;
			select_cell(current_index, false);

			// 2. Select new
			selected_y = index / num_columns;
			selected_x = index % num_columns;
			select_cell(index, true);
		}

		void osk_dialog::set_visible(bool visible)
		{
			if (m_use_separate_windows)
			{
				if (visible && continuous_mode == CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE)
				{
					continuous_mode = CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW;
				}
				else if (!visible && continuous_mode == CELL_OSKDIALOG_CONTINUOUS_MODE_SHOW)
				{
					continuous_mode = CELL_OSKDIALOG_CONTINUOUS_MODE_HIDE;
				}
			}

			if (this->visible != visible)
			{
				this->visible = visible;

				if (m_use_separate_windows)
				{
					osk.notice("set_visible: sending CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED with %s", visible ? "CELL_OSKDIALOG_DISPLAY_STATUS_SHOW" : "CELL_OSKDIALOG_DISPLAY_STATUS_HIDE");
					sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_DISPLAY_CHANGED, visible ? CELL_OSKDIALOG_DISPLAY_STATUS_SHOW : CELL_OSKDIALOG_DISPLAY_STATUS_HIDE);
				}
			}
		}

		void osk_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (!pad_input_enabled || ignore_device_events)
				return;

			if (input_device.exchange(CELL_OSKDIALOG_INPUT_DEVICE_PAD) != CELL_OSKDIALOG_INPUT_DEVICE_PAD)
			{
				osk.notice("on_button_pressed: sending CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED with CELL_OSKDIALOG_INPUT_DEVICE_PAD");
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED, CELL_OSKDIALOG_INPUT_DEVICE_PAD);
			}

			if (input_device != CELL_OSKDIALOG_INPUT_DEVICE_PAD)
			{
				return;
			}

			// Always show the pad input panel if the pad is enabled and in use.
			if (!m_show_panel)
			{
				m_show_panel = true;
				update_panel();
			}

			// Make sure to show the dialog and send necessary events
			set_visible(true);

			std::lock_guard lock(m_preview_mutex);

			const u32 grid_size = num_columns * num_rows;

			const auto on_accept = [this]()
			{
				const u32 current_index = (selected_y * num_columns) + selected_x;
				const auto& current_cell = m_grid[current_index];

				u32 output_count = 0;

				if (m_selected_charset < current_cell.outputs.size())
				{
					output_count = ::size32(current_cell.outputs[m_selected_charset]);
				}

				if (output_count)
				{
					const auto _z = std::clamp<u32>(selected_z, 0u, output_count - 1u);
					const auto& str = current_cell.outputs[m_selected_charset][_z];

					if (current_cell.callback)
					{
						current_cell.callback(str);
					}
					else
					{
						on_default_callback(str);
					}
				}
			};

			// Increase auto repeat interval for some buttons
			switch (button_press)
			{
			case pad_button::rs_left:
			case pad_button::rs_right:
			case pad_button::rs_down:
			case pad_button::rs_up:
				m_auto_repeat_ms_interval = 10;
				break;
			default:
				m_auto_repeat_ms_interval = m_auto_repeat_ms_interval_default;
				break;
			}

			bool play_cursor_sound = true;

			switch (button_press)
			{
			case pad_button::L1:
			{
				m_preview.move_caret(edit_text::direction::left);
				m_update = true;
				break;
			}
			case pad_button::R1:
			{
				m_preview.move_caret(edit_text::direction::right);
				m_update = true;
				break;
			}
			case pad_button::dpad_right:
			case pad_button::ls_right:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (true)
				{
					const auto current = get_cell_geometry(current_index);
					current_index = current.first + current.second;

					if (current_index >= grid_size)
					{
						break;
					}

					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						break;
					}
				}
				m_reset_pulse = true;
				break;
			}
			case pad_button::dpad_left:
			case pad_button::ls_left:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (current_index > 0)
				{
					const auto current = get_cell_geometry(current_index);
					if (current.first)
					{
						current_index = current.first - 1;

						if (m_grid[get_cell_geometry(current_index).first].enabled)
						{
							update_selection_by_index(current_index);
							break;
						}
					}
					else
					{
						break;
					}
				}
				m_reset_pulse = true;
				break;
			}
			case pad_button::dpad_down:
			case pad_button::ls_down:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (true)
				{
					current_index += num_columns;
					if (current_index >= grid_size)
					{
						break;
					}

					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						break;
					}
				}
				m_reset_pulse = true;
				break;
			}
			case pad_button::dpad_up:
			case pad_button::ls_up:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (current_index >= num_columns)
				{
					current_index -= num_columns;
					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						break;
					}
				}
				m_reset_pulse = true;
				break;
			}
			case pad_button::select:
			{
				on_shift(U"");
				break;
			}
			case pad_button::start:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_oskenter.wav");
				Close(CELL_OSKDIALOG_CLOSE_CONFIRM);
				play_cursor_sound = false;
				break;
			}
			case pad_button::triangle:
			{
				on_space(U"");
				break;
			}
			case pad_button::square:
			{
				on_backspace(U"");
				break;
			}
			case pad_button::cross:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_oskenter.wav");
				on_accept();
				m_reset_pulse = true;
				play_cursor_sound = false;
				break;
			}
			case pad_button::circle:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_oskcancel.wav");
				Close(CELL_OSKDIALOG_CLOSE_CANCEL);
				play_cursor_sound = false;
				break;
			}
			case pad_button::L2:
			{
				step_panel(false);
				break;
			}
			case pad_button::R2:
			{
				step_panel(true);
				break;
			}
			case pad_button::rs_left:
			case pad_button::rs_right:
			case pad_button::rs_down:
			case pad_button::rs_up:
			{
				if (!(flags & CELL_OSKDIALOG_NO_INPUT_ANALOG))
				{
					switch (button_press)
					{
					case pad_button::rs_left:  m_x_input_pos -= 5; m_x_panel_pos -= 5; break;
					case pad_button::rs_right: m_x_input_pos += 5; m_x_panel_pos += 5; break;
					case pad_button::rs_down:  m_y_input_pos += 5; m_y_panel_pos += 5; break;
					case pad_button::rs_up:    m_y_input_pos -= 5; m_y_panel_pos -= 5; break;
					default: break;
					}
					update_panel();
				}
				play_cursor_sound = false;
				break;
			}
			default:
				break;
			}

			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			if (play_cursor_sound && (!is_auto_repeat || m_auto_repeat_ms_interval >= m_auto_repeat_ms_interval_default))
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}

			if (m_reset_pulse)
			{
				m_update = true;
			}
		}

		void osk_dialog::on_key_pressed(u32 led, u32 mkey, u32 key_code, u32 out_key_code, bool pressed, std::u32string key)
		{
			if (!pressed || !keyboard_input_enabled || ignore_device_events)
				return;

			if (input_device.exchange(CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD) != CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD)
			{
				osk.notice("on_key_pressed: sending CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED with CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD");
				sysutil_send_system_cmd(CELL_SYSUTIL_OSKDIALOG_INPUT_DEVICE_CHANGED, CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD);
			}

			if (input_device != CELL_OSKDIALOG_INPUT_DEVICE_KEYBOARD)
			{
				return;
			}

			if (m_use_separate_windows && m_show_panel)
			{
				// Hide the pad input panel if the keyboard is in use during separate windows.
				m_show_panel = false;
				update_panel();
			}

			// Make sure to show the dialog and send necessary events
			set_visible(true);

			std::lock_guard lock(m_preview_mutex);

			// The key should normally be empty unless the backend couldn't find a match.
			const bool is_key_string_fallback = !key.empty();

			// Pure meta keys need to be treated with care, as their out key code contains the meta key code instead of the normal key code.
			const bool is_meta_key = mkey != 0 && key_code == CELL_KEYC_NO_EVENT && key.empty();

			osk.notice("osk_dialog::on_key_pressed(led=%d, mkey=%d, key_code=%d, out_key_code=%d, pressed=%d, is_key_string_fallback=%d, is_meta_key=%d)", led, mkey, key_code, out_key_code, pressed, is_key_string_fallback, is_meta_key);

			// Find matching key in the OSK
			const auto find_key = [&]() -> bool
			{
				if (is_meta_key)
				{
					// We don't need to process meta keys in the grid at the moment.
					// The key is valid either way, so we return true.
					// Only on_osk_key_input_entered is called later.
					return true;
				}

				// Get the string representation of this key (unless it's already set by the backend)
				if (key.empty())
				{
					// Get keyboard layout
					const u32 kb_mapping = static_cast<u32>(g_cfg.sys.keyboard_type.get());

					// Convert key to its u32string presentation
					const u16 converted_out_key = cellKbCnvRawCode(kb_mapping, mkey, led, out_key_code);
					std::u16string utf16_string;
					utf16_string.push_back(converted_out_key);
					key = utf16_to_u32string(utf16_string);
				}

				if (key.empty())
				{
					return false;
				}

				for (const cell& current_cell : m_grid)
				{
					for (const auto& output : current_cell.outputs)
					{
						for (const auto& str : output)
						{
							if (str == key)
							{
								// Apply key press
								if (current_cell.callback)
								{
									current_cell.callback(str);
								}
								else
								{
									on_default_callback(str);
								}

								return true;
							}
						}
					}
				}

				return false;
			};

			const bool found_key = find_key();

			if (is_key_string_fallback)
			{
				// We don't have a keycode, so we can't process any of the following code anyway
				return;
			}

			// Handle special input
			if (!found_key)
			{
				switch (out_key_code)
				{
				case CELL_KEYC_SPACE:
					on_space(key);
					break;
				case CELL_KEYC_BS:
					on_backspace(key);
					break;
				case CELL_KEYC_DELETE:
					on_delete(key);
					break;
				case CELL_KEYC_ESCAPE:
					Close(CELL_OSKDIALOG_CLOSE_CANCEL);
					break;
				case CELL_KEYC_RIGHT_ARROW:
					on_move_cursor(key, edit_text::direction::right);
					break;
				case CELL_KEYC_LEFT_ARROW:
					on_move_cursor(key, edit_text::direction::left);
					break;
				case CELL_KEYC_DOWN_ARROW:
					on_move_cursor(key, edit_text::direction::down);
					break;
				case CELL_KEYC_UP_ARROW:
					on_move_cursor(key, edit_text::direction::up);
					break;
				case CELL_KEYC_ENTER:
					if ((flags & CELL_OSKDIALOG_NO_RETURN))
					{
						Close(CELL_OSKDIALOG_CLOSE_CONFIRM);
					}
					else
					{
						on_enter(key);
					}
					break;
				default:
					break;
				}
			}

			if (on_osk_key_input_entered)
			{
				CellOskDialogKeyMessage key_message{};
				key_message.led = led;
				key_message.mkey = mkey;
				key_message.keycode = out_key_code;
				on_osk_key_input_entered(key_message);
			}
		}

		void osk_dialog::on_text_changed()
		{
			const std::u16string ws = u32string_to_utf16(m_preview.value);
			const usz length = std::min(osk_text.size(), ws.length() + 1) * sizeof(char16_t);
			memcpy(osk_text.data(), ws.c_str(), length);

			osk.notice("on_text_changed: osk_text='%s'", utf16_to_ascii8(ws));

			// Muted contrast for placeholder text
			m_preview.fore_color.a = m_preview.value.empty() ? 0.5f : 1.f;

			m_update = true;
		}

		void osk_dialog::on_default_callback(const std::u32string& str)
		{
			if (str.empty())
			{
				return;
			}

			// Append to output text
			if (m_preview.value.empty())
			{
				m_preview.caret_position = str.length();
				m_preview.set_unicode_text(str);
			}
			else
			{
				if (m_preview.value.length() >= char_limit)
				{
					return;
				}

				if ((m_preview.value.length() + str.length()) <= char_limit)
				{
					m_preview.insert_text(str);
				}
			}

			on_text_changed();
		}

		void osk_dialog::on_shift(const std::u32string&)
		{
			const u32 max = num_shift_layers_by_charset[m_selected_charset];
			selected_z = (selected_z + 1) % max;
			m_update = true;
		}

		void osk_dialog::on_layer(const std::u32string&)
		{
			const u32 num_charsets = std::max<u32>(::size32(num_shift_layers_by_charset), 1);
			m_selected_charset = (m_selected_charset + 1) % num_charsets;

			const u32 max_z_layer = num_shift_layers_by_charset[m_selected_charset] - 1;

			if (selected_z > max_z_layer)
			{
				selected_z = max_z_layer;
			}

			update_controls();

			m_update = true;
		}

		void osk_dialog::on_space(const std::u32string&)
		{
			if (!(flags & CELL_OSKDIALOG_NO_SPACE))
			{
				on_default_callback(U" ");
			}
			else
			{
				// Beep or give some other kind of visual feedback
			}
		}

		void osk_dialog::on_backspace(const std::u32string&)
		{
			m_preview.erase();
			on_text_changed();
		}

		void osk_dialog::on_delete(const std::u32string&)
		{
			m_preview.del();
			on_text_changed();
		}

		void osk_dialog::on_enter(const std::u32string&)
		{
			if (!(flags & CELL_OSKDIALOG_NO_RETURN))
			{
				on_default_callback(U"\n");
			}
			else
			{
				// Beep or give some other kind of visual feedback
			}
		}

		void osk_dialog::on_move_cursor(const std::u32string&, edit_text::direction dir)
		{
			m_preview.move_caret(dir);
			m_update = true;
		}

		std::u32string osk_dialog::get_placeholder() const
		{
			const localized_string_id id = m_password_mode
				? localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_PASSWORD
				: localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_TEXT;
			return get_localized_u32string(id);
		}

		void osk_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
				m_update = true;
			}

			osk_info& info = g_fxo->get<osk_info>();

			if (const bool pointer_enabled = info.pointer_enabled; pointer_enabled != m_pointer.visible())
			{
				m_pointer.set_expiration(pointer_enabled ? u64{umax} : 0);
				m_pointer.update_visibility(get_system_time());
				m_update = true;
			}

			if (m_pointer.visible() && m_pointer.set_position(static_cast<s16>(info.pointer_x), static_cast<s16>(info.pointer_y)))
			{
				m_update = true;
			}
		}

		compiled_resource osk_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			if (!m_update)
			{
				fade_animation.apply(m_cached_resource);
				return m_cached_resource;
			}

			m_cached_resource.clear();
			m_cached_resource.add(m_background.get_compiled());

			if (m_use_separate_windows && !m_show_panel)
			{
				std::lock_guard lock(m_preview_mutex);
				m_cached_resource.add(m_preview.get_compiled());
			}
			else
			{
				m_cached_resource.add(m_panel_frame.get_compiled());

				if (!m_use_separate_windows)
				{
					// The title is omitted in separate window mode
					m_cached_resource.add(m_input_frame.get_compiled());
					m_cached_resource.add(m_title.get_compiled());
				}
				{
					std::lock_guard lock(m_preview_mutex);
					m_cached_resource.add(m_preview.get_compiled());
				}
				m_cached_resource.add(m_btn_accept.get_compiled());
				m_cached_resource.add(m_btn_cancel.get_compiled());
				m_cached_resource.add(m_btn_shift.get_compiled());
				m_cached_resource.add(m_btn_space.get_compiled());
				m_cached_resource.add(m_btn_delete.get_compiled());

				overlay_element tmp;
				u16 buffered_cell_count = 0;
				bool render_label = false;

				const color4f disabled_back_color = { 0.3f, 0.3f, 0.3f, 1.f };
				const color4f disabled_fore_color = { 0.8f, 0.8f, 0.8f, 1.f };
				const color4f normal_fore_color = { 0.f, 0.f, 0.f, 1.f };

				label label;
				label.back_color = { 0.f, 0.f, 0.f, 0.f };
				label.set_padding(0, 0, get_scaled(10), 0);

				const auto scale_font = [this](overlay_element& elem)
				{
					if (const font* fnt = elem.get_font())
					{
						elem.set_font(fnt->get_name().data(), get_scaled(fnt->get_size_pt()));
					}
				};
				scale_font(label);

				if (m_reset_pulse)
				{
					// Reset the pulse slightly above 0 falling on each user interaction
					m_key_pulse_cache.set_sinus_offset(0.6f);
				}

				for (const auto& c : m_grid)
				{
					s16 x = static_cast<s16>(c.pos.x);
					s16 y = static_cast<s16>(c.pos.y);
					u16 w = cell_size_x;
					u16 h = cell_size_y;

					if (c.flags & border_flags::left)
					{
						x++;
						w--;
						buffered_cell_count = 0;
					}

					if (c.flags & border_flags::right)
					{
						w--;

						u32 output_count = 0;

						if (m_selected_charset < c.outputs.size())
						{
							output_count = ::size32(c.outputs[m_selected_charset]);
						}

						if (output_count)
						{
							const s16 offset_x = static_cast<s16>(buffered_cell_count * cell_size_x);
							const u16 full_width = static_cast<u16>(offset_x + cell_size_x);

							label.set_pos(x - offset_x, y);
							label.set_size(full_width, cell_size_y);
							label.fore_color = c.enabled ? normal_fore_color : disabled_fore_color;

							const auto _z = (selected_z < output_count) ? selected_z : output_count - 1u;
							label.set_unicode_text(c.outputs[m_selected_charset][_z]);
							label.align_text(rsx::overlays::overlay_element::text_align::center);
							render_label = true;
						}
					}

					if (c.flags & border_flags::top)
					{
						y++;
						h--;
					}

					if (c.flags & border_flags::bottom)
					{
						h--;
					}

					buffered_cell_count++;

					tmp.back_color = c.enabled? c.backcolor : disabled_back_color;
					tmp.set_pos(x, y);
					tmp.set_size(w, h);
					tmp.pulse_effect_enabled = c.selected;
					tmp.pulse_sinus_offset = m_key_pulse_cache.pulse_sinus_offset;

					m_cached_resource.add(tmp.get_compiled());

					if (render_label)
					{
						label.pulse_effect_enabled = c.selected;
						label.pulse_sinus_offset = m_key_pulse_cache.pulse_sinus_offset;
						m_cached_resource.add(label.get_compiled());
					}
				}

				m_cached_resource.add(m_pointer.get_compiled());
				m_reset_pulse = false;
			}

			m_update = false;

			fade_animation.apply(m_cached_resource);
			return m_cached_resource;
		}

		void osk_dialog::Create(const osk_params& params)
		{
			state = OskDialogState::Open;
			flags = params.prohibit_flags;
			char_limit = params.charlimit;
			m_layout = params.layout;
			m_input_layout = params.input_layout;
			m_panel_layout = params.panel_layout;
			m_input_field_window_width = params.input_field_window_width;
			m_scaling = params.initial_scale;
			m_input_frame.back_color.r = params.base_color.r;
			m_input_frame.back_color.g = params.base_color.g;
			m_input_frame.back_color.b = params.base_color.b;
			m_input_frame.back_color.a = params.base_color.a;
			m_panel_frame.back_color = m_input_frame.back_color;
			m_background.back_color.a = params.dimmer_enabled ? 0.8f : 0.0f;
			m_start_pad_interception = params.intercept_input;
			m_use_separate_windows = params.use_separate_windows;

			if (m_use_separate_windows)
			{
				// When using separate windows, we show the text field, but hide the pad input panel if the input device is a pad.
				m_show_panel = pad_input_enabled && input_device == CELL_OSKDIALOG_INPUT_DEVICE_PAD;
				m_preview.back_color.a = std::clamp(params.input_field_background_transparency, 0.0f, 1.0f);
			}
			else
			{
				m_title.back_color.a = 0.7f; // Uses the dimmed color of the frame background
			}

			const callback_t shift_cb  = [this](const std::u32string& text){ on_shift(text); };
			const callback_t layer_cb  = [this](const std::u32string& text){ on_layer(text); };
			const callback_t space_cb  = [this](const std::u32string& text){ on_space(text); };
			const callback_t delete_cb = [this](const std::u32string& text){ on_backspace(text); };
			const callback_t enter_cb  = [this](const std::u32string& text){ on_enter(text); };

			const auto is_supported = [&](u32 mode) -> bool
			{
				switch (mode)
				{
				case CELL_OSKDIALOG_PANELMODE_POLISH:
				case CELL_OSKDIALOG_PANELMODE_KOREAN:
				case CELL_OSKDIALOG_PANELMODE_TURKEY:
				case CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE:
				case CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE:
				case CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL:
				case CELL_OSKDIALOG_PANELMODE_DANISH:
				case CELL_OSKDIALOG_PANELMODE_SWEDISH:
				case CELL_OSKDIALOG_PANELMODE_NORWEGIAN:
				case CELL_OSKDIALOG_PANELMODE_FINNISH:
					return (params.panel_flag & mode) && (params.support_language & mode);
				default:
					return (params.panel_flag & mode);
				}
			};

			const auto has_language_support = [&](CellSysutilLang language)
			{
				switch (language)
				{
				case CELL_SYSUTIL_LANG_KOREAN: return is_supported(CELL_OSKDIALOG_PANELMODE_KOREAN);
				case CELL_SYSUTIL_LANG_FINNISH: return is_supported(CELL_OSKDIALOG_PANELMODE_FINNISH);
				case CELL_SYSUTIL_LANG_SWEDISH: return is_supported(CELL_OSKDIALOG_PANELMODE_SWEDISH);
				case CELL_SYSUTIL_LANG_DANISH: return is_supported(CELL_OSKDIALOG_PANELMODE_DANISH);
				case CELL_SYSUTIL_LANG_NORWEGIAN: return is_supported(CELL_OSKDIALOG_PANELMODE_NORWEGIAN);
				case CELL_SYSUTIL_LANG_POLISH: return is_supported(CELL_OSKDIALOG_PANELMODE_POLISH);
				case CELL_SYSUTIL_LANG_PORTUGUESE_BR: return is_supported(CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL);
				case CELL_SYSUTIL_LANG_TURKISH: return is_supported(CELL_OSKDIALOG_PANELMODE_TURKEY);
				case CELL_SYSUTIL_LANG_CHINESE_T: return is_supported(CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE);
				case CELL_SYSUTIL_LANG_CHINESE_S: return is_supported(CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE);
				default: return true;
				}
			};

			if (params.panel_flag & CELL_OSKDIALOG_PANELMODE_PASSWORD)
			{
				// If password was requested, then password has to be the only osk panel mode available to the user
				// first_view_panel can be ignored

				add_panel(osk_panel_password(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));

				m_password_mode = true;
			}
			else if (params.panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT || params.panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE)
			{
				// Prefer the systems settings
				// first_view_panel is ignored

				CellSysutilLang language = g_cfg.sys.language;

				// Fall back to english if the panel is not supported
				if (!has_language_support(language))
				{
					language = CELL_SYSUTIL_LANG_ENGLISH_US;
				}

				switch (g_cfg.sys.language)
				{
				case CELL_SYSUTIL_LANG_JAPANESE:
					if (params.panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE)
						add_panel(osk_panel_english(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					else
						add_panel(osk_panel_japanese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_FRENCH:
					add_panel(osk_panel_french(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_SPANISH:
					add_panel(osk_panel_spanish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_GERMAN:
					add_panel(osk_panel_german(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_ITALIAN:
					add_panel(osk_panel_italian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_DANISH:
					add_panel(osk_panel_danish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_NORWEGIAN:
					add_panel(osk_panel_norwegian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_DUTCH:
					add_panel(osk_panel_dutch(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_FINNISH:
					add_panel(osk_panel_finnish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_SWEDISH:
					add_panel(osk_panel_swedish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_PORTUGUESE_PT:
					add_panel(osk_panel_portuguese_pt(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_PORTUGUESE_BR:
					add_panel(osk_panel_portuguese_br(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_TURKISH:
					add_panel(osk_panel_turkey(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_POLISH:
					add_panel(osk_panel_polish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_RUSSIAN:
					add_panel(osk_panel_russian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_KOREAN:
					add_panel(osk_panel_korean(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_CHINESE_T:
					add_panel(osk_panel_traditional_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_CHINESE_S:
					add_panel(osk_panel_simplified_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				case CELL_SYSUTIL_LANG_ENGLISH_US:
				case CELL_SYSUTIL_LANG_ENGLISH_GB:
				default:
					add_panel(osk_panel_english(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
					break;
				}
			}
			else
			{
				// Append osk modes.

				// TODO: find out the exact order

				if (is_supported(CELL_OSKDIALOG_PANELMODE_LATIN))
				{
					add_panel(osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_ENGLISH))
				{
					add_panel(osk_panel_english(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_FRENCH))
				{
					add_panel(osk_panel_french(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_SPANISH))
				{
					add_panel(osk_panel_spanish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_ITALIAN))
				{
					add_panel(osk_panel_italian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_GERMAN))
				{
					add_panel(osk_panel_german(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_TURKEY))
				{
					add_panel(osk_panel_turkey(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_POLISH))
				{
					add_panel(osk_panel_polish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_RUSSIAN))
				{
					add_panel(osk_panel_russian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_DANISH))
				{
					add_panel(osk_panel_danish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_NORWEGIAN))
				{
					add_panel(osk_panel_norwegian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_DUTCH))
				{
					add_panel(osk_panel_dutch(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_SWEDISH))
				{
					add_panel(osk_panel_swedish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_FINNISH))
				{
					add_panel(osk_panel_finnish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_PORTUGUESE))
				{
					add_panel(osk_panel_portuguese_pt(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL))
				{
					add_panel(osk_panel_portuguese_br(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_KOREAN))
				{
					add_panel(osk_panel_korean(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE))
				{
					add_panel(osk_panel_traditional_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE))
				{
					add_panel(osk_panel_simplified_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_JAPANESE))
				{
					add_panel(osk_panel_japanese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA))
				{
					add_panel(osk_panel_japanese_hiragana(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA))
				{
					add_panel(osk_panel_japanese_katakana(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_ALPHABET))
				{
					add_panel(osk_panel_alphabet_half_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH))
				{
					add_panel(osk_panel_alphabet_full_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_NUMERAL))
				{
					add_panel(osk_panel_numeral_half_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH))
				{
					add_panel(osk_panel_numeral_full_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (is_supported(CELL_OSKDIALOG_PANELMODE_URL))
				{
					add_panel(osk_panel_url(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}

				// Get initial panel based on first_view_panel
				for (usz i = 0; i < m_panels.size(); ++i)
				{
					if (params.first_view_panel == m_panels[i].osk_panel_mode)
					{
						m_panel_index = i;
						break;
					}
				}
			}

			// Fallback to english in case we forgot something
			if (m_panels.empty())
			{
				osk.error("No OSK panel found. Using english panel.");
				add_panel(osk_panel_english(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
			}

			initialize_layout(utf16_to_u32string(params.message), utf16_to_u32string(params.init_text));

			update_panel();

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "OSK",
				[notify]() { *notify = true; notify->notify_one(); }
			);

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}
		}
	}
}
