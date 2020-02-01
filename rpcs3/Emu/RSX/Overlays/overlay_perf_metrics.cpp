#include "stdafx.h"
#include "overlay_perf_metrics.h"
#include "../GSRender.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Utilities/sysinfo.h"

#include <algorithm>
#include <utility>

namespace rsx
{
	namespace overlays
	{
		inline color4f convert_color_code(std::string hex_color, f32 opacity = 1.0f)
		{
			if (hex_color.length() > 0 && hex_color[0] == '#')
			{
				hex_color.erase(0, 1);
			}

			unsigned long hexval;
			const size_t len = hex_color.length();

			try
			{
				if (len != 6 && len != 8)
				{
					fmt::throw_exception("wrong length: %d", len);
				}
				hexval = std::stoul(hex_color, nullptr, 16);
			}
			catch (const std::exception& e)
			{
				rsx_log.error("Overlays: tried to convert incompatible color code: '%s' exception: '%s'", hex_color, e.what());
				return color4f(0.0f, 0.0f, 0.0f, 0.0f);
			}

			const int r = (len == 8 ? (hexval >> 24) : (hexval >> 16)) & 0xff;
			const int g = (len == 8 ? (hexval >> 16) : (hexval >> 8)) & 0xff;
			const int b = (len == 8 ? (hexval >> 8) : (hexval >> 0)) & 0xff;
			const int a = len == 8 ? ((hexval >> 0) & 0xff) : 255;

			return color4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f * opacity);
		}

		void perf_metrics_overlay::reset_transform(label& elm, u16 bottom_margin) const
		{
			const u32 text_padding = m_font_size / 2;

			// left, top, right, bottom
			const areau padding { text_padding, text_padding - 4, text_padding, text_padding };
			const positionu margin { m_margin_x, m_margin_y };
			positionu pos;

			const auto overlay_width = m_body.w + margin.x;
			const auto overlay_height = m_body.h + margin.y;

			switch (m_quadrant)
			{
			case screen_quadrant::top_left:
				pos.x = margin.x;
				pos.y = margin.y;
				break;
			case screen_quadrant::top_right:
				pos.x = virtual_width - overlay_width;
				pos.y = margin.y;
				break;
			case screen_quadrant::bottom_left:
				pos.x = margin.x;
				pos.y = virtual_height - overlay_height - bottom_margin;
				break;
			case screen_quadrant::bottom_right:
				pos.x = virtual_width - overlay_width;
				pos.y = virtual_height - overlay_height - bottom_margin;
				break;
			}

			if (m_center_x)
			{
				pos.x = (virtual_width - m_body.w) / 2;
			}

			if (m_center_y)
			{
				pos.y = (virtual_height - m_body.h - bottom_margin) / 2;
			}

			elm.set_pos(pos.x, pos.y);
			elm.set_padding(padding.x1, padding.x2, padding.y1, padding.y2);
		}

		void perf_metrics_overlay::reset_transforms()
		{
			const u16 perf_overlay_padding = m_font_size / 2;
			const u16 graphs_padding = m_font_size * 2;

			const u16 fps_graph_h = 60;
			const u16 frametime_graph_h = 45;

			u16 bottom_margin{};

			if (m_framerate_graph_enabled || m_frametime_graph_enabled)
			{
				// Adjust body size to account for the graphs
				// TODO: Bit hacky, could do this with margins if overlay_element had bottom margin (or negative top at least)
				bottom_margin = perf_overlay_padding;

				if (m_framerate_graph_enabled)
				{
					bottom_margin += fps_graph_h;

					if (m_frametime_graph_enabled)
					{
						bottom_margin += graphs_padding;
					}
				}

				if (m_frametime_graph_enabled)
				{
					bottom_margin += frametime_graph_h;
				}
			}

			// Set body/titles transform
			if (m_is_initialised && m_force_repaint)
			{
				m_force_repaint = false;
				reset_body(bottom_margin);
				reset_titles(bottom_margin);
			}
			else
			{
				reset_transform(m_body, bottom_margin);
				reset_transform(m_titles, bottom_margin);
			}

			if (m_framerate_graph_enabled || m_frametime_graph_enabled)
			{
				// Position the graphs within the body
				const u16 graphs_width = m_body.w;
				const u16 body_left = m_body.x;
				const u16 body_bottom = m_body.y + m_body.h + perf_overlay_padding;

				if (m_framerate_graph_enabled)
				{
					m_fps_graph.update();
					m_fps_graph.set_pos(body_left, body_bottom);
					m_fps_graph.set_size(graphs_width, fps_graph_h);
				}

				if (m_frametime_graph_enabled)
				{
					m_frametime_graph.update();

					u16 y_offset{};

					if (m_framerate_graph_enabled)
					{
						y_offset = m_fps_graph.get_height();
					}

					m_frametime_graph.set_pos(body_left, body_bottom + y_offset);
					m_frametime_graph.set_size(graphs_width, frametime_graph_h);
				}
			}
		}

