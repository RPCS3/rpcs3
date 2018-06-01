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
		void perf_metrics_overlay::reset_body()
		{
			m_body.set_font("n023055ms.ttf", m_font_size);
			m_body.set_pos(50, 50);
			m_body.fore_color     = {0xFF / 255.f, 0xe1 / 255.f, 0x38 / 255.f, 1.0f};
			m_body.back_color     = {0x00 / 255.f, 0x23 / 255.f, 0x39 / 255.f, 0.7f};
			m_body.padding_top    = 0;
			m_body.padding_bottom = -14;
			m_body.set_margin(5, 5, 0, 0);
		}

		void perf_metrics_overlay::reset_titles()
		{
			m_titles.set_font("n023055ms.ttf", m_font_size);
			m_titles.set_pos(50, 50);
			m_titles.fore_color     = {0xf2 / 256.0, 0x6C / 256.0, 0x24 / 256.0, 1.0f};
			m_titles.back_color     = {0.0f, 0.0f, 0.0f, 0.0f};
			m_titles.padding_top    = 0;
			m_titles.padding_bottom = -14;
			m_titles.set_margin(5, 5, 0, 0);

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
			reset_titles();
			reset_body();

			force_next_update();
			update();
			m_update_timer.Start();
		}

		perf_metrics_overlay::perf_metrics_overlay(bool initialize)
		{
			// Default values, will change based on config options
			m_update_interval = 350;
			m_detail          = detail_level::high;
			m_font_size       = 10;

			if (initialize)
				init();
		}

		// In ms
		void perf_metrics_overlay::set_update_interval(u32 update_interval)
		{
			m_update_interval = update_interval;
		}

		void perf_metrics_overlay::set_detail_level(detail_level level)
		{
			m_detail = level;
			reset_titles();
		}

		void perf_metrics_overlay::set_font_size(u32 font_size)
		{
			m_font_size = font_size;
			reset_titles();
			reset_body();
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
				f32 fps{0};
				f32 frametime{0};

				u64 ppu_cycles{0};
				u64 spu_cycles{0};
				u64 rsx_cycles{0};
				u64 total_cycles{0};

				u32 ppus{0};
				u32 spus{0};
				u32 rawspus{0};

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
					ppus = idm::select<ppu_thread>([&ppu_cycles](u32, ppu_thread& ppu) { ppu_cycles += ppu.get()->get_cycles(); });

					spus = idm::select<SPUThread>([&spu_cycles](u32, SPUThread& spu) { spu_cycles += spu.get()->get_cycles(); });

					rawspus = idm::select<RawSPUThread>([&spu_cycles](u32, RawSPUThread& rawspu) { spu_cycles += rawspu.get()->get_cycles(); });

					if (!rsx_thread)
						rsx_thread = fxm::get<GSRender>();

					rsx_cycles += rsx_thread->get()->get_cycles();

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
					    fps, frametime, std::string(title1_high.size(), ' '), ppu_usage, ppus, spu_usage, spus + rawspus, rsx_usage, cpu_usage, total_threads, std::string(title2.size(), ' '), rsx_load);
					break;
				}
				}

				m_body.text = perf_text;
				m_body.auto_resize();
				m_body.refresh();

				if (!m_force_update)
				{
					m_frames = 0;
					m_update_timer.Start();
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
