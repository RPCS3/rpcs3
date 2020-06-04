#include "stdafx.h"
#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		void edit_text::move_caret(direction dir)
		{
			switch (dir)
			{
			case left:
			{
				if (caret_position)
				{
					caret_position--;
					refresh();
				}
				break;
			}
			case right:
			{
				if (caret_position < text.length())
				{
					caret_position++;
					refresh();
				}
				break;
			}
			case up:
			{
				if (caret_position == 0)
				{
					break;
				}
				u64 current_line_start = text.rfind('\n', caret_position - 1); // search from previous char
				if (current_line_start == std::string::npos)
				{
					// This is the first line, so caret moves to the very beginning
					caret_position = 0;
					refresh();
					break;
				}
				else
				{
					// if line is not first, than caret should move forward as find result points to '\n'
					current_line_start += 1;
				}
				const u64 caret_pos_in_line = caret_position - current_line_start;
				const u64 prev_line_end     = current_line_start - 2; // ignore current char and \n before it
				u64 prev_line_start         = text.rfind('\n', prev_line_end);
				if (prev_line_start == std::string::npos)
				{
					// if previous line is the first line, there is no '\n' at the beginning
					prev_line_start = 0;
				}
				else
				{
					// if previous line is not first, than caret should move forward as find result points to '\n'
					prev_line_start += 1;
				}
				const u64 prev_line_length = prev_line_end - prev_line_start + 1; // delta in indexes requires +1
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				if (prev_line_length < caret_pos_in_line)
				{
					// Previous line is shorter than current caret position, so caret moves to the very end of the previous line
					caret_position = prev_line_end + 1;
				}
				else
				{
					// Previous line is longer than current caret position, so caret moves from prev_line_beginning the exact same distance as in current line
					caret_position = prev_line_start + caret_pos_in_line;
				}
				refresh();
				break;
			}
			case down:
			{
				if (caret_position == text.length())
				{
					break;
				}
				u64 current_line_end = text.find('\n', caret_position);
				if (current_line_end == std::string::npos)
				{
					// This is the last line, so caret moves to the very end
					caret_position = text.length();
					refresh();
					break;
				}
				else
				{
					// move to the last character of the current line
					current_line_end -= 1;
				}
				u64 current_line_start = text.rfind('\n', current_line_end);
				if (current_line_start == std::string::npos)
				{
					// if current line is the first one - there is no '\n' before it, so line starts from index 0
					current_line_start = 0;
				}
				else
				{
					// if line is not first, than caret should move forward as find result points to '\n'
					current_line_start += 1;
				}
				const u64 caret_pos_in_line = caret_position - current_line_start;
				const u64 next_line_start   = current_line_end + 2; // ignore last char and '\n'
				u64 next_line_end           = text.find('\n', next_line_start);
				if (next_line_end == std::string::npos)
				{
					// if next line is the last line, there is no '\n' at the end, so next_line_end is set to text end
					next_line_end = text.length() - 1;
				}
				else
				{
					next_line_end -= 1; // move to the last char in the line, as find result points to '\n'
				}
				const u64 next_line_length = next_line_end - next_line_start + 1; // delta in indexes requires +1
				// TODO : Save caret position to some kind of buffer, so after switching back and forward, caret would be on initial position
				if (next_line_length < caret_pos_in_line)
				{
					// Next line is shorter than current caret position, so caret moves to the very end of the next line
					caret_position = next_line_end + 1;
				}
				else
				{
					// Next line is longer than current caret position, so caret moves from next_line_start the exact same distance as in current line
					caret_position = next_line_start + caret_pos_in_line;
				}
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
