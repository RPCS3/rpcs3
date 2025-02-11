#include "stdafx.h"

#include "GLSLCommon.h"
#include "RSXFragmentProgram.h"

#include "Emu/RSX/gcm_enums.h"
#include "Utilities/StrFmt.h"

namespace program_common
{
	template <typename T>
	void define_glsl_constants(std::ostream& OS, std::initializer_list<std::pair<const char*, T>> enums)
	{
		for (const auto& e : enums)
		{
			if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
			{
				OS << "#define " << e.first << " " << static_cast<int>(e.second) << "\n";
			}
			else
			{
				OS << "#define " << e.first << " " << e.second << "\n";
			}
		}

		OS << "\n";
	}

	void define_glsl_switches(std::ostream& OS, std::vector<std::string_view>& enums)
	{
		for (const auto& e : enums)
		{
			OS << "#define " << e << "\n";
		}

		OS << "\n";
	}
}

namespace glsl
{
	std::string getFloatTypeNameImpl(usz elementCount)
	{
		switch (elementCount)
		{
		default:
			abort();
		case 1:
			return "float";
		case 2:
			return "vec2";
		case 3:
			return "vec3";
		case 4:
			return "vec4";
		}
	}

	std::string getHalfTypeNameImpl(usz elementCount)
	{
		switch (elementCount)
		{
		default:
			abort();
		case 1:
			return "float16_t";
		case 2:
			return "f16vec2";
		case 3:
			return "f16vec3";
		case 4:
			return "f16vec4";
		}
	}

