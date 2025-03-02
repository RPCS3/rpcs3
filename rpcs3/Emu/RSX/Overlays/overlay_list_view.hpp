#pragma once

#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		struct list_view : public vertical_layout
		{
		private:
			std::unique_ptr<image_view> m_scroll_indicator_top;
			std::unique_ptr<image_view> m_scroll_indicator_bottom;
			std::unique_ptr<image_button> m_cancel_btn;
			std::unique_ptr<image_button> m_accept_btn;
			std::unique_ptr<image_button> m_deny_btn;
			std::unique_ptr<overlay_element> m_highlight_box;

			u16 m_elements_height = 0;
			s32 m_selected_entry = -1;
			u16 m_elements_count = 0;

			bool m_use_separators = false;
			bool m_cancel_only = false;

		public:
			list_view(u16 width, u16 height, bool use_separators = true, bool can_deny = false);

			void update_selection();

			void select_entry(s32 entry);
			void select_next(u16 count = 1);
			void select_previous(u16 count = 1);

			void add_entry(std::unique_ptr<overlay_element>& entry);

			int get_selected_index() const;
			bool get_cancel_only() const;
			const overlay_element* get_selected_entry() const;

			void set_cancel_only(bool cancel_only);
			void translate(s16 _x, s16 _y) override;

			compiled_resource& get_compiled() override;
		};
	}
}
