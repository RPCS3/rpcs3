#pragma once

#include "overlays.h"

namespace rsx
{
	namespace overlays
	{
		struct perf_metrics_overlay : overlay
		{
		private:
			// minimal - fps
			// low - fps, total cpu usage
			// medium - fps, detailed cpu usage
			// high - fps, frametime, detailed cpu usage, thread number, rsx load
			detail_level m_detail{};

			screen_quadrant m_quadrant{};
			positioni m_position{};

			label m_body{};
			label m_titles{};

			bool m_framerate_graph_enabled{};
			bool m_frametime_graph_enabled{};
			graph m_fps_graph;
			graph m_frametime_graph;

			CPUStats m_cpu_stats{};
			Timer m_update_timer{};
			Timer m_frametime_timer{};
			u32 m_update_interval{}; // in ms
			u32 m_frames{};
			std::string m_font{};
			u32 m_font_size{};
			u32 m_margin_x{}; // horizontal distance to the screen border relative to the screen_quadrant in px
			u32 m_margin_y{}; // vertical distance to the screen border relative to the screen_quadrant in px
			f32 m_opacity{};  // 0..1

			bool m_force_update{};
			bool m_is_initialised{};

			const std::string title1_medium{ "CPU Utilization:" };
			const std::string title1_high{ "Host Utilization (CPU):" };
			const std::string title2{ "Guest Utilization (PS3):" };

			void reset_transform(label& elm, u16 bottom_margin = 0) const;
			void reset_transforms();
			void reset_body();
			void reset_titles();
			void reset_text();

		public:
			void init();

			void set_framerate_graph_enabled(bool enabled);
			void set_frametime_graph_enabled(bool enabled);
			void set_detail_level(detail_level level);
			void set_position(screen_quadrant quadrant);
			void set_update_interval(u32 update_interval);
			void set_font(std::string font);
			void set_font_size(u32 font_size);
			void set_margins(u32 margin_x, u32 margin_y);
			void set_opacity(f32 opacity);
			void force_next_update();

			void update() override;

			compiled_resource get_compiled() override;
		};
	}
}