	std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1, bool scalar)
	{
		if (scalar)
		{
			switch (f)
			{
			case COMPARE::SEQ:
				return fmt::format("CMP_FIXUP(%s) == CMP_FIXUP(%s)", Op0, Op1);
			case COMPARE::SGE:
				return fmt::format("CMP_FIXUP(%s) >= CMP_FIXUP(%s)", Op0, Op1);
			case COMPARE::SGT:
				return fmt::format("CMP_FIXUP(%s) > CMP_FIXUP(%s)", Op0, Op1);
			case COMPARE::SLE:
				return fmt::format("CMP_FIXUP(%s) <= CMP_FIXUP(%s)", Op0, Op1);
			case COMPARE::SLT:
				return fmt::format("CMP_FIXUP(%s) < CMP_FIXUP(%s)", Op0, Op1);
			case COMPARE::SNE:
				return fmt::format("CMP_FIXUP(%s) != CMP_FIXUP(%s)", Op0, Op1);
			}
		}
		else
		{
			switch (f)
			{
			case COMPARE::SEQ:
				return fmt::format("equal(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			case COMPARE::SGE:
				return fmt::format("greaterThanEqual(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			case COMPARE::SGT:
				return fmt::format("greaterThan(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			case COMPARE::SLE:
				return fmt::format("lessThanEqual(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			case COMPARE::SLT:
				return fmt::format("lessThan(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			case COMPARE::SNE:
				return fmt::format("notEqual(CMP_FIXUP(%s), CMP_FIXUP(%s))", Op0, Op1);
			}
		}

		fmt::throw_exception("Unknown compare function");
	}

	void insert_vertex_input_fetch(std::stringstream& OS, glsl_rules rules, bool glsl4_compliant)
	{
		std::string vertex_id_name = (rules != glsl_rules_vulkan) ? "gl_VertexID" : "gl_VertexIndex";

		// Actually decode a vertex attribute from a raw byte stream
		program_common::define_glsl_constants<int>(OS,
		{
			{ "VTX_FMT_SNORM16", RSX_VERTEX_BASE_TYPE_SNORM16 },
			{ "VTX_FMT_FLOAT32", RSX_VERTEX_BASE_TYPE_FLOAT },
			{ "VTX_FMT_FLOAT16", RSX_VERTEX_BASE_TYPE_HALF_FLOAT },
			{ "VTX_FMT_UNORM8 ", RSX_VERTEX_BASE_TYPE_UNORM8 },
			{ "VTX_FMT_SINT16 ", RSX_VERTEX_BASE_TYPE_SINT16 },
			{ "VTX_FMT_COMP32 ", RSX_VERTEX_BASE_TYPE_CMP32 },
			{ "VTX_FMT_UINT8  ", RSX_VERTEX_BASE_TYPE_UINT8 }
		});

		// For intel GPUs which cannot access vectors in indexed mode (driver bug? or glsl version too low?)
		// Note: Tested on Mesa iris with HD 530 and compilant path works fine, may be a bug on Windows proprietary drivers
		if (!glsl4_compliant)
		{
			OS << "#define _INTEL_GLSL\n";
		}

		OS <<
			#include "GLSLSnippets/RSXProg/RSXVertexFetch.glsl"
		;
	}

	void insert_blend_prologue(std::ostream& OS)
	{
		OS <<
			#include "GLSLSnippets/RSXProg/RSXProgrammableBlendPrologue.glsl"
			;
	}

	void insert_rop_init(std::ostream& OS)
	{
		OS <<
			#include "GLSLSnippets/RSXProg/RSXROPPrologue.glsl"
			;
	}

	void insert_rop(std::ostream& OS, const shader_properties& /*props*/)
	{
		OS <<
			#include "GLSLSnippets/RSXProg/RSXROPEpilogue.glsl"
			;
	}

	void insert_glsl_legacy_function(std::ostream& OS, const shader_properties& props)
	{
		std::vector<std::string_view> enabled_options;
		if (props.low_precision_tests)
		{
			enabled_options.push_back("_GPU_LOW_PRECISION_COMPARE");
		}

		if (props.require_lit_emulation)
		{
			enabled_options.push_back("_ENABLE_LIT_EMULATION");
		}

		OS << "#define _select mix\n";
		OS << "#define _saturate(x) clamp(x, 0., 1.)\n";
		OS << "#define _get_bits(x, off, count) bitfieldExtract(x, off, count)\n";
		OS << "#define _set_bits(x, y, off, count) bitfieldInsert(x, y, off, count)\n";
		OS << "#define _test_bit(x, y) (_get_bits(x, y, 1) != 0)\n";
		OS << "#define _rand(seed) fract(sin(dot(seed.xy, vec2(12.9898f, 78.233f))) * 43758.5453f)\n\n";


		if (props.domain == glsl::program_domain::glsl_fragment_program)
		{
			OS << "// ROP control\n";
			program_common::define_glsl_constants<rsx::ROP_control_bits>(OS,
			{
				{ "ALPHA_TEST_ENABLE_BIT       ", rsx::ROP_control_bits::ALPHA_TEST_ENABLE_BIT },
				{ "SRGB_FRAMEBUFFER_BIT        ", rsx::ROP_control_bits::SRGB_FRAMEBUFFER_BIT },
				{ "ALPHA_TO_COVERAGE_ENABLE_BIT", rsx::ROP_control_bits::ALPHA_TO_COVERAGE_ENABLE_BIT },
				{ "MSAA_WRITE_ENABLE_BIT       ", rsx::ROP_control_bits::MSAA_WRITE_ENABLE_BIT },
				{ "INT_FRAMEBUFFER_BIT         ", rsx::ROP_control_bits::INT_FRAMEBUFFER_BIT },
				{ "POLYGON_STIPPLE_ENABLE_BIT  ", rsx::ROP_control_bits::POLYGON_STIPPLE_ENABLE_BIT },
				{ "ALPHA_TEST_FUNC_OFFSET      ", rsx::ROP_control_bits::ALPHA_FUNC_OFFSET },
				{ "ALPHA_TEST_FUNC_LENGTH      ", rsx::ROP_control_bits::ALPHA_FUNC_NUM_BITS },
				{ "MSAA_SAMPLE_CTRL_OFFSET     ", rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_OFFSET },
				{ "MSAA_SAMPLE_CTRL_LENGTH     ", rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_NUM_BITS },
				{ "ROP_CMD_MASK                ", rsx::ROP_control_bits::ROP_CMD_MASK }
			});

			program_common::define_glsl_constants<const char*>(OS,
			{
				{ "col0", props.fp32_outputs ? "r0" : "h0" },
				{ "col1", props.fp32_outputs ? "r2" : "h4" },
				{ "col2", props.fp32_outputs ? "r3" : "h6" },
				{ "col3", props.fp32_outputs ? "r4" : "h8" }
			});

			if (props.fp32_outputs || !props.supports_native_fp16)
			{
				enabled_options.push_back("_32_BIT_OUTPUT");
			}

			if (!props.fp32_outputs)
			{
				enabled_options.push_back("_ENABLE_FRAMEBUFFER_SRGB");
			}

			if (props.disable_early_discard)
			{
				enabled_options.push_back("_DISABLE_EARLY_DISCARD");
			}

			if (props.ROP_output_rounding)
			{
				enabled_options.push_back("_ENABLE_ROP_OUTPUT_ROUNDING");
			}

			enabled_options.push_back("_ENABLE_POLYGON_STIPPLE");
		}

		// Import common header
		program_common::define_glsl_switches(OS, enabled_options);
		enabled_options.clear();

		OS <<
			#include "GLSLSnippets/RSXProg/RSXProgramCommon.glsl"
		;

		if (props.domain == glsl::program_domain::glsl_vertex_program)
		{
			if (props.require_explicit_invariance)
			{
				enabled_options.push_back("_FORCE_POSITION_INVARIANCE");
			}

			if (props.emulate_zclip_transform)
			{
				if (props.emulate_depth_clip_only)
				{
					enabled_options.push_back("_EMULATE_ZCLIP_XFORM_STANDARD");
				}
				else
				{
					enabled_options.push_back("_EMULATE_ZCLIP_XFORM_FALLBACK");
				}
			}

			if (props.require_instanced_render)
			{
				enabled_options.push_back("_ENABLE_INSTANCED_CONSTANTS");
			}

			// Import vertex header
			program_common::define_glsl_switches(OS, enabled_options);

			OS <<
				#include "GLSLSnippets/RSXProg/RSXVertexPrologue.glsl"
			;

			return;
		}

		if (props.emulate_coverage_tests)
		{
			enabled_options.push_back("_EMULATE_COVERAGE_TEST");
		}

		if (!props.fp32_outputs || props.require_linear_to_srgb)
		{
			enabled_options.push_back("_ENABLE_LINEAR_TO_SRGB");
		}

		if (props.require_texture_ops || props.require_srgb_to_linear)
		{
			enabled_options.push_back("_ENABLE_SRGB_TO_LINEAR");
		}

		if (props.require_wpos)
		{
			enabled_options.push_back("_ENABLE_WPOS");
		}

		if (props.require_fog_read)
		{
			program_common::define_glsl_constants<rsx::fog_mode>(OS,
			{
				{ "FOG_LINEAR    ", rsx::fog_mode::linear },
				{ "FOG_EXP       ", rsx::fog_mode::exponential },
				{ "FOG_EXP2      ", rsx::fog_mode::exponential2 },
				{ "FOG_LINEAR_ABS", rsx::fog_mode::linear_abs },
				{ "FOG_EXP_ABS   ", rsx::fog_mode::exponential_abs },
				{ "FOG_EXP2_ABS  ", rsx::fog_mode::exponential2_abs },
			});

			enabled_options.push_back("_ENABLE_FOG_READ");
		}

		// Import fragment header
		program_common::define_glsl_switches(OS, enabled_options);
		enabled_options.clear();

		OS <<
			#include "GLSLSnippets/RSXProg/RSXFragmentPrologue.glsl"
		;

		if (props.require_texture_ops)
		{
			// Declare special texture control flags
			program_common::define_glsl_constants<rsx::texture_control_bits>(OS,
			{
				{ "GAMMA_R_BIT " , rsx::texture_control_bits::GAMMA_R },
				{ "GAMMA_G_BIT " , rsx::texture_control_bits::GAMMA_G },
				{ "GAMMA_B_BIT " , rsx::texture_control_bits::GAMMA_B },
				{ "GAMMA_A_BIT " , rsx::texture_control_bits::GAMMA_A },
				{ "EXPAND_R_BIT" , rsx::texture_control_bits::EXPAND_R },
				{ "EXPAND_G_BIT" , rsx::texture_control_bits::EXPAND_G },
				{ "EXPAND_B_BIT" , rsx::texture_control_bits::EXPAND_B },
				{ "EXPAND_A_BIT" , rsx::texture_control_bits::EXPAND_A },
				{ "SEXT_R_BIT" , rsx::texture_control_bits::SEXT_R },
				{ "SEXT_G_BIT" , rsx::texture_control_bits::SEXT_G },
				{ "SEXT_B_BIT" , rsx::texture_control_bits::SEXT_B },
				{ "SEXT_A_BIT" , rsx::texture_control_bits::SEXT_A },
				{ "WRAP_S_BIT", rsx::texture_control_bits::WRAP_S },
				{ "WRAP_T_BIT", rsx::texture_control_bits::WRAP_T },
				{ "WRAP_R_BIT", rsx::texture_control_bits::WRAP_R },

				{ "ALPHAKILL    ", rsx::texture_control_bits::ALPHAKILL },
				{ "RENORMALIZE  ", rsx::texture_control_bits::RENORMALIZE },
				{ "DEPTH_FLOAT  ", rsx::texture_control_bits::DEPTH_FLOAT },
				{ "DEPTH_COMPARE", rsx::texture_control_bits::DEPTH_COMPARE_OP },
				{ "FILTERED_MAG_BIT", rsx::texture_control_bits::FILTERED_MAG },
				{ "FILTERED_MIN_BIT", rsx::texture_control_bits::FILTERED_MIN },
				{ "INT_COORDS_BIT  ", rsx::texture_control_bits::UNNORMALIZED_COORDS },
				{ "CLAMP_COORDS_BIT", rsx::texture_control_bits::CLAMP_TEXCOORDS_BIT }
			});

			if (props.require_texture_expand)
			{
				enabled_options.push_back("_ENABLE_TEXTURE_EXPAND");
			}

			if (props.emulate_shadow_compare)
			{
				enabled_options.push_back("_EMULATED_TEXSHADOW");
			}

			if (props.require_tex_shadow_ops)
			{
				enabled_options.push_back("_ENABLE_SHADOW");
			}

			if (props.require_tex1D_ops)
			{
				enabled_options.push_back("_ENABLE_TEX1D");
			}

			if (props.require_tex2D_ops)
			{
				enabled_options.push_back("_ENABLE_TEX2D");
			}

			if (props.require_tex3D_ops)
			{
				enabled_options.push_back("_ENABLE_TEX3D");
			}

			if (props.require_shadowProj_ops)
			{
				enabled_options.push_back("_ENABLE_SHADOWPROJ");
			}

			program_common::define_glsl_switches(OS, enabled_options);
			enabled_options.clear();

			OS <<
				#include "GLSLSnippets/RSXProg/RSXFragmentTextureOps.glsl"
			;

			if (props.require_depth_conversion)
			{
				OS <<
					#include "GLSLSnippets/RSXProg/RSXFragmentTextureDepthConversion.glsl"
				;
			}

			if (props.require_msaa_ops)
			{
				OS <<
					#include "GLSLSnippets/RSXProg/RSXFragmentTextureMSAAOps.glsl"
				;

				// Generate multiple versions of the actual sampler code.
				// We could use defines to generate these, but I don't trust some OpenGL compilers to do the right thing.
				const std::string_view msaa_sampling_impl =
					#include "GLSLSnippets/RSXProg/RSXFragmentTextureMSAAOpsInternal.glsl"
				;

				OS << fmt::replace_all(msaa_sampling_impl, "_MSAA_SAMPLER_TYPE_", "sampler2DMS");
				if (props.require_depth_conversion)
				{
					OS << fmt::replace_all(msaa_sampling_impl, "_MSAA_SAMPLER_TYPE_", "usampler2DMS");
				}
			}
		}
	}

	std::string getFunctionImpl(FUNCTION f)
	{
		switch (f)
		{
		default:
			abort();
		case FUNCTION::DP2:
			return "$Ty(dot($0.xy, $1.xy))";
		case FUNCTION::DP2A:
			return "$Ty(dot($0.xy, $1.xy) + $2.x)";
		case FUNCTION::DP3:
			return "$Ty(dot($0.xyz, $1.xyz))";
		case FUNCTION::DP4:
			return "$Ty(dot($0, $1))";
		case FUNCTION::DPH:
			return "$Ty(dot(vec4($0.xyz, 1.0), $1))";
		case FUNCTION::SFL:
			return "$Ty(0.)";
		case FUNCTION::STR:
			return "$Ty(1.)";
		case FUNCTION::FRACT:
			return "fract($0)";
		case FUNCTION::REFL:
			return "reflect($0, $1)";
		case FUNCTION::TEXTURE_SAMPLE1D:
			return "TEX1D($_i, $0.x)";
		case FUNCTION::TEXTURE_SAMPLE1D_BIAS:
			return "TEX1D_BIAS($_i, $0.x, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE1D_PROJ:
			return "TEX1D_PROJ($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE1D_LOD:
			return "TEX1D_LOD($_i, $0.x, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE1D_GRAD:
			return "TEX1D_GRAD($_i, $0.x, $1.x, $2.x)";
		case FUNCTION::TEXTURE_SAMPLE1D_SHADOW:
		case FUNCTION::TEXTURE_SAMPLE1D_SHADOW_PROJ:
			// Unimplemented
			break;
		case FUNCTION::TEXTURE_SAMPLE1D_DEPTH_RGBA:
			return "TEX1D_Z24X8_RGBA8($_i, $0.x)";
		case FUNCTION::TEXTURE_SAMPLE1D_DEPTH_RGBA_PROJ:
			return "TEX1D_Z24X8_RGBA8($_i, ($0.x / $0.w))";
		case FUNCTION::TEXTURE_SAMPLE2D:
			return "TEX2D($_i, $0.xy)";
		case FUNCTION::TEXTURE_SAMPLE2D_BIAS:
			return "TEX2D_BIAS($_i, $0.xy, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE2D_PROJ:
			return "TEX2D_PROJ($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE2D_LOD:
			return "TEX2D_LOD($_i, $0.xy, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE2D_GRAD:
			return "TEX2D_GRAD($_i, $0.xy, $1.xy, $2.xy)";
		case FUNCTION::TEXTURE_SAMPLE2D_SHADOW:
			return "TEX2D_SHADOW($_i, $0.xyz)";
		case FUNCTION::TEXTURE_SAMPLE2D_SHADOW_PROJ:
			return "TEX2D_SHADOWPROJ($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE2D_DEPTH_RGBA:
			return "TEX2D_Z24X8_RGBA8($_i, $0.xy)";
		case FUNCTION::TEXTURE_SAMPLE2D_DEPTH_RGBA_PROJ:
			return "TEX2D_Z24X8_RGBA8($_i, ($0.xy / $0.w))";
		case FUNCTION::TEXTURE_SAMPLE3D:
			return "TEX3D($_i, $0.xyz)";
		case FUNCTION::TEXTURE_SAMPLE3D_BIAS:
			return "TEX3D_BIAS($_i, $0.xyz, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE3D_PROJ:
			return "TEX3D_PROJ($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE3D_LOD:
			return "TEX3D_LOD($_i, $0.xyz, $1.x)";
		case FUNCTION::TEXTURE_SAMPLE3D_GRAD:
			return "TEX3D_GRAD($_i, $0.xyz, $1.xyz, $2.xyz)";
		case FUNCTION::TEXTURE_SAMPLE3D_SHADOW:
			return "TEX3D_SHADOW($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE3D_SHADOW_PROJ:
			// Impossible
			break;
		case FUNCTION::TEXTURE_SAMPLE3D_DEPTH_RGBA:
			return "TEX3D_Z24X8_RGBA8($_i, $0.xyz)";
		case FUNCTION::TEXTURE_SAMPLE3D_DEPTH_RGBA_PROJ:
			return "TEX3D_Z24X8_RGBA8($_i, ($0.xyz / $0.w))";
		case FUNCTION::TEXTURE_SAMPLE2DMS:
		case FUNCTION::TEXTURE_SAMPLE2DMS_BIAS:
			return "TEX2D_MS($_i, $0.xy)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_PROJ:
			return "TEX2D_MS($_i, $0.xy / $0.w)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_LOD:
		case FUNCTION::TEXTURE_SAMPLE2DMS_GRAD:
			return "TEX2D_MS($_i, $0.xy)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_SHADOW:
			return "TEX2D_SHADOW_MS($_i, $0.xyz)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_SHADOW_PROJ:
			return "TEX2D_SHADOWPROJ_MS($_i, $0)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_DEPTH_RGBA:
			return "TEX2D_Z24X8_RGBA8_MS($_i, $0.xy)";
		case FUNCTION::TEXTURE_SAMPLE2DMS_DEPTH_RGBA_PROJ:
			return "TEX2D_Z24X8_RGBA8_MS($_i, ($0.xy / $0.w))";
		case FUNCTION::DFDX:
			return "dFdx($0)";
		case FUNCTION::DFDY:
			return "dFdy($0)";
		case FUNCTION::VERTEX_TEXTURE_FETCH1D:
			return "textureLod($t, $0.x, 0)";
		case FUNCTION::VERTEX_TEXTURE_FETCH2D:
			return "textureLod($t, $0.xy, 0)";
		case FUNCTION::VERTEX_TEXTURE_FETCH3D:
		case FUNCTION::VERTEX_TEXTURE_FETCHCUBE:
			return "textureLod($t, $0.xyz, 0)";
		case FUNCTION::VERTEX_TEXTURE_FETCH2DMS:
			return "texelFetch($t, ivec2($0.xy * textureSize($t)), 0)";
		}

		rsx_log.error("Unexpected function request: %d", static_cast<int>(f));
		return "$Ty(0.)";
	}

	void insert_subheader_block(std::ostream& OS)
	{
		// Global types and stuff
		// Must be compatible with std140 packing rules
		OS <<
			#include "GLSLSnippets/RSXProg/RSXDefines2.glsl"
		;
	}

	void insert_fragment_shader_inputs_block(
		std::stringstream& OS,
		const std::string_view ext_flavour,
		const RSXFragmentProgram& prog,
		const std::vector<ParamType>& params,
		const two_sided_lighting_config& _2sided_lighting,
		std::function<int(std::string_view)> varying_location)
	{
		struct _varying_register_config
		{
			int location;
			std::string name;
			std::string type;
		};

		std::vector<_varying_register_config> varying_list;

		for (const ParamType& PT : params)
		{
			for (const ParamItem& PI : PT.items)
			{
				// ssa is defined in the program body and is not a varying type
				if (PI.name == "ssa") continue;

				const auto reg_location = varying_location(PI.name);
				std::string var_name = PI.name;

				if (var_name == "fogc")
				{
					var_name = "fog_c";
				}
				else if (prog.two_sided_lighting)
				{
					if (var_name == "diff_color")
					{
						var_name = "diff_color0";
					}
					else if (var_name == "spec_color")
					{
						var_name = "spec_color0";
					}
				}

				varying_list.push_back({ reg_location, var_name, PT.type });
			}
		}

		if (prog.two_sided_lighting)
		{
			if (_2sided_lighting.two_sided_color)
			{
				varying_list.push_back({ varying_location("diff_color1"), "diff_color1", "vec4" });
			}

			if (_2sided_lighting.two_sided_specular)
			{
				varying_list.push_back({ varying_location("spec_color1"), "spec_color1", "vec4" });
			}
		}

		if (varying_list.empty())
		{
			return;
		}

		// Make the output a little nicer
		std::sort(varying_list.begin(), varying_list.end(), FN(x.location < y.location));

		if (!(prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION))
		{
			for (const auto& reg : varying_list)
			{
				OS << "layout(location=" << reg.location << ") in " << reg.type << " " << reg.name << ";\n";
			}

			OS << "\n";
			return;
		}

		for (const auto& reg : varying_list)
		{
			OS << "layout(location=" << reg.location << ") pervertex" << ext_flavour << " in " << reg.type << " " << reg.name << "_raw[3];\n";
		}

		// Interpolate the input attributes manually.
		// Matches AMD behavior where gl_BaryCoordSmoothAMD only provides x and y with z being autogenerated.
		std::string interpolate_function_block =
			"\n"
			"vec4 _interpolate_varying3(const in vec4[3] v)\n"
			"{\n"
			// In the corner case where v[0] == v[1] == v[2], this algorithm generates a perfect result vs alternatives that use weighted multiply + add.
			// Due to the finite precision of floating point arithmetic, adding together the result of different multiplies yeields a slightly inaccurate result which breaks things.
			"	const vec4 p10 = v[1] - v[0];\n"
			"	const vec4 p20 = v[2] - v[0];\n"
			"	return v[0] + p10 * $gl_BaryCoord.y + p20 * $gl_BaryCoord.z;\n"
			"}\n\n";
		OS << fmt::replace_all(interpolate_function_block, {{ "$gl_BaryCoord", "gl_BaryCoord"s + std::string(ext_flavour) }});

		for (const auto& reg : varying_list)
		{
			OS << "vec4 " << reg.name << " = _interpolate_varying3(" << reg.name << "_raw);\n";
		}

		OS << "\n";
	}
}
