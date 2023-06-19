#include "stdafx.h"

#include "GLSLCommon.h"
#include "RSXFragmentProgram.h"

#include "Emu/system_config.h"
#include "Emu/RSX/gcm_enums.h"
#include "Utilities/StrFmt.h"

namespace program_common
{
	template <typename T>
	void define_glsl_constants(std::ostream& OS, std::initializer_list<std::pair<const char*, T>> enums)
	{
		for (const auto& e : enums)
		{
			OS << "#define " << e.first << " " << static_cast<int>(e.second) << "\n";
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
		std::string vertex_id_name = (rules != glsl_rules_spirv) ? "gl_VertexID" : "gl_VertexIndex";

		// Actually decode a vertex attribute from a raw byte stream
		program_common::define_glsl_constants<int>(OS,
		{
			{ "VTX_FMT_SNORM16", RSX_VERTEX_BASE_TYPE_SNORM16 },
			{ "VTX_FMT_FLOAT32", RSX_VERTEX_BASE_TYPE_FLOAT },
			{ "VTX_FMT_FLOAT16", RSX_VERTEX_BASE_TYPE_HALF_FLOAT },
			{ "VTX_FMT_UNORM8", RSX_VERTEX_BASE_TYPE_UNORM8 },
			{ "VTX_FMT_SINT16", RSX_VERTEX_BASE_TYPE_SINT16 },
			{ "VTX_FMT_COMP32", RSX_VERTEX_BASE_TYPE_CMP32 },
			{ "VTX_FMT_UINT8", RSX_VERTEX_BASE_TYPE_UINT8 }
		});

		// For intel GPUs which cannot access vectors in indexed mode (driver bug? or glsl version too low?)
		// Note: Tested on Mesa iris with HD 530 and compilant path works fine, may be a bug on Windows proprietary drivers
		if (!glsl4_compliant)
		{
			OS <<
			"void mov(inout uvec4 vector, const in int index, const in uint scalar)\n"
			"{\n"
			"	switch(index)\n"
			"	{\n"
			"		case 0: vector.x = scalar; return;\n"
			"		case 1: vector.y = scalar; return;\n"
			"		case 2: vector.z = scalar; return;\n"
			"		case 3: vector.w = scalar; return;\n"
			"	}\n"
			"}\n\n"

			"uint ref(const in uvec4 vector, const in int index)\n"
			"{\n"
			"	switch(index)\n"
			"	{\n"
			"		case 0: return vector.x;\n"
			"		case 1: return vector.y;\n"
			"		case 2: return vector.z;\n"
			"		case 3: return vector.w;\n"
			"	}\n"
			"}\n\n";
		}
		else
		{
			OS <<
			"#define mov(v, i, s) v[i] = s\n"
			"#define ref(v, i) v[i]\n\n";
		}

		OS <<
		"struct attribute_desc\n"
		"{\n"
		"	uint type;\n"
		"	uint attribute_size;\n"
		"	uint starting_offset;\n"
		"	uint stride;\n"
		"	uint frequency;\n"
		"	bool swap_bytes;\n"
		"	bool is_volatile;\n"
		"	bool modulo;\n"
		"};\n\n"

		"uint gen_bits(const in uint x, const in uint y, const in uint z, const in uint w, const in bool swap)\n"
		"{\n"
		"	return (swap) ?\n"
		"		_set_bits(_set_bits(_set_bits(w, z, 8, 8), y, 16, 8), x, 24, 8) :\n"
		"		_set_bits(_set_bits(_set_bits(x, y, 8, 8), z, 16, 8), w, 24, 8);\n"
		"}\n\n"

		"uint gen_bits(const in uint x, const in uint y, const in bool swap)\n"
		"{\n"
		"	return (swap)? _set_bits(y, x, 8, 8) : _set_bits(x, y, 8, 8);\n"
		"}\n\n"

		// NOTE: (int(n) or int(n)) is broken on some NVIDIA and INTEL hardware when the sign bit is involved.
		// See https://github.com/RPCS3/rpcs3/issues/8990
		"vec4 sext(const in ivec4 bits)\n"
		"{\n"
		"	// convert raw 16 bit values into signed 32-bit float4 counterpart\n"
		"	bvec4 sign_check = lessThan(bits, ivec4(0x8000));\n"
		"	return _select(bits - 65536, bits, sign_check);\n"
		"}\n\n"

		"float sext(const in int bits)\n"
		"{\n"
		"	return (bits < 0x8000) ? float(bits) : float(bits - 65536); \n"
		"}\n\n"

		"vec4 fetch_attribute(const in attribute_desc desc, const in int vertex_id, usamplerBuffer input_stream)\n"
		"{\n"
		"	const int elem_size_table[] = { 0, 2, 4, 2, 1, 2, 4, 1 };\n"
		"	const float scaling_table[] = { 1., 32767.5, 1., 1., 255., 1., 32767., 1. };\n"
		"	const int elem_size = elem_size_table[desc.type];\n"
		"	const vec4 scale = scaling_table[desc.type].xxxx;\n\n"

		"	uvec4 tmp, result = uvec4(0u);\n"
		"	vec4 ret;\n"
		"	int n, i = int((vertex_id * desc.stride) + desc.starting_offset);\n\n"

		"	for (n = 0; n < desc.attribute_size; n++)\n"
		"	{\n"
		"		tmp.x = texelFetch(input_stream, i++).x;\n"
		"		if (elem_size == 2)\n"
		"		{\n"
		"			tmp.y = texelFetch(input_stream, i++).x;\n"
		"			tmp.x = gen_bits(tmp.x, tmp.y, desc.swap_bytes);\n"
		"		}\n"
		"		else if (elem_size == 4)\n"
		"		{\n"
		"			tmp.y = texelFetch(input_stream, i++).x;\n"
		"			tmp.z = texelFetch(input_stream, i++).x;\n"
		"			tmp.w = texelFetch(input_stream, i++).x;\n"
		"			tmp.x = gen_bits(tmp.x, tmp.y, tmp.z, tmp.w, desc.swap_bytes);\n"
		"		}\n\n"

		"		mov(result, n, tmp.x);\n"
		"	}\n\n"

		"	// Actual decoding step is done in vector space, outside the loop\n"
		"	if (desc.type == VTX_FMT_SNORM16 || desc.type == VTX_FMT_SINT16)\n"
		"	{\n"
		"		ret = sext(ivec4(result));\n"
		"		ret = fma(vec4(0.5), vec4(desc.type == VTX_FMT_SNORM16), ret);\n"
		"	}\n"
		"	else if (desc.type == VTX_FMT_FLOAT32)\n"
		"	{\n"
		"		ret = uintBitsToFloat(result);\n"
		"	}\n"
		"	else if (desc.type == VTX_FMT_FLOAT16)\n"
		"	{\n"
		"		tmp.x = _set_bits(result.x, result.y, 16, 16);\n"
		"		tmp.y = _set_bits(result.z, result.w, 16, 16);\n"
		"		ret.xy = unpackHalf2x16(tmp.x);\n"
		"		ret.zw = unpackHalf2x16(tmp.y);\n"
		"	}\n"
		"	else if (elem_size == 1) //(desc.type == VTX_FMT_UINT8 || desc.type == VTX_FMT_UNORM8)\n"
		"	{\n"
		"		// Ignore bswap on single byte channels\n"
		"		ret = vec4(result);\n"
		"	}\n"
		"	else //if (desc.type == VTX_FMT_COMP32)\n"
		"	{\n"
		"		result = uvec4(_get_bits(result.x, 0, 11),\n"
		"			_get_bits(result.x, 11, 11),\n"
		"			_get_bits(result.x, 22, 10),\n"
		"			uint(scale.x));\n"
		"		ret = sext(ivec4(result) << ivec4(5, 5, 6, 0));\n"
		"	}\n\n"

		"	if (desc.attribute_size < 4)\n"
		"	{\n"
		"		ret.w = scale.x;\n"
		"	}\n\n"

		"	return ret / scale; \n"
		"}\n\n"

		"attribute_desc fetch_desc(const in int location)\n"
		"{\n"
		"	// Each descriptor is 64 bits wide\n"
		"	// [0-8] attribute stride\n"
		"	// [8-24] attribute divisor\n"
		"	// [24-27] attribute type\n"
		"	// [27-30] attribute size\n"
		"	// [30-31] reserved\n"
		"	// [32-60] starting offset\n"
		"	// [60-61] swap bytes flag\n"
		"	// [61-62] volatile flag\n"
		"	// [62-63] modulo enable flag\n\n";

		if (rules == glsl_rules_opengl4)
		{
			// Data is packed into a ubo
			OS <<
			"	int block = (location >> 1);\n"
			"	int sub_block = (location & 1) << 1;\n"
			"	uvec2 attrib = uvec2(\n"
			"		ref(input_attributes_blob[block], sub_block + 0),\n"
			"		ref(input_attributes_blob[block], sub_block + 1));\n\n";
		}
		else
		{
			// Fetch parameters streamed separately from draw parameters
			OS <<
			"	uvec2 attrib = texelFetch(vertex_layout_stream, location + int(layout_ptr_offset)).xy;\n\n";
		}

		OS <<
		"	attribute_desc result;\n"
		"	result.stride = _get_bits(attrib.x, 0, 8);\n"
		"	result.frequency = _get_bits(attrib.x, 8, 16);\n"
		"	result.type = _get_bits(attrib.x, 24, 3);\n"
		"	result.attribute_size = _get_bits(attrib.x, 27, 3);\n"
		"	result.starting_offset = _get_bits(attrib.y, 0, 29);\n"
		"	result.swap_bytes = _test_bit(attrib.y, 29);\n"
		"	result.is_volatile = _test_bit(attrib.y, 30);\n"
		"	result.modulo = _test_bit(attrib.y, 31);\n"
		"	return result;\n"
		"}\n\n"

		"vec4 read_location(const in int location)\n"
		"{\n"
		"	attribute_desc desc = fetch_desc(location);\n"
		"	int vertex_id = " << vertex_id_name << " - int(vertex_base_index);\n"
		"	if (desc.frequency == 0)\n"
		"	{\n"
		"		vertex_id = 0;\n"
		"	}\n"
		"	else if (desc.modulo)\n"
		"	{\n"
		"		//if a vertex modifier is active; vertex_base must be 0 and is ignored\n"
		"		vertex_id = (" << vertex_id_name << " + int(vertex_index_offset)) % int(desc.frequency);\n"
		"	}\n"
		"	else\n"
		"	{\n"
		"		vertex_id /= int(desc.frequency); \n"
		"	}\n\n"

		"	if (desc.is_volatile)\n"
		"		return fetch_attribute(desc, vertex_id, volatile_input_stream);\n"
		"	else\n"
		"		return fetch_attribute(desc, vertex_id, persistent_input_stream);\n"
		"}\n\n";
	}

	void insert_rop_init(std::ostream& OS)
	{
		OS <<
		"	if (_test_bit(rop_control, POLYGON_STIPPLE_ENABLE_BIT))\n"
		"	{\n"
		"		// Convert x,y to linear address\n"
		"		const uvec2 stipple_coord = uvec2(gl_FragCoord.xy) % uvec2(32, 32);\n"
		"		const uint address = stipple_coord.y * 32u + stipple_coord.x;\n"
		"		const uint bit_offset = (address & 31u);\n"
		"		const uint word_index = _get_bits(address, 7, 3);\n"
		"		const uint sub_index = _get_bits(address, 5, 2);\n\n"

		"		if (!_test_bit(stipple_pattern[word_index][sub_index], int(bit_offset)))\n"
		"		{\n"
		"			_kill();\n"
		"		}\n"
		"	}\n\n";
	}

	void insert_rop(std::ostream& OS, const shader_properties& props)
	{
		const std::string reg0 = props.fp32_outputs ? "r0" : "h0";
		const std::string reg1 = props.fp32_outputs ? "r2" : "h4";
		const std::string reg2 = props.fp32_outputs ? "r3" : "h6";
		const std::string reg3 = props.fp32_outputs ? "r4" : "h8";

		if (props.disable_early_discard)
		{
			OS <<
			"	if (_fragment_discard)\n"
			"	{\n"
			"		discard;\n"
			"	}\n\n";
		}

		// Pre-output stages
		if (!props.fp32_outputs)
		{
			// Tested using NPUB90375; some shaders (32-bit output only?) do not obey srgb flags
			const auto vtype = (props.fp32_outputs || !props.supports_native_fp16) ? "vec4" : "f16vec4";
			OS <<
			"	if (_test_bit(rop_control, SRGB_FRAMEBUFFER_BIT))\n"
			"	{\n"
			"		" << reg0 << " = " << vtype << "(linear_to_srgb(" << reg0 << ").rgb, " << reg0 << ".a);\n"
			"		" << reg1 << " = " << vtype << "(linear_to_srgb(" << reg1 << ").rgb, " << reg1 << ".a);\n"
			"		" << reg2 << " = " << vtype << "(linear_to_srgb(" << reg2 << ").rgb, " << reg2 << ".a);\n"
			"		" << reg3 << " = " << vtype << "(linear_to_srgb(" << reg3 << ").rgb, " << reg3 << ".a);\n"
			"	}\n\n";
		}

		// Output conversion
		if (props.ROP_output_rounding)
		{
			OS <<
			"	if (_test_bit(rop_control, INT_FRAMEBUFFER_BIT))\n"
			"	{\n"
			"		" << reg0 << " = round_to_8bit(" << reg0 << ");\n"
			"		" << reg1 << " = round_to_8bit(" << reg1 << ");\n"
			"		" << reg2 << " = round_to_8bit(" << reg2 << ");\n"
			"		" << reg3 << " = round_to_8bit(" << reg3 << ");\n"
			"	}\n\n";
		}

		// Post-output stages
		// TODO: Implement all ROP options like CSAA and ALPHA_TO_ONE here
		OS <<
		// Alpha Testing
		"	if (_test_bit(rop_control, ALPHA_TEST_ENABLE_BIT))\n"
		"	{\n"
		"		const uint alpha_func = _get_bits(rop_control, ALPHA_TEST_FUNC_OFFSET, ALPHA_TEST_FUNC_LENGTH);\n"
		"		if (!comparison_passes(" << reg0 << ".a, alpha_ref, alpha_func)) discard;\n"
		"	}\n\n";

		// ALPHA_TO_COVERAGE
		if (props.emulate_coverage_tests)
		{
			OS <<
			"	if (_test_bit(rop_control, ALPHA_TO_COVERAGE_ENABLE_BIT))\n"
			"	{\n"
			"		if (!_test_bit(rop_control, MSAA_WRITE_ENABLE_BIT) ||\n"
			"			!coverage_test_passes(" << reg0 << "))\n"
			"		{\n"
			"			discard;\n"
			"		}\n"
			"	}\n\n";
		}

		// Commit
		OS <<
		"	ocol0 = " << reg0 << ";\n"
		"	ocol1 = " << reg1 << ";\n"
		"	ocol2 = " << reg2 << ";\n"
		"	ocol3 = " << reg3 << ";\n\n";
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
				{ "ALPHA_TEST_ENABLE_BIT        ", rsx::ROP_control_bits::ALPHA_TEST_ENABLE_BIT },
				{ "SRGB_FRAMEBUFFER_BIT         ", rsx::ROP_control_bits::SRGB_FRAMEBUFFER_BIT },
				{ "ALPHA_TO_COVERAGE_ENABLE_BIT ", rsx::ROP_control_bits::ALPHA_TO_COVERAGE_ENABLE_BIT },
				{ "MSAA_WRITE_ENABLE_BIT        ", rsx::ROP_control_bits::MSAA_WRITE_ENABLE_BIT },
				{ "INT_FRAMEBUFFER_BIT          ", rsx::ROP_control_bits::INT_FRAMEBUFFER_BIT },
				{ "POLYGON_STIPPLE_ENABLE_BIT   ", rsx::ROP_control_bits::POLYGON_STIPPLE_ENABLE_BIT },
				{ "ALPHA_TEST_FUNC_OFFSET       ", rsx::ROP_control_bits::ALPHA_FUNC_OFFSET },
				{ "ALPHA_TEST_FUNC_LENGTH       ", rsx::ROP_control_bits::ALPHA_FUNC_NUM_BITS },
				{ "MSAA_SAMPLE_CTRL_OFFSET      ", rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_OFFSET },
				{ "MSAA_SAMPLE_CTRL_LENGTH      ", rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_NUM_BITS },
				{ "ROP_CMD_MASK                 ", rsx::ROP_control_bits::ROP_CMD_MASK }
			});

