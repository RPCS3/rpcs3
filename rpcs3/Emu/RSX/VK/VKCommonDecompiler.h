#pragma once
#include "../Common/ShaderParam.h"
#include "VKHelpers.h"

namespace vk
{
	struct varying_register_t
	{
		std::string name;
		int reg_location;
	};

	std::string getFloatTypeNameImpl(size_t elementCount);
	std::string getFunctionImpl(FUNCTION f);
	std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1);
	void insert_glsl_legacy_function(std::ostream& OS, glsl::program_domain domain);

	const varying_register_t& get_varying_register(const std::string& name);
	bool compile_glsl_to_spv(std::string& shader, glsl::program_domain domain, std::vector<u32> &spv);

	void initialize_compiler_context();
	void finalize_compiler_context();
}
