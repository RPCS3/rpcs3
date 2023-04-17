#include "stdafx.h"
#include "Utilities/StrFmt.h"

#include "GLSLCommon.h"
#include "RSXFragmentProgram.h"

#include "../gcm_enums.h"

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

	void insert_compare_op(std::ostream& OS)
	{
		OS <<
		"bool comparison_passes(const in float a, const in float b, const in uint func)\n"
		"{\n"
		"	switch (func)\n"
		"	{\n"
		"		default:\n"
		"		case 0: return false; //never\n"
		"		case 1: return (CMP_FIXUP(a) < CMP_FIXUP(b)); //less\n"
		"		case 2: return (CMP_FIXUP(a) == CMP_FIXUP(b)); //equal\n"
		"		case 3: return (CMP_FIXUP(a) <= CMP_FIXUP(b)); //lequal\n"
		"		case 4: return (CMP_FIXUP(a) > CMP_FIXUP(b)); //greater\n"
		"		case 5: return (CMP_FIXUP(a) != CMP_FIXUP(b)); //nequal\n"
		"		case 6: return (CMP_FIXUP(a) >= CMP_FIXUP(b)); //gequal\n"
		"		case 7: return true; //always\n"
		"	}\n"
		"}\n\n";
	}

	void insert_compare_op_vector(std::ostream& OS)
	{
		OS <<
		"bvec4 comparison_passes(const in vec4 a, const in vec4 b, const in uint func)\n"
		"{\n"
		"	switch (func)\n"
		"	{\n"
		"		default:\n"
		"		case 0: return bvec4(false); //never\n"
		"		case 1: return lessThan(CMP_FIXUP(a), CMP_FIXUP(b)); //less\n"
		"		case 2: return equal(CMP_FIXUP(a), CMP_FIXUP(b)); //equal\n"
		"		case 3: return lessThanEqual(CMP_FIXUP(a), CMP_FIXUP(b)); //lequal\n"
		"		case 4: return greaterThan(CMP_FIXUP(a), CMP_FIXUP(b)); //greater\n"
		"		case 5: return notEqual(CMP_FIXUP(a), CMP_FIXUP(b)); //nequal\n"
		"		case 6: return greaterThanEqual(CMP_FIXUP(a), CMP_FIXUP(b)); //gequal\n"
		"		case 7: return bvec4(true); //always\n"
		"	}\n"
		"}\n\n";
	}

	void insert_fog_declaration(std::ostream& OS, std::string_view wide_vector_type, std::string_view input_coord)
	{
		define_glsl_constants<rsx::fog_mode>(OS,
		{
			{ "FOG_LINEAR", rsx::fog_mode::linear },
			{ "FOG_EXP", rsx::fog_mode::exponential },
			{ "FOG_EXP2", rsx::fog_mode::exponential2 },
			{ "FOG_LINEAR_ABS", rsx::fog_mode::linear_abs },
			{ "FOG_EXP_ABS", rsx::fog_mode::exponential_abs },
			{ "FOG_EXP2_ABS", rsx::fog_mode::exponential2_abs }
		});

		std::string template_body = "$T fetch_fog_value(const in uint mode)\n";

		template_body +=
		"{\n"
		"	$T result = $T($I.x, 0., 0., 0.);\n"
		"	switch(mode)\n"
		"	{\n"
		"	default:\n"
		"		return result;\n"
		"	case FOG_LINEAR:\n"
		"		//linear\n"
		"		result.y = fog_param1 * $I.x + (fog_param0 - 1.);\n"
		"		break;\n"
		"	case FOG_EXP:\n"
		"		//exponential\n"
		"		result.y = exp(11.084 * (fog_param1 * $I.x + fog_param0 - 1.5));\n"
		"		break;\n"
		"	case FOG_EXP2:\n"
		"		//exponential2\n"
		"		result.y = exp(-pow(4.709 * (fog_param1 * $I.x + fog_param0 - 1.5), 2.));\n"
		"		break;\n"
		"	case FOG_EXP_ABS:\n"
		"		//exponential_abs\n"
		"		result.y = exp(11.084 * (fog_param1 * abs($I.x) + fog_param0 - 1.5));\n"
		"		break;\n"
		"	case FOG_EXP2_ABS:\n"
		"		//exponential2_abs\n"
		"		result.y = exp(-pow(4.709 * (fog_param1 * abs($I.x) + fog_param0 - 1.5), 2.));\n"
		"		break;\n"
		" case FOG_LINEAR_ABS:\n"
		"		//linear_abs\n"
		"		result.y = fog_param1 * abs($I.x) + (fog_param0 - 1.);\n"
		"		break;\n"
		"	}\n"
		"\n"
		"	result.y = clamp(result.y, 0., 1.);\n"
		"	return result;\n"
		"}\n\n";

		std::pair<std::string_view, std::string> replacements[] =
		{
			std::make_pair("$T", std::string(wide_vector_type)),
			std::make_pair("$I", std::string(input_coord))
		};

		OS << fmt::replace_all(template_body, replacements);
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
		OS << "#define _select mix\n";
		OS << "#define _saturate(x) clamp(x, 0., 1.)\n";
		OS << "#define _get_bits(x, off, count) bitfieldExtract(x, off, count)\n";
		OS << "#define _set_bits(x, y, off, count) bitfieldInsert(x, y, off, count)\n";
		OS << "#define _test_bit(x, y) (_get_bits(x, y, 1) != 0)\n";
		OS << "#define _rand(seed) fract(sin(dot(seed.xy, vec2(12.9898f, 78.233f))) * 43758.5453f)\n\n";

		if (props.low_precision_tests)
		{
			OS << "#define CMP_FIXUP(a) (sign(a) * 16. + a)\n\n";
		}
		else
		{
			OS << "#define CMP_FIXUP(a) (a)\n\n";
		}

		if (props.domain == glsl::program_domain::glsl_fragment_program)
		{
			OS << "// ROP control\n";
			OS << "#define ALPHA_TEST_ENABLE_BIT        " << rsx::ROP_control_bits::ALPHA_TEST_ENABLE_BIT << "\n";
			OS << "#define SRGB_FRAMEBUFFER_BIT         " << rsx::ROP_control_bits::SRGB_FRAMEBUFFER_BIT << "\n";
			OS << "#define ALPHA_TO_COVERAGE_ENABLE_BIT " << rsx::ROP_control_bits::ALPHA_TO_COVERAGE_ENABLE_BIT << "\n";
			OS << "#define MSAA_WRITE_ENABLE_BIT        " << rsx::ROP_control_bits::MSAA_WRITE_ENABLE_BIT << "\n";
			OS << "#define INT_FRAMEBUFFER_BIT          " << rsx::ROP_control_bits::INT_FRAMEBUFFER_BIT << "\n";
			OS << "#define POLYGON_STIPPLE_ENABLE_BIT   " << rsx::ROP_control_bits::POLYGON_STIPPLE_ENABLE_BIT << "\n";
			OS << "#define ALPHA_TEST_FUNC_OFFSET       " << rsx::ROP_control_bits::ALPHA_FUNC_OFFSET << "\n";
			OS << "#define ALPHA_TEST_FUNC_LENGTH       " << rsx::ROP_control_bits::ALPHA_FUNC_NUM_BITS << "\n";
			OS << "#define MSAA_SAMPLE_CTRL_OFFSET      " << rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_OFFSET << "\n";
			OS << "#define MSAA_SAMPLE_CTRL_LENGTH      " << rsx::ROP_control_bits::MSAA_SAMPLE_CTRL_NUM_BITS << "\n";
			OS << "#define ROP_CMD_MASK                 " << rsx::ROP_control_bits::ROP_CMD_MASK << "\n\n";

			// 8-bit rounding/quantization
			{
				const auto _16bit_outputs = (!props.fp32_outputs && props.supports_native_fp16);
				const auto _255 = _16bit_outputs ? "f16vec4(255.)" : "vec4(255.)";
				const auto _1_over_2 = _16bit_outputs ? "f16vec4(0.5)" : "vec4(0.5)";
				OS << "#define round_to_8bit(v4) (floor(fma(v4, " << _255 << ", " << _1_over_2 << ")) / " << _255 << ")\n\n";
			}

			OS << "// Workaround for broken early discard in some drivers\n";
			if (props.disable_early_discard)
			{
				OS << "bool _fragment_discard = false;\n";
				OS << "#define _kill() _fragment_discard = true\n\n";
			}
			else
			{
				OS << "#define _kill() discard\n\n";
			}

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
			}
		}

		if (props.require_lit_emulation)
		{
			OS <<
			"vec4 lit_legacy(const in vec4 val)"
			"{\n"
			"	vec4 clamped_val = val;\n"
			"	clamped_val.x = max(val.x, 0.);\n"
			"	clamped_val.y = max(val.y, 0.);\n"
			"	vec4 result;\n"
			"	result.x = 1.;\n"
			"	result.w = 1.;\n"
			"	result.y = clamped_val.x;\n"
			"	result.z = clamped_val.x > 0. ? exp(clamped_val.w * log(max(clamped_val.y, 0.0000000001))) : 0.;\n"
			"	return result;\n"
			"}\n\n";
		}

		if (props.domain == glsl::program_domain::glsl_vertex_program)
		{
			if (props.require_explicit_invariance)
			{
				// PS3 has shader invariance, but we don't really care about most attributes outside ATTR0
				OS << "invariant gl_Position;\n\n";
			}

			if (props.emulate_zclip_transform)
			{
				if (props.emulate_depth_clip_only)
				{
					// Technically the depth value here is the 'final' depth that should be stored in the Z buffer.
					// Forward mapping eqn is d' = d * (f - n) + n, where d' is the stored Z value (this) and d is the normalized API value.
					OS <<
					"vec4 apply_zclip_xform(const in vec4 pos, const in float near_plane, const in float far_plane)\n"
					"{\n"
					"	if (pos.w != 0.0)\n"
					"	{\n"
					"		const float real_n = min(far_plane, near_plane);\n"
					"		const float real_f = max(far_plane, near_plane);\n"
					"		const double depth_range = double(real_f - real_n);\n"
					"		const double inv_range = (depth_range > 0.000001) ? (1.0 / (depth_range * pos.w)) : 0.0;\n"
					"		const double actual_d = (double(pos.z) - double(real_n * pos.w)) * inv_range;\n"
					"		const double nearest_d = floor(actual_d + 0.5);\n"
					"		const double epsilon = (inv_range * pos.w) / 16777215.;\n"     // Epsilon value is the minimum discernable change in Z that should affect the stored Z
					"		const double d = _select(actual_d, nearest_d, abs(actual_d - nearest_d) < epsilon);\n"
					"		return vec4(pos.xy, float(d * pos.w), pos.w);\n"
					"	}\n"
					"	else\n"
					"	{\n"
					"		return pos;\n" // Only values where Z=0 can ever pass this clip
					"	}\n"
					"}\n\n";
				}
				else
				{
					OS <<
					"vec4 apply_zclip_xform(const in vec4 pos, const in float near_plane, const in float far_plane)\n"
					"{\n"
					"	float d = float(pos.z / pos.w);\n"
					"	if (d < 0.f && d >= near_plane)\n"
					"	{\n"
					"		// Clamp\n"
					"		d = 0.f;\n"
					"	}\n"
					"	else if (d > 1.f && d <= far_plane)\n"
					"	{\n"
					"		// Compress Z and store towards highest end of the range\n"
					"		d = min(1., 0.99 + (0.01 * (pos.z - near_plane) / (far_plane - near_plane)));\n"
					"	}\n"
					"	else\n" // This catch-call also handles w=0 since d=inf
					"	{\n"
					"		return pos;\n"
					"	}\n"
					"\n"
					"	return vec4(pos.x, pos.y, d * pos.w, pos.w);\n"
					"}\n\n";
				}
			}

			return;
		}

		program_common::insert_compare_op(OS);

		if (props.emulate_coverage_tests)
		{
			// Purely stochastic
			OS <<
			"bool coverage_test_passes(const in vec4 _sample)\n"
			"{\n"
			"	float random  = _rand(gl_FragCoord);\n"
			"	return (_sample.a > random);\n"
			"}\n\n";
		}

		if (!props.fp32_outputs || props.require_linear_to_srgb)
		{
			OS <<
			"vec4 linear_to_srgb(const in vec4 cl)\n"
			"{\n"
			"	vec4 low = cl * 12.92;\n"
			"	vec4 high = 1.055 * pow(cl, vec4(1. / 2.4)) - 0.055;\n"
			"	bvec4 selection = lessThan(cl, vec4(0.0031308));\n"
			"	return clamp(mix(high, low, selection), 0., 1.);\n"
			"}\n\n";
		}

		if (props.require_texture_ops || props.require_srgb_to_linear)
		{
			OS <<
			"vec4 srgb_to_linear(const in vec4 cs)\n"
			"{\n"
			"	vec4 a = cs / 12.92;\n"
			"	vec4 b = pow((cs + 0.055) / 1.055, vec4(2.4));\n"
			"	return _select(a, b, greaterThan(cs, vec4(0.04045)));\n"
			"}\n\n";
		}

		if (props.require_depth_conversion)
		{
			ensure(props.require_texture_ops);

			//NOTE: Memory layout is fetched as byteswapped BGRA [GBAR] (GOW collection, DS2, DeS)
			//The A component (Z) is useless (should contain stencil8 or just 1)
			OS <<
			"vec4 decode_depth24(const in float depth_value, const in bool depth_float)\n"
			"{\n"
			"	uint value;\n"
			"	if (!depth_float)\n"
			"		value = uint(depth_value * 16777215.);\n"
			"	else\n"
			"		value = _get_bits(floatBitsToUint(depth_value), 7, 24);\n"
			"\n"
			"	uint b = _get_bits(value, 0, 8);\n"
			"	uint g = _get_bits(value, 8, 8);\n"
			"	uint r = _get_bits(value, 16, 8);\n"
			"	return vec4(float(g)/255., float(b)/255., 1., float(r)/255.);\n"
			"}\n\n"

			"vec4 remap_vector(const in vec4 color, const in uint remap)\n"
			"{\n"
			"	vec4 result;\n"
			"	if (_get_bits(remap, 0, 8) == 0xE4)\n"
			"	{\n"
			"		result = color;\n"
			"	}\n"
			"	else\n"
			"	{\n"
			"		uvec4 remap_channel = uvec4(remap) >> uvec4(2, 4, 6, 0);\n"
			"		remap_channel &= 3;\n"
			"		remap_channel = (remap_channel + 3) % 4; // Map A-R-G-B to R-G-B-A\n\n"

			"		// Generate remapped result\n"
			"		result.a = color[remap_channel.a];\n"
			"		result.r = color[remap_channel.r];\n"
			"		result.g = color[remap_channel.g];\n"
			"		result.b = color[remap_channel.b];\n"
			"	}\n\n"

			"	if (_get_bits(remap, 8, 8) == 0xAA)\n"
			"		return result;\n\n"

			"	uvec4 remap_select = uvec4(remap) >> uvec4(10, 12, 14, 8);\n"
			"	remap_select &= 3;\n"
			"	bvec4 choice = lessThan(remap_select, uvec4(2));\n"
			"	return _select(result, vec4(remap_select), choice);\n"
			"}\n\n"

			"vec4 convert_z24x8_to_rgba8(const in vec2 depth_stencil, const in uint remap, const in uint flags)\n"
			"{\n"
			"	vec4 result = decode_depth24(depth_stencil.x, _test_bit(flags, DEPTH_FLOAT));\n"
			"	result.z = depth_stencil.y / 255.;\n\n"

			"	if (remap == 0xAAE4)\n"
			" 		return result;\n\n"

			"	return remap_vector(result, remap);\n"
			"}\n\n";
		}

		if (props.require_texture_ops)
		{
			OS <<

			//TODO: Move all the texture read control operations here
			"vec4 process_texel(in vec4 rgba, const in uint control_bits)\n"
			"{\n"
			"	if (control_bits == 0)\n"
			"	{\n"
			"		return rgba;\n"
			"	}\n"
			"\n"
			"	if (_test_bit(control_bits, ALPHAKILL))\n"
			"	{\n"
			"		// Alphakill\n"
			"		if (rgba.a < 0.000001)\n"
			"		{\n"
			"			_kill();\n"
			"			return rgba;\n"
			"		}\n"
			"	}\n"
			"\n"
			"	if (_test_bit(control_bits, RENORMALIZE))\n"
			"	{\n"
			"		// Renormalize to 8-bit (PS3) accuracy\n"
			"		rgba = floor(rgba * 255.);\n"
			"		rgba /= 255.;\n"
			"	}\n"
			"\n"
			"	uvec4 mask;\n"
			"	vec4 convert;\n"
			"	uint op_mask = control_bits & uint(SIGN_EXPAND_MASK);\n"
			"\n"
			"	if (op_mask != 0)\n"
			"	{\n"
			"		// Expand to signed normalized\n"
			"		mask = uvec4(op_mask) & uvec4(EXPAND_R_MASK, EXPAND_G_MASK, EXPAND_B_MASK, EXPAND_A_MASK);\n"
			"		convert = (rgba * 2.f - 1.f);\n"
			"		rgba = _select(rgba, convert, notEqual(mask, uvec4(0)));\n"
			"	}\n"
			"\n"
			"	op_mask = control_bits & uint(GAMMA_CTRL_MASK);\n"
			"	if (op_mask != 0u)\n"
			"	{\n"
			"		// Gamma correction\n"
			"		mask = uvec4(op_mask) & uvec4(GAMMA_R_MASK, GAMMA_G_MASK, GAMMA_B_MASK, GAMMA_A_MASK);\n"
			"		convert = srgb_to_linear(rgba);\n"
			"		return _select(rgba, convert, notEqual(mask, uvec4(0)));\n"
			"	}\n"
			"\n"
			"	return rgba;\n"
			"}\n\n";

			if (props.require_texture_expand)
			{
				OS <<
				"uint _texture_flag_override = 0;\n"
				"#define _enable_texture_expand() _texture_flag_override = SIGN_EXPAND_MASK\n"
				"#define _disable_texture_expand() _texture_flag_override = 0\n"
				"#define TEX_FLAGS(index) (texture_parameters[index].flags | _texture_flag_override)\n";
			}
			else
			{
				OS <<
				"#define TEX_FLAGS(index) texture_parameters[index].flags\n";
			}

			OS <<
			"#define TEX_NAME(index) tex##index\n"
			"#define TEX_NAME_STENCIL(index) tex##index##_stencil\n\n"

			"#define COORD_SCALE1(index, coord1) ((coord1 + texture_parameters[index].scale_bias.w) * texture_parameters[index].scale_bias.x)\n"
			"#define COORD_SCALE2(index, coord2) ((coord2 + texture_parameters[index].scale_bias.w) * texture_parameters[index].scale_bias.xy)\n"
			"#define COORD_SCALE3(index, coord3) ((coord3 + texture_parameters[index].scale_bias.w) * texture_parameters[index].scale_bias.xyz)\n\n"

			"#define TEX1D(index, coord1) process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1)), TEX_FLAGS(index))\n"
			"#define TEX1D_BIAS(index, coord1, bias) process_texel(texture(TEX_NAME(index), COORD_SCALE1(index, coord1), bias), TEX_FLAGS(index))\n"
			"#define TEX1D_LOD(index, coord1, lod) process_texel(textureLod(TEX_NAME(index), COORD_SCALE1(index, coord1), lod), TEX_FLAGS(index))\n"
			"#define TEX1D_GRAD(index, coord1, dpdx, dpdy) process_texel(textureGrad(TEX_NAME(index), COORD_SCALE1(index, coord1), dpdx, dpdy), TEX_FLAGS(index))\n"
			"#define TEX1D_PROJ(index, coord4) process_texel(textureProj(TEX_NAME(index), vec2(COORD_SCALE1(index, coord4.x), coord4.w)), TEX_FLAGS(index))\n"

			"#define TEX2D(index, coord2) process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2)), TEX_FLAGS(index))\n"
			"#define TEX2D_BIAS(index, coord2, bias) process_texel(texture(TEX_NAME(index), COORD_SCALE2(index, coord2), bias), TEX_FLAGS(index))\n"
			"#define TEX2D_LOD(index, coord2, lod) process_texel(textureLod(TEX_NAME(index), COORD_SCALE2(index, coord2), lod), TEX_FLAGS(index))\n"
			"#define TEX2D_GRAD(index, coord2, dpdx, dpdy) process_texel(textureGrad(TEX_NAME(index), COORD_SCALE2(index, coord2), dpdx, dpdy), TEX_FLAGS(index))\n"
			"#define TEX2D_PROJ(index, coord4) process_texel(textureProj(TEX_NAME(index), vec4(COORD_SCALE2(index, coord4.xy), coord4.z, coord4.w)), TEX_FLAGS(index))\n\n";

			if (props.emulate_shadow_compare)
			{
				OS <<
				"#define SHADOW_COORD(index, coord3) vec3(COORD_SCALE2(index, coord3.xy), _test_bit(TEX_FLAGS(index), DEPTH_FLOAT)? coord3.z : min(float(coord3.z), 1.0))\n"
				"#define SHADOW_COORD4(index, coord4) vec4(SHADOW_COORD(index, coord4.xyz), coord4.w)\n"
				"#define SHADOW_COORD_PROJ(index, coord4) vec4(COORD_SCALE2(index, coord4.xy), _test_bit(TEX_FLAGS(index), DEPTH_FLOAT)? coord4.z : min(coord4.z, coord4.w), coord4.w)\n\n"

				"#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), SHADOW_COORD(index, coord3))\n"
				"#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), SHADOW_COORD4(index, coord4))\n"
				"#define TEX2D_SHADOWPROJ(index, coord4) textureProj(TEX_NAME(index), SHADOW_COORD_PROJ(index, coord4))\n";
			}
			else
			{
				OS <<
				"#define TEX2D_SHADOW(index, coord3) texture(TEX_NAME(index), vec3(COORD_SCALE2(index, coord3.xy), coord3.z))\n"
				"#define TEX3D_SHADOW(index, coord4) texture(TEX_NAME(index), vec4(COORD_SCALE3(index, coord4.xyz), coord4.w))\n"
				"#define TEX2D_SHADOWPROJ(index, coord4) textureProj(TEX_NAME(index), vec4(COORD_SCALE2(index, coord4.xy), coord4.zw))\n";
			}

			OS <<
			"#define TEX3D(index, coord3) process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3)), TEX_FLAGS(index))\n"
			"#define TEX3D_BIAS(index, coord3, bias) process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord3), bias), TEX_FLAGS(index))\n"
			"#define TEX3D_LOD(index, coord3, lod) process_texel(textureLod(TEX_NAME(index), COORD_SCALE3(index, coord3), lod), TEX_FLAGS(index))\n"
			"#define TEX3D_GRAD(index, coord3, dpdx, dpdy) process_texel(textureGrad(TEX_NAME(index), COORD_SCALE3(index, coord3), dpdx, dpdy), TEX_FLAGS(index))\n"
			"#define TEX3D_PROJ(index, coord4) process_texel(texture(TEX_NAME(index), COORD_SCALE3(index, coord4.xyz) / coord4.w), TEX_FLAGS(index))\n\n";

			if (props.require_depth_conversion)
			{
				OS <<
				"#define ZS_READ(index, coord) vec2(texture(TEX_NAME(index), coord).r, float(texture(TEX_NAME_STENCIL(index), coord).x))\n"
				"#define TEX1D_Z24X8_RGBA8(index, coord1) process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE1(index, coord1)), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))\n"
				"#define TEX2D_Z24X8_RGBA8(index, coord2) process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE2(index, coord2)), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))\n"
				"#define TEX3D_Z24X8_RGBA8(index, coord3) process_texel(convert_z24x8_to_rgba8(ZS_READ(index, COORD_SCALE3(index, coord3)), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))\n\n";
			}

			if (props.require_msaa_ops)
			{
				OS <<
				"#define ZCOMPARE_FUNC(index) _get_bits(TEX_FLAGS(index), DEPTH_COMPARE, 3)\n"
				"#define ZS_READ_MS(index, coord) vec2(sampleTexture2DMS(TEX_NAME(index), coord, index).r, float(sampleTexture2DMS(TEX_NAME_STENCIL(index), coord, index).x))\n"
				"#define TEX2D_MS(index, coord2) process_texel(sampleTexture2DMS(TEX_NAME(index), coord2, index), TEX_FLAGS(index))\n"
				"#define TEX2D_SHADOW_MS(index, coord3) vec4(comparison_passes(sampleTexture2DMS(TEX_NAME(index), coord3.xy, index).x, coord3.z, ZCOMPARE_FUNC(index)))\n"
				"#define TEX2D_SHADOWPROJ_MS(index, coord4) TEX2D_SHADOW_MS(index, (coord4.xyz / coord4.w))\n"
				"#define TEX2D_Z24X8_RGBA8_MS(index, coord2) process_texel(convert_z24x8_to_rgba8(ZS_READ_MS(index, coord2), texture_parameters[index].remap, TEX_FLAGS(index)), TEX_FLAGS(index))\n\n";

				OS <<
				"vec3 compute2x2DownsampleWeights(const in float coord, const in float uv_step, const in float actual_step)"
				"{\n"
				"	const float next_sample_point = coord + actual_step;\n"
				"	const float next_coord_step = fma(floor(coord / uv_step), uv_step, uv_step);\n"
				"	const float next_coord_step_plus_one = next_coord_step + uv_step;\n"
				"	vec3 weights = vec3(next_coord_step, min(next_coord_step_plus_one, next_sample_point), max(next_coord_step_plus_one, next_sample_point)) - vec3(coord, next_coord_step, next_coord_step_plus_one);\n"
				"	return weights / actual_step;\n"
				"}\n\n";

				auto insert_msaa_sample_code = [&OS](const std::string_view& sampler_type)
				{
					OS <<
					"vec4 texelFetch2DMS(in " << sampler_type << " tex, const in vec2 sample_count, const in ivec2 icoords, const in int index, const in ivec2 offset)\n"
					"{\n"
					"	const vec2 resolve_coords = vec2(icoords + offset);\n"
					"	const vec2 aa_coords = floor(resolve_coords / sample_count);\n"               // AA coords = real_coords / sample_count
					"	const vec2 sample_loc = fma(aa_coords, -sample_count, resolve_coords);\n"     // Sample ID = real_coords % sample_count
					"	const float sample_index = fma(sample_loc.y, sample_count.y, sample_loc.x);\n"
					"	return texelFetch(tex, ivec2(aa_coords), int(sample_index));\n"
					"}\n\n"

					"vec4 sampleTexture2DMS(in " << sampler_type << " tex, const in vec2 coords, const in int index)\n"
					"{\n"
					"	const uint flags = TEX_FLAGS(index);\n"
					"	const vec2 normalized_coords = COORD_SCALE2(index, coords);\n"
					"	const vec2 sample_count = vec2(2., textureSamples(tex) * 0.5);\n"
					"	const vec2 image_size = textureSize(tex) * sample_count;\n"
					"	const ivec2 icoords = ivec2(normalized_coords * image_size);\n"
					"	const vec4 sample0 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(0));\n"
					"\n"
					"	if (_get_bits(flags, FILTERED_MAG_BIT, 2) == 0)\n"
					"	{\n"
					"		return sample0;\n"
					"	}\n"
					"\n"
					"	// Bilinear scaling, with upto 2x2 downscaling with simple weights\n"
					"	const vec2 uv_step = 1.0 / vec2(image_size);\n"
					"	const vec2 actual_step = vec2(dFdx(normalized_coords.x), dFdy(normalized_coords.y));\n"
					"\n"
					"	const bvec2 no_filter = lessThan(abs(uv_step - actual_step), vec2(0.000001));\n"
					"	if (no_filter.x && no_filter.y)\n"
					"	{\n"
					"		return sample0;\n"
					"	}\n"
					"\n"
					"	vec4 a, b;\n"
					"	float factor;\n"
					"	const vec4 sample2 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(0, 1));     // Top left\n"
					"\n"
					"	if (no_filter.x)\n"
					"	{\n"
					"		// No scaling, 1:1\n"
					"		a = sample0;\n"
					"		b = sample2;\n"
					"	}\n"
					"	else\n"
					"	{\n"
					"		// Filter required, sample more data\n"
					"		const vec4 sample1 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(1, 0));     // Bottom right\n"
					"		const vec4 sample3 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(1, 1));     // Top right\n"
					"\n"
					"		if (actual_step.x > uv_step.x)\n"
					"		{\n"
					"		    // Downscale in X, centered\n"
					"		    const vec3 weights = compute2x2DownsampleWeights(normalized_coords.x, uv_step.x, actual_step.x);\n"
					"\n"
					"		    const vec4 sample4 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(2, 0));    // Further bottom right\n"
					"		    a = fma(sample0, weights.xxxx, sample1 * weights.y) + (sample4 * weights.z);  // Weighted sum\n"
					"\n"
					"		    if (!no_filter.y)\n"
					"		    {\n"
					"		        const vec4 sample5 = texelFetch2DMS(tex, sample_count, icoords, index, ivec2(2, 1));    // Further top right\n"
					"		        b = fma(sample2, weights.xxxx, sample3 * weights.y) + (sample5 * weights.z);  // Weighted sum\n"
					"		    }\n"
					"		}\n"
					"		else if (actual_step.x < uv_step.x)\n"
					"		{\n"
					"		    // Upscale in X\n"
					"		    factor = fract(normalized_coords.x * image_size.x);\n"
					"		    a = mix(sample0, sample1, factor);\n"
					"		    b = mix(sample2, sample3, factor);\n"
					"		}\n"
					"	}\n"
					"\n"
					"	if (no_filter.y)\n"
					"	{\n"
					"		// 1:1 no scale\n"
					"		return a;\n"
					"	}\n"
					"	else if (actual_step.y > uv_step.y)\n"
					"	{\n"
					"		// Downscale in Y\n"
					"		const vec3 weights = compute2x2DownsampleWeights(normalized_coords.y, uv_step.y, actual_step.y);\n"
					"		// We only have 2 rows computed for performance reasons, so combine rows 1 and 2\n"
					"		return a * weights.x + b * (weights.y + weights.z);\n"
					"	}\n"
					"	else if (actual_step.y < uv_step.y)\n"
					"	{\n"
					"		// Upscale in Y\n"
					"		factor = fract(normalized_coords.y * image_size.y);\n"
					"		return mix(a, b, factor);\n"
					"	}\n"
					"}\n\n";
				};

				insert_msaa_sample_code("sampler2DMS");
				if (props.require_depth_conversion)
				{
					insert_msaa_sample_code("usampler2DMS");
				}
			}
		}

		if (props.require_wpos)
		{
			OS <<
			"vec4 get_wpos()\n"
			"{\n"
			"	float abs_scale = abs(wpos_scale);\n"
			"	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);\n"
			"}\n\n";
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
		"struct sampler_info\n"
		"{\n"
		"	vec4 scale_bias;\n"
		"	uint remap;\n"
		"	uint flags;\n"
		"};\n\n";
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
		OS << fmt::replace_all(
			"\n"
			"vec4 _interpolate_varying3(const in vec4[3] v)\n"
			"{\n"
			"	const double _gl_BaryCoord_x = double($gl_BaryCoord.x);\n"
			"	const double _gl_BaryCoord_y = double($gl_BaryCoord.y);\n"
			"	const double _gl_BaryCoord_z = double(1.0) - (_gl_BaryCoord_x + _gl_BaryCoord_y);\n"
			"	return vec4(_gl_BaryCoord_x * v[0] + _gl_BaryCoord_y * v[1] + _gl_BaryCoord_z * v[2]);\n"
			"}\n\n",
			{
				{ "$gl_BaryCoord", "gl_BaryCoord"s + std::string(ext_flavour) }
			});

		for (const auto& reg : varying_list)
		{
			OS << "vec4 " << reg.name << " = _interpolate_varying3(" << reg.name << "_raw);\n";
		}

		OS << "\n";
	}
}
