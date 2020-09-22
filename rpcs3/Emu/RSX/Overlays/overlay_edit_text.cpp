﻿#include "stdafx.h"
#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		static size_t get_line_start(const std::u32string& text, size_t pos)
		{
			if (pos == 0)
			{
				return 0;
			}
			const size_t line_start = text.rfind('\n', pos - 1);
			if (line_start == std::string::npos)
			{
				return 0;
			}
			return line_start + 1;
		}

		static size_t get_line_end(const std::u32string& text, size_t pos)
		{
			const size_t line_end = text.find('\n', pos);
			if (line_end == std::string::npos)
			{
				return text.length();
			}
			return line_end;
		}

		void edit_text::move_caret(direction dir)
		{
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
				if (caret_position < text.length())
				{
					caret_position++;
					refresh();
				}
				break;
			}
			case direction::up:
			{
				const size_t current_line_start = get_line_start(text, caret_position);
				if (current_line_start == 0)
				{
					// This is the first line, so caret moves to the very beginning
					caret_position = 0;
					refresh();
					break;
				}
				const size_t caret_pos_in_line = caret_position - current_line_start;
				const size_t prev_line_end     = current_line_start - 1;
				const size_t prev_line_start   = get_line_start(text, prev_line_end);
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				caret_position = std::min(prev_line_end, prev_line_start + caret_pos_in_line);

				refresh();
				break;
			}
			case direction::down:
			{
				const size_t current_line_end = get_line_end(text, caret_position);
				if (current_line_end == text.length())
				{
					// This is the last line, so caret moves to the very end
					caret_position = current_line_end;
					refresh();
					break;
				}
				const size_t current_line_start = get_line_start(text, caret_position);
				const size_t caret_pos_in_line  = caret_position - current_line_start;
				const size_t next_line_start    = current_line_end + 1;
				const size_t next_line_end      = get_line_end(text, next_line_start);
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				caret_position = std::min(next_line_end, next_line_start + caret_pos_in_line);

				refresh();
				break;
			}
			}
		}

		void edit_text::insert_text(const std::u32string& str)
		{
			if (caret_position == 0)
			{
				// Start
				text = str + text;
			}
			else if (caret_position == text.length())
			{
				// End
				text += str;
			}
			else
			{
				// Middle
				text.insert(caret_position, str);
			}

			caret_position += ::narrow<u16>(str.length());
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
				text = text.length() > 1 ? text.substr(1) : U"";
			}
			else if (caret_position == text.length())
			{
				text = text.substr(0, caret_position - 1);
			}
			else
			{
				text = text.substr(0, caret_position - 1) + text.substr(caret_position);
			}

			caret_position--;
			refresh();
		}

		compiled_resource& edit_text::get_compiled()
		{
			if (!is_compiled)
			{
				auto& compiled = label::get_compiled();

				overlay_element caret;
				auto renderer        = get_font();
				const auto caret_loc = renderer->get_char_offset(text.c_str(), caret_position, clip_text ? w : UINT16_MAX, wrap_text);

				caret.set_pos(u16(caret_loc.first + padding_left + x), u16(caret_loc.second + padding_top + y));
				caret.set_size(1, u16(renderer->get_size_px() + 2));
				caret.fore_color           = fore_color;
				caret.back_color           = fore_color;
				caret.pulse_effect_enabled = true;

				compiled.add(caret.get_compiled());

				for (auto& cmd : compiled.draw_commands)
				{
					// TODO: Scrolling by using scroll offset
					cmd.config.clip_region = true;
					cmd.config.clip_rect   = {f32(x), f32(y), f32(x + w), f32(y + h)};
				}

				is_compiled = true;
			}

			return compiled_resources;
		}
	} // namespace overlays
} // namespace rsx
