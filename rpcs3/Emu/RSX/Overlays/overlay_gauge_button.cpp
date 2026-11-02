#include "stdafx.h"
#include "overlay_gauge_button.h"

namespace rsx::overlays
{
	gauge_button::gauge_button()
	{
		back_color = { 0.f, 0.f, 0.f, 0.f };
		fore_color = { 1.f, 1.f, 1.f, 1.f };
	}

	void gauge_button::update_layout()
	{
		// The pie is centered on the icon and extends past it by the gap plus the ring thickness on each side
		const s16 extent = static_cast<s16>(m_gauge_gap + m_gauge_width);
		const u16 diameter = static_cast<u16>(std::max(0, h + (extent * 2)));

		m_gauge.set_pos(x - extent, y - extent);
		m_gauge.set_size(diameter, diameter);
		m_is_compiled = false;
	}

	void gauge_button::set_value(f32 value)
	{
		value = std::clamp(value, 0.f, 1.f);
		if (value != m_value)
		{
			m_value = value;

			// Easing formula makes the animation speed up at the start and slow at the end.
			// This feels way more responsive than just a linear response. It also doesn't affect timing.
			const f32 eased = 1.f - ((1.f - value) * (1.f - value));
			m_gauge.set_cutoff_angle(eased * 360.f);
			m_is_compiled = false;
		}
	}

	void gauge_button::set_gauge_width(u8 width)
	{
		m_gauge_width = width;
		update_layout();
	}

	void gauge_button::set_gauge_gap(s8 gap)
	{
		m_gauge_gap = gap;
		update_layout();
	}

	void gauge_button::set_image_resource(u8 resource_id)
	{
		m_button.set_image_resource(resource_id);
		m_button.refresh();
		m_is_compiled = false;
	}

	void gauge_button::set_text(std::string_view text)
	{
		m_button.set_text(text);
		m_is_compiled = false;
	}

	void gauge_button::set_unicode_text(std::u32string_view text)
	{
		m_button.set_unicode_text(text);
		m_is_compiled = false;
	}

	void gauge_button::set_font(u16 font_size, std::string_view font_name)
	{
		m_button.set_font(font_size, font_name);
		m_is_compiled = false;
	}

	void gauge_button::set_pos(s16 _x, s16 _y)
	{
		overlay_element::set_pos(_x, _y);
		m_button.set_pos(_x, _y);
		update_layout();
	}

	void gauge_button::set_size(u16 _w, u16 _h)
	{
		// The button icon is always square (h x h), the width only matters for the label
		overlay_element::set_size(_w, _h);
		m_button.set_size(_w, _h);
		update_layout();
	}

	void gauge_button::translate(s16 dx, s16 dy)
	{
		set_pos(x + dx, y + dy);
	}

	compiled_resource& gauge_button::get_compiled()
	{
		if (is_compiled())
		{
			return compiled_resources;
		}

		compiled_resources.clear();

		if (!is_visible())
		{
			m_is_compiled = true;
			return compiled_resources;
		}

		m_gauge.back_color = back_color;
		m_gauge.border_color = fore_color;
		m_gauge.border_size = m_gauge_width;
		m_gauge.refresh();

		// Gauge first so the button covers its center and only the outer ring remains visible
		compiled_resources.add(m_gauge.get_compiled());
		compiled_resources.add(m_button.get_compiled());

		m_is_compiled = true;
		return compiled_resources;
	}
}
