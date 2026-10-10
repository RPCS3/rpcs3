#include "stdafx.h"
#include "overlay_checkbox.h"

namespace rsx::overlays
{
	checkbox::checkbox()
	{
		fore_color = color4f(1.f);
		back_color = color4f(0.3f, 0.3f, 0.3f, 1.f);
	}

	void checkbox::set_checked(bool checked)
	{
		if (m_is_checked == checked)
		{
			return;
		}

		m_is_checked = checked;
		refresh();
	}

	void checkbox::set_size(u16 w, u16 h)
	{
		const u16 dim = std::min(w, h);
		box_layout::set_size(w, h);

		clear_items();
		refresh();

		m_border_box = nullptr;
		m_inner_box = nullptr;

		if (dim < 3)
		{
			return;
		}

		auto border_widget = std::make_unique<overlay_element>();
		auto inner_widget = std::make_unique<overlay_element>();

		border_widget->set_size(dim, dim);
		inner_widget->set_size(dim - 2, dim - 2);
		inner_widget->set_pos(1, 1);

		m_border_box = add_element(border_widget);
		m_inner_box = add_element(inner_widget);
	}

	compiled_resource& checkbox::get_compiled()
	{
		if (is_compiled())
		{
			return compiled_resources;
		}

		compiled_resources.clear();

		if (!m_border_box || !m_inner_box)
		{
			m_is_compiled = true;
			return compiled_resources;
		}

		m_border_box->back_color = this->fore_color;
		m_inner_box->back_color = this->back_color;
		m_inner_box->set_visible(!m_is_checked);

		m_border_box->refresh();
		m_inner_box->refresh();

		compiled_resources.add(m_border_box->get_compiled());
		compiled_resources.add(m_inner_box->get_compiled());

		m_is_compiled = true;
		return compiled_resources;
	}

	switchbox::switchbox()
	{
		fore_color = color4f(0.5647f, 0.7922f, 0.9765f, 1.f);
		back_color = color4f(0.75f, 0.75f, 0.75f, 1.f);
	}

	void switchbox::set_size(u16 w, u16 h)
	{
		const u16 dim = std::max<u16>(std::min(w, h), 14);
		box_layout::set_size(w, h);

		clear_items();
		refresh();

		m_back_ellipse = nullptr;
		m_front_circle = nullptr;

		auto ellipse_part = std::make_unique<rounded_rect>();
		auto circle_part = std::make_unique<ellipse>();

		ellipse_part->set_size(dim * 2, dim);
		ellipse_part->set_padding(1);
		ellipse_part->set_pos(0, 0);
		ellipse_part->border_radius = (dim - 4) / 2; // Avoid perfect capsule shape since we want a border and perfect capsules can have a false border along the midline due to subgroup shenanigans

		circle_part->set_size(dim, dim);
		circle_part->set_padding(4);
		circle_part->set_pos(0, 0);

		m_back_ellipse = add_element(ellipse_part);
		m_front_circle = add_element(circle_part);
	}

	compiled_resource& switchbox::get_compiled()
	{
		if (is_compiled())
		{
			return compiled_resources;
		}

		compiled_resources.clear();

		if (!m_back_ellipse || !m_front_circle)
		{
			m_is_compiled = true;
			return compiled_resources;
		}

		if (m_is_checked)
		{
			m_back_ellipse->border_color.a = 0.f;
			m_back_ellipse->border_size = 0;
			m_back_ellipse->back_color = this->fore_color * 0.75f;
			m_back_ellipse->back_color.a = 1.f;
			m_front_circle->back_color = color4f(1.f);
			m_front_circle->set_pos(this->x + m_front_circle->w, this->y);
		}
		else
		{
			m_back_ellipse->border_color = this->back_color * 0.75f;
			m_back_ellipse->border_color.a = 1.f;
			m_back_ellipse->border_size = 1;
			m_back_ellipse->back_color = this->back_color * 0.5f;
			m_back_ellipse->back_color.a = 1.f;
			m_front_circle->back_color = this->back_color;
			m_front_circle->set_pos(this->x, this->y);
		}

		m_back_ellipse->refresh();
		m_front_circle->refresh();

		compiled_resources.add(m_back_ellipse->get_compiled());
		compiled_resources.add(m_front_circle->get_compiled());
		return compiled_resources;
	}
}
