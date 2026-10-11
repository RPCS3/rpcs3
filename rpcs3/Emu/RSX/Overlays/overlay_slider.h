#pragma once

#include "overlay_controls.h"

namespace rsx::overlays
{
	struct slider : public horizontal_layout
	{
		slider()
		{
			init();
		}

		slider(f64 min, f64 max, f64 step)
			: m_min_value(min)
			, m_max_value(max)
			, m_step(step)
		{
			init();
		}

		void set_size(u16 w, u16 h) override;
		void set_value_format(std::function<std::string(f64)> formatter);
		void set_show_labels(bool show);

		void inc_value() { set_value(m_current_value + m_step); }
		void dec_value() { set_value(m_current_value - m_step); }
		void set_value(f64 value);

		compiled_resource& get_compiled() override;

	private:
		f64 m_min_value = 0.0;
		f64 m_max_value = 100.0;
		f64 m_current_value = 0.0;
		f64 m_step = 1.0;

		bool m_show_labels = true;

		std::function<std::string(f64)> m_formatter =
			[](f64 v) { return std::to_string(v); };

		overlay_element* m_slider_component = nullptr;
		overlay_element* m_rail_component = nullptr;
		overlay_element* m_indicator_component = nullptr;
		label* m_value_component = nullptr;

		void init();
		void update_elements();
	};
}
