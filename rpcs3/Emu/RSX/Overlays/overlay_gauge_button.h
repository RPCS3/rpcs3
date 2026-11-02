#pragma once

#include "overlay_controls.h"

namespace rsx::overlays
{
	// Image button wrapped in a circular gauge that fills clockwise.
	// Position and size refer to the button itself, the ring extends outside of those bounds.
	struct gauge_button : public overlay_element
	{
	private:
		pie m_gauge;
		image_button m_button;

		f32 m_value = 0.f;     // Gauge fill in the range [0, 1]
		u8 m_gauge_width = 4;  // Thickness of the visible ring
		s8 m_gauge_gap = 0;    // Spacing between the icon bounds and the inner edge of the ring. Negative values tuck the ring under the icon.

		void update_layout();

	public:
		gauge_button();

		void set_value(f32 value);
		f32 get_value() const { return m_value; }

		void set_gauge_width(u8 width);
		void set_gauge_gap(s8 gap);
		void set_image_resource(u8 resource_id);

		using overlay_element::set_text;
		void set_text(std::string_view text) override;
		void set_unicode_text(std::u32string_view text) override;
		void set_font(u16 font_size, std::string_view font_name = {}) override;

		void set_pos(s16 _x, s16 _y) override;
		void set_size(u16 _w, u16 _h) override;
		void translate(s16 dx, s16 dy) override;

		compiled_resource& get_compiled() override;
	};
}
