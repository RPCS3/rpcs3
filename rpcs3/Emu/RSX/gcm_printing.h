#pragma once
#include <Utilities/types.h>
#include <string>
#include <functional>

namespace rsx
{
	std::string get_method_name(const u32 id);

	std::function<std::string(u32)> get_pretty_printing_function(const u32 id);
}
