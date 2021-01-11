#include "stdafx.h"
#include "overlay_osk.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/Modules/cellSysutil.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

LOG_CHANNEL(osk, "OSK");

namespace rsx
{
	namespace overlays
	{
		void osk_dialog::Close(bool ok)
		{
			fade_animation.current = color4f(1.f);
			fade_animation.end = color4f(0.f);
			fade_animation.duration = 0.5f;

			fade_animation.on_finish = [this, ok]
			{
				if (on_osk_close)
				{
					Emu.CallAfter([this, ok]()
					{
						on_osk_close(ok ? CELL_MSGDIALOG_BUTTON_OK : CELL_MSGDIALOG_BUTTON_ESCAPE);
					});
				}

				visible = false;
				close(true, true);
			};

			fade_animation.active = true;
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
			cell_size_x = panel.cell_size_x;
			cell_size_y = panel.cell_size_y;

			update_layout();

			const u32 cell_count = num_rows * num_columns;

			m_grid.resize(cell_count);
			num_shift_layers_by_charset.clear();

			const position2u grid_origin = { m_frame.x, m_frame.y + 30u + m_preview.h };

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
							num_shift_layers_by_charset.push_back(u32(cell_shift_layers));
						}
						else
						{
							num_shift_layers_by_charset[layer] = std::max(num_shift_layers_by_charset[layer], u32(cell_shift_layers));
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
			const u16 preview_height = (flags & CELL_OSKDIALOG_NO_RETURN) ? 40 : 90;

			// Place elements with absolute positioning
			u16 frame_w = u16(num_columns * cell_size_x);
			u16 frame_h = u16(num_rows * cell_size_y) + 30 + preview_height;
			u16 frame_x = (1280 - frame_w) / 2;
			u16 frame_y = (720 - frame_h) / 2;

			m_frame.set_pos(frame_x, frame_y);
			m_frame.set_size(frame_w, frame_h);

			m_title.set_pos(frame_x, frame_y);
			m_title.set_size(frame_w, 30);
			m_title.set_padding(15, 0, 5, 0);

			m_preview.set_pos(frame_x, frame_y + 30);
			m_preview.set_size(frame_w, preview_height);
			m_preview.set_padding(15, 0, 10, 0);

			m_btn_cancel.set_pos(frame_x, frame_y + frame_h + 10);
			m_btn_cancel.set_size(140, 30);
			m_btn_cancel.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_CANCEL);
			m_btn_cancel.set_text_vertical_adjust(5);

			m_btn_space.set_pos(frame_x + 100, frame_y + frame_h + 10);
			m_btn_space.set_size(100, 30);
			m_btn_space.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SPACE);
			m_btn_space.set_text_vertical_adjust(5);

