#include "stdafx.h"
#include "overlay_slider.h"

namespace rsx::overlays
{
	constexpr u16 slider_rail_thickness = 5;
	constexpr u16 slider_cover_thickness = 6;
	constexpr u16 slider_indicator_radius = 8;
	constexpr u16 slider_indicator_dia = slider_indicator_radius * 2;
	constexpr const char* slider_label_font_family = "Arial";
	constexpr int slider_label_font_size = 11;

	void slider::init()
	{
		back_color = color4f(0.3f, 0.3f, 0.3f, 0.f);
		fore_color = color4f(0.5647f, 0.7922f, 0.9765f, 1.f);
		auto_resize = false;
		m_current_value = m_min_value;
	}

	void slider::set_size(u16 w, u16 h)
	{
		horizontal_layout::set_size(w, h);
		clear_items();

		// Clamp the height
		h = std::max<u16>(h, 8);

		// Base components
		auto background = std::make_unique<rounded_rect>();
		auto foreground = std::make_unique<rounded_rect>();
		auto indicator = std::make_unique<ellipse>();
		auto value_label = std::make_unique<label>();
		auto min_label = std::make_unique<label>();
		auto max_label = std::make_unique<label>();

		indicator->border_radius = slider_indicator_radius;
		indicator->set_size(slider_indicator_dia + 2, slider_indicator_dia + 2);
		indicator->set_padding(2);
		indicator->set_pos(0, -2);
		indicator->back_color = color4f(1.f);

		background->border_radius = slider_rail_thickness / 2;
		background->back_color = this->back_color;
		background->back_color.a = 1.f;
		background->set_size(w, slider_rail_thickness);
		background->set_pos(0, (slider_indicator_dia / 2) - background->border_radius);

		foreground->border_radius = slider_cover_thickness / 2;
		foreground->back_color = this->fore_color;
		foreground->set_size(0, slider_cover_thickness);
		foreground->set_pos(0, (slider_indicator_dia / 2) - foreground->border_radius);

		value_label->set_padding(2);
		value_label->set_font(slider_label_font_family, slider_label_font_size);
		value_label->set_text(m_formatter(m_current_value));
		value_label->auto_resize();
		value_label->back_color.a = 0.f;
		value_label->fore_color = color4f(1.f);

		min_label->fore_color = color4f(0.75f, 0.75f, 0.75f, 1.f);
		min_label->back_color.a = 0.f;
		min_label->set_font(slider_label_font_family, slider_label_font_size);
		min_label->set_text(m_formatter(m_min_value));
		min_label->set_pos(0, (value_label->h / 2) + slider_indicator_radius + 8); // Margin = Text Height + Indicator Rad (gets to half point) - Text Height / 2 (Offset back to box origin)
		min_label->auto_resize();

		max_label->fore_color = color4f(0.75f, 0.75f, 0.75f, 1.f);
		max_label->back_color.a = 0.f;
		max_label->set_font(slider_label_font_family, slider_label_font_size);
		max_label->set_text(m_formatter(m_max_value));
		max_label->set_pos(0, (value_label->h / 2) + slider_indicator_radius + 8); // Margin
		max_label->auto_resize();

		const u16 horizontal_padding = slider_indicator_radius + 4;

		if (m_show_labels)
		{
			const u16 unusable_space = max_label->w + min_label->w + (2 * horizontal_padding);
			if (unusable_space < w)
			{
				background->w -= unusable_space;
				foreground->w = unusable_space;
			}
		}

		// Pack the slider part
		auto slider_part = std::make_unique<box_layout>();
		slider_part->set_size(background->w, 8);
		m_rail_component = slider_part->add_element(background);
		m_slider_component = slider_part->add_element(foreground);
		m_indicator_component = slider_part->add_element(indicator);

		// Middle part of the sandwich
		auto stack_part = std::make_unique<vertical_layout>();
		m_value_component = stack_part->add_element(value_label);
		stack_part->add_spacer(8);
		stack_part->add_element(slider_part);

		if (m_show_labels)
		{
			// Assemble the sandwich
			add_element(min_label);
			add_spacer(horizontal_padding);
			add_element(stack_part);
			add_spacer(horizontal_padding);
			add_element(max_label);
		}
		else
		{
			add_element(stack_part);
		}

		update_elements();
	}

	void slider::set_value(f64 value)
	{
		value = std::clamp(value, m_min_value, m_max_value);
		if (value == m_current_value)
		{
			return;
		}

		m_current_value = value;
		update_elements();
	}

	void slider::set_show_labels(bool show)
	{
		if (m_show_labels == show)
		{
			return;
		}

		m_show_labels = show;
		set_size(w, h);
	}

	void slider::update_elements()
	{
		if (!m_slider_component || !m_value_component || !m_rail_component)
		{
			return;
		}

		const auto slider_w = (std::abs(m_current_value - m_min_value) * m_rail_component->w) / std::abs(m_max_value - m_min_value);
		const auto slider_w16 = static_cast<u16>(std::floor(slider_w));

		m_slider_component->set_size(slider_w16, m_slider_component->h);
		m_indicator_component->set_pos(m_slider_component->x + slider_w16 - slider_indicator_radius, m_indicator_component->y);

		m_value_component->set_text(m_formatter(m_current_value));
		m_value_component->auto_resize();

		// If the label would touch the end, flip it
		u16 offset = (slider_w16 + m_value_component->w) >= m_rail_component->w ? m_value_component->w : 0;
		m_value_component->set_pos(m_slider_component->x + slider_w16 - offset, m_value_component->y);

		// Hide indicator when slider is completely empty or completely full
		m_value_component->visible = (m_current_value != m_min_value) && (m_current_value != m_max_value);
	}

	void slider::set_value_format(std::function<std::string(f64)> formatter)
	{
		m_formatter = formatter;
		set_size(w, h);
	}

	compiled_resource& slider::get_compiled()
	{
		if (is_compiled())
		{
			return compiled_resources;
		}

		horizontal_layout::get_compiled();

		m_is_compiled = true;
		return compiled_resources;
	}
}
