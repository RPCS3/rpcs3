#include "stdafx.h"
#include "overlay_progress_bar.hpp"

namespace rsx
{
	namespace overlays
	{
		progress_bar::progress_bar()
		{
			text_view.back_color = {0.f, 0.f, 0.f, 0.f};
		}

		void progress_bar::inc(f32 value)
		{
			set_value(m_value + value);
		}

		void progress_bar::dec(f32 value)
		{
			set_value(m_value - value);
		}

		void progress_bar::set_limit(f32 limit)
		{
			m_limit     = limit;
			m_is_compiled = false;
		}

		void progress_bar::set_value(f32 value)
		{
			m_value = std::clamp(value, 0.f, m_limit);

			f32 indicator_width = (w * m_value) / m_limit;
			indicator.set_size(static_cast<u16>(indicator_width), h);
			m_is_compiled = false;
		}

		void progress_bar::set_pos(s16 _x, s16 _y)
		{
			u16 text_w, text_h;
			text_view.measure_text(text_w, text_h);
			text_h += 13;

			overlay_element::set_pos(_x, _y + text_h);
			indicator.set_pos(_x, _y + text_h);
			text_view.set_pos(_x, _y);
		}

		void progress_bar::set_size(u16 _w, u16 _h)
		{
			overlay_element::set_size(_w, _h);
			text_view.set_size(_w, text_view.h);
			set_value(m_value);
		}

		void progress_bar::translate(s16 dx, s16 dy)
		{
			set_pos(x + dx, y + dy);
		}

		void progress_bar::set_text(const std::string& str)
		{
			text_view.set_text(str);
			text_view.align_text(text_align::center);

			u16 text_w, text_h;
			text_view.measure_text(text_w, text_h);
			text_view.set_size(w, text_h);

			set_pos(text_view.x, text_view.y);
			m_is_compiled = false;
		}

		compiled_resource& progress_bar::get_compiled()
		{
			if (!is_compiled())
			{
				auto& compiled = overlay_element::get_compiled();
				compiled.add(text_view.get_compiled());

				indicator.back_color = fore_color;
				indicator.refresh();
				compiled.add(indicator.get_compiled());
			}

			return compiled_resources;
		}
	} // namespace overlays
} // namespace rsx
