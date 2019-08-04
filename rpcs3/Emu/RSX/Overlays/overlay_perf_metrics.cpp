#include "stdafx.h"
#include "overlays.h"
#include "../GSRender.h"

#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Utilities/sysinfo.h"

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
				LOG_ERROR(RSX, "Overlays: tried to convert incompatible color code: '%s' exception: '%s'", hex_color, e.what());
				return color4f(0.0f, 0.0f, 0.0f, 0.0f);
			}

			const int r = (len == 8 ? (hexval >> 24) : (hexval >> 16)) & 0xff;
			const int g = (len == 8 ? (hexval >> 16) : (hexval >> 8)) & 0xff;
			const int b = (len == 8 ? (hexval >> 8) : (hexval >> 0)) & 0xff;
			const int a = len == 8 ? ((hexval >> 0) & 0xff) : 255;

			return color4f(r / 255.f, g / 255.f, b / 255.f, a / 255.f * opacity);
		}

		void perf_metrics_overlay::reset_transform(label& elm) const
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
				pos.y = virtual_height - overlay_height;
				break;
			case screen_quadrant::bottom_right:
				pos.x = virtual_width - overlay_width;
				pos.y = virtual_height - overlay_height;
				break;
			}

			if (g_cfg.video.perf_overlay.center_x)
			{
				pos.x = (virtual_width - m_body.w) / 2;
			}

			if (g_cfg.video.perf_overlay.center_y)
			{
				pos.y = (virtual_height - m_body.h) / 2;
			}

			elm.set_pos(pos.x, pos.y);
			elm.set_padding(padding.x1, padding.x2, padding.y1, padding.y2);
		}

		void perf_metrics_overlay::reset_transforms()
		{
			reset_transform(m_body);
			reset_transform(m_titles);
		}

		void perf_metrics_overlay::reset_body()
		{
			m_body.set_font(m_font.c_str(), m_font_size);
			m_body.fore_color = convert_color_code(g_cfg.video.perf_overlay.color_body, m_opacity);
			m_body.back_color = convert_color_code(g_cfg.video.perf_overlay.background_body, m_opacity);
			reset_transform(m_body);
		}

		void perf_metrics_overlay::reset_titles()
		{
			m_titles.set_font(m_font.c_str(), m_font_size);
			m_titles.fore_color = convert_color_code(g_cfg.video.perf_overlay.color_title, m_opacity);
			m_titles.back_color = convert_color_code(g_cfg.video.perf_overlay.background_title, m_opacity);
			reset_transform(m_titles);

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

		void perf_metrics_overlay::reset_text()
		{
			reset_body();
			reset_titles();
		}

		void perf_metrics_overlay::init()
		{
			reset_text();
			force_next_update();
			update();
			m_update_timer.Start();

			m_is_initialised = true;
		}

		void perf_metrics_overlay::set_detail_level(detail_level level)
		{
			m_detail = level;

			if (m_is_initialised)
			{
				reset_titles();
			}
		}

		void perf_metrics_overlay::set_position(screen_quadrant quadrant)
		{
			m_quadrant = quadrant;

			if (m_is_initialised)
			{
				reset_transforms();
			}
		}

		// In ms
		void perf_metrics_overlay::set_update_interval(u32 update_interval)
		{
			m_update_interval = update_interval;
		}

		void perf_metrics_overlay::set_font(std::string font)
		{
			m_font = std::move(font);

			if (m_is_initialised)
			{
				reset_text();
			}
		}

		void perf_metrics_overlay::set_font_size(u32 font_size)
		{
			m_font_size = font_size;

			if (m_is_initialised)
			{
				reset_text();
			}
		}

		void perf_metrics_overlay::set_margins(u32 margin_x, u32 margin_y)
		{
			m_margin_x = margin_x;
			m_margin_y = margin_y;

			if (m_is_initialised)
			{
				reset_transforms();
			}
		}

		void perf_metrics_overlay::set_opacity(f32 opacity)
		{
			m_opacity = opacity;

			if (m_is_initialised)
			{
				reset_text();
			}
		}

		void perf_metrics_overlay::force_next_update()
		{
			m_force_update = true;
		}

		void perf_metrics_overlay::update()
		{
			const auto elapsed = m_update_timer.GetElapsedTimeInMilliSec();

			if (!m_force_update)
			{
				++m_frames;
			}

			if (elapsed >= m_update_interval || m_force_update)
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

				std::shared_ptr<GSRender> rsx_thread;

				std::string perf_text;

				// 1. Fetch/calculate metrics we'll need
				switch (m_detail)
				{
				case detail_level::high:
				{
					frametime = m_force_update ? 0 : std::max(0.0, elapsed / m_frames);

					rsx_thread = fxm::get<GSRender>();
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

					if (!rsx_thread)
						rsx_thread = fxm::get<GSRender>();

					rsx_cycles += rsx_thread->get_cycles();

					total_cycles = ppu_cycles + spu_cycles + rsx_cycles;
					cpu_usage = m_cpu_stats.get_usage();

					ppu_usage = std::clamp(cpu_usage * ppu_cycles / total_cycles, 0.f, 100.f);
					spu_usage = std::clamp(cpu_usage * spu_cycles / total_cycles, 0.f, 100.f);
					rsx_usage = std::clamp(cpu_usage * rsx_cycles / total_cycles, 0.f, 100.f);

					// fallthrough
				}
				case detail_level::low:
				{
					if (cpu_usage == -1.f)
						cpu_usage = m_cpu_stats.get_usage();

					// fallthrough
				}
				case detail_level::minimal:
				{
					fps = m_force_update ? 0 : std::max(0.0, static_cast<f32>(m_frames) / (elapsed / 1000));
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
		}
	} // namespace overlays
} // namespace rsx
