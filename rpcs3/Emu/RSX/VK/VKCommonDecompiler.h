#pragma once
#include "../Common/ShaderParam.h"
#include "../Common/GLSLCommon.h"

namespace vk
{
	using namespace ::glsl;

	struct varying_register_t
	{
		std::string name;
		int reg_location;
	};

	const varying_register_t& get_varying_register(const std::string& name);
	bool compile_glsl_to_spv(std::string& shader, program_domain domain, std::vector<u32> &spv);

	void initialize_compiler_context();
	void finalize_compiler_context();
}
