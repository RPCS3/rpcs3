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
			case down:
				// TODO
				break;
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
