#pragma once
#include <sstream>

#include "ShaderParam.h"

namespace program_common
{
	static void insert_compare_op(std::ostream& OS)
	{
		OS << "bool comparison_passes(float a, float b, uint func)\n";
		OS << "{\n";
		OS << "	switch (func)\n";
		OS << "	{\n";
		OS << "		default:\n";
		OS << "		case 0: return false; //never\n";
		OS << "		case 1: return (a < b); //less\n";
		OS << "		case 2: return (a == b); //equal\n";
		OS << "		case 3: return (a <= b); //lequal\n";
		OS << "		case 4: return (a > b); //greater\n";
		OS << "		case 5: return (a != b); //nequal\n";
		OS << "		case 6: return (a >= b); //gequal\n";
		OS << "		case 7: return true; //always\n";
		OS << "	}\n";
		OS << "}\n\n";
	}

	static void insert_fog_declaration(std::ostream& OS, const std::string wide_vector_type, const std::string input_coord, bool declare = false)
	{
		std::string template_body;

		if (!declare)
			template_body += "$T fetch_fog_value(uint mode)\n";
		else
			template_body += "$T fetch_fog_value(uint mode, $T $I)\n";

		template_body += "{\n";
		template_body += "	$T result = $T(0., 0., 0., 0.);\n";
		template_body += "	switch(mode)\n";
		template_body += "	{\n";
		template_body += "	default:\n";
		template_body += "		return result;\n";
		template_body += "	case 0:\n";
		template_body += "		//linear\n";
		template_body += "		result = $T(fog_param1 * $I.x + (fog_param0 - 1.), fog_param1 * $I.x + (fog_param0 - 1.), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += "	case 1:\n";
		template_body += "		//exponential\n";
		template_body += "		result = $T(11.084 * (fog_param1 * $I.x + fog_param0 - 1.5), exp(11.084 * (fog_param1 * $I.x + fog_param0 - 1.5)), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += "	case 2:\n";
		template_body += "		//exponential2\n";
		template_body += "		result = $T(4.709 * (fog_param1 * $I.x + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * $I.x + fog_param0 - 1.5), 2.)), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += "	case 3:\n";
		template_body += "		//exponential_abs\n";
		template_body += "		result = $T(11.084 * (fog_param1 * abs($I.x) + fog_param0 - 1.5), exp(11.084 * (fog_param1 * abs($I.x) + fog_param0 - 1.5)), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += "	case 4:\n";
		template_body += "		//exponential2_abs\n";
		template_body += "		result = $T(4.709 * (fog_param1 * abs($I.x) + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * abs($I.x) + fog_param0 - 1.5), 2.)), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += " case 5:\n";
		template_body += "		//linear_abs\n";
		template_body += "		result = $T(fog_param1 * abs($I.x) + (fog_param0 - 1.), fog_param1 * abs($I.x) + (fog_param0 - 1.), 0., 0.);\n";
		template_body += "		break;\n";
		template_body += "	}\n";
		template_body += "\n";
		template_body += "	result.x = max(result.x, 0.);\n";
		template_body += "	result.y = clamp(result.y, 0., 1.);\n";
		template_body += "	return result;\n";
		template_body += "}\n\n";

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
		OS << "struct attribute_desc\n";
		OS << "{\n";
		OS << "	int type;\n";
		OS << "	int attribute_size;\n";
		OS << "	int starting_offset;\n";
		OS << "	int stride;\n";
		OS << "	int swap_bytes;\n";
		OS << "	int is_volatile;\n";
		OS << "	int frequency;\n";
		OS << "	int divisor;\n";
		OS << "	int modulo;\n";
		OS << "};\n\n";

		OS << "uint get_bits(uvec4 v, int swap)\n";
		OS << "{\n";
		OS << "	if (swap != 0) return (v.w | v.z << 8 | v.y << 16 | v.x << 24);\n";
		OS << "	return (v.x | v.y << 8 | v.z << 16 | v.w << 24);\n";
		OS << "}\n\n";

		OS << "uint get_bits(uvec2 v, int swap)\n";
		OS << "{\n";
		OS << "	if (swap != 0) return (v.y | v.x << 8);\n";
		OS << "	return (v.x | v.y << 8);\n";
		OS << "}\n\n";

		OS << "int preserve_sign_s16(uint bits)\n";
		OS << "{\n";
		OS << "	//convert raw 16 bit value into signed 32-bit integer counterpart\n";
		OS << "	uint sign = bits & 0x8000;\n";
		OS << "	if (sign != 0) return int(bits | 0xFFFF0000);\n";
		OS << "	return int(bits);\n";
		OS << "}\n\n";

		//For intel GPUs which cannot access vectors in indexed mode (driver bug? or glsl version too low?)
		if (!glsl4_compliant)
		{
			OS << "void mov(inout vec4 vector, in int index, in float scalar)\n";
			OS << "{\n";
			OS << "	switch(index)\n";
			OS << "	{\n";
			OS << "		case 0: vector.x = scalar; return;\n";
			OS << "		case 1: vector.y = scalar; return;\n";
			OS << "		case 2: vector.z = scalar; return;\n";
			OS << "		case 3: vector.w = scalar; return;\n";
			OS << "	}\n";
			OS << "}\n";
		}
		else
		{
			OS << "#define mov(v, i, s) v[i] = s\n";
		}

		OS << "#define get_s16(v, s) preserve_sign_s16(get_bits(v, s))\n\n";

		OS << "vec4 fetch_attribute(attribute_desc desc, int vertex_id, usamplerBuffer input_stream)\n";
		OS << "{\n";
		OS << "	vec4 result = vec4(0., 0., 0., 1.);\n";
		OS << "	vec4 scale = vec4(1.);\n";
		OS << "	uvec4 tmp;\n";
		OS << "	uint bits;\n";
		OS << "	bool reverse_order = false;\n";
		OS << "\n";
		OS << "	int first_byte = (vertex_id * desc.stride) + desc.starting_offset;\n";
		OS << "	for (int n = 0; n < 4; n++)\n";
		OS << "	{\n";
		OS << "		if (n == desc.attribute_size) break;\n";
		OS << "\n";
		OS << "		switch (desc.type)\n";
		OS << "		{\n";
		OS << "		case 0:\n";
		OS << "			//signed normalized 16-bit\n";
		OS << "			tmp.x = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.y = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			mov(result, n, get_s16(tmp.xy, desc.swap_bytes));\n";
		OS << "			mov(scale, n, 32767.);\n";
		OS << "			break;\n";
		OS << "		case 1:\n";
		OS << "			//float\n";
		OS << "			tmp.x = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.y = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.z = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.w = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			mov(result, n, uintBitsToFloat(get_bits(tmp, desc.swap_bytes)));\n";
		OS << "			break;\n";
		OS << "		case 2:\n";
		OS << "			//half\n";
		OS << "			tmp.x = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.y = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			mov(result, n, unpackHalf2x16(uint(get_bits(tmp.xy, desc.swap_bytes))).x);\n";
		OS << "			break;\n";
		OS << "		case 3:\n";
		OS << "			//unsigned byte\n";
		OS << "			mov(result, n, texelFetch(input_stream, first_byte++).x);\n";
		OS << "			mov(scale, n, 255.);\n";
		OS << "			reverse_order = (desc.swap_bytes != 0);\n";
		OS << "			break;\n";
		OS << "		case 4:\n";
		OS << "			//signed word\n";
		OS << "			tmp.x = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.y = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			mov(result, n, get_s16(tmp.xy, desc.swap_bytes));\n";
		OS << "			break;\n";
		OS << "		case 5:\n";
		OS << "			//cmp\n";
		OS << "			tmp.x = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.y = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.z = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp.w = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			bits = get_bits(tmp, desc.swap_bytes);\n";
		OS << "			result.x = preserve_sign_s16((bits & 0x7FF) << 5);\n";
		OS << "			result.y = preserve_sign_s16(((bits >> 11) & 0x7FF) << 5);\n";
		OS << "			result.z = preserve_sign_s16(((bits >> 22) & 0x3FF) << 6);\n";
		OS << "			result.w = 1.;\n";
		OS << "			scale = vec4(32767., 32767., 32767., 1.);\n";
		OS << "			break;\n";
		OS << "		case 6:\n";
		OS << "			//ub256\n";
		OS << "			mov(result, n, float(texelFetch(input_stream, first_byte++).x));\n";
		OS << "			reverse_order = (desc.swap_bytes != 0);\n";
		OS << "			break;\n";
		OS << "		}\n";
		OS << "	}\n\n";
		OS << "	result /= scale;\n";
		OS << "	return (reverse_order)? result.wzyx: result;\n";
		OS << "}\n\n";

		OS << "attribute_desc fetch_desc(int location)\n";
		OS << "{\n";
		OS << "	attribute_desc result;\n";
		OS << "	int attribute_flags = input_attributes[location].w;\n";
		OS << "	result.type = input_attributes[location].x;\n";
		OS << "	result.attribute_size = input_attributes[location].y;\n";
		OS << "	result.starting_offset = input_attributes[location].z;\n";
		OS << "	result.stride = attribute_flags & 0xFF;\n";
		OS << "	result.swap_bytes = (attribute_flags >> 8) & 0x1;\n";
		OS << "	result.is_volatile = (attribute_flags >> 9) & 0x1;\n";
		OS << "	result.frequency = (attribute_flags >> 10) & 0x3;\n";
		OS << "	result.modulo = (attribute_flags >> 12) & 0x1;\n";
		OS << "	result.divisor = (attribute_flags >> 13) & 0xFFFF;\n";
		OS << "	return result;\n";
		OS << "}\n\n";

		OS << "vec4 read_location(int location)\n";
		OS << "{\n";
		OS << "	attribute_desc desc = fetch_desc(location);\n";
		OS << "	if (desc.attribute_size == 0)\n";
		OS << "	{\n";
		OS << "		//default values\n";
		OS << "		const vec4 defaults[] = \n";
		OS << "		{	vec4(0., 0., 0., 1.), //position\n";
		OS << "			vec4(0.), vec4(0.), //weight, normals\n";
		OS << "			vec4(1.), //diffuse\n";
		OS << "			vec4(0.), vec4(0.), //specular, fog\n";
		OS << "			vec4(1.), //point size\n";
		OS << "			vec4(0.), //in_7\n";
		OS << "			//in_tc registers\n";
		OS << "			vec4(0.), vec4(0.), vec4(0.), vec4(0.),\n";
		OS << "			vec4(0.), vec4(0.), vec4(0.), vec4(0.)\n";
		OS << "		};\n";
		OS << "		return defaults[location];\n";
		OS << "	}\n\n";
		OS << "	int vertex_id = " << vertex_id_name << " - int(vertex_base_index);\n";
		OS << "	if (desc.frequency == 0)\n";
		OS << "		vertex_id = 0;\n";
		OS << "	else if (desc.frequency > 1)\n";
		OS << "	{\n";
		OS << "		//if a vertex modifier is active; vertex_base must be 0 and is ignored\n";
		OS << "		if (desc.modulo != 0)\n";
		OS << "			vertex_id = " << vertex_id_name << " % desc.divisor;\n";
		OS << "		else\n";
		OS << "			vertex_id = " << vertex_id_name << " / desc.divisor;\n";
		OS << "	}\n";
		OS << "\n";
		OS << "	if (desc.is_volatile != 0)\n";
		OS << "		return fetch_attribute(desc, vertex_id, volatile_input_stream);\n";
		OS << "	else\n";
		OS << "		return fetch_attribute(desc, vertex_id, persistent_input_stream);\n";
		OS << "}\n\n";
	}

