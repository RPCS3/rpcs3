#pragma once

namespace glsl
{
	enum program_domain
	{
		glsl_vertex_program = 0,
		glsl_fragment_program = 1,
		glsl_compute_program = 2
	};

	enum glsl_rules
	{
		glsl_rules_opengl4,
		glsl_rules_spirv
	};

	struct shader_properties
	{
		glsl::program_domain domain;
		// Applicable in vertex stage
		bool require_lit_emulation;

		// Only relevant for fragment programs
		bool require_wpos;
		bool require_depth_conversion;
		bool require_texture_ops;
		bool emulate_shadow_compare;
		bool low_precision_tests;
	};
};
