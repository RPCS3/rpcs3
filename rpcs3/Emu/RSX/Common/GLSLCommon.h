#pragma once
#include <sstream>

#include "ShaderParam.h"

namespace glsl
{
	enum program_domain
	{
		glsl_vertex_program = 0,
		glsl_fragment_program = 1
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

	static std::string compareFunctionImpl(COMPARE f, const std::string &Op0, const std::string &Op1)
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
		fmt::throw_exception("Unknown compare function" HERE);
	}

	static void insert_vertex_input_fetch(std::stringstream& OS, glsl_rules rules)
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

		/* TODO: For intel GPUs that seemingly cannot generate fp32 values from raw bits
		OS << "float convert_to_f32(uint bits)\n";
		OS << "{\n";
		OS << "	uint sign = (bits >> 31) & 1;\n";
		OS << "	uint exp = (bits >> 23) & 0xff;\n";
		OS << "	uint mantissa = bits & 0x7fffff;\n";
		OS << "	float base = (sign != 0)? -1.f: 1.f;\n";
		OS << "	base *= exp2(exp - 127);\n";
		OS << "	float scale = 0.f;\n\n";
		OS << "	for (int x = 0; x < 23; x++)\n";
		OS << "	{\n";
		OS << "		int inv = (22 - x);\n";
		OS << "		if ((mantissa & (1 << inv)) == 0) continue;\n";
		OS << "		scale += 1.f / pow(2.f, float(inv));\n";
		OS << "	}\n";
		OS << "	return base * scale;\n";
		OS << "}\n";*/

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
		OS << "			tmp[0] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[1] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			result[n] = get_s16(tmp.xy, desc.swap_bytes);\n";
		OS << "			scale[n] = 32767.;\n";
		OS << "			break;\n";
		OS << "		case 1:\n";
		OS << "			//float\n";
		OS << "			tmp[0] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[1] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[2] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[3] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			result[n] = uintBitsToFloat(get_bits(tmp, desc.swap_bytes));\n";
		OS << "			break;\n";
		OS << "		case 2:\n";
		OS << "			//half\n";
		OS << "			tmp[0] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[1] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			result[n] = unpackHalf2x16(uint(get_bits(tmp.xy, desc.swap_bytes))).x;\n";
		OS << "			break;\n";
		OS << "		case 3:\n";
		OS << "			//unsigned byte\n";
		OS << "			result[n] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			scale[n] = 255.;\n";
		OS << "			reverse_order = (desc.swap_bytes != 0);\n";
		OS << "			break;\n";
		OS << "		case 4:\n";
		OS << "			//signed word\n";
		OS << "			tmp[0] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[1] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			result[n] = get_s16(tmp.xy, desc.swap_bytes);\n";
		OS << "			break;\n";
		OS << "		case 5:\n";
		OS << "			//cmp\n";
		OS << "			tmp[0] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[1] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[2] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			tmp[3] = texelFetch(input_stream, first_byte++).x;\n";
		OS << "			bits = get_bits(tmp, desc.swap_bytes);\n";
		OS << "			result.x = preserve_sign_s16((bits & 0x7FF) << 5);\n";
		OS << "			result.y = preserve_sign_s16(((bits >> 11) & 0x7FF) << 5);\n";
		OS << "			result.z = preserve_sign_s16(((bits >> 22) & 0x3FF) << 6);\n";
		OS << "			result.w = 1.;\n";
		OS << "			scale = vec4(32767., 32767., 32767., 1.);\n";
		OS << "			break;\n";
		OS << "		case 6:\n";
		OS << "			//ub256\n";
		OS << "			result[n] = float(texelFetch(input_stream, first_byte++).x);\n";
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
		OS << "\n";
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

	static void insert_glsl_legacy_function(std::ostream& OS, glsl::program_domain domain)
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
		OS << "	result.z = clamped_val.x > 0. ? exp(clamped_val.w * log(max(clamped_val.y, 1.E-10))) : 0.;\n";
		OS << "	return result;\n";
		OS << "}\n\n";

		if (domain == glsl::program_domain::glsl_vertex_program)
			return;

		//NOTE: After testing with GOW, the w component is either the original depth or wraps around to the x component
		//Since component.r == depth_value with some precision loss, just use the precise depth value for now (further testing needed)
		OS << "vec4 decodeLinearDepth(float depth_value)\n";
		OS << "{\n";
		OS << "	uint value = uint(depth_value * 16777215);\n";
		OS << "	uint b = (value & 0xff);\n";
		OS << "	uint g = (value >> 8) & 0xff;\n";
		OS << "	uint r = (value >> 16) & 0xff;\n";
		OS << "	return vec4(float(r)/255., float(g)/255., float(b)/255., depth_value);\n";
		OS << "}\n\n";

		OS << "vec4 texture2DReconstruct(sampler2D tex, vec2 coord)\n";
		OS << "{\n";
		OS << "	return decodeLinearDepth(texture(tex, coord.xy).r);\n";
		OS << "}\n\n";

		OS << "vec4 texture2DReconstruct(sampler2DRect tex, vec2 coord)\n";
		OS << "{\n";
		OS << "	return decodeLinearDepth(texture(tex, coord.xy).r);\n";
		OS << "}\n\n";
	}
}