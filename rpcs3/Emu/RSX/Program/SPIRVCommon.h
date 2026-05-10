#pragma once

namespace glsl
{
	enum program_domain : unsigned char;
	enum glsl_rules : unsigned char;
}

namespace spirv
{
	bool compile_glsl_to_spv(std::vector<u32>& spv, std::string& shader, ::glsl::program_domain domain, ::glsl::glsl_rules rules);

	void initialize_compiler_context();
	void finalize_compiler_context();
}
