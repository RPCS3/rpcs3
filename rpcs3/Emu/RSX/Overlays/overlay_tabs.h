#pragma once

#include "overlays.h"
#include "overlay_list_view.hpp"

namespace rsx::overlays
{
	struct tabbed_container : public horizontal_layout
	{
		tabbed_container(u16 header_width = 0);

		void add_tab(std::string_view title, std::shared_ptr<overlay_element>& panel);
		void add_tab(std::unique_ptr<overlay_element>& header, std::shared_ptr<overlay_element>& panel);

		virtual void set_size(u16 _w, u16 _h) override;
		virtual void set_headers_width(u16 size);
		virtual void set_headers_pulse_effect(bool pulse);

		overlay_element* set_selected_tab(u32 index);
		overlay_element* get_selected() const;
		u32 get_selected_idx() const;

		compiled_resource& get_compiled() override;

	protected:
		void reflow_layout();
		void set_current_tab(overlay_element* view);

		overlay_element* add_element(std::unique_ptr<overlay_element>& item, int offset = -1) override;

	private:
		list_view* m_tab_headers = nullptr;
		std::vector<std::shared_ptr<overlay_element>> m_tab_contents;
		overlay_element* m_current_view = nullptr;

		u16 m_headers_width = 0;
		u32 m_selected_idx = umax;
	};
}