			if (props.fp32_outputs || !props.supports_native_fp16)
			{
				enabled_options.push_back("_32_BIT_OUTPUT");
			}

			if (props.disable_early_discard)
			{
				enabled_options.push_back("_DISABLE_EARLY_DISCARD");
			}
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
				{ "FOG_LINEAR", rsx::fog_mode::linear },
				{ "FOG_EXP", rsx::fog_mode::exponential },
				{ "FOG_EXP2", rsx::fog_mode::exponential2 },
				{ "FOG_LINEAR_ABS", rsx::fog_mode::linear_abs },
				{ "FOG_EXP_ABS", rsx::fog_mode::exponential_abs },
				{ "FOG_EXP2_ABS", rsx::fog_mode::exponential2_abs },
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
			OS << "#define GAMMA_R_MASK  (1 << " << rsx::texture_control_bits::GAMMA_R << ")\n";
			OS << "#define GAMMA_G_MASK  (1 << " << rsx::texture_control_bits::GAMMA_G << ")\n";
			OS << "#define GAMMA_B_MASK  (1 << " << rsx::texture_control_bits::GAMMA_B << ")\n";
			OS << "#define GAMMA_A_MASK  (1 << " << rsx::texture_control_bits::GAMMA_A << ")\n";
			OS << "#define EXPAND_R_MASK (1 << " << rsx::texture_control_bits::EXPAND_R << ")\n";
			OS << "#define EXPAND_G_MASK (1 << " << rsx::texture_control_bits::EXPAND_G << ")\n";
			OS << "#define EXPAND_B_MASK (1 << " << rsx::texture_control_bits::EXPAND_B << ")\n";
			OS << "#define EXPAND_A_MASK (1 << " << rsx::texture_control_bits::EXPAND_A << ")\n\n";

