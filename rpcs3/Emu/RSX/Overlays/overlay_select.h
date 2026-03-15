#pragma once

#include "overlay_arrow.h"
#include "overlay_list_view.hpp"

namespace rsx::overlays
{
	struct select_popup : public list_view
	{
		select_popup(u16 w, u16 h);
	};

	struct select : public box_layout
	{
		select(u16 w, u16 h, const std::vector<std::string>& options);

		select(u16 w, u16 h, const std::vector<std::u32string>& options);

		select(u16 w, u16 h, std::vector<std::unique_ptr<overlay_element>>& options);

		void set_size(u16 w, u16 h) override;

		void auto_resize();

		void select_item(int selection);

		void select_previous() { m_popup->select_previous(); }

		void select_next() { m_popup->select_next(); }

		int get_selected_index() const { return m_popup ? m_popup->get_selected_index() : -1; }

		// Returns the drawable popup for selection.
		select_popup* get_popup() { return m_popup.get(); }

		compiled_resource& get_compiled() override;

	protected:
		std::unique_ptr<select_popup> m_popup;
		label* m_label = nullptr;

		void update_text();
		void create_popup(std::vector<std::unique_ptr<overlay_element>>& options);
	};
}