	static void insert_rop(std::ostream& OS, bool _32_bit_exports)
	{
		const std::string reg0 = _32_bit_exports ? "r0" : "h0";
		const std::string reg1 = _32_bit_exports ? "r2" : "h4";
		const std::string reg2 = _32_bit_exports ? "r3" : "h6";
		const std::string reg3 = _32_bit_exports ? "r4" : "h8";

		//TODO: Implement all ROP options like CSAA and ALPHA_TO_ONE here
		OS << "	if ((rop_control & 0xFF) != 0)\n";
		OS << "	{\n";
		OS << "		bool alpha_test = (rop_control & 0x11) > 0;\n";
		OS << "		uint alpha_func = ((rop_control >> 16) & 0x7);\n";
		OS << "		bool srgb_convert = (rop_control & 0x2) > 0;\n\n";
		OS << "		if (alpha_test && !comparison_passes(" << reg0 << ".a, alpha_ref, alpha_func))\n";
		OS << "		{\n";
		OS << "			discard;\n";
		OS << "		}\n";

		if (!_32_bit_exports)
		{
			//Tested using NPUB90375; some shaders (32-bit output only?) do not obey srgb flags
			OS << "		else if (srgb_convert)\n";
			OS << "		{\n";
			OS << "			" << reg0 << ".rgb = linear_to_srgb(" << reg0 << ").rgb;\n";
			OS << "			" << reg1 << ".rgb = linear_to_srgb(" << reg1 << ").rgb;\n";
			OS << "			" << reg2 << ".rgb = linear_to_srgb(" << reg2 << ").rgb;\n";
			OS << "			" << reg3 << ".rgb = linear_to_srgb(" << reg3 << ").rgb;\n";
			OS << "		}\n";
		}

		OS << "	}\n\n";

		OS << "	ocol0 = " << reg0 << ";\n";
		OS << "	ocol1 = " << reg1 << ";\n";
		OS << "	ocol2 = " << reg2 << ";\n";
		OS << "	ocol3 = " << reg3 << ";\n\n";
	}

