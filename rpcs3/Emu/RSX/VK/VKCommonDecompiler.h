#pragma once
#include "../Program/GLSLTypes.h"

namespace vk
{
	using namespace ::glsl;

	int get_varying_register_location(std::string_view varying_register_name);

	int get_texture_index(std::string_view name);
}
