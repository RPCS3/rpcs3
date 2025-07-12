#pragma once

namespace glsl
{
	enum program_domain : unsigned char
	{
		glsl_vertex_program = 0,
		glsl_fragment_program = 1,
		glsl_compute_program = 2,

		// Meta
		glsl_invalid_program = 0xff
	};

	enum glsl_rules : unsigned char
	{
		glsl_rules_opengl4,
		glsl_rules_vulkan
	};

	struct shader_properties
	{
		glsl::program_domain domain : 3;

		// Applicable in vertex stage
		bool require_lit_emulation : 1;
		bool require_explicit_invariance : 1;
		bool require_instanced_render : 1;
		bool emulate_zclip_transform : 1;
		bool emulate_depth_clip_only : 1;

		// Only relevant for fragment programs
		bool fp32_outputs : 1;
		bool require_wpos : 1;
		bool require_srgb_to_linear : 1;
		bool require_linear_to_srgb : 1;
		bool require_fog_read : 1;
		bool emulate_coverage_tests : 1;
		bool emulate_shadow_compare : 1;
		bool low_precision_tests : 1;
		bool disable_early_discard : 1;
		bool supports_native_fp16 : 1;
		bool ROP_output_rounding : 1;

		// Texturing spec
		bool require_texture_ops : 1;           // Global switch to enable/disable all texture code
		bool require_depth_conversion : 1;      // Include DSV<->RTV bitcast emulation
		bool require_tex_shadow_ops : 1;        // Include shadow compare emulation
		bool require_msaa_ops : 1;              // Include MSAA<->Resolved bitcast emulation
		bool require_texture_expand : 1;        // Include sign-expansion emulation
		bool require_tex1D_ops : 1;             // Include 1D texture stuff
		bool require_tex2D_ops : 1;             // Include 2D texture stuff
		bool require_tex3D_ops : 1;             // Include 3D texture stuff (including cubemap)
		bool require_shadowProj_ops : 1;        // Include shadow2DProj projection textures (1D is unsupported anyway)
	};
};