	static void insert_glsl_legacy_function(std::ostream& OS, glsl::program_domain domain, bool require_lit_emulation, bool require_depth_conversion = false, bool require_wpos = false, bool require_texture_ops = true)
	{
		if (require_lit_emulation)
		{
			OS << "vec4 lit_legacy(vec4 val)";
			OS << "{\n";
			OS << "	vec4 clamped_val = val;\n";
			OS << "	clamped_val.x = max(val.x, 0.);\n";
			OS << "	clamped_val.y = max(val.y, 0.);\n";
			OS << "	vec4 result;\n";
			OS << "	result.x = 1.;\n";
			OS << "	result.w = 1.;\n";
			OS << "	result.y = clamped_val.x;\n";
			OS << "	result.z = clamped_val.x > 0. ? exp(clamped_val.w * log(max(clamped_val.y, 0.0000000001))) : 0.;\n";
			OS << "	return result;\n";
			OS << "}\n\n";
		}

		if (domain == glsl::program_domain::glsl_vertex_program)
		{
			OS << "vec4 apply_zclip_xform(vec4 pos, float near_plane, float far_plane)\n";
			OS << "{\n";
			OS << "	float d = pos.z / pos.w;\n";
			OS << "	if (d < 0.f && d >= near_plane)\n";
			OS << "		d = 0.f;\n"; //force clamp negative values
			OS << "	else if (d > 1.f && d <= far_plane)\n";
			OS << "		d = min(1., 0.99 + (0.01 * (pos.z - near_plane) / (far_plane - near_plane)));\n";
			OS << "	else\n";
			OS << "		return pos; //d = (0.99 * d);\n"; //range compression for normal values is disabled until a solution to ops comparing z is found
			OS << "\n";
			OS << "	pos.z = d * pos.w;\n";
			OS << "	return pos;\n";
			OS << "}\n\n";

			return;
		}

		program_common::insert_compare_op(OS);

		if (require_depth_conversion)
		{
			//NOTE: Memory layout is fetched as byteswapped BGRA [GBAR] (GOW collection, DS2, DeS)
			//The A component (Z) is useless (should contain stencil8 or just 1)
			OS << "vec4 decodeLinearDepth(float depth_value)\n";
			OS << "{\n";
			OS << "	uint value = uint(depth_value * 16777215);\n";
			OS << "	uint b = (value & 0xff);\n";
			OS << "	uint g = (value >> 8) & 0xff;\n";
			OS << "	uint r = (value >> 16) & 0xff;\n";
			OS << "	return vec4(float(g)/255., float(b)/255., 1., float(r)/255.);\n";
			OS << "}\n\n";

			OS << "float read_value(vec4 src, uint remap_index)\n";
			OS << "{\n";
			OS << "	switch (remap_index)\n";
			OS << "	{\n";
			OS << "		case 0: return src.a;\n";
			OS << "		case 1: return src.r;\n";
			OS << "		case 2: return src.g;\n";
			OS << "		case 3: return src.b;\n";
			OS << "	}\n";
			OS << "}\n\n";

			OS << "vec4 texture2DReconstruct(sampler2D tex, vec2 coord, float remap)\n";
			OS << "{\n";
			OS << "	vec4 result = decodeLinearDepth(texture(tex, coord.xy).r);\n";
			OS << "	uint remap_vector = floatBitsToUint(remap) & 0xFF;\n";
			OS << "	if (remap_vector == 0xE4) return result;\n\n";
			OS << "	vec4 tmp;\n";
			OS << "	uint remap_a = remap_vector & 0x3;\n";
			OS << "	uint remap_r = (remap_vector >> 2) & 0x3;\n";
			OS << "	uint remap_g = (remap_vector >> 4) & 0x3;\n";
			OS << "	uint remap_b = (remap_vector >> 6) & 0x3;\n";
			OS << "	tmp.a = read_value(result, remap_a);\n";
			OS << "	tmp.r = read_value(result, remap_r);\n";
			OS << "	tmp.g = read_value(result, remap_g);\n";
			OS << "	tmp.b = read_value(result, remap_b);\n";
			OS << "	return tmp;\n";
			OS << "}\n\n";
		}

		if (require_texture_ops)
		{
			OS << "vec4 linear_to_srgb(vec4 cl)\n";
			OS << "{\n";
			OS << "	vec4 low = cl * 12.92;\n";
			OS << "	vec4 high = 1.055 * pow(cl, vec4(1. / 2.4)) - 0.055;\n";
			OS << "	bvec4 select = lessThan(cl, vec4(0.0031308));\n";
			OS << "	return clamp(mix(high, low, select), 0., 1.);\n";
			OS << "}\n\n";

			OS << "float srgb_to_linear(float cs)\n";
			OS << "{\n";
			OS << "	if (cs <= 0.04045) return cs / 12.92;\n";
			OS << "	return pow((cs + 0.055) / 1.055, 2.4);\n";
			OS << "}\n\n";

			//TODO: Move all the texture read control operations here
			OS << "vec4 process_texel(vec4 rgba, uint control_bits)\n";
			OS << "{\n";
			OS << "	if (control_bits == 0) return rgba;\n\n";
			OS << "	if ((control_bits & 0x10) > 0)\n";
			OS << "	{\n";
			OS << "		//Alphakill\n";
			OS << "		if (rgba.a < 0.0000000001)\n";
			OS << "		{\n";
			OS << "			discard;\n";
			OS << "			return rgba;\n";
			OS << "		}\n";
			OS << "	}\n\n";
			OS << "	//TODO: Verify gamma control bit ordering, looks to be 0x7 for rgb, 0xF for rgba\n";
			OS << "	uint srgb_in = (control_bits & 0xF);\n";
			OS << "	if ((srgb_in & 0x1) > 0) rgba.r = srgb_to_linear(rgba.r);\n";
			OS << "	if ((srgb_in & 0x2) > 0) rgba.g = srgb_to_linear(rgba.g);\n";
			OS << "	if ((srgb_in & 0x4) > 0) rgba.b = srgb_to_linear(rgba.b);\n";
			OS << "	if ((srgb_in & 0x8) > 0) rgba.a = srgb_to_linear(rgba.a);\n";
			OS << "	return rgba;\n";
			OS << "}\n\n";

			OS << "#define TEX1D(index, tex, coord1) process_texel(texture(tex, coord1 * texture_parameters[index].x), uint(texture_parameters[index].w))\n";
			OS << "#define TEX1D_BIAS(index, tex, coord1, bias) process_texel(texture(tex, coord1 * texture_parameters[index].x, bias), uint(texture_parameters[index].w))\n";
			OS << "#define TEX1D_LOD(index, tex, coord1, lod) process_texel(textureLod(tex, coord1 * texture_parameters[index].x, lod), uint(texture_parameters[index].w))\n";
			OS << "#define TEX1D_GRAD(index, tex, coord1, dpdx, dpdy) process_texel(textureGrad(tex, coord1 * texture_parameters[index].x, dpdx, dpdy), uint(texture_parameters[index].w))\n";
			OS << "#define TEX1D_PROJ(index, tex, coord2) process_texel(textureProj(tex, coord2 * vec2(texture_parameters[index].x, 1.)), uint(texture_parameters[index].w))\n";

			OS << "#define TEX2D(index, tex, coord2) process_texel(texture(tex, coord2 * texture_parameters[index].xy), uint(texture_parameters[index].w))\n";
			OS << "#define TEX2D_BIAS(index, tex, coord2, bias) process_texel(texture(tex, coord2 * texture_parameters[index].xy, bias), uint(texture_parameters[index].w))\n";
			OS << "#define TEX2D_LOD(index, tex, coord2, lod) process_texel(textureLod(tex, coord2 * texture_parameters[index].xy, lod), uint(texture_parameters[index].w))\n";
			OS << "#define TEX2D_GRAD(index, tex, coord2, dpdx, dpdy) process_texel(textureGrad(tex, coord2 * texture_parameters[index].xy, dpdx, dpdy), uint(texture_parameters[index].w))\n";
			OS << "#define TEX2D_PROJ(index, tex, coord4) process_texel(textureProj(tex, coord4 * vec4(texture_parameters[index].xy, 1., 1.)), uint(texture_parameters[index].w))\n";

			OS << "#define TEX2D_DEPTH_RGBA8(index, tex, coord2) process_texel(texture2DReconstruct(tex, coord2 * texture_parameters[index].xy, texture_parameters[index].z), uint(texture_parameters[index].w))\n";
			OS << "#define TEX2D_SHADOW(index, tex, coord3) texture(tex, coord3 * vec3(texture_parameters[index].xy, 1.))\n";
			OS << "#define TEX2D_SHADOWPROJ(index, tex, coord4) textureProj(tex, coord4 * vec4(texture_parameters[index].xy, 1., 1.))\n";

			OS << "#define TEX3D(index, tex, coord3) process_texel(texture(tex, coord3), uint(texture_parameters[index].w))\n";
			OS << "#define TEX3D_BIAS(index, tex, coord3, bias) process_texel(texture(tex, coord3, bias), uint(texture_parameters[index].w))\n";
			OS << "#define TEX3D_LOD(index, tex, coord3, lod) process_texel(textureLod(tex, coord3, lod), uint(texture_parameters[index].w))\n";
			OS << "#define TEX3D_GRAD(index, tex, coord3, dpdx, dpdy) process_texel(textureGrad(tex, coord3, dpdx, dpdy), uint(texture_parameters[index].w))\n";
			OS << "#define TEX3D_PROJ(index, tex, coord4) process_texel(textureProj(tex, coord4), uint(texture_parameters[index].w))\n\n";
		}

		if (require_wpos)
		{
			OS << "vec4 get_wpos()\n";
			OS << "{\n";
			OS << "	float abs_scale = abs(wpos_scale);\n";
			OS << "	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);\n";
			OS << "}\n\n";
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