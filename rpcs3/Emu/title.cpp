#include "stdafx.h"
#include "title.h"
#include "rpcs3_version.h"

#include "util/sysinfo.hpp"
#include "Emu/system_config.h"

#if !(defined(__ANDROID__) || defined(__APPLE__))
#include "Emu/RSX/GL/glutils/capabilities.h"
#endif

namespace rpcs3
{
	std::string get_formatted_title(const title_format_data& title_data)
	{
		// Parse title format string
		std::string title_string;

		for (usz i = 0; i < title_data.format.size();)
		{
			const char c1 = title_data.format[i];

			if (c1 == '\0')
			{
				break;
			}

			switch (c1)
			{
			case '%':
			{
				const char c2 = title_data.format[i + 1];

				if (c2 == '\0')
				{
					title_string += '%';
					i++;
					continue;
				}

				switch (c2)
				{
				case '%':
				{
					title_string += '%';
					break;
				}
				case 'A':
				{
					title_string += utils::get_architecture();
					break;
				}
				case 'T':
				{
					title_string += title_data.title;
					break;
				}
				case 't':
				{
					title_string += title_data.title_id;
					break;
				}
				case 'R':
				{
					fmt::append(title_string, "%s", title_data.renderer);
					break;
				}
				case 'V':
				{
					static const std::string version = rpcs3::get_version_and_branch();
					title_string += version;
					break;
				}
				case 'F':
				{
					fmt::append(title_string, "%.2f", title_data.fps);
					break;
				}
				case 'G':
				{
					switch (g_cfg.video.renderer.get())
					{
					case video_renderer::null:
						title_string += "null";
						break;
					case video_renderer::opengl:
#if !(defined(__ANDROID__) || defined(__APPLE__))
						title_string += gl::get_device_name();
#else
						title_string += "OpenGL GPU";
#endif
						break;
					case video_renderer::vulkan:
						title_string += title_data.vulkan_adapter;
						break;
					}
					break;
				}
				case 'C':
				{
					static const std::string brand = utils::get_cpu_brand();
					title_string += brand;
					break;
				}
				case 'c':
				{
					fmt::append(title_string, "%d", utils::get_thread_count());
					break;
				}
				case 'M':
				{
					fmt::append(title_string, "%.2f", utils::get_total_memory() / (1024.0f * 1024 * 1024));
					break;
				}
				default:
				{
					title_string += '%';
					title_string += c2;
					break;
				}
				}

				i += 2;
				break;
			}
			default:
			{
				title_string += c1;
				i += 1;
				break;
			}
			}
		}

		return title_string;
	}
}
