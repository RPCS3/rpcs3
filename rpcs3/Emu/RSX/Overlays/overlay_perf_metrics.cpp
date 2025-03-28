#include "stdafx.h"
#include "overlay_manager.h"
#include "overlay_perf_metrics.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUThread.h"

#include <algorithm>
#include <utility>

#include "util/cpu_stats.hpp"

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

			unsigned hexval = 0;
			const auto len = hex_color.length();

			if (len != 6 && len != 8)
			{
				rsx_log.error("Incompatible color code: '%s' has wrong length: %d", hex_color, len);
				return color4f(0.0f, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				// auto&& [ptr, ec] = std::from_chars(hex_color.c_str(), hex_color.c_str() + len, &hexval, 16);

				// if (ptr != hex_color.c_str() + len || ec)
				// {
				// 	rsx_log.error("Overlays: tried to convert incompatible color code: '%s'", hex_color);
				// 	return color4f(0.0f, 0.0f, 0.0f, 0.0f);
				// }
				for (u32 i = 0; i < len; i++)
				{
					hexval <<= 4;

					switch (char c = hex_color[i])
					{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						hexval |= (c - '0');
						break;
					case 'a':
					case 'b':
					case 'c':
					case 'd':
					case 'e':
					case 'f':
						hexval |= (c - 'a' + 10);
						break;
					case 'A':
					case 'B':
					case 'C':
					case 'D':
					case 'E':
					case 'F':
						hexval |= (c - 'A' + 10);
						break;
					default:
					{
						rsx_log.error("Overlays: invalid characters in color code: '%s'", hex_color);
						return color4f(0.0f, 0.0f, 0.0f, 0.0f);
					}
					}
				}
			}

			const int r = (len == 8 ? (hexval >> 24) : (hexval >> 16)) & 0xff;
			const int g = (len == 8 ? (hexval >> 16) : (hexval >> 8)) & 0xff;
			const int b = (len == 8 ? (hexval >> 8) : (hexval >> 0)) & 0xff;
			const int a = len == 8 ? ((hexval >> 0) & 0xff) : 255;

			return color4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f * opacity);
		}

		void perf_metrics_overlay::reset_transform(label& elm) const
		{
			// left, top, right, bottom
			const areau padding { m_padding, m_padding - std::min<u32>(4, m_padding), m_padding, m_padding };
			const positionu margin { m_margin_x, m_margin_y };
			positionu pos;

			u16 graph_width = 0;
			u16 graph_height = 0;

			if (m_framerate_graph_enabled)
			{
				graph_width = std::max(graph_width, m_fps_graph.w);
				graph_height += m_fps_graph.get_height();
			}

			if (m_frametime_graph_enabled)
			{
				graph_width = std::max(graph_width, m_frametime_graph.w);
				graph_height += m_frametime_graph.get_height();
			}

			if (graph_height > 0 && m_body.h > 0)
			{
				graph_height += m_padding;
			}

			switch (m_quadrant)
			{
			case screen_quadrant::top_left:
				pos.x = margin.x;
				pos.y = margin.y;
				break;
			case screen_quadrant::top_right:
				pos.x = virtual_width - std::max(m_body.w, graph_width) - margin.x;
				pos.y = margin.y;
				break;
			case screen_quadrant::bottom_left:
				pos.x = margin.x;
				pos.y = virtual_height - m_body.h - graph_height - margin.y;
				break;
			case screen_quadrant::bottom_right:
				pos.x = virtual_width - std::max(m_body.w, graph_width) - margin.x;
				pos.y = virtual_height - m_body.h - graph_height - margin.y;
				break;
			}

			if (m_center_x)
			{
				pos.x = (virtual_width - std::max(m_body.w, graph_width)) / 2;
			}

			if (m_center_y)
			{
				pos.y = (virtual_height - m_body.h - graph_height) / 2;
			}

			elm.set_pos(pos.x, pos.y);
			elm.set_padding(padding.x1, padding.x2, padding.y1, padding.y2);
		}

		void perf_metrics_overlay::reset_transforms()
		{
			const u16 fps_graph_h = 60;
			const u16 frametime_graph_h = 45;

			if (m_framerate_graph_enabled)
			{
				m_fps_graph.set_size(m_fps_graph.w, fps_graph_h);
			}

			if (m_frametime_graph_enabled)
			{
				m_frametime_graph.set_size(m_frametime_graph.w, frametime_graph_h);
			}

			// Set body/titles transform
			if (m_force_repaint)
			{
				reset_body();
				reset_titles();
			}
			else
			{
				reset_transform(m_body);
				reset_transform(m_titles);
			}

			if (m_framerate_graph_enabled || m_frametime_graph_enabled)
			{
				// Position the graphs within the body
				const u16 graphs_width = m_body.w;
				const u16 body_left = m_body.x;
				s16 y_offset = m_body.y;

				if (m_body.h > 0)
				{
					y_offset += static_cast<s16>(m_body.h + m_padding);
				}

				if (m_framerate_graph_enabled)
				{
					if (m_force_repaint)
					{
						m_fps_graph.set_font_size(static_cast<u16>(m_font_size * 0.8));
					}
					m_fps_graph.update();
					m_fps_graph.set_pos(body_left, y_offset);
					m_fps_graph.set_size(graphs_width, fps_graph_h);

					y_offset += m_fps_graph.get_height();
				}

				if (m_frametime_graph_enabled)
				{
					if (m_force_repaint)
					{
						m_frametime_graph.set_font_size(static_cast<u16>(m_font_size * 0.8));
					}
					m_frametime_graph.update();
					m_frametime_graph.set_pos(body_left, y_offset);
					m_frametime_graph.set_size(graphs_width, frametime_graph_h);
				}
			}

			m_force_repaint = false;
		}

		void perf_metrics_overlay::reset_body()
		{
			m_body.set_font(m_font.c_str(), m_font_size);
			m_body.fore_color = convert_color_code(m_color_body, m_opacity);
			m_body.back_color = convert_color_code(m_background_body, m_opacity);
			reset_transform(m_body);
		}

		void perf_metrics_overlay::reset_titles()
		{
			m_titles.set_font(m_font.c_str(), m_font_size);
			m_titles.fore_color = convert_color_code(m_color_title, m_opacity);
			m_titles.back_color = convert_color_code(m_background_title, m_opacity);
			reset_transform(m_titles);

			switch (m_detail)
			{
			case detail_level::none: [[fallthrough]];
			case detail_level::minimal: [[fallthrough]];
			case detail_level::low: m_titles.set_text(""); break;
			case detail_level::medium: m_titles.set_text(fmt::format("\n\n%s", title1_medium)); break;
			case detail_level::high: m_titles.set_text(fmt::format("\n\n%s\n\n\n\n\n\n%s", title1_high, title2)); break;
			}
			m_titles.auto_resize();
			m_titles.refresh();
		}

		void perf_metrics_overlay::init()
		{
			m_padding = m_font_size / 2;
			m_fps_graph.set_one_percent_sort_high(false);
			m_frametime_graph.set_one_percent_sort_high(true);

			reset_transforms();
			force_next_update();

			if (!m_is_initialised)
			{
				m_update_timer.Start();
				m_frametime_timer.Start();
			}

			update(get_system_time());

			// The text might have changed during the update. Recalculate positions.
			reset_transforms();

			m_is_initialised = true;
			visible = true;
		}

		void perf_metrics_overlay::set_framerate_graph_enabled(bool enabled)
		{
			if (m_framerate_graph_enabled == enabled)
				return;

			m_framerate_graph_enabled = enabled;

			if (enabled)
			{
				m_fps_graph.set_title("Framerate: 00.0");
				m_fps_graph.set_font_size(static_cast<u16>(m_font_size * 0.8));
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
				m_frametime_graph.set_title("Frametime: 0.0");
				m_frametime_graph.set_font_size(static_cast<u16>(m_font_size * 0.8));
				m_frametime_graph.set_color(convert_color_code(m_color_body, m_opacity));
				m_frametime_graph.set_guide_interval(8);
			}

			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_framerate_datapoint_count(u32 datapoint_count)
		{
			if (m_fps_graph.get_datapoint_count() == datapoint_count)
				return;

			m_fps_graph.set_count(datapoint_count);
			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_frametime_datapoint_count(u32 datapoint_count)
		{
			if (m_frametime_graph.get_datapoint_count() == datapoint_count)
				return;

			m_frametime_graph.set_count(datapoint_count);
			m_force_repaint = true;
		}

		void perf_metrics_overlay::set_graph_detail_levels(perf_graph_detail_level framerate_level, perf_graph_detail_level frametime_level)
		{
			m_fps_graph.set_labels_visible(
				framerate_level == perf_graph_detail_level::show_all || framerate_level == perf_graph_detail_level::show_min_max,
				framerate_level == perf_graph_detail_level::show_all || framerate_level == perf_graph_detail_level::show_one_percent_avg);
			m_frametime_graph.set_labels_visible(
				frametime_level == perf_graph_detail_level::show_all || frametime_level == perf_graph_detail_level::show_min_max,
				frametime_level == perf_graph_detail_level::show_all || frametime_level == perf_graph_detail_level::show_one_percent_avg);

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
			m_padding = m_font_size / 2;

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

		void perf_metrics_overlay::update(u64 /*timestamp_us*/)
		{
			const auto elapsed_update = m_update_timer.GetElapsedTimeInMilliSec();
			const bool do_update = m_force_update || elapsed_update >= m_update_interval;

			if (m_is_initialised)
			{
				if (m_frametime_graph_enabled && !m_force_update)
				{
					const float elapsed_frame = static_cast<float>(m_frametime_timer.GetElapsedTimeInMilliSec());
					m_frametime_timer.Start();
					m_frametime_graph.record_datapoint(elapsed_frame, do_update);
					m_frametime_graph.set_title(fmt::format("Frametime: %4.1f", elapsed_frame).c_str());
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

			if (do_update)
			{
				// 1. Fetch/calculate metrics we'll need
				if (!m_is_initialised || !m_force_update)
				{
					m_update_timer.Start();

					auto& rsx_thread = g_fxo->get<rsx::thread>();

					switch (m_detail)
					{
					case detail_level::high:
					{
						m_frametime = std::max(0.f, static_cast<float>(elapsed_update / m_frames));

						m_rsx_load = rsx_thread.get_load();

						m_total_threads = utils::cpu_stats::get_current_thread_count();

						[[fallthrough]];
					}
					case detail_level::medium:
					{
						m_ppus = idm::select<named_thread<ppu_thread>>([this](u32, named_thread<ppu_thread>& ppu)
						{
							m_ppu_cycles += thread_ctrl::get_cycles(ppu);
						});

						m_spus = idm::select<named_thread<spu_thread>>([this](u32, named_thread<spu_thread>& spu)
						{
							m_spu_cycles += thread_ctrl::get_cycles(spu);
						});

						m_rsx_cycles += rsx_thread.get_cycles();

						m_total_cycles = std::max<u64>(1, m_ppu_cycles + m_spu_cycles + m_rsx_cycles);
						m_cpu_usage    = static_cast<f32>(m_cpu_stats.get_usage());

						m_ppu_usage = std::clamp(m_cpu_usage * m_ppu_cycles / m_total_cycles, 0.f, 100.f);
						m_spu_usage = std::clamp(m_cpu_usage * m_spu_cycles / m_total_cycles, 0.f, 100.f);
						m_rsx_usage = std::clamp(m_cpu_usage * m_rsx_cycles / m_total_cycles, 0.f, 100.f);

						[[fallthrough]];
					}
					case detail_level::low:
					{
						if (m_detail == detail_level::low) // otherwise already acquired in medium
							m_cpu_usage = static_cast<f32>(m_cpu_stats.get_usage());

						[[fallthrough]];
					}
					case detail_level::minimal:
					{
						[[fallthrough]];
					}
					case detail_level::none:
					{
						m_fps = std::max(0.f, static_cast<f32>(m_frames / (elapsed_update / 1000)));
						if (m_is_initialised && m_framerate_graph_enabled)
						{
							m_fps_graph.record_datapoint(m_fps, true);
							m_fps_graph.set_title(fmt::format("Framerate: %04.1f", m_fps).c_str());
						}
						break;
					}
					}
				}

				// 2. Format output string
				std::string perf_text;

				switch (m_detail)
				{
				case detail_level::none:
				{
					break;
				}
				case detail_level::minimal:
				{
					fmt::append(perf_text, "FPS : %05.2f", m_fps);
					break;
				}
				case detail_level::low:
				{
					fmt::append(perf_text, "FPS : %05.2f\n"
					                         "CPU : %04.1f %%",
					    m_fps, m_cpu_usage);
					break;
				}
				case detail_level::medium:
				{
					fmt::append(perf_text, "FPS : %05.2f\n\n"
					                         "%s\n"
					                         " PPU   : %04.1f %%\n"
					                         " SPU   : %04.1f %%\n"
					                         " RSX   : %04.1f %%\n"
					                         " Total : %04.1f %%",
					    m_fps, std::string(title1_medium.size(), ' '), m_ppu_usage, m_spu_usage, m_rsx_usage, m_cpu_usage, std::string(title2.size(), ' '));
					break;
				}
				case detail_level::high:
				{
					fmt::append(perf_text, "FPS : %05.2f (%03.1fms)\n\n"
					                         "%s\n"
					                         " PPU   : %04.1f %% (%2u)\n"
					                         " SPU   : %04.1f %% (%2u)\n"
					                         " RSX   : %04.1f %% ( 1)\n"
					                         " Total : %04.1f %% (%2u)\n\n"
					                         "%s\n"
					                         " RSX   : %02u %%",
					    m_fps, m_frametime, std::string(title1_high.size(), ' '), m_ppu_usage, m_ppus, m_spu_usage, m_spus, m_rsx_usage, m_cpu_usage, m_total_threads, std::string(title2.size(), ' '), m_rsx_load);
					break;
				}
				}

				m_body.set_text(perf_text);

				if (perf_text.empty())
				{
					if (m_body.w > 0 || m_body.h > 0)
					{
						m_body.set_size(0, 0);
						reset_transforms();
					}
				}
				else if (m_body.auto_resize())
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

				if (m_framerate_graph_enabled)
				{
					m_fps_graph.update();
				}

				if (m_frametime_graph_enabled)
				{
					m_frametime_graph.update();
				}
			}
		}

		compiled_resource perf_metrics_overlay::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource compiled_resources = m_body.get_compiled();

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
			m_label.alignment = text_align::center;
			m_label.fore_color = { 1.f, 1.f, 1.f, 1.f };
			m_label.back_color = { 0.f, 0.f, 0.f, .7f };

			back_color = { 0.f, 0.f, 0.f, 0.5f };
		}

		void graph::set_pos(s16 _x, s16 _y)
		{
			m_label.set_pos(_x, _y);
			overlay_element::set_pos(_x, _y + m_label.h);
		}

		void graph::set_size(u16 _w, u16 _h)
		{
			m_label.set_size(_w, m_label.h);
			overlay_element::set_size(_w, _h);
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
			const auto font_name = m_label.get_font()->get_name().data();
			m_label.set_font(font_name, font_size);
		}

		void graph::set_count(u32 datapoint_count)
		{
			m_datapoint_count = datapoint_count;

			if (m_datapoints.empty())
			{
				m_datapoints.resize(m_datapoint_count, -1.0f);
			}
			else if (m_datapoints.empty() || m_datapoint_count < m_datapoints.size())
			{
				std::copy(m_datapoints.begin() + m_datapoints.size() - m_datapoint_count, m_datapoints.end(), m_datapoints.begin());
				m_datapoints.resize(m_datapoint_count);
			}
			else
			{
				m_datapoints.insert(m_datapoints.begin(), m_datapoint_count - m_datapoints.size(), -1.0f);
			}
		}

		void graph::set_color(color4f color)
		{
			m_color = color;
		}

		void graph::set_guide_interval(f32 guide_interval)
		{
			m_guide_interval = guide_interval;
		}

		void graph::set_labels_visible(bool show_min_max, bool show_1p_avg)
		{
			m_show_min_max = show_min_max;
			m_show_1p_avg = show_1p_avg;
		}

		void graph::set_one_percent_sort_high(bool sort_1p_high)
		{
			m_1p_sort_high = sort_1p_high;
		}

		u16 graph::get_height() const
		{
			return h + m_label.h + m_label.padding_top + m_label.padding_bottom;
		}

		u32 graph::get_datapoint_count() const
		{
			return m_datapoint_count;
		}

		void graph::record_datapoint(f32 datapoint, bool update_metrics)
		{
			ensure(datapoint >= 0.0f);

			// std::dequeue is only faster for large sizes, so just use a std::vector and resize once in while

			// Record datapoint
			m_datapoints.push_back(datapoint);

			// Cull vector when it gets large
			if (m_datapoints.size() > m_datapoint_count * 16ull)
			{
				std::copy(m_datapoints.begin() + m_datapoints.size() - m_datapoint_count, m_datapoints.end(), m_datapoints.begin());
				m_datapoints.resize(m_datapoint_count);
			}

			if (!update_metrics)
			{
				return;
			}

			m_min = max_v<f32>;
			m_max = 0.0f;
			m_avg = 0.0f;
			m_1p = 0.0f;

			std::vector<f32> valid_datapoints;

			// Make sure min/max reflects the data being displayed, not the entire datapoints vector
			for (usz i = m_datapoints.size() - m_datapoint_count; i < m_datapoints.size(); i++)
			{
				const f32& dp = m_datapoints[i];

				if (dp < 0) continue; // Skip initial negative values. They don't count.

				m_min = std::min(m_min, dp);
				m_max = std::max(m_max, dp);
				m_avg += dp;

				if (m_show_1p_avg)
				{
					valid_datapoints.push_back(dp);
				}
			}

			// Sanitize min value
			m_min = std::min(m_min, m_max);

			if (m_show_1p_avg && !valid_datapoints.empty())
			{
				// Sort datapoints (we are only interested in the lowest/highest 1%)
				const usz i_1p = valid_datapoints.size() / 100;
				const usz n_1p = i_1p + 1;

				if (m_1p_sort_high)
					std::nth_element(valid_datapoints.begin(), valid_datapoints.begin() + i_1p, valid_datapoints.end(), std::greater<f32>());
				else
					std::nth_element(valid_datapoints.begin(), valid_datapoints.begin() + i_1p, valid_datapoints.end());

				// Calculate statistics
				m_avg /= valid_datapoints.size();
				m_1p = std::accumulate(valid_datapoints.begin(), valid_datapoints.begin() + n_1p, 0.0f) / static_cast<float>(n_1p);
			}
		}

		void graph::update()
		{
			std::string fps_info = m_title;

			if (m_show_1p_avg)
			{
				fmt::append(fps_info, "\n1%%:%4.1f av:%4.1f", m_1p, m_avg);
			}

			if (m_show_min_max)
			{
				fmt::append(fps_info, "\nmn:%4.1f mx:%4.1f", m_min, m_max);
			}

			m_label.set_text(fps_info);
			m_label.set_padding(4, 4, 0, 4);

			m_label.auto_resize();
			m_label.refresh();

			// If label horizontal end is larger, widen graph width to match it
			set_size(std::max(m_label.w, w), h);
		}

		compiled_resource& graph::get_compiled()
		{
			if (is_compiled())
			{
				return compiled_resources;
			}

			overlay_element::get_compiled();

			const f32 normalize_factor = f32(h) / (m_max != 0.0f ? m_max : 1.0f);

			// Don't show guide lines if they'd be more dense than 1 guide line every 3 pixels
			const bool guides_too_dense = (m_max / m_guide_interval) > (h / 3.0f);

			if (m_guide_interval > 0 && !guides_too_dense)
			{
				auto& cmd_guides = compiled_resources.append({});
				auto& config_guides = cmd_guides.config;

				config_guides.color = { 1.f, 1.f, 1.f, .2f };
				config_guides.primitives = primitive_type::line_list;

				auto& verts_guides = compiled_resources.draw_commands.back().verts;

				for (auto y_off = m_guide_interval; y_off < m_max; y_off += m_guide_interval)
				{
					const f32 guide_y = y + h - y_off * normalize_factor;
					verts_guides.emplace_back(x, guide_y);
					verts_guides.emplace_back(static_cast<float>(x + w), guide_y);
				}
			}

			auto& cmd_graph = compiled_resources.append({});
			auto& config_graph = cmd_graph.config;

			config_graph.color = m_color;
			config_graph.primitives = primitive_type::line_strip;

			auto& verts_graph = compiled_resources.draw_commands.back().verts;

			f32 x_stride = w;
			if (m_datapoint_count > 2)
			{
				x_stride /= (m_datapoint_count - 1);
			}

			const usz tail_index_offset = m_datapoints.size() - m_datapoint_count;

			for (u32 i = 0; i < m_datapoint_count; ++i)
			{
				const f32 x_line = x + i * x_stride;
				const f32 y_line = y + h - (std::max(0.0f, m_datapoints[i + tail_index_offset]) * normalize_factor);
				verts_graph.emplace_back(x_line, y_line);
			}

			compiled_resources.add(m_label.get_compiled());

			return compiled_resources;
		}

		extern void reset_performance_overlay()
		{
			if (!g_cfg.misc.use_native_interface)
				return;

			if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
			{
				auto& perf_settings = g_cfg.video.perf_overlay;
				auto perf_overlay = manager->get<rsx::overlays::perf_metrics_overlay>();

				if (perf_settings.perf_overlay_enabled)
				{
					if (!perf_overlay)
					{
						perf_overlay = manager->create<rsx::overlays::perf_metrics_overlay>();
					}

					std::lock_guard lock(*manager);

					perf_overlay->set_detail_level(perf_settings.level);
					perf_overlay->set_position(perf_settings.position);
					perf_overlay->set_update_interval(perf_settings.update_interval);
					perf_overlay->set_font(perf_settings.font);
					perf_overlay->set_font_size(perf_settings.font_size);
					perf_overlay->set_margins(perf_settings.margin_x, perf_settings.margin_y, perf_settings.center_x.get(), perf_settings.center_y.get());
					perf_overlay->set_opacity(perf_settings.opacity / 100.f);
					perf_overlay->set_body_colors(perf_settings.color_body, perf_settings.background_body);
					perf_overlay->set_title_colors(perf_settings.color_title, perf_settings.background_title);
					perf_overlay->set_framerate_datapoint_count(perf_settings.framerate_datapoint_count);
					perf_overlay->set_frametime_datapoint_count(perf_settings.frametime_datapoint_count);
					perf_overlay->set_framerate_graph_enabled(perf_settings.framerate_graph_enabled.get());
					perf_overlay->set_frametime_graph_enabled(perf_settings.frametime_graph_enabled.get());
					perf_overlay->set_graph_detail_levels(perf_settings.framerate_graph_detail_level.get(), perf_settings.frametime_graph_detail_level.get());
					perf_overlay->init();
				}
				else if (perf_overlay)
				{
					manager->remove<rsx::overlays::perf_metrics_overlay>();
				}
			}
		}
	} // namespace overlays
} // namespace rsx
