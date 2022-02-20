#pragma once

namespace glsl
{
	enum program_domain : unsigned char
	{
		glsl_vertex_program = 0,
		glsl_fragment_program = 1,
		glsl_compute_program = 2
	};

	enum glsl_rules : unsigned char
	{
		glsl_rules_opengl4,
		glsl_rules_spirv
	};

	struct shader_properties
	{
		glsl::program_domain domain : 3;
		// Applicable in vertex stage
		bool require_lit_emulation : 1;

		// Only relevant for fragment programs
		bool fp32_outputs : 1;
		bool require_wpos : 1;
		bool require_depth_conversion : 1;
		bool require_texture_ops : 1;
		bool require_shadow_ops : 1;
		bool require_texture_expand : 1;
		bool require_srgb_to_linear : 1;
		bool require_linear_to_srgb : 1;
		bool emulate_coverage_tests : 1;
		bool emulate_shadow_compare : 1;
		bool emulate_zclip_transform : 1;
		bool emulate_depth_clip_only : 1;
		bool low_precision_tests : 1;
		bool disable_early_discard : 1;
		bool supports_native_fp16 : 1;
		bool srgb_output_rounding : 1;
	};
};
