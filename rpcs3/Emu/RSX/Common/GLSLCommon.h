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
		OS << "		switch (location)\n";
		OS << "		{\n";
		OS << "		case 0:\n";
		OS << "			//position\n";
		OS << "			return vec4(0., 0., 0., 1.);\n";
		OS << "		case 1:\n";
		OS << "		case 2:\n";
		OS << "			//weight, normals\n";
		OS << "			return vec4(0.);\n";
		OS << "		case 3:\n";
		OS << "			//diffuse\n";
		OS << "			return vec4(1.);\n";
		OS << "		case 4:\n";
		OS << "			//specular\n";
		OS << "			return vec4(0.);\n";
		OS << "		case 5:\n";
		OS << "			//fog\n";
		OS << "			return vec4(0.);\n";
		OS << "		case 6:\n";
		OS << "			//point size\n";
		OS << "			return vec4(1.);\n";
		OS << "		default:\n";
		OS << "			//mostly just texture coordinates\n";
		OS << "			return vec4(0.);\n";
		OS << "		}\n";
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

		program_common::insert_compare_op(OS);

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

		OS << "vec4 get_wpos()\n";
		OS << "{\n";
		OS << "	float abs_scale = abs(wpos_scale);\n";
		OS << "	return (gl_FragCoord * vec4(abs_scale, wpos_scale, 1., 1.)) + vec4(0., wpos_bias, 0., 0.);\n";
		OS << "}\n\n";
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
			return "texture($t, $0.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ:
			return "textureProj($t, $0.x, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD:
			return "textureLod($t, $0.x, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD:
			return "textureGrad($t, $0.x, $1.x, $2.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D:
			return "texture($t, $0.xy * texture_parameters[$_i].xy)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ:
			return "textureProj($t, $0 , $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD:
			return "textureLod($t, $0.xy * texture_parameters[$_i].xy, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD:
			return "textureGrad($t, $0.xy * texture_parameters[$_i].xy , $1.xy, $2.xy)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D:
			return "texture($t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ:
			return "textureProj($t, $0, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE:
			return "texture($t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ:
			return "texture($t, ($0.xyz / $0.w))";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD:
			return "textureLod($t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD:
			return "textureGrad($t, $0.xyz, $1.xyz, $2.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D:
			return "texture($t, $0.xyz)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ:
			return "textureProj($t, $0.xyzw, $1.x)"; // Note: $1.x is bias
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD:
			return "textureLod($t, $0.xyz, $1.x)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD:
			return "textureGrad($t, $0.xyz, $1.xyz, $2.xyz)";
		case FUNCTION::FUNCTION_DFDX:
			return "dFdx($0)";
		case FUNCTION::FUNCTION_DFDY:
			return "dFdy($0)";
		case FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH2D:
			return "textureLod($t, $0.xy, 0)";
		case FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA:
			return "texture2DReconstruct($t, $0.xy * texture_parameters[$_i].xy, texture_parameters[$_i].z)";
		}
	}
}