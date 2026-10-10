#pragma once

#include "overlay_controls.h"

namespace rsx::overlays
{
	struct checkbox : public box_layout
	{
		checkbox();

		void set_checked(bool checked);
		bool is_checked() const { return m_is_checked; }

		void set_size(u16 w, u16 h) override;
		compiled_resource& get_compiled() override;

	protected:
		bool m_is_checked = false;

	private:
		overlay_element* m_border_box = nullptr;
		overlay_element* m_inner_box = nullptr;
	};

	struct switchbox : public checkbox
	{
		switchbox();

		void set_size(u16 w, u16 h) override;
		compiled_resource& get_compiled() override;

	private:
		overlay_element* m_back_ellipse = nullptr;
		overlay_element* m_front_circle = nullptr;
	};
}
