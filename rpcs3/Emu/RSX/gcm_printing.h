#pragma once

#include "util/types.hpp"
#include <string>
#include <functional>

namespace rsx
{
	std::string get_method_name(u32 id);

	std::function<std::string(u32)> get_pretty_printing_function(u32 id);
}
