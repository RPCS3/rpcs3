#include "stdafx.h"
#include "overlay_edit_text.hpp"

namespace rsx
{
	namespace overlays
	{
		static usz get_line_start(std::u32string_view text, usz pos)
		{
			if (pos == 0)
			{
				return 0;
			}
			const usz line_start = text.rfind('\n', pos - 1);
			if (line_start == umax)
			{
				return 0;
			}
			return line_start + 1;
		}

		static usz get_line_end(std::u32string_view text, usz pos)
		{
			const usz line_end = text.find('\n', pos);
			if (line_end == umax)
			{
				return text.length();
			}
			return line_end;
		}

		void edit_text::move_caret(direction dir)
		{
			m_reset_caret_pulse = true;

			switch (dir)
			{
			case direction::left:
			{
				if (caret_position)
				{
					caret_position--;
					refresh();
				}
				break;
			}
			case direction::right:
			{
				if (caret_position < value.length())
				{
					caret_position++;
					refresh();
				}
				break;
			}
			case direction::up:
			{
				const usz current_line_start = get_line_start(value, caret_position);
				if (current_line_start == 0)
				{
					// This is the first line, so caret moves to the very beginning
					caret_position = 0;
					refresh();
					break;
				}
				const usz caret_pos_in_line = caret_position - current_line_start;
				const usz prev_line_end     = current_line_start - 1;
				const usz prev_line_start   = get_line_start(value, prev_line_end);
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				caret_position = std::min(prev_line_end, prev_line_start + caret_pos_in_line);

				refresh();
				break;
			}
			case direction::down:
			{
				const usz current_line_end = get_line_end(value, caret_position);
				if (current_line_end == value.length())
				{
					// This is the last line, so caret moves to the very end
					caret_position = current_line_end;
					refresh();
					break;
				}
				const usz current_line_start = get_line_start(value, caret_position);
				const usz caret_pos_in_line  = caret_position - current_line_start;
				const usz next_line_start    = current_line_end + 1;
				const usz next_line_end      = get_line_end(value, next_line_start);
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				caret_position = std::min(next_line_end, next_line_start + caret_pos_in_line);

				refresh();
				break;
			}
			}
		}

		void edit_text::set_text(const std::string& text)
		{
			set_unicode_text(utf8_to_u32string(text));
		}

		void edit_text::set_unicode_text(const std::u32string& text)
		{
			value = text;

			if (value.empty())
			{
				overlay_element::set_unicode_text(placeholder);
			}
			else if (password_mode)
			{
				overlay_element::set_unicode_text(std::u32string(value.size(), U"＊"[0]));
			}
			else
			{
				overlay_element::set_unicode_text(value);
			}
		}

		void edit_text::set_placeholder(const std::u32string& placeholder_text)
		{
			placeholder = placeholder_text;
		}

		void edit_text::insert_text(const std::u32string& str)
		{
			if (caret_position == 0)
			{
				// Start
				value = str + text;
			}
			else if (caret_position == text.length())
			{
				// End
				value += str;
			}
			else
			{
				// Middle
				value.insert(caret_position, str);
			}

			caret_position += str.length();
			m_reset_caret_pulse = true;
			set_unicode_text(value);
			refresh();
		}

		void edit_text::erase()
		{
			if (!caret_position)
			{
				return;
			}

			if (caret_position == 1)
			{
				value = value.length() > 1 ? value.substr(1) : U"";
			}
			else if (caret_position == text.length())
			{
				value = value.substr(0, caret_position - 1);
			}
			else
			{
				value = value.substr(0, caret_position - 1) + value.substr(caret_position);
			}

			caret_position--;
			m_reset_caret_pulse = true;
			set_unicode_text(value);
			refresh();
		}

		void edit_text::del()
		{
			if (caret_position >= text.length())
			{
				return;
			}

			if (caret_position == 0)
			{
				value = value.length() > 1 ? value.substr(1) : U"";
			}
			else
			{
				value = value.substr(0, caret_position) + value.substr(caret_position + 1);
			}

			m_reset_caret_pulse = true;
			set_unicode_text(value);
			refresh();
		}

		compiled_resource& edit_text::get_compiled()
		{
			if (!is_compiled())
			{
				auto renderer = get_font();
				const auto [caret_x, caret_y] = renderer->get_char_offset(text.c_str(), caret_position, clip_text ? w : -1, wrap_text);

				overlay_element caret;
				caret.set_pos(static_cast<u16>(caret_x) + padding_left + x, static_cast<u16>(caret_y) + padding_top + y);
				caret.set_size(1, static_cast<u16>(renderer->get_size_px() + 2));
				caret.fore_color           = fore_color;
				caret.back_color           = fore_color;
				caret.pulse_effect_enabled = true;

				if (m_reset_caret_pulse)
				{
					// Reset the pulse slightly below 1 rising on each user interaction
					caret.set_sinus_offset(1.6f);
					m_reset_caret_pulse = false;
				}

				// Check if we have to scroll horizontally
				// TODO: Vertical scrolling
				const s32 available_width = w - padding_left - padding_right;
				const f32 offset_right = caret_x + caret.w - available_width;

				if (text.empty())
				{
					// Scroll to the beginning
					horizontal_scroll_offset = 0.0f;
				}
				else if (horizontal_scroll_offset >= caret_x)
				{
					// Scroll to the left so that the entire caret is visible
					horizontal_scroll_offset = caret_x;
				}
				else if (horizontal_scroll_offset <= offset_right && offset_right > 0.0f)
				{
					// Scroll to the right so that the entire caret is visible
					horizontal_scroll_offset = offset_right;
				}
				else if (horizontal_scroll_offset > 0.0f && offset_right > 0.0f && caret_position >= text.size())
				{
					// Scroll to the left so that the entire caret is visible and reveal preceding text
					horizontal_scroll_offset = offset_right;
				}

				horizontal_scroll_offset = std::max(0.0f, horizontal_scroll_offset);

				auto& compiled = label::get_compiled();
				compiled.add(caret.get_compiled(), -horizontal_scroll_offset, -vertical_scroll_offset);

				for (auto& cmd : compiled.draw_commands)
				{
					cmd.config.clip_region = true;
					cmd.config.clip_rect   = {static_cast<f32>(x), static_cast<f32>(y), static_cast<f32>(x + w), static_cast<f32>(y + h)};
				}

				m_is_compiled = true;
			}

			return compiled_resources;
		}
	} // namespace overlays
} // namespace rsx
