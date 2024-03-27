#pragma once
#include "../Program/GLSLTypes.h"

namespace vk
{
	using namespace ::glsl;

	int get_varying_register_location(std::string_view varying_register_name);
}