		void perf_metrics_overlay::reset_body(u16 bottom_margin)
		{
			m_body.set_font(m_font.c_str(), m_font_size);
			m_body.fore_color = convert_color_code(m_color_body, m_opacity);
			m_body.back_color = convert_color_code(m_background_body, m_opacity);
			reset_transform(m_body, bottom_margin);
		}

		void perf_metrics_overlay::reset_titles(u16 bottom_margin)
		{
			m_titles.set_font(m_font.c_str(), m_font_size);
			m_titles.fore_color = convert_color_code(m_color_title, m_opacity);
			m_titles.back_color = convert_color_code(m_background_title, m_opacity);
			reset_transform(m_titles, bottom_margin);

			switch (m_detail)
			{
			case detail_level::minimal:
			case detail_level::low: m_titles.text = ""; break;
			case detail_level::medium: m_titles.text = fmt::format("\n\n%s", title1_medium); break;
			case detail_level::high: m_titles.text = fmt::format("\n\n%s\n\n\n\n\n\n%s", title1_high, title2); break;
			}
			m_titles.auto_resize();
			m_titles.refresh();
		}

		void perf_metrics_overlay::init()
		{
			reset_transforms();
			force_next_update();

			m_update_timer.Start();
			m_frametime_timer.Start();

			update();

			m_is_initialised = true;
		}