			OS << "#define ALPHAKILL     " << rsx::texture_control_bits::ALPHAKILL << "\n";
			OS << "#define RENORMALIZE   " << rsx::texture_control_bits::RENORMALIZE << "\n";
			OS << "#define DEPTH_FLOAT   " << rsx::texture_control_bits::DEPTH_FLOAT << "\n";
			OS << "#define DEPTH_COMPARE " << rsx::texture_control_bits::DEPTH_COMPARE_OP << "\n";
			OS << "#define FILTERED_MAG_BIT  " << rsx::texture_control_bits::FILTERED_MAG << "\n";
			OS << "#define FILTERED_MIN_BIT  " << rsx::texture_control_bits::FILTERED_MIN << "\n";
			OS << "#define INT_COORDS_BIT    " << rsx::texture_control_bits::UNNORMALIZED_COORDS << "\n";
			OS << "#define GAMMA_CTRL_MASK  (GAMMA_R_MASK|GAMMA_G_MASK|GAMMA_B_MASK|GAMMA_A_MASK)\n";
			OS << "#define SIGN_EXPAND_MASK (EXPAND_R_MASK|EXPAND_G_MASK|EXPAND_B_MASK|EXPAND_A_MASK)\n";
			OS << "#define FILTERED_MASK    (FILTERED_MAG_BIT|FILTERED_MIN_BIT)\n\n";

			if (props.require_texture_expand)
			{
				enabled_options.push_back("_ENABLE_TEXTURE_EXPAND");
			}

			if (props.emulate_shadow_compare)
			{
				enabled_options.push_back("_EMULATED_TEXSHADOW");
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
