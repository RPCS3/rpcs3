#pragma once

#include "util/types.hpp"
#include <string>
#include <functional>

namespace rsx
{
	std::string get_method_name(u32 id);

	std::add_pointer_t<std::string(u32, u32)> get_pretty_printing_function(u32 id);
}
