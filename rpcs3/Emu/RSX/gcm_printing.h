#pragma once

#include "util/types.hpp"
#include <string>

namespace rsx
{
	std::pair<std::string_view, std::string_view> get_method_name(u32 id, std::string& result_str);

	std::add_pointer_t<void(std::string&, u32, u32)> get_pretty_printing_function(u32 id);
}