			m_btn_delete.set_pos(frame_x + 200, frame_y + frame_h + 10);
			m_btn_delete.set_size(100, 30);
			m_btn_delete.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_BACKSPACE);
			m_btn_delete.set_text_vertical_adjust(5);

			m_btn_shift.set_pos(frame_x + 320, frame_y + frame_h + 10);
			m_btn_shift.set_size(80, 30);
			m_btn_shift.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_SHIFT);
			m_btn_shift.set_text_vertical_adjust(5);

			m_btn_accept.set_pos(frame_x + 400, frame_y + frame_h + 10);
			m_btn_accept.set_size(100, 30);
			m_btn_accept.set_text(localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ACCEPT);
			m_btn_accept.set_text_vertical_adjust(5);

			m_update = true;
		}

		void osk_dialog::initialize_layout(const std::u32string & title, const std::u32string & initial_text)
		{
			m_background.set_size(1280, 720);
			m_background.back_color.a = 0.8f;

			m_frame.back_color = { 0.2f, 0.2f, 0.2f, 1.f };

			m_title.set_text(title);
			m_title.back_color.a = 0.f;

			if (initial_text.empty())
			{
				m_preview.set_text(get_placeholder());
				m_preview.caret_position = 0;
				m_preview.fore_color.a = 0.5f; // Muted contrast for hint text
			}
			else
			{
				m_preview.set_text(initial_text);
				m_preview.caret_position = ::narrow<u16>(initial_text.length());
				m_preview.fore_color.a = 1.f;
			}

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
			visible = true;
			exit = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.duration = 0.5f;
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
			const auto index_limit = (num_columns * num_rows) - 1;
			u32 start_index = index;
			u32 count = 0;

			while (start_index > index_limit && start_index >= num_columns)
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
				const auto current_index = (start_index + count);
				ensure(current_index <= index_limit);

				if (m_grid[current_index].flags & border_flags::right)
				{
					++count;
					break;
				}

				++count;
			}

			return std::make_pair(start_index, count);
		}

		void osk_dialog::update_selection_by_index(u32 index)
		{
			auto select_cell = [&](u32 index, bool state)
			{
				const auto info = get_cell_geometry(index);

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

		void osk_dialog::on_button_pressed(pad_button button_press)
		{
			const auto index_limit = (num_columns * num_rows) - 1;

			auto on_accept = [&]()
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
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (true)
				{
					const auto current = get_cell_geometry(current_index);
					current_index = current.first + current.second;

					if (current_index > index_limit)
					{
						break;
					}

					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						m_update = true;
						break;
					}
				}

				break;
			}
			case pad_button::dpad_left:
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
							m_update = true;
							break;
						}
					}
					else
					{
						break;
					}
				}
				break;
			}
			case pad_button::dpad_down:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (true)
				{
					current_index += num_columns;
					if (current_index > index_limit)
					{
						break;
					}

					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						m_update = true;
						break;
					}
				}
				break;
			}
			case pad_button::dpad_up:
			{
				u32 current_index = (selected_y * num_columns) + selected_x;
				while (current_index >= num_columns)
				{
					current_index -= num_columns;
					if (m_grid[get_cell_geometry(current_index).first].enabled)
					{
						update_selection_by_index(current_index);
						m_update = true;
						break;
					}
				}
				break;
			}
			case pad_button::select:
			{
				on_shift(U"");
				break;
			}
			case pad_button::start:
			{
				Close(true);
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
				on_accept();
				break;
			}
			case pad_button::circle:
			{
				Close(false);
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
			default:
				break;
			}
		}

		void osk_dialog::on_text_changed()
		{
			const auto ws = u32string_to_utf16(m_preview.text);
			const auto length = (ws.length() + 1) * sizeof(char16_t);
			memcpy(osk_text, ws.c_str(), length);

			if (on_osk_input_entered)
			{
				on_osk_input_entered();
			}

			m_update = true;
		}

		void osk_dialog::on_default_callback(const std::u32string& str)
		{
			if (str.empty())
			{
				return;
			}

			// Append to output text
			if (m_preview.text == get_placeholder())
			{
				m_preview.caret_position = ::narrow<u16>(str.length());
				m_preview.set_text(str);
				m_preview.fore_color.a = 1.f;
			}
			else
			{
				if (m_preview.text.length() == char_limit)
				{
					return;
				}

				const auto new_str = m_preview.text + str;
				if (new_str.length() <= char_limit)
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

			if (m_preview.text.empty())
			{
				m_preview.set_text(get_placeholder());
			}

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

		std::u32string osk_dialog::get_placeholder()
		{
			const localized_string_id id = m_password_mode
				? localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_PASSWORD
				: localized_string_id::RSX_OVERLAYS_OSK_DIALOG_ENTER_TEXT;
			return get_localized_u32string(id);
		}

		void osk_dialog::update()
		{
			if (fade_animation.active)
			{
				fade_animation.update(rsx::get_current_renderer()->vblank_count);
				m_update = true;
			}
		}

		compiled_resource osk_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			if (m_update)
			{
				m_cached_resource.clear();
				m_cached_resource.add(m_background.get_compiled());
				m_cached_resource.add(m_frame.get_compiled());
				m_cached_resource.add(m_title.get_compiled());
				m_cached_resource.add(m_preview.get_compiled());
				m_cached_resource.add(m_btn_accept.get_compiled());
				m_cached_resource.add(m_btn_cancel.get_compiled());
				m_cached_resource.add(m_btn_shift.get_compiled());
				m_cached_resource.add(m_btn_space.get_compiled());
				m_cached_resource.add(m_btn_delete.get_compiled());

				overlay_element tmp;
				label m_label;
				u16   buffered_cell_count = 0;
				bool  render_label = false;

				const color4f disabled_back_color = { 0.3f, 0.3f, 0.3f, 1.f };
				const color4f disabled_fore_color = { 0.8f, 0.8f, 0.8f, 1.f };
				const color4f normal_fore_color = { 0.f, 0.f, 0.f, 1.f };

				m_label.back_color = { 0.f, 0.f, 0.f, 0.f };
				m_label.set_padding(0, 0, 10, 0);

				for (const auto& c : m_grid)
				{
					u16 x = u16(c.pos.x);
					u16 y = u16(c.pos.y);
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
							const u16 offset_x = u16(buffered_cell_count * cell_size_x);
							const u16 full_width = u16(offset_x + cell_size_x);

							m_label.set_pos(x - offset_x, y);
							m_label.set_size(full_width, cell_size_y);
							m_label.fore_color = c.enabled ? normal_fore_color : disabled_fore_color;

							const auto _z = (selected_z < output_count) ? selected_z : output_count - 1u;
							m_label.set_text(c.outputs[m_selected_charset][_z]);
							m_label.align_text(rsx::overlays::overlay_element::text_align::center);
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

					m_cached_resource.add(tmp.get_compiled());

					if (render_label)
					{
						m_label.pulse_effect_enabled = c.selected;
						m_cached_resource.add(m_label.get_compiled());
					}
				}

				m_update = false;
			}

			fade_animation.apply(m_cached_resource);
			return m_cached_resource;
		}

		void osk_dialog::Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 prohibit_flags, u32 panel_flag, u32 first_view_panel)
		{
			state = OskDialogState::Open;
			flags = prohibit_flags;
			char_limit = charlimit;

			callback_t shift_cb = std::bind(&osk_dialog::on_shift, this, std::placeholders::_1);
			callback_t layer_cb = std::bind(&osk_dialog::on_layer, this, std::placeholders::_1);
			callback_t space_cb = std::bind(&osk_dialog::on_space, this, std::placeholders::_1);
			callback_t delete_cb = std::bind(&osk_dialog::on_backspace, this, std::placeholders::_1);
			callback_t enter_cb = std::bind(&osk_dialog::on_enter, this, std::placeholders::_1);

			if (panel_flag & CELL_OSKDIALOG_PANELMODE_PASSWORD)
			{
				// If password was requested, then password has to be the only osk panel mode available to the user
				// first_view_panel can be ignored

				add_panel(osk_panel_password(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));

				// TODO: hide entered text with *

				m_password_mode = true;
			}
			else if (panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT || panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE)
			{
				// Prefer the systems settings
				// first_view_panel is ignored

				switch (g_cfg.sys.language)
				{
				case CELL_SYSUTIL_LANG_JAPANESE:
					if (panel_flag == CELL_OSKDIALOG_PANELMODE_DEFAULT_NO_JAPANESE)
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

				if (panel_flag & CELL_OSKDIALOG_PANELMODE_LATIN)
				{
					add_panel(osk_panel_latin(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_ENGLISH)
				{
					add_panel(osk_panel_english(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_FRENCH)
				{
					add_panel(osk_panel_french(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_SPANISH)
				{
					add_panel(osk_panel_spanish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_ITALIAN)
				{
					add_panel(osk_panel_italian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_GERMAN)
				{
					add_panel(osk_panel_german(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_TURKEY)
				{
					add_panel(osk_panel_turkey(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_POLISH)
				{
					add_panel(osk_panel_polish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_RUSSIAN)
				{
					add_panel(osk_panel_russian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_DANISH)
				{
					add_panel(osk_panel_danish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_NORWEGIAN)
				{
					add_panel(osk_panel_norwegian(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_DUTCH)
				{
					add_panel(osk_panel_dutch(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_SWEDISH)
				{
					add_panel(osk_panel_swedish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_FINNISH)
				{
					add_panel(osk_panel_finnish(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_PORTUGUESE)
				{
					add_panel(osk_panel_portuguese_pt(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_PORTUGUESE_BRAZIL)
				{
					add_panel(osk_panel_portuguese_br(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_KOREAN)
				{
					add_panel(osk_panel_korean(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_TRADITIONAL_CHINESE)
				{
					add_panel(osk_panel_traditional_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_SIMPLIFIED_CHINESE)
				{
					add_panel(osk_panel_simplified_chinese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_JAPANESE)
				{
					add_panel(osk_panel_japanese(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_JAPANESE_HIRAGANA)
				{
					add_panel(osk_panel_japanese_hiragana(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA)
				{
					add_panel(osk_panel_japanese_katakana(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_ALPHABET)
				{
					add_panel(osk_panel_alphabet_half_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_ALPHABET_FULL_WIDTH)
				{
					add_panel(osk_panel_alphabet_full_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_NUMERAL)
				{
					add_panel(osk_panel_numeral_half_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH)
				{
					add_panel(osk_panel_numeral_full_width(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}
				if (panel_flag & CELL_OSKDIALOG_PANELMODE_URL)
				{
					add_panel(osk_panel_url(shift_cb, layer_cb, space_cb, delete_cb, enter_cb));
				}

				// Get initial panel based on first_view_panel
				for (usz i = 0; i < m_panels.size(); ++i)
				{
					if (first_view_panel == m_panels[i].osk_panel_mode)
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

			initialize_layout(utf16_to_u32string(message), utf16_to_u32string(init_text));

			update_panel();

			g_fxo->init<named_thread>("OSK Thread", [this, tbit = alloc_thread_bit()]
			{
				g_thread_bit = tbit;

				if (auto error = run_input_loop())
				{
					rsx_log.error("Osk input loop exited with error code=%d", error);
				}

				thread_bits &= ~tbit;
				thread_bits.notify_all();
			});
		}
	}
}
