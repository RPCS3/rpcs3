#include "stdafx.h"
#include "overlay_tabs.h"

namespace rsx::overlays
{
	tabbed_container::tabbed_container(u16 w, u16 h, u16 header_width)
		: m_headers_width(header_width)
	{
		auto_resize = false;
		set_size(w, h);
	}

	overlay_element* tabbed_container::add_element(std::unique_ptr<overlay_element>& item, int offset)
	{
		return horizontal_layout::add_element(item, offset);
	}

	void tabbed_container::add_tab(std::string_view title, std::shared_ptr<overlay_element>& panel)
	{
		std::unique_ptr<overlay_element> label_widget = std::make_unique<label>(title);
		label_widget->set_size(m_headers_width, 60);
		label_widget->set_font("Arial", 18);
		label_widget->back_color.a = 0.f;
		label_widget->set_padding(16, 4, 16, 4);
		add_tab(label_widget, panel);
	}

	void tabbed_container::add_tab(std::unique_ptr<overlay_element>& header, std::shared_ptr<overlay_element>& panel)
	{
		if (!m_tab_headers)
		{
			reflow_layout();
		}

		m_tab_headers->add_entry(header);

		auto current = panel.get();
		m_tab_contents.push_back(std::move(panel));
		set_current_tab(current);
	}

	void tabbed_container::set_size(u16 width, u16 height)
	{
		horizontal_layout::set_size(width, height);
		reflow_layout();
	}

	void tabbed_container::set_headers_width(u16 size)
	{
		m_headers_width = size;
		m_tab_headers = nullptr;
		horizontal_layout::clear_items();

		reflow_layout();
	}

	void tabbed_container::set_headers_background_color(const color4f& color)
	{
		if (!m_tab_headers)
		{
			reflow_layout();
		}

		m_tab_headers->back_color = color;
	}

	void tabbed_container::set_headers_pulse_effect(bool pulse)
	{
		if (!m_tab_headers)
		{
			return;
		}

		m_tab_headers->disable_selection_pulse(!pulse);
	}

	void tabbed_container::set_current_tab(overlay_element* view)
	{
		m_current_view = view;
		reflow_layout();
	}

	void tabbed_container::reflow_layout()
	{
		std::lock_guard lock(m_mutex);

		if (m_headers_width > w)
		{
			// Invalid config
			return;
		}

		if (!m_tab_headers)
		{
			std::unique_ptr<overlay_element> tab_headers = std::make_unique<list_view>(m_headers_width, h, false);
			auto ptr = horizontal_layout::add_element(tab_headers);
			m_tab_headers = ensure(dynamic_cast<list_view*>(ptr));
			m_tab_headers->set_pos(x, y);
			m_tab_headers->advance_pos = 16;
			m_tab_headers->hide_prompt_buttons();
			m_tab_headers->back_color.a = 0.95f;
		}

		if (m_current_view)
		{
			m_current_view->set_pos(x + m_headers_width, y);
			m_current_view->set_size(w - m_headers_width, h);
		}

		refresh();
	}

	overlay_element* tabbed_container::set_selected_tab(u32 index)
	{
		if (index >= ::size32(m_tab_contents))
		{
			return nullptr;
		}

		auto view = m_tab_contents[index].get();
		set_current_tab(view);
		m_selected_idx = index;
		m_tab_headers->select_entry(static_cast<s32>(index));
		return view;
	}

	overlay_element* tabbed_container::get_selected() const
	{
		return m_current_view;
	}

	u32 tabbed_container::get_selected_idx() const
	{
		return m_selected_idx;
	}

	void tabbed_container::toggle_selection_mode()
	{
		m_is_in_tab_selection_mode = !m_is_in_tab_selection_mode;
		set_headers_pulse_effect(m_is_in_tab_selection_mode);
	}

	compiled_resource& tabbed_container::get_compiled()
	{
		std::lock_guard lock(m_mutex);

		// NOTE: Caching is difficult as the subviews are themselves dynamic.
		// Doable but not worth the effort
		compiled_resources.clear();

		if (m_tab_headers)
		{
			compiled_resources.add(m_tab_headers->get_compiled());
		}

		if (m_current_view)
		{
			compiled_resources.add(m_current_view->get_compiled());
		}

		m_is_compiled = true;
		return compiled_resources;
	}
}
