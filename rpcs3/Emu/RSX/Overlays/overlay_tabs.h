#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"

namespace rsx::overlays
{
	struct tabbed_container : public horizontal_layout
	{
		tabbed_container(u16 w, u16 h, u16 header_width = 0);

		template <typename T>
			requires std::is_base_of_v<overlay_element, T>
		void add_tab(std::unique_ptr<T>& header, std::shared_ptr<overlay_element>& panel)
		{
			std::unique_ptr<overlay_element> elem{ header.release() };
			add_tab(elem, panel);
		}

		void add_tab(std::string_view title, std::shared_ptr<overlay_element>& panel);
		void add_tab(std::unique_ptr<overlay_element>& header, std::shared_ptr<overlay_element>& panel);
		u32 tab_count() const { return m_tab_headers ? m_tab_headers->get_elements_count() : 0u; }

		virtual void set_size(u16 _w, u16 _h) override;
		virtual void set_headers_width(u16 size);
		void set_headers_background_color(const color4f& color);

		overlay_element* set_selected_tab(u32 index);
		overlay_element* get_selected() const;
		u32 get_selected_idx() const;

		void toggle_selection_mode();
		bool is_in_selection_mode() const { return m_is_in_tab_selection_mode; }

		compiled_resource& get_compiled() override;

	protected:
		void reflow_layout();
		void set_current_tab(overlay_element* view);
		void set_headers_pulse_effect(bool pulse);

		overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override;

	private:
		list_view* m_tab_headers = nullptr;
		std::vector<std::shared_ptr<overlay_element>> m_tab_contents;
		overlay_element* m_current_view = nullptr;

		u16 m_headers_width = 0;
		u32 m_selected_idx = umax;
		bool m_is_in_tab_selection_mode = true;

		std::mutex m_mutex;
	};
}

