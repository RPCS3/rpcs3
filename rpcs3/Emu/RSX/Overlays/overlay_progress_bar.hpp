#pragma once

#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		struct progress_bar : public overlay_element
		{
		private:
			overlay_element indicator;
			label text_view;

			f32 m_limit = 100.f;
			f32 m_value = 0.f;

		public:
			progress_bar();
			void inc(f32 value);
			void dec(f32 value);
			void set_limit(f32 limit);
			void set_value(f32 value);
			void set_pos(s16 _x, s16 _y) override;
			void set_size(u16 _w, u16 _h) override;
			void translate(s16 dx, s16 dy) override;
			void set_text(const std::string& str) override;

			compiled_resource& get_compiled() override;
		};
	}
}
