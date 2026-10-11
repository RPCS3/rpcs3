#pragma once

#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		struct list_view : public vertical_layout
		{
		private:
			std::unique_ptr<box_layout> m_scroll_indicator;
			std::unique_ptr<image_button> m_cancel_btn;
			std::unique_ptr<image_button> m_accept_btn;
			std::unique_ptr<image_button> m_deny_btn;
			std::unique_ptr<overlay_element> m_highlight_box;

			overlay_element* m_scroll_indicator_track = nullptr;
			rounded_rect* m_scroll_indicator_grip = nullptr;

			u16 m_elements_height = 0;
			s32 m_selected_entry = -1;
			u16 m_elements_count = 0;

			bool m_use_separators = false;
			bool m_cancel_only = false;

			void update_scroll_indicator();

		public:
			list_view(u16 width, u16 height, bool use_separators = true, bool can_deny = false);

			void update_selection();

			virtual void select_entry(s32 entry);
			virtual void select_next(u16 count = 1);
			virtual void select_previous(u16 count = 1);

			template<typename T>
				requires std::is_base_of_v<overlay_element, T>
			void add_entry(std::unique_ptr<T>& ptr)
			{
				auto _ptr = ensure(dynamic_cast<overlay_element*>(ptr.release()));
				std::unique_ptr<overlay_element> e{ _ptr };
				add_entry(e);
			}

			void add_entry(std::unique_ptr<overlay_element>& entry);
			void clear_items() override;

			u16 get_elements_count() const;
			s32 get_selected_index() const;
			bool get_cancel_only() const;
			const overlay_element* get_selected_entry() const;

			void hide_prompt_buttons(bool hidden = true);
			void hide_scroll_indicator(bool hidden = true);
			void hide_row_highliter(bool hidden = false);
			void disable_selection_pulse(bool disabled = true);

			void set_cancel_only(bool cancel_only);
			void translate(s16 _x, s16 _y) override;
			void set_size(u16 w, u16 h) override;
			void set_pos(s16 x, s16 y) override;

			compiled_resource& get_compiled() override;
		};
	}
}
