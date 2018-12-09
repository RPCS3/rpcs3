#pragma once
#include <sstream>

#include "ShaderParam.h"

namespace program_common
{
	static void insert_compare_op(std::ostream& OS)
	{
		OS <<
		"bool comparison_passes(float a, float b, uint func)\n"
		"{\n"
		"	switch (func)\n"
		"	{\n"
		"		default:\n"
		"		case 0: return false; //never\n"
		"		case 1: return (a < b); //less\n"
		"		case 2: return (a == b); //equal\n"
		"		case 3: return (a <= b); //lequal\n"
		"		case 4: return (a > b); //greater\n"
		"		case 5: return (a != b); //nequal\n"
		"		case 6: return (a >= b); //gequal\n"
		"		case 7: return true; //always\n"
		"	}\n"
		"}\n\n";
	}

	static void insert_fog_declaration(std::ostream& OS, const std::string wide_vector_type, const std::string input_coord, bool declare = false)
	{
		std::string template_body;

		if (!declare)
			template_body += "$T fetch_fog_value(uint mode)\n";
		else
			template_body += "$T fetch_fog_value(uint mode, $T $I)\n";

		template_body +=
		"{\n"
		"	$T result = $T($I.x, 0., 0., 0.);\n"
		"	switch(mode)\n"
		"	{\n"
		"	default:\n"
		"		return result;\n"
		"	case 0:\n"
		"		//linear\n"
		"		result.y = fog_param1 * $I.x + (fog_param0 - 1.);\n"
		"		break;\n"
		"	case 1:\n"
		"		//exponential\n"
		"		result.y = exp(11.084 * (fog_param1 * $I.x + fog_param0 - 1.5));\n"
		"		break;\n"
		"	case 2:\n"
		"		//exponential2\n"
		"		result.y = exp(-pow(4.709 * (fog_param1 * $I.x + fog_param0 - 1.5), 2.));\n"
		"		break;\n"
		"	case 3:\n"
		"		//exponential_abs\n"
		"		result.y = exp(11.084 * (fog_param1 * abs($I.x) + fog_param0 - 1.5));\n"
		"		break;\n"
		"	case 4:\n"
		"		//exponential2_abs\n"
		"		result.y = exp(-pow(4.709 * (fog_param1 * abs($I.x) + fog_param0 - 1.5), 2.));\n"
		"		break;\n"
		" case 5:\n"
		"		//linear_abs\n"
		"		result.y = fog_param1 * abs($I.x) + (fog_param0 - 1.);\n"
		"		break;\n"
		"	}\n"
		"\n"
		"	result.y = clamp(result.y, 0., 1.);\n"
		"	return result;\n"
		"}\n\n";

		std::pair<std::string, std::string> replacements[] =
			{std::make_pair("$T", wide_vector_type),
			 std::make_pair("$I", input_coord)};

		OS << fmt::replace_all(template_body, replacements);
	}
}

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
		glsl_rules_rpirv
	};

	static std::string getFloatTypeNameImpl(size_t elementCount)
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

	static std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1, bool scalar = false)
	{
		if (scalar)
		{
			switch (f)
			{
			case COMPARE::FUNCTION_SEQ:
				return Op0 + " == " + Op1;
			case COMPARE::FUNCTION_SGE:
				return Op0 + " >= " + Op1;
			case COMPARE::FUNCTION_SGT:
				return Op0 + " > " + Op1;
			case COMPARE::FUNCTION_SLE:
				return Op0 + " <= " + Op1;
			case COMPARE::FUNCTION_SLT:
				return Op0 + " < " + Op1;
			case COMPARE::FUNCTION_SNE:
				return Op0 + " != " + Op1;
			}
		}
		else
		{
			switch (f)
			{
			case COMPARE::FUNCTION_SEQ:
				return "equal(" + Op0 + ", " + Op1 + ")";
			case COMPARE::FUNCTION_SGE:
				return "greaterThanEqual(" + Op0 + ", " + Op1 + ")";
			case COMPARE::FUNCTION_SGT:
				return "greaterThan(" + Op0 + ", " + Op1 + ")";
			case COMPARE::FUNCTION_SLE:
				return "lessThanEqual(" + Op0 + ", " + Op1 + ")";
			case COMPARE::FUNCTION_SLT:
				return "lessThan(" + Op0 + ", " + Op1 + ")";
			case COMPARE::FUNCTION_SNE:
				return "notEqual(" + Op0 + ", " + Op1 + ")";
			}
		}

		fmt::throw_exception("Unknown compare function" HERE);
	}

	static void insert_vertex_input_fetch(std::stringstream& OS, glsl_rules rules, bool glsl4_compliant=true)
	{
		std::string vertex_id_name = (rules == glsl_rules_opengl4) ? "gl_VertexID" : "gl_VertexIndex";

		//Actually decode a vertex attribute from a raw byte stream
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

		"uint get_bits(uvec4 v, bool swap)\n"
		"{\n"
		"	if (swap) return (v.w | v.z << 8 | v.y << 16 | v.x << 24);\n"
		"	return (v.x | v.y << 8 | v.z << 16 | v.w << 24);\n"
		"}\n\n"

		"uint get_bits(uvec2 v, bool swap)\n"
		"{\n"
		"	if (swap) return (v.y | v.x << 8);\n"
		"	return (v.x | v.y << 8);\n"
		"}\n\n"

		"int preserve_sign_s16(uint bits)\n"
		"{\n"
		"	//convert raw 16 bit value into signed 32-bit integer counterpart\n"
		"	uint sign = bits & 0x8000;\n"
		"	if (sign != 0) return int(bits | 0xFFFF0000);\n"
		"	return int(bits);\n"
		"}\n\n"

		"#define get_s16(v, s) preserve_sign_s16(get_bits(v, s))\n\n";

		//For intel GPUs which cannot access vectors in indexed mode (driver bug? or glsl version too low?)
		if (!glsl4_compliant)
		{
			OS <<
			"void mov(inout vec4 vector, in int index, in float scalar)\n"
			"{\n"
			"	switch(index)\n"
			"	{\n"
			"		case 0: vector.x = scalar; return;\n"
			"		case 1: vector.y = scalar; return;\n"
			"		case 2: vector.z = scalar; return;\n"
			"		case 3: vector.w = scalar; return;\n"
			"	}\n"
			"}\n";

			OS <<
			"uint ref(in uvec4 vector, in int index)\n"
			"{\n"
			"	switch(index)\n"
			"	{\n"
			"		case 0: return vector.x;\n"
			"		case 1: return vector.y;\n"
			"		case 2: return vector.z;\n"
			"		case 3: return vector.w;\n"
			"	}\n"
			"}\n";
		}
		else
		{
			OS <<
			"#define mov(v, i, s) v[i] = s\n"
			"#define ref(v, i) v[i]\n";
		}

		OS <<
		"vec4 fetch_attribute(attribute_desc desc, int vertex_id, usamplerBuffer input_stream)\n"
		"{\n"
		"	vec4 result = vec4(0., 0., 0., 1.);\n"
		"	vec4 scale = vec4(1.);\n"
		"	uvec4 tmp;\n"
		"	uint bits;\n"
		"	bool reverse_order = false;\n"
		"\n"
		"	int first_byte = int((vertex_id * desc.stride) + desc.starting_offset);\n"
		"	for (int n = 0; n < 4; n++)\n"
		"	{\n"
		"		if (n == desc.attribute_size) break;\n"
		"\n"
		"		switch (desc.type)\n"
		"		{\n"
		"		case 0:\n"
		"			//signed normalized 16-bit\n"
		"			tmp.x = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.y = texelFetch(input_stream, first_byte++).x;\n"
		"			mov(result, n, get_s16(tmp.xy, desc.swap_bytes));\n"
		"			mov(scale, n, 32767.);\n"
		"			break;\n"
		"		case 1:\n"
		"			//float\n"
		"			tmp.x = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.y = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.z = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.w = texelFetch(input_stream, first_byte++).x;\n"
		"			mov(result, n, uintBitsToFloat(get_bits(tmp, desc.swap_bytes)));\n"
		"			break;\n"
		"		case 2:\n"
		"			//half\n"
		"			tmp.x = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.y = texelFetch(input_stream, first_byte++).x;\n"
		"			mov(result, n, unpackHalf2x16(uint(get_bits(tmp.xy, desc.swap_bytes))).x);\n"
		"			break;\n"
		"		case 3:\n"
		"			//unsigned byte\n"
		"			mov(result, n, texelFetch(input_stream, first_byte++).x);\n"
		"			mov(scale, n, 255.);\n"
		"			reverse_order = desc.swap_bytes;\n"
		"			break;\n"
		"		case 4:\n"
		"			//signed word\n"
		"			tmp.x = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.y = texelFetch(input_stream, first_byte++).x;\n"
		"			mov(result, n, get_s16(tmp.xy, desc.swap_bytes));\n"
		"			break;\n"
		"		case 5:\n"
		"			//cmp\n"
		"			tmp.x = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.y = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.z = texelFetch(input_stream, first_byte++).x;\n"
		"			tmp.w = texelFetch(input_stream, first_byte++).x;\n"
		"			bits = get_bits(tmp, desc.swap_bytes);\n"
		"			result.x = preserve_sign_s16((bits & 0x7FF) << 5);\n"
		"			result.y = preserve_sign_s16(((bits >> 11) & 0x7FF) << 5);\n"
		"			result.z = preserve_sign_s16(((bits >> 22) & 0x3FF) << 6);\n"
		"			result.w = 1.;\n"
		"			scale = vec4(32767., 32767., 32767., 1.);\n"
		"			break;\n"
		"		case 6:\n"
		"			//ub256\n"
		"			mov(result, n, float(texelFetch(input_stream, first_byte++).x));\n"
		"			reverse_order = desc.swap_bytes;\n"
		"			break;\n"
		"		}\n"
		"	}\n\n"
		"	result /= scale;\n"
		"	return (reverse_order)? result.wzyx: result;\n"
		"}\n\n"

		"attribute_desc fetch_desc(int location)\n"
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
		"	// [62-63] modulo enable flag\n"
		"	int block = (location >> 1);\n"
		"	int sub_block = (location & 1) << 1;\n"
		"	uint attrib0 = ref(input_attributes_blob[block], sub_block + 0);\n"
		"	uint attrib1 = ref(input_attributes_blob[block], sub_block + 1);\n"
		"	attribute_desc result;\n"
		"	result.stride = attrib0 & 0xFF;\n"
		"	result.frequency = (attrib0 >> 8) & 0xFFFF;\n"
		"	result.type = (attrib0 >> 24) & 0x7;\n"
		"	result.attribute_size = (attrib0 >> 27) & 0x7;\n"
		"	result.starting_offset = (attrib1 & 0x1FFFFFFF);\n"
		"	result.swap_bytes = ((attrib1 >> 29) & 0x1) != 0;\n"
		"	result.is_volatile = ((attrib1 >> 30) & 0x1) != 0;\n"
		"	result.modulo = ((attrib1 >> 31) & 0x1) != 0;\n"
		"	return result;\n"
		"}\n\n"

		"vec4 read_location(int location)\n"
		"{\n"
		"	attribute_desc desc = fetch_desc(location);\n"
		"	if (desc.attribute_size == 0)\n"
		"	{\n"
		"		//default values\n"
		"		const vec4 defaults[] = \n"
		"		{	vec4(0., 0., 0., 1.), //position\n"
		"			vec4(0.), vec4(0.), //weight, normals\n"
		"			vec4(1.), //diffuse\n"
		"			vec4(0.), vec4(0.), //specular, fog\n"
		"			vec4(1.), //point size\n"
		"			vec4(0.), //in_7\n"
		"			//in_tc registers\n"
		"			vec4(0.), vec4(0.), vec4(0.), vec4(0.),\n"
		"			vec4(0.), vec4(0.), vec4(0.), vec4(0.)\n"
		"		};\n"
		"		return defaults[location];\n"
		"	}\n\n"
		"	int vertex_id = " << vertex_id_name << " - int(vertex_base_index);\n"
		"	if (desc.frequency == 0)\n"
		"	{\n"
		"		vertex_id = 0;\n"
		"	}\n"
		"	else if (desc.frequency > 1)\n"
		"	{\n"
		"		//if a vertex modifier is active; vertex_base must be 0 and is ignored\n"
		"		if (desc.modulo)\n"
		"		{\n"
		"			vertex_id = " << vertex_id_name << " % int(desc.frequency);\n"
		"		}\n"
		"		else\n"
		"		{\n"
		"			vertex_id = " << vertex_id_name << " / int(desc.frequency); \n"
		"		}\n"
		"	}\n"
		"\n"
		"	if (desc.is_volatile)\n"
		"		return fetch_attribute(desc, vertex_id, volatile_input_stream);\n"
		"	else\n"
		"		return fetch_attribute(desc, vertex_id, persistent_input_stream);\n"
		"}\n\n";
	}

	static void insert_rop(std::ostream& OS, bool _32_bit_exports)
	{
		const std::string reg0 = _32_bit_exports ? "r0" : "h0";
		const std::string reg1 = _32_bit_exports ? "r2" : "h4";
		const std::string reg2 = _32_bit_exports ? "r3" : "h6";
		const std::string reg3 = _32_bit_exports ? "r4" : "h8";

		//TODO: Implement all ROP options like CSAA and ALPHA_TO_ONE here
		OS <<
		"	if ((rop_control & 0xFF) != 0)\n"
		"	{\n"
		"		bool alpha_test = (rop_control & 0x11) > 0;\n"
		"		uint alpha_func = ((rop_control >> 16) & 0x7);\n"
		"		bool srgb_convert = (rop_control & 0x2) > 0;\n\n"
		"		if (alpha_test && !comparison_passes(" << reg0 << ".a, alpha_ref, alpha_func))\n"
		"		{\n"
		"			discard;\n"
		"		}\n";

		if (!_32_bit_exports)
		{
			//Tested using NPUB90375; some shaders (32-bit output only?) do not obey srgb flags
			OS <<
			"		else if (srgb_convert)\n"
			"		{\n"
			"			" << reg0 << ".rgb = linear_to_srgb(" << reg0 << ").rgb;\n"
			"			" << reg1 << ".rgb = linear_to_srgb(" << reg1 << ").rgb;\n"
			"			" << reg2 << ".rgb = linear_to_srgb(" << reg2 << ").rgb;\n"
			"			" << reg3 << ".rgb = linear_to_srgb(" << reg3 << ").rgb;\n"
			"		}\n";
		}

		OS <<
		"	}\n\n"

		"	ocol0 = " << reg0 << ";\n"
		"	ocol1 = " << reg1 << ";\n"
		"	ocol2 = " << reg2 << ";\n"
		"	ocol3 = " << reg3 << ";\n\n";
	}

	static void insert_glsl_legacy_function(std::ostream& OS, glsl::program_domain domain, bool require_lit_emulation, bool require_depth_conversion = false, bool require_wpos = false, bool require_texture_ops = true)
	{
		if (require_lit_emulation)
		{
			OS <<
			"vec4 lit_legacy(vec4 val)"
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

		if (domain == glsl::program_domain::glsl_vertex_program)
		{
			OS <<
			"vec4 apply_zclip_xform(vec4 pos, float near_plane, float far_plane)\n"
			"{\n"
			"	float d = pos.z / pos.w;\n"
			"	if (d < 0.f && d >= near_plane)\n"
			"		d = 0.f;\n" //force clamp negative values
			"	else if (d > 1.f && d <= far_plane)\n"
			"		d = min(1., 0.99 + (0.01 * (pos.z - near_plane) / (far_plane - near_plane)));\n"
			"	else\n"
			"		return pos; //d = (0.99 * d);\n" //range compression for normal values is disabled until a solution to ops comparing z is found
			"\n"
			"	pos.z = d * pos.w;\n"
			"	return pos;\n"
			"}\n\n";

			return;
		}

		program_common::insert_compare_op(OS);

		if (require_depth_conversion)
		{
			//NOTE: Memory layout is fetched as byteswapped BGRA [GBAR] (GOW collection, DS2, DeS)
			//The A component (Z) is useless (should contain stencil8 or just 1)
			OS <<
			"vec4 decodeLinearDepth(float depth_value)\n"
			"{\n"
			"	uint value = uint(depth_value * 16777215);\n"
			"	uint b = (value & 0xff);\n"
			"	uint g = (value >> 8) & 0xff;\n"
			"	uint r = (value >> 16) & 0xff;\n"
			"	return vec4(float(g)/255., float(b)/255., 1., float(r)/255.);\n"
			"}\n\n"

			"float read_value(vec4 src, uint remap_index)\n"
			"{\n"
			"	switch (remap_index)\n"
			"	{\n"
			"		case 0: return src.a;\n"
			"		case 1: return src.r;\n"
			"		case 2: return src.g;\n"
			"		case 3: return src.b;\n"
			"	}\n"
			"}\n\n"

			"vec4 texture2DReconstruct(sampler2D tex, vec2 coord, float remap)\n"
			"{\n"
			"	vec4 result = decodeLinearDepth(texture(tex, coord.xy).r);\n"
			"	uint remap_vector = floatBitsToUint(remap) & 0xFF;\n"
			"	if (remap_vector == 0xE4) return result;\n\n"
			"	vec4 tmp;\n"
			"	uint remap_a = remap_vector & 0x3;\n"
			"	uint remap_r = (remap_vector >> 2) & 0x3;\n"
			"	uint remap_g = (remap_vector >> 4) & 0x3;\n"
			"	uint remap_b = (remap_vector >> 6) & 0x3;\n"
			"	tmp.a = read_value(result, remap_a);\n"
			"	tmp.r = read_value(result, remap_r);\n"
			"	tmp.g = read_value(result, remap_g);\n"
			"	tmp.b = read_value(result, remap_b);\n"
			"	return tmp;\n"
			"}\n\n";
		}

		if (require_texture_ops)
		{
			OS <<
			"vec4 linear_to_srgb(vec4 cl)\n"
			"{\n"
			"	vec4 low = cl * 12.92;\n"
			"	vec4 high = 1.055 * pow(cl, vec4(1. / 2.4)) - 0.055;\n"
			"	bvec4 select = lessThan(cl, vec4(0.0031308));\n"
			"	return clamp(mix(high, low, select), 0., 1.);\n"
			"}\n\n"

			"float srgb_to_linear(float cs)\n"
			"{\n"
			"	if (cs <= 0.04045) return cs / 12.92;\n"
			"	return pow((cs + 0.055) / 1.055, 2.4);\n"
			"}\n\n"

#ifdef __APPLE__
			"vec4 remap_vector(vec4 rgba, uint remap_bits)\n"
			"{\n"
			"	uvec4 selector = (uvec4(remap_bits) >> uvec4(3, 6, 9, 0)) & 0x7;\n"
			"	bvec4 choice = greaterThan(selector, uvec4(1));\n"
			"\n"
			"	vec4 direct = vec4(selector);\n"
			"	selector = min(selector - 2, selector);\n"
			"	vec4 indexed = vec4(rgba[selector.r], rgba[selector.g], rgba[selector.b], rgba[selector.a]);\n"
			"	return mix(direct, indexed, choice);\n"
			"}\n\n"
#endif

			//TODO: Move all the texture read control operations here
			"vec4 process_texel(vec4 rgba, uint control_bits)\n"
			"{\n"
#ifdef __APPLE__
			"	uint remap_bits = (control_bits >> 16) & 0xFFFF;\n"
			"	if (remap_bits != 0x8D5) rgba = remap_vector(rgba, remap_bits);\n\n"
#endif
			"	if ((control_bits & 0xFFFF) == 0) return rgba;\n\n"
			"	if ((control_bits & 0x10) > 0)\n"
			"	{\n"
			"		//Alphakill\n"
			"		if (rgba.a < 0.0000000001)\n"
			"		{\n"
			"			discard;\n"
			"			return rgba;\n"
			"		}\n"
			"	}\n\n"
			"	//TODO: Verify gamma control bit ordering, looks to be 0x7 for rgb, 0xF for rgba\n"
			"	uint srgb_in = (control_bits & 0xF);\n"
			"	if ((srgb_in & 0x1) > 0) rgba.r = srgb_to_linear(rgba.r);\n"
			"	if ((srgb_in & 0x2) > 0) rgba.g = srgb_to_linear(rgba.g);\n"
			"	if ((srgb_in & 0x4) > 0) rgba.b = srgb_to_linear(rgba.b);\n"
			"	if ((srgb_in & 0x8) > 0) rgba.a = srgb_to_linear(rgba.a);\n"
			"	return rgba;\n"
			"}\n\n"

			"#define TEX1D(index, tex, coord1) process_texel(texture(tex, coord1 * texture_parameters[index].x), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX1D_BIAS(index, tex, coord1, bias) process_texel(texture(tex, coord1 * texture_parameters[index].x, bias), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX1D_LOD(index, tex, coord1, lod) process_texel(textureLod(tex, coord1 * texture_parameters[index].x, lod), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX1D_GRAD(index, tex, coord1, dpdx, dpdy) process_texel(textureGrad(tex, coord1 * texture_parameters[index].x, dpdx, dpdy), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX1D_PROJ(index, tex, coord2) process_texel(textureProj(tex, coord2 * vec2(texture_parameters[index].x, 1.)), floatBitsToUint(texture_parameters[index].w))\n"

			"#define TEX2D(index, tex, coord2) process_texel(texture(tex, coord2 * texture_parameters[index].xy), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX2D_BIAS(index, tex, coord2, bias) process_texel(texture(tex, coord2 * texture_parameters[index].xy, bias), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX2D_LOD(index, tex, coord2, lod) process_texel(textureLod(tex, coord2 * texture_parameters[index].xy, lod), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX2D_GRAD(index, tex, coord2, dpdx, dpdy) process_texel(textureGrad(tex, coord2 * texture_parameters[index].xy, dpdx, dpdy), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX2D_PROJ(index, tex, coord4) process_texel(textureProj(tex, coord4 * vec4(texture_parameters[index].xy, 1., 1.)), floatBitsToUint(texture_parameters[index].w))\n"

			"#define TEX2D_DEPTH_RGBA8(index, tex, coord2) process_texel(texture2DReconstruct(tex, coord2 * texture_parameters[index].xy, texture_parameters[index].z), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX2D_SHADOW(index, tex, coord3) texture(tex, coord3 * vec3(texture_parameters[index].xy, 1.))\n"
			"#define TEX2D_SHADOWPROJ(index, tex, coord4) textureProj(tex, coord4 * vec4(texture_parameters[index].xy, 1., 1.))\n"

			"#define TEX3D(index, tex, coord3) process_texel(texture(tex, coord3), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX3D_BIAS(index, tex, coord3, bias) process_texel(texture(tex, coord3, bias), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX3D_LOD(index, tex, coord3, lod) process_texel(textureLod(tex, coord3, lod), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX3D_GRAD(index, tex, coord3, dpdx, dpdy) process_texel(textureGrad(tex, coord3, dpdx, dpdy), floatBitsToUint(texture_parameters[index].w))\n"
			"#define TEX3D_PROJ(index, tex, coord4) process_texel(textureProj(tex, coord4), floatBitsToUint(texture_parameters[index].w))\n\n";
		}

		if (require_wpos)
		{
			OS <<
			"vec4 get_wpos()\n"
			"{\n"
			"	float abs_scale = abs(wpos_scale);\n"
			"	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);\n"
			"}\n\n";
		}
	}

	static void insert_fog_declaration(std::ostream& OS)
	{
		program_common::insert_fog_declaration(OS, "vec4", "fog_c");
	}

	static std::string getFunctionImpl(FUNCTION f)
	{
		switch (f)
		{
		default:
			abort();
		case FUNCTION::FUNCTION_DP2:
			return "vec4(dot($0.xy, $1.xy))";
		case FUNCTION::FUNCTION_DP2A:
			return "vec4(dot($0.xy, $1.xy) + $2.x)";
		case FUNCTION::FUNCTION_DP3:
			return "vec4(dot($0.xyz, $1.xyz))";
		case FUNCTION::FUNCTION_DP4:
			return "vec4(dot($0, $1))";
		case FUNCTION::FUNCTION_DPH:
			return "vec4(dot(vec4($0.xyz, 1.0), $1))";
		case FUNCTION::FUNCTION_SFL:
			return "vec4(0., 0., 0., 0.)";
		case FUNCTION::FUNCTION_STR:
			return "vec4(1., 1., 1., 1.)";
		case FUNCTION::FUNCTION_FRACT:
			return "fract($0)";
		case FUNCTION::FUNCTION_REFL:
			return "vec4($0 - 2.0 * (dot($0, $1)) * $1)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D:
			return "TEX1D($_i, $t, $0.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_BIAS:
			return "TEX1D_BIAS($_i, $t, $0.x, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ:
			return "TEX1D_PROJ($_i, $t, $0.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
			return "TEX1D_LOD($_i, $t, $0.x, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
			return "TEX1D_GRAD($_i, $t, $0.x, $1.x, $2.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
			return "TEX2D($_i, $t, $0.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_BIAS:
			return "TEX2D_BIAS($_i, $t, $0.xy, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
			return "TEX2D_PROJ($_i, $t, $0)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
			return "TEX2D_LOD($_i, $t, $0.xy, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
			return "TEX2D_GRAD($_i, $t, $0.xy, $1.xy, $2.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D:
			return "TEX2D_SHADOW($_i, $t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ:
			return "TEX2D_SHADOWPROJ($_i, $t, $0)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
			return "TEX3D($_i, $t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_BIAS:
			return "TEX3D_BIAS($_i, $t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
			return "TEX3D($_i, $t, ($0.xyz / $0.w))";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
			return "TEX3D_LOD($_i, $t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
			return "TEX3D_GRAD($_i, $t, $0.xyz, $1.xyz, $2.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
			return "TEX3D($_i, $t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_BIAS:
			return "TEX3D_BIAS($_i, $t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
			return "TEX3D_PROJ($_i, $t, $0)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
			return "TEX3D_LOD($_i, $t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD:
			return "TEX3D_GRAD($_i, $t, $0.xyz, $1.xyz, $2.xyz)";
		case FUNCTION::FUNCTION_DFDX:
			return "dFdx($0)";
		case FUNCTION::FUNCTION_DFDY:
			return "dFdy($0)";
		case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH1D:
			return "textureLod($t, $0.x, 0)";
		case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH2D:
			return "textureLod($t, $0.xy, 0)";
		case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH3D:
		case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCHCUBE:
			return "textureLod($t, $0.xyz, 0)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA:
			return "TEX2D_DEPTH_RGBA8($_i, $t, $0.xy)";
		}
	}
}