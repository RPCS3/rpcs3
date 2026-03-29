#include "stdafx.h"
#include "overlay_select.h"

namespace rsx::overlays
{
	constexpr u16 max_dropdown_arrow_dimension = 12;
	constexpr u16 dropdown_arrow_spacing = 4;
	constexpr u16 dropdown_pack_padding = 6;

	select_popup::select_popup(u16 w, u16 h)
		: list_view(w, h, false)
	{
		hide_prompt_buttons();
		hide_scroll_indicator();
		pack_padding = dropdown_pack_padding;
	}

	select::select(u16 w, u16 h, const std::vector<std::string>& options)
	{
		std::vector<std::unique_ptr<overlay_element>> labels;
		for (const auto& option : options)
		{
			std::unique_ptr<overlay_element> element = std::make_unique<overlays::label>();
			labels.push_back(std::move(element));

			auto label = dynamic_cast<overlays::label*>(labels.back().get());
			label->set_padding(8, 0, 0, 0);
			label->set_text(option);
			label->set_font("Arial", 14);
			label->set_padding(4);
			label->back_color.a = 0.f;
			label->auto_resize();
		}

		create_popup(labels);
		set_padding(8, 10, 6, 12);
		set_size(w, h);
		select_item(0);
	}

	select::select(u16 w, u16 h, const std::vector<std::u32string>& options)
	{
		std::vector<std::unique_ptr<overlay_element>> labels;
		for (const auto& option : options)
		{
			std::unique_ptr<overlay_element> element = std::make_unique<overlays::label>();
			labels.push_back(std::move(element));

			auto label = dynamic_cast<overlays::label*>(labels.back().get());
			label->set_padding(8, 0, 0, 0);
			label->set_unicode_text(option);
			label->set_padding(4);
			label->set_font("Arial", 14);
			label->back_color.a = 0.f;
			label->auto_resize();
		}

		create_popup(labels);
		set_padding(8, 10, 6, 12);
		set_size(w, h);
		select_item(0);
	}

	select::select(u16 w, u16 h, std::vector<std::unique_ptr<overlay_element>>& options)
	{
		create_popup(options);
		set_padding(8, 10, 6, 12);
		set_size(w, h);
		select_item(0);
	}

	void select::create_popup(std::vector<std::unique_ptr<overlay_element>>& options)
	{
		u16 max_w = 0;
		u16 max_h = 0;

		for (const auto& option : options)
		{
			max_w = std::max(max_w, option->w);
			max_h += option->h + dropdown_pack_padding;
		}

		m_popup = std::make_unique<select_popup>(max_w, max_h + dropdown_pack_padding + 4);
		m_popup->back_color = color4f(0.3f, 0.3f, 0.3f, 1.0f);

		for (auto& option : options)
		{
			m_popup->add_entry(option);
		}
	}

	void select::auto_resize()
	{
		u16 _h = max_dropdown_arrow_dimension;              // Minimum arrow dimension
		if (m_label)
		{
			_h = std::max(_h, m_label->h);
		}

		// Compute length of longest entry
		u16 text_w = 0, text_h = 0;
		for (auto& option : m_popup->m_items)
		{
			if (option->text.empty())
			{
				continue;
			}

			u16 tw = 0, th = 0;
			option->measure_text(tw, th, true);
			text_w = std::max<u16>(text_w, tw);
			text_h = std::max<u16>(text_h, th);
		}

		_h = std::max<u16>(_h, text_h);
		_h += padding_top + padding_bottom;

		if (text_w == 0)
		{
			// Avoid using auto_resize with manually generated options list
			text_w = m_popup->w;
		}

		u16 _w = text_w + dropdown_arrow_spacing + std::max<u16>(_h / 2, max_dropdown_arrow_dimension);  // Enough to fit longest string + space for the arrow and padding
		_w += padding_left + padding_right;
		set_size(_w, _h);
	}

	void select::set_size(u16 w, u16 h)
	{
		clear_items();

		if (const u16 min_x = (padding_left + padding_right + max_dropdown_arrow_dimension + (2 * dropdown_arrow_spacing)); w < min_x)
		{
			w = min_x;
		}

		if (const u16 min_y = (padding_top + padding_bottom + 16); h < min_y)
		{
			h = min_y;
		}

		box_layout::set_size(w, h);

		auto background = std::make_unique<rounded_rect>();
		background->set_size(w, h);
		background->border_radius = std::min(h / 4, 5);
		background->back_color = color4f(0.3f, 0.3f, 0.3f, 1.0f);

		const u16 arrow_size = std::min<u16>(h / 2, max_dropdown_arrow_dimension);
		auto arrow = std::make_unique<arrow_down>();
		arrow->set_size(arrow_size, arrow_size);
		arrow->back_color = color4f(0.8f, 0.8f, 0.8f, 1.f);

		auto textfield = std::make_unique<label>();
		textfield->set_font("Arial", 14);
		textfield->align_text(text_align::left);
		textfield->back_color.a = 0.f;

		// Stretch the textfield
		textfield->set_size(w - arrow->w - padding_left - padding_right - dropdown_arrow_spacing, h);

		// Center align the arrow vertically to align with the text
		if (textfield->h > arrow->h)
		{
			arrow->set_pos(0, textfield->compute_vertically_centered(arrow.get()));
			arrow->y = std::max<u16>(arrow->y, padding_top) - padding_top;
		}

		// Align items using a container
		auto packer = std::make_unique<horizontal_layout>();
		packer->set_pos(padding_left, padding_top);
		m_label = packer->add_element(textfield);
		packer->add_spacer(dropdown_arrow_spacing);
		packer->add_element(arrow);

		// Pack
		add_element(background);
		add_element(packer);

		// Extend the popup's width
		m_popup->set_size(w, m_popup->h);

		update_text();
	}

	void select::select_item(int selection)
	{
		m_popup->select_entry(selection);
		update_text();
	}

	void select::update_text()
	{
		if (m_popup->get_selected_index() < 0)
		{
			return;
		}

		auto element = m_popup->get_selected_entry();
		m_label->set_unicode_text(element->text);
		m_label->auto_resize();
		m_is_compiled = false;
	}

	compiled_resource& select::get_compiled()
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

		box_layout::get_compiled();
		m_is_compiled = true;
		return compiled_resources;
	}
}
