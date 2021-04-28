#pragma once
#include "../Common/GLSLTypes.h"

namespace vk
{
	using namespace ::glsl;

	int get_varying_register_location(std::string_view varying_register_name);
	bool compile_glsl_to_spv(std::string& shader, program_domain domain, std::vector<u32> &spv);

	void initialize_compiler_context();
	void finalize_compiler_context();
}
