#include "stdafx.h"
#include "overlays.h"
#include "Emu/RSX/RSXThread.h"

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

				m_visible = false;
				close();
			};

			fade_animation.active = true;
		}

		void osk_dialog::initialize_layout(const std::vector<grid_entry_ctor>& layout, const std::string& title, const std::string& initial_text)
		{
			const u32 cell_count = num_rows * num_columns;

			m_grid.resize(cell_count);
			u32 index = 0;

			for (const auto& props : layout)
			{
				for (u32 c = 0; c < props.num_cell_hz; ++c)
				{
					const auto row = (index / num_columns);
					const auto col = (index % num_columns);
					verify(HERE), row < num_rows && col < num_columns;

					auto &_cell = m_grid[index++];
					_cell.pos = { col * cell_size_x, row * cell_size_y };
					_cell.backcolor = props.color;
					_cell.callback = props.callback;
					_cell.outputs = props.outputs;
					_cell.selected = false;

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
					}

					num_layers = std::max(num_layers, u32(_cell.outputs.size()));

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

			verify(HERE), num_layers;

			// TODO: Should just scan for the first enabled cell
			selected_x = selected_y = selected_z = 0;
			m_grid[0].selected = true;

			m_background.set_size(1280, 720);
			m_background.back_color.a = 0.8f;

			const int preview_height = (flags & CELL_OSKDIALOG_NO_RETURN) ? 40 : 90;

			// Place elements with absolute positioning
			u16 frame_w = u16(num_columns * cell_size_x);
			u16 frame_h = u16(num_rows * cell_size_y) + 30 + preview_height;
			u16 frame_x = (1280 - frame_w) / 2;
			u16 frame_y = (720 - frame_h) / 2;

			m_frame.set_pos(frame_x, frame_y);
			m_frame.set_size(frame_w, frame_h);
			m_frame.back_color = { 0.2f, 0.2f, 0.2f, 1.f };

			m_title.set_pos(frame_x, frame_y);
			m_title.set_size(frame_w, 30);
			m_title.set_text(title);
			m_title.set_padding(15, 0, 5, 0);
			m_title.back_color.a = 0.f;

			m_preview.set_pos(frame_x, frame_y + 30);
			m_preview.set_size(frame_w, preview_height);
			m_preview.set_padding(15, 0, 10, 0);

			if (initial_text.empty())
			{
				m_preview.set_text("[Enter Text]");
				m_preview.caret_position = 0;
				m_preview.fore_color.a = 0.5f; // Muted contrast for hint text
			}
			else
			{
				m_preview.set_text(initial_text);
				m_preview.caret_position = ::narrow<u16>(initial_text.length());
				m_preview.fore_color.a = 1.f;
			}

			position2u grid_origin = { frame_x, frame_y + 30u + preview_height };
			for (auto &_cell : m_grid)
			{
				_cell.pos += grid_origin;
			}

			m_btn_cancel.set_pos(frame_x, frame_y + frame_h + 10);
			m_btn_cancel.set_size(140, 30);
			m_btn_cancel.set_text("Cancel");
			m_btn_cancel.set_text_vertical_adjust(5);

			m_btn_space.set_pos(frame_x + 100, frame_y + frame_h + 10);
			m_btn_space.set_size(100, 30);
			m_btn_space.set_text("Space");
			m_btn_space.set_text_vertical_adjust(5);

			m_btn_delete.set_pos(frame_x + 200, frame_y + frame_h + 10);
			m_btn_delete.set_size(100, 30);
			m_btn_delete.set_text("Backspace");
			m_btn_delete.set_text_vertical_adjust(5);

			m_btn_shift.set_pos(frame_x + 320, frame_y + frame_h + 10);
			m_btn_shift.set_size(80, 30);
			m_btn_shift.set_text("Shift");
			m_btn_shift.set_text_vertical_adjust(5);

			m_btn_accept.set_pos(frame_x + 400, frame_y + frame_h + 10);
			m_btn_accept.set_size(100, 30);
			m_btn_accept.set_text("Accept");
			m_btn_accept.set_text_vertical_adjust(5);

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

			m_visible = true;
			m_update = true;
			exit = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.duration = 0.5f;
			fade_animation.active = true;

			thread_ctrl::spawn("osk input thread", [this]
			{
				if (auto error = run_input_loop())
				{
					rsx_log.error("Osk input loop exited with error code=%d", error);
				}
			});
		}

		void osk_dialog::on_button_pressed(pad_button button_press)
		{
			const auto index_limit = (num_columns * num_rows) - 1;

			auto get_cell_geometry = [&](u32 index)
			{
				u32 start_index = index, count = 0;

				// Find first cell
				while (!(m_grid[start_index].flags & border_flags::left) && start_index)
				{
					start_index--;
				}

				// Find last cell
				while (true)
				{
					const auto current_index = (start_index + count);
					verify(HERE), current_index <= index_limit;

					if (m_grid[current_index].flags & border_flags::right)
					{
						count++;
						break;
					}

					count++;
				}

				return std::make_pair(start_index, count);
			};

			auto select_cell = [&](u32 index, bool state)
			{
				const auto info = get_cell_geometry(index);

				// Tag all in range
				for (u32 _index = info.first, _ctr = 0; _ctr < info.second; ++_index, ++_ctr)
				{
					m_grid[_index].selected = state;
				}
			};

			auto decode_index = [&](u32 index)
			{
				// 1. Deselect current
				auto current_index = (selected_y * num_columns) + selected_x;
				select_cell(current_index, false);

				// 2. Select new
				selected_y = index / num_columns;
				selected_x = index % num_columns;
				select_cell(index, true);
			};

			auto on_accept = [&]()
			{
				const u32 current_index = (selected_y * num_columns) + selected_x;
				const u32 output_count = ::size32(m_grid[current_index].outputs);

				if (output_count)
				{
					const auto _z = std::clamp<u32>(selected_z, 0u, output_count - 1u);
					const auto& str = m_grid[current_index].outputs[_z];

					if (m_grid[current_index].callback)
					{
						m_grid[current_index].callback(str);
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
						decode_index(current_index);
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
							decode_index(current_index);
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
						decode_index(current_index);
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
						decode_index(current_index);
						m_update = true;
						break;
					}
				}
				break;
			}
			case pad_button::select:
			{
				on_shift("");
				break;
			}
			case pad_button::start:
			{
				Close(true);
				break;
			}
			case pad_button::triangle:
			{
				on_space("");
				break;
			}
			case pad_button::square:
			{
				on_backspace("");
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
			default:
				break;
			}
		}

		void osk_dialog::on_text_changed()
		{
			const auto ws = ascii8_to_utf16(m_preview.text);
			const auto length = (ws.length() + 1) * sizeof(char16_t);
			memcpy(osk_text, ws.c_str(), length);

			if (on_osk_input_entered)
			{
				on_osk_input_entered();
			}

			m_update = true;
		}

		void osk_dialog::on_default_callback(const std::string& str)
		{
			// Append to output text
			if (m_preview.text == "[Enter Text]")
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

				auto new_str = m_preview.text + str;
				if (new_str.length() <= char_limit)
				{
					m_preview.insert_text(str);
				}
			}

			on_text_changed();
		}

		void osk_dialog::on_shift(const std::string&)
		{
			selected_z = (selected_z + 1) % num_layers;
			m_update = true;
		}

		void osk_dialog::on_space(const std::string&)
		{
			if (!(flags & CELL_OSKDIALOG_NO_SPACE))
			{
				on_default_callback(" ");
			}
			else
			{
				// Beep or give some other kind of visual feedback
			}
		}

		void osk_dialog::on_backspace(const std::string&)
		{
			if (m_preview.text.empty())
			{
				return;
			}

			m_preview.erase();
			on_text_changed();
		}

		void osk_dialog::on_enter(const std::string&)
		{
			if (!(flags & CELL_OSKDIALOG_NO_RETURN))
			{
				on_default_callback("\n");
			}
			else
			{
				// Beep or give some other kind of visual feedback
			}
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
			if (!m_visible)
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

						if (!c.outputs.empty())
						{
							u16 offset_x = u16(buffered_cell_count * cell_size_x);
							u16 full_width = u16(offset_x + cell_size_x);

							m_label.set_pos(x - offset_x, y);
							m_label.set_size(full_width, cell_size_y);
							m_label.fore_color = c.enabled ? normal_fore_color : disabled_fore_color;

							auto _z = (selected_z < c.outputs.size()) ? selected_z : u32(c.outputs.size()) - 1u;
							m_label.set_text(c.outputs[_z]);
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

		// Language specific implementations
		void osk_latin::Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 options)
		{
			state = OskDialogState::Open;
			flags = options;
			char_limit = charlimit;

			color4f default_bg = { 0.7f, 0.7f, 0.7f, 1.f };
			color4f special_bg = { 0.2f, 0.7f, 0.7f, 1.f };
			color4f special2_bg = { 0.83f, 0.81f, 0.57f, 1.f };

			num_rows = 5;
			num_columns = 10;
			cell_size_x = 50;
			cell_size_y = 40;

			callback_t shift_callback = std::bind(&osk_dialog::on_shift, this, std::placeholders::_1);
			callback_t space_callback = std::bind(&osk_dialog::on_space, this, std::placeholders::_1);
			callback_t delete_callback = std::bind(&osk_dialog::on_backspace, this, std::placeholders::_1);
			callback_t enter_callback = std::bind(&osk_dialog::on_enter, this, std::placeholders::_1);

			std::vector<osk_dialog::grid_entry_ctor> layout =
			{
				// Alphanumeric
				{{"1", "!"}, default_bg, 1},
				{{"2", "@"}, default_bg, 1},
				{{"3", "#"}, default_bg, 1},
				{{"4", "$"}, default_bg, 1},
				{{"5", "%"}, default_bg, 1},
				{{"6", "^"}, default_bg, 1},
				{{"7", "&"}, default_bg, 1},
				{{"8", "*"}, default_bg, 1},
				{{"9", "("}, default_bg, 1},
				{{"0", ")"}, default_bg, 1},

				// Alpha
				{{"q", "Q"}, default_bg, 1},
				{{"w", "W"}, default_bg, 1},
				{{"e", "E"}, default_bg, 1},
				{{"r", "R"}, default_bg, 1},
				{{"t", "T"}, default_bg, 1},
				{{"y", "Y"}, default_bg, 1},
				{{"u", "U"}, default_bg, 1},
				{{"i", "I"}, default_bg, 1},
				{{"o", "O"}, default_bg, 1},
				{{"p", "P"}, default_bg, 1},
				{{"a", "A"}, default_bg, 1},
				{{"s", "S"}, default_bg, 1},
				{{"d", "D"}, default_bg, 1},
				{{"f", "F"}, default_bg, 1},
				{{"g", "G"}, default_bg, 1},
				{{"h", "H"}, default_bg, 1},
				{{"j", "J"}, default_bg, 1},
				{{"k", "K"}, default_bg, 1},
				{{"l", "L"}, default_bg, 1},
				{{"'", "\""}, default_bg, 1},
				{{"z", "Z"}, default_bg, 1},
				{{"x", "X"}, default_bg, 1},
				{{"c", "C"}, default_bg, 1},
				{{"v", "V"}, default_bg, 1},
				{{"b", "B"}, default_bg, 1},
				{{"n", "N"}, default_bg, 1},
				{{"m", "M"}, default_bg, 1},
				{{"-", "_"}, default_bg, 1},
				{{"+", "="}, default_bg, 1},
				{{",", "?"}, default_bg, 1},

				// Special
				{{"Shift"}, special2_bg, 2, button_flags::_default, shift_callback },
				{{"Space"}, special_bg, 4, button_flags::_space, space_callback },
				{{"Backspace"}, special_bg, 2, button_flags::_default, delete_callback },
				{{"Enter"}, special2_bg, 2, button_flags::_return, enter_callback },
			};

			// Narrow to utf-8 as native does not have support for non-ascii glyphs
			// TODO: Full multibyte string support in all of rsx::overlays (kd-11)
			initialize_layout(layout, utf16_to_ascii8(message), utf16_to_ascii8(init_text));
		}
	}
}