		void perf_metrics_overlay::set_framerate_graph_enabled(bool enabled)
		{
			if (m_framerate_graph_enabled == enabled)
				return;

			m_framerate_graph_enabled = enabled;

			if (enabled)
			{
				m_fps_graph.set_title("   Framerate");
				m_fps_graph.set_font_size(m_font_size * 0.8);
				m_fps_graph.set_count(50);
				m_fps_graph.set_color(convert_color_code(m_color_body, m_opacity));
				m_fps_graph.set_guide_interval(10);
			}

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_frametime_graph_enabled(bool enabled)
		{
			if (m_frametime_graph_enabled == enabled)
				return;

			m_frametime_graph_enabled = enabled;

			if (enabled)
			{
				m_frametime_graph.set_title("   Frametime");
				m_frametime_graph.set_font_size(m_font_size * 0.8);
				m_frametime_graph.set_count(170);
				m_frametime_graph.set_color(convert_color_code(m_color_body, m_opacity));
				m_frametime_graph.set_guide_interval(8);
			}

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_detail_level(detail_level level)
		{
			if (m_detail == level)
				return;

			m_detail = level;

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_position(screen_quadrant quadrant)
		{
			if (m_quadrant == quadrant)
				return;

			m_quadrant = quadrant;

			m_force_repaint = true;
		}

		// In ms
		void perf_metrics_overlay::set_update_interval(u32 update_interval)
		{
			m_update_interval = update_interval;
		}

		void perf_metrics_overlay::set_font(std::string font)
		{
			if (m_font == font)
				return;

			m_font = std::move(font);

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_font_size(u16 font_size)
		{
			if (m_font_size == font_size)
				return;

			m_font_size = font_size;

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_margins(u32 margin_x, u32 margin_y, bool center_x, bool center_y)
		{
			if (m_margin_x == margin_x && m_margin_y == margin_y && m_center_x == center_x && m_center_y == center_y)
				return;

			m_margin_x = margin_x;
			m_margin_y = margin_y;
			m_center_x = center_x;
			m_center_y = center_y;

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_opacity(f32 opacity)
		{
			if (m_opacity == opacity)
				return;

			m_opacity = opacity;

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_body_colors(std::string color, std::string background)
		{
			if (m_color_body == color && m_background_body == background)
				return;

			m_color_body = std::move(color);
			m_background_body = std::move(background);

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_title_colors(std::string color, std::string background)
		{
			if (m_color_title == color && m_background_title == background)
				return;

			m_color_title = std::move(color);
			m_background_title = std::move(background);

			m_force_repaint = true;
		}

		void perf_metrics_overlay::force_next_update()
		{
			m_force_update = true;
		}

		void perf_metrics_overlay::update()
		{
			const auto elapsed_update = m_update_timer.GetElapsedTimeInMilliSec();

			if (m_is_initialised)
			{
				if (m_frametime_graph_enabled)
				{
					const auto elapsed_frame = m_frametime_timer.GetElapsedTimeInMilliSec();
					m_frametime_graph.record_datapoint(elapsed_frame);
				}

				if (m_force_repaint)
				{
					reset_transforms();
				}
			}

			if (!m_force_update)
			{
				++m_frames;
			}

			if (elapsed_update >= m_update_interval || m_force_update)
			{
				if (!m_force_update)
				{
					m_update_timer.Start();
				}

				f32 fps{0};
				f32 frametime{0};

				u64 ppu_cycles{0};
				u64 spu_cycles{0};
				u64 rsx_cycles{0};
				u64 total_cycles{0};

				u32 ppus{0};
				u32 spus{0};

				f32 cpu_usage{-1.f};
				u32 total_threads{0};

				f32 ppu_usage{0};
				f32 spu_usage{0};
				f32 rsx_usage{0};
				u32 rsx_load{0};

				const auto rsx_thread = g_fxo->get<rsx::thread>();

				std::string perf_text;

				// 1. Fetch/calculate metrics we'll need
				switch (m_detail)
				{
				case detail_level::high:
				{
					frametime = m_force_update ? 0 : std::max(0.0, elapsed_update / m_frames);

					rsx_load = rsx_thread->get_load();

					total_threads = CPUStats::get_thread_count();

					// fallthrough
				}
				case detail_level::medium:
				{
					ppus = idm::select<named_thread<ppu_thread>>([&ppu_cycles](u32, named_thread<ppu_thread>& ppu)
					{
						ppu_cycles += thread_ctrl::get_cycles(ppu);
					});

					spus = idm::select<named_thread<spu_thread>>([&spu_cycles](u32, named_thread<spu_thread>& spu)
					{
						spu_cycles += thread_ctrl::get_cycles(spu);
					});

					rsx_cycles += rsx_thread->get_cycles();

					total_cycles = std::max<u64>(1, ppu_cycles + spu_cycles + rsx_cycles);
					cpu_usage = m_cpu_stats.get_usage();

					ppu_usage = std::clamp(cpu_usage * ppu_cycles / total_cycles, 0.f, 100.f);
					spu_usage = std::clamp(cpu_usage * spu_cycles / total_cycles, 0.f, 100.f);
					rsx_usage = std::clamp(cpu_usage * rsx_cycles / total_cycles, 0.f, 100.f);

					// fallthrough
				}
				case detail_level::low:
				{
					if (cpu_usage < 0.)
						cpu_usage = m_cpu_stats.get_usage();

					// fallthrough
				}
				case detail_level::minimal:
				{
					fps = m_force_update ? 0 : std::max(0.0, static_cast<f32>(m_frames) / (elapsed_update / 1000));
					if (m_is_initialised && m_framerate_graph_enabled)
						m_fps_graph.record_datapoint(fps);
				}
				}

				// 2. Format output string
				switch (m_detail)
				{
				case detail_level::minimal:
				{
					perf_text += fmt::format("FPS : %05.2f", fps);
					break;
				}
				case detail_level::low:
				{
					perf_text += fmt::format("FPS : %05.2f\n"
					                         "CPU : %04.1f %%",
					    fps, cpu_usage);
					break;
				}
				case detail_level::medium:
				{
					perf_text += fmt::format("FPS : %05.2f\n\n"
					                         "%s\n"
					                         " PPU   : %04.1f %%\n"
					                         " SPU   : %04.1f %%\n"
					                         " RSX   : %04.1f %%\n"
					                         " Total : %04.1f %%",
					    fps, std::string(title1_medium.size(), ' '), ppu_usage, spu_usage, rsx_usage, cpu_usage, std::string(title2.size(), ' '));
					break;
				}
				case detail_level::high:
				{
					perf_text += fmt::format("FPS : %05.2f (%03.1fms)\n\n"
					                         "%s\n"
					                         " PPU   : %04.1f %% (%2u)\n"
					                         " SPU   : %04.1f %% (%2u)\n"
					                         " RSX   : %04.1f %% ( 1)\n"
					                         " Total : %04.1f %% (%2u)\n\n"
					                         "%s\n"
					                         " RSX   : %02u %%",
					    fps, frametime, std::string(title1_high.size(), ' '), ppu_usage, ppus, spu_usage, spus, rsx_usage, cpu_usage, total_threads, std::string(title2.size(), ' '), rsx_load);
					break;
				}
				}

				m_body.text = perf_text;

				if (m_body.auto_resize())
				{
					reset_transforms();
				}

				m_body.refresh();

				if (!m_force_update)
				{
					m_frames = 0;
				}
				else
				{
					// Only force once
					m_force_update = false;
				}
			}

			if (m_framerate_graph_enabled)
			{
				m_fps_graph.update();
			}

			if (m_frametime_graph_enabled)
			{
				m_frametime_graph.update();
				m_frametime_timer.Start();
			}
		}

		compiled_resource perf_metrics_overlay::get_compiled()
		{
			auto compiled_resources = m_body.get_compiled();

			compiled_resources.add(m_titles.get_compiled());

			if (m_framerate_graph_enabled)
			{
				compiled_resources.add(m_fps_graph.get_compiled());
			}

			if (m_frametime_graph_enabled)
			{
				compiled_resources.add(m_frametime_graph.get_compiled());
			}

			return compiled_resources;
		}

		graph::graph()
		{
			m_label.set_font("e046323ms.ttf", 8);
			m_label.fore_color = { 1.f, 1.f, 1.f, 1.f };
			m_label.back_color = { 0.f, 0.f, 0.f, .7f };

			back_color = { 0.f, 0.f, 0.f, 0.5f };
		}

		void graph::set_pos(u16 _x, u16 _y)
		{
			m_label.set_pos(_x, _y);
			overlay_element::set_pos(_x, _y + m_label.h);
		}

		void graph::set_size(u16 _w, u16 _h)
		{
			overlay_element::set_size(_w, _h);

			// Place label horizontally in the middle of the graph rect
			const u16 label_x = std::max(x, u16(x + (w / 2) - (m_label.w / 2)));
			m_label.set_pos(label_x, m_label.y);
		}

		void graph::set_title(const char* title)
		{
			m_title = title;
		}

		void graph::set_font(const char* font_name, u16 font_size)
		{
			m_label.set_font(font_name, font_size);
		}

		void graph::set_font_size(u16 font_size)
		{
			const auto font_name = m_label.get_font()->font_name.c_str();
			m_label.set_font(font_name, font_size);
		}

		void graph::set_count(u32 datapoint_count)
		{
			m_datapoint_count = datapoint_count;
			m_datapoints.resize(datapoint_count, 0);
		}

		void graph::set_color(color4f color)
		{
			m_color = color;
		}

		void graph::set_guide_interval(f32 guide_interval)
		{
			m_guide_interval = guide_interval;
		}

		u16 graph::get_height() const
		{
			return h + m_label.h + m_label.padding_top + m_label.padding_bottom;
		}

		void graph::record_datapoint(f32 datapoint)
		{
			// std::dequeue is only faster for large sizes, so just use a std::vector and resize once in while

			// Record datapoint
			m_datapoints.push_back(datapoint);

			// Calculate new min/max
			// Make sure min/max reflects the data being displayed, not the entire datapoints vector
			m_min = *std::min_element(m_datapoints.end() - m_datapoint_count, m_datapoints.end());
			m_max = *std::max_element(m_datapoints.end() - m_datapoint_count, m_datapoints.end());

			// Cull vector when it gets large
			if (m_datapoints.size() > m_datapoint_count * 16ull)
			{
				std::copy(m_datapoints.begin() + m_datapoints.size() - m_datapoint_count, m_datapoints.end(), m_datapoints.begin());
				m_datapoints.resize(m_datapoint_count);
			}
		}

		void graph::update()
		{
			m_label.set_text(fmt::format("%s\nmn:%4.1f mx:%4.1f", m_title.c_str(), m_min, m_max));
			m_label.set_padding(4, 4, 0, 4);

			m_label.auto_resize();
			m_label.refresh();

			// If label horizontal end is larger, widen graph width to match it
			set_size(std::max(m_label.w, w), h);
		}

		compiled_resource& graph::get_compiled()
		{
			refresh();
			overlay_element::get_compiled();

			const f32 normalize_factor = f32(h) / m_max;

			// Don't show guide lines if they'd be more dense than 1 guide line every 3 pixels
			const bool guides_too_dense = (m_max / m_guide_interval) > (h / 3);

			if (m_guide_interval > 0 && !guides_too_dense)
			{
				auto& cmd_guides = compiled_resources.append({});
				auto& config_guides = cmd_guides.config;

				config_guides.color = { 1.f, 1.f, 1.f, .2f };
				config_guides.primitives = primitive_type::line_list;

				auto& verts_guides = compiled_resources.draw_commands.back().verts;

				for (auto y_off = m_guide_interval; y_off < m_max; y_off += m_guide_interval)
				{
					const f32 guide_y = y + y_off * normalize_factor;
					verts_guides.emplace_back(x, guide_y);
					verts_guides.emplace_back(x + w, guide_y);
				}
			}

			auto& cmd_graph = compiled_resources.append({});
			auto& config_graph = cmd_graph.config;

			config_graph.color = m_color;
			config_graph.primitives = primitive_type::line_strip;

			auto& verts_graph = compiled_resources.draw_commands.back().verts;

			const f32 x_stride = f32(w) / m_datapoint_count;
			const u32 tail_index_offset = m_datapoints.size() - m_datapoint_count;

			for (size_t i = 0; i < m_datapoint_count; ++i)
			{
				const f32 x_line = x + i * x_stride;
				const f32 y_line = y + h - (m_datapoints[tail_index_offset + i] * normalize_factor);
				verts_graph.emplace_back(x_line, y_line);
			}

			compiled_resources.add(m_label.get_compiled());

			return compiled_resources;
		}

		void reset_performance_overlay()
		{
			if (!g_cfg.misc.use_native_interface)
				return;

			if (auto manager = g_fxo->get<rsx::overlays::display_manager>())
			{
				auto& perf_settings = g_cfg.video.perf_overlay;
				auto perf_overlay = manager->get<rsx::overlays::perf_metrics_overlay>();

				if (perf_settings.perf_overlay_enabled)
				{
					const bool existed = !!perf_overlay;

					if (!existed)
					{
						perf_overlay = manager->create<rsx::overlays::perf_metrics_overlay>();
					}

					perf_overlay->set_detail_level(perf_settings.level);
					perf_overlay->set_position(perf_settings.position);
					perf_overlay->set_update_interval(perf_settings.update_interval);
					perf_overlay->set_font(perf_settings.font);
					perf_overlay->set_font_size(perf_settings.font_size);
					perf_overlay->set_margins(perf_settings.margin_x, perf_settings.margin_y, perf_settings.center_x.get(), perf_settings.center_y.get());
					perf_overlay->set_opacity(perf_settings.opacity / 100.f);
					perf_overlay->set_body_colors(perf_settings.color_body, perf_settings.background_body);
					perf_overlay->set_title_colors(perf_settings.color_title, perf_settings.background_title);
					perf_overlay->set_framerate_graph_enabled(perf_settings.framerate_graph_enabled.get());
					perf_overlay->set_frametime_graph_enabled(perf_settings.frametime_graph_enabled.get());

					if (!existed)
					{
						perf_overlay->init();
					}
				}
				else if (perf_overlay)
				{
					manager->remove<rsx::overlays::perf_metrics_overlay>();
				}
			}
		}
	} // namespace overlays
} // namespace rsx
