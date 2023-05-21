#pragma once

#include <string>

namespace rpcs3
{
	struct title_format_data
	{
		std::string format;
		std::string title;
		std::string title_id;
		std::string renderer;
		std::string vulkan_adapter;
		double fps = .0;
	};

	std::string get_formatted_title(const title_format_data& title_data);
}
