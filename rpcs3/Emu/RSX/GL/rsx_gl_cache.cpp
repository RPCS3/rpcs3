#include "stdafx.h"
#include "rsx_gl_cache.h"
#include "gl_helpers.h"
#include "../GCM.h"

static void insert_texture_fetch_function(std::string &dst, const rsx::decompiled_shader &shader, const rsx::program_state &state)
{
	if (shader.textures.empty())
	{
		return;
	}

	dst += "vec4 texture_fetch(int index, vec4 coord)\n{\n";
	dst += "\tswitch (index)\n\t{\n";

	for (auto &texture : shader.textures)
	{
		dst += "\tcase " + std::to_string(texture.id) + ": return ";

		switch (state.textures[texture.id])
		{
		case rsx::texture_target::none: dst += "vec4(0.0)"; break;
		case rsx::texture_target::_1: dst += "texture(" + texture.name + ", coord.x)"; break;
		case rsx::texture_target::_2: dst += "texture(" + texture.name + ", coord.xy)"; break;

		case rsx::texture_target::cube:
		case rsx::texture_target::_3: dst += "texture(" + texture.name + ", coord.xyz)"; break;
		}

		dst += ";\n";
	}

	dst += "\t}\n";
	dst += "}\n";
}

static void insert_texture_bias_fetch_function(std::string &dst, const rsx::decompiled_shader &shader, const rsx::program_state &state)
{
	if (shader.textures.empty())
	{
		return;
	}

	dst += "vec4 texture_bias_fetch(int index, vec4 coord, float bias)\n{\n";
	dst += "\tswitch (index)\n\t{\n";

	for (auto &texture : shader.textures)
	{
		dst += "\tcase " + std::to_string(texture.id) + ": return ";

		switch (state.textures[texture.id])
		{
		case rsx::texture_target::none: dst += "vec4(0.0)"; break;
		case rsx::texture_target::_1: dst += "texture(" + texture.name + ", coord.x, bias)"; break;
		case rsx::texture_target::_2: dst += "texture(" + texture.name + ", coord.xy, bias)"; break;

		case rsx::texture_target::cube:
		case rsx::texture_target::_3: dst += "texture(" + texture.name + ", coord.xyz, bias)"; break;
		}

		dst += ";\n";
	}

	dst += "\t}\n";
	dst += "}\n";
}

static void insert_texture_grad_fetch_function(std::string &dst, const rsx::decompiled_shader &shader, const rsx::program_state &state)
{
	if (shader.textures.empty())
	{
		return;
	}

	dst += "vec4 texture_grad_fetch(int index, vec4 coord, vec4 dPdx, vec4 dPdy)\n{\n";
	dst += "\tswitch (index)\n\t{\n";

	for (auto &texture : shader.textures)
	{
		dst += "\tcase " + std::to_string(texture.id) + ": return ";

		switch (state.textures[texture.id])
		{
		case rsx::texture_target::none: dst += "vec4(0.0)"; break;
		case rsx::texture_target::_1: dst += "textureGrad(" + texture.name + ", coord.x, dPdx.x, dPdy.x)"; break;
		case rsx::texture_target::_2: dst += "textureGrad(" + texture.name + ", coord.xy, dPdx.xy, dPdy.xy)"; break;

		case rsx::texture_target::cube:
		case rsx::texture_target::_3: dst += "textureGrad(" + texture.name + ", coord.xyz, dPdx.xyz, dPdy.xyz)"; break;
		}

		dst += ";\n";
	}

	dst += "\t}\n";
	dst += "}\n";
}

static void insert_texture_lod_fetch_function(std::string &dst, const rsx::decompiled_shader &shader, const rsx::program_state &state)
{
	if (shader.textures.empty())
	{
		return;
	}

	dst += "vec4 texture_lod_fetch(int index, vec4 coord, float lod)\n{\n";
	dst += "\tswitch (index)\n\t{\n";

	for (auto &texture : shader.textures)
	{
		dst += "\tcase " + std::to_string(texture.id) + ": return ";

		switch (state.textures[texture.id])
		{
		case rsx::texture_target::none: dst += "vec4(0.0)"; break;
		case rsx::texture_target::_1: dst += "textureLod(" + texture.name + ", coord.x, lod)"; break;
		case rsx::texture_target::_2: dst += "textureLod(" + texture.name + ", coord.xy, lod)"; break;

		case rsx::texture_target::cube:
		case rsx::texture_target::_3: dst += "textureLod(" + texture.name + ", coord.xyz, lod)"; break;
		}

		dst += ";\n";
	}

	dst += "\t}\n";
	dst += "}\n";
}

static void insert_texture_proj_fetch_function(std::string &dst, const rsx::decompiled_shader &shader, const rsx::program_state &state)
{
	if (shader.textures.empty())
	{
		return;
	}

	dst += "vec4 texture_proj_fetch(int index, vec4 coord, float bias)\n{\n";
	dst += "\tswitch (index)\n\t{\n";

	for (auto &texture : shader.textures)
	{
		dst += "\tcase " + std::to_string(texture.id) + ": return ";

		switch (state.textures[texture.id])
		{
		case rsx::texture_target::cube:
		case rsx::texture_target::none: dst += "vec4(0.0)"; break;
		case rsx::texture_target::_1: dst += "textureProj(" + texture.name + ", coord.xy, bias)"; break;
		case rsx::texture_target::_2: dst += "textureProj(" + texture.name + ", coord.xyz, bias)"; break;
		case rsx::texture_target::_3: dst += "textureProj(" + texture.name + ", coord, bias)"; break;
		}

		dst += ";\n";
	}

	dst += "\t}\n";
	dst += "}\n";
}


rsx::complete_shader glsl_complete_shader(const rsx::decompiled_shader &shader, rsx::program_state state)
{
	rsx::complete_shader result;
	result.decompiled = &shader;
	result.code = "#version 430\n\n";

	if (shader.raw->type == rsx::program_type::vertex)
	{
		result.code += "layout(std140, binding = 0) uniform MatrixBuffer\n{\n"
			"\tmat4 viewport_matrix;\n"
			"\tmat4 window_matrix;\n"
			"\tmat4 normalize_matrix;\n"
			"};\n";
	}
	else if (shader.raw->type == rsx::program_type::fragment)
	{
		result.code += "layout(std140, binding = 3) uniform StateParameters\n{\n"
			"\tfloat fog_param0;\n"
			"\tfloat fog_param1;\n"
			"\tuint alpha_test;\n"
			"\tfloat alpha_ref;\n"
			"};\n";
	}

	if (!shader.constants.empty())
	{
		if (shader.raw->type == rsx::program_type::vertex)
		{
			result.code += "layout(std140, binding = 1) uniform VertexConstantsBuffer\n";
		}
		else
		{
			result.code += "layout(std140, binding = 2) uniform FragmentConstantsBuffer\n";
		}

		result.code += "{\n";

		for (const rsx::constant_info& constant : shader.constants)
		{
			result.code += "\tvec4 " + constant.name + ";\n";
		}

		result.code += "};\n\n";
	}

	for (const rsx::register_info& temporary : shader.temporary_registers)
	{
		std::string value;
		std::string type;
		switch (temporary.type)
		{
		case rsx::register_type::half_float_point:
		case rsx::register_type::single_float_point:
			type = "vec4";
			if (temporary.name == "o0")
			{
				value = "vec4(vec3(0.0), 1.0)";
			}
			else
			{
				value = "vec4(0.0)";
			}
			break;

		case rsx::register_type::integer:
			type = "ivec4";
			value = "ivec4(0)";
			break;

		default:
			throw;
		}

		result.code += type + " " + temporary.name + " = " + value + ";\n";
	}

	result.code += "\n";

	std::string prepare;
	std::string finalize;
	int location = 1;

	switch (shader.raw->type)
	{
	case rsx::program_type::fragment:
		for (const rsx::texture_info& texture : shader.textures)
		{
			result.code += "uniform vec4 " + texture.name + "_cm = vec4(1.0);\n";
			rsx::texture_target target = state.textures[texture.id];

			result.code += "uniform sampler";

			switch (target)
			{
			default:
			case rsx::texture_target::_1: result.code += "1D"; break;
			case rsx::texture_target::_2: result.code += "2D"; break;
			case rsx::texture_target::_3: result.code += "3D"; break;
			case rsx::texture_target::cube: result.code += "Cube"; break;
			}
			result.code += " " + texture.name + ";\n";
		}

		insert_texture_fetch_function(result.code, shader, state);
		insert_texture_bias_fetch_function(result.code, shader, state);
		insert_texture_grad_fetch_function(result.code, shader, state);
		insert_texture_lod_fetch_function(result.code, shader, state);
		insert_texture_proj_fetch_function(result.code, shader, state);

		result.code += "\n";
		result.code += "layout(location = 0) out vec4 ocol;\n";

		if (state.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS)
		{
			if (0)
			{
				finalize += "\tocol = vec4(1.0, 0.0, 1.0, 1.0);\n";
			}
			else
			{
				finalize += "\tocol = r0;\n";
			}

			if (shader.temporary_registers.find({ "r2" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 1) out vec4 ocol1;\n";
				finalize += "\tocol1 = r2;\n";
			}
			if (shader.temporary_registers.find({ "r3" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 2) out vec4 ocol2;\n";
				finalize += "\tocol2 = r3;\n";
			}
			if (shader.temporary_registers.find({ "r4" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 3) out vec4 ocol3;\n";
				finalize += "\tocol3 = r4;\n";
			}
		}
		else
		{
			finalize += "\tocol = h0;\n";

			if (shader.temporary_registers.find({ "h4" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 1) out vec4 ocol1;\n";
				finalize += "\tocol1 = h4;\n";
			}
			if (shader.temporary_registers.find({ "h6" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 2) out vec4 ocol2;\n";
				finalize += "\tocol2 = h6;\n";
			}
			if (shader.temporary_registers.find({ "h8" }) != shader.temporary_registers.end())
			{
				result.code += "layout(location = 3) out vec4 ocol3;\n";
				finalize += "\tocol3 = h8;\n";
			}
		}

		if (state.ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		{
			if (state.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS)
			{
				if (shader.temporary_registers.find({ "r1" }) != shader.temporary_registers.end())
				{
					finalize += "\tgl_FragDepth = r1.z;\n";
				}
			}
			else
			{
				if (shader.temporary_registers.find({ "h2" }) != shader.temporary_registers.end())
				{
					finalize += "\tgl_FragDepth = h2.z;\n";
				}
			}
		}

		{
			if (~state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_FOG)
			{
				result.code += "vec4 fog = vec4(0.0);\n";
			}

			result.code += "vec4 fogc;\n";

			std::string body;
			switch ((rsx::fog_mode)state.fog_mode)
			{
			case rsx::fog_mode::linear:
				body = "fog_param1 * fog.x + (fog_param0 - 1.0), fog_param1 * fog.x + (fog_param0 - 1.0)";
				break;
			case rsx::fog_mode::exponential:
				body = "11.084 * (fog_param1 * fog.x + fog_param0 - 1.5), exp(11.084 * (fog_param1 * fog.x + fog_param0 - 1.5))";
				break;
			case rsx::fog_mode::exponential2:
				body = "4.709 * (fog_param1 * fog.x + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * fog.x + fog_param0 - 1.5), 2.0))";
				break;
			case rsx::fog_mode::linear_abs:
				body = "fog_param1 * abs(fog.x) + (fog_param0 - 1.0), fog_param1 * abs(fog.x) + (fog_param0 - 1.0)";
				break;
			case rsx::fog_mode::exponential_abs:
				body = "11.084 * (fog_param1 * abs(fog.x) + fog_param0 - 1.5), exp(11.084 * (fog_param1 * abs(fog.x) + fog_param0 - 1.5))";
				break;
			case rsx::fog_mode::exponential2_abs:
				body = "4.709 * (fog_param1 * abs(fog.x) + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * abs(fog.x) + fog_param0 - 1.5), 2.0))";
				break;

			default:
				body = "0.0, 0.0";
			}

			prepare += "\tfogc = clamp(vec4(" + body + ", 0.0, 0.0), 0.0, 1.0);\n";
		}

		{
			u32 diffuse_color = state.output_attributes & (CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE);
			u32 specular_color = state.output_attributes & (CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR);

			if (diffuse_color)
			{
				result.code += "vec4 col0;\n";
			}

			if (specular_color)
			{
				result.code += "vec4 col1;\n";
			}

			if (diffuse_color == (CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE) &&
				specular_color == (CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR))
			{
				prepare += "\tif (gl_FrontFacing)\n\t{";
				prepare += "\t\tcol0 = front_diffuse_color;\n";
				prepare += "\t\tcol1 = front_specular_color;\n";
				prepare += "\t}\nelse\n\t{\n";
				prepare += "\t\tcol0 = back_diffuse_color;\n";
				prepare += "\t\tcol1 = back_specular_color;\n";
				prepare += "\t}";
			}
			else
			{
				switch (diffuse_color)
				{
				case CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE:
					prepare += "\tcol0 = gl_FrontFacing ? front_diffuse_color : back_diffuse_color;\n";
					break;

				case CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE:
					prepare += "\tcol0 = front_diffuse_color;\n";
					break;

				case CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE:
					prepare += "\tcol0 = back_diffuse_color;\n";
					break;

				default:
					if (shader.input_attributes & (1 << 1))
					{
						result.code += "vec4 col0 = vec4(0.0);\n";
					}
					break;
				}

				switch (specular_color)
				{
				case CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR:
					prepare += "\tcol1 = gl_FrontFacing ? front_specular_color : back_specular_color;\n";
					break;

				case CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR:
					prepare += "\tcol1 = front_specular_color;\n";
					break;

				case CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR:
					prepare += "\tcol1 = back_specular_color;\n";
					break;

				default:
					if (shader.input_attributes & (1 << 2))
					{
						result.code += "vec4 col1 = vec4(0.0);\n";

						if (diffuse_color)
						{
							prepare += "\tcol1 = col0;\n";
						}
					}
					break;
				}
			}
		}

		result.code += "in vec4 " + rsx::fragment_program::input_attrib_names[0] + ";\n";

		for (std::size_t index = 0; index < 22; ++index)
		{
			if (state.output_attributes & (1 << index))
			{
				result.code += "in vec4 " + rsx::vertex_program::output_attrib_names[index] + ";\n";
			}
		}

		{
			auto make_comparsion_test = [](rsx::comparaison_function compare_func, const std::string &test, const std::string &a, const std::string &b) -> std::string
			{
				if (compare_func == rsx::comparaison_function::always)
				{
					return{};
				}

				if (compare_func == rsx::comparaison_function::never)
				{
					return "\tdiscard;\n";
				}

				std::string compare;

				switch (compare_func)
				{
				case rsx::comparaison_function::equal:
					compare = "==";
					break;

				case rsx::comparaison_function::not_equal:
					compare = "!=";
					break;

				case rsx::comparaison_function::less_or_equal:
					compare = "<=";
					break;

				case rsx::comparaison_function::less:
					compare = "<";
					break;

				case rsx::comparaison_function::greater:
					compare = ">";
					break;

				case rsx::comparaison_function::greater_or_equal:
					compare = ">=";
					break;
				}

				return "\tif (" + test + "!(" + a + " " + compare + " " + b + ")) discard;\n";
			};

			for (u8 index = 0; index < 16; ++index)
			{
				if (state.textures_alpha_kill[index])
				{
					std::string index_string = std::to_string(index);
					std::string fetch_texture = "texture_fetch(" + index_string + ", tex" + index_string + " * ftexture" + index_string + "_cm).a";
					finalize += make_comparsion_test((rsx::comparaison_function)state.textures_zfunc[index], "", "0", fetch_texture);
				}
			}

			finalize += make_comparsion_test((rsx::comparaison_function)state.alpha_func, "alpha_test != 0 && ", "ocol.a", "alpha_ref");
		}
		break;

	case rsx::program_type::vertex:
		for (const rsx::texture_info& texture : shader.textures)
		{
			result.code += "uniform vec4 " + texture.name + "_cm = vec4(1.0);\n";

			rsx::texture_target target = state.vertex_textures[texture.id];

			result.code += "uniform sampler";

			switch (target)
			{
			default:
			case rsx::texture_target::_1: result.code += "1D"; break;
			case rsx::texture_target::_2: result.code += "2D"; break;
			case rsx::texture_target::_3: result.code += "3D"; break;
			case rsx::texture_target::cube: result.code += "Cube"; break;
			}
			result.code += " " + texture.name + ";\n";
		}

		insert_texture_lod_fetch_function(result.code, shader, state);

		result.code += "out vec4 wpos;\n";

		finalize +=
			"	wpos = window_matrix * viewport_matrix * vec4(o0.xyz, 1.0);\n"
			"	gl_Position = normalize_matrix * vec4(wpos.xyz, 1.0);\n"
			"	gl_Position.w = o0.w;\n";

		{
			std::string code_end;

			for (std::size_t index = 0; index < 16; ++index)
			{
				if (shader.input_attributes & (1 << index))
				{
					const std::string &attrib_name = rsx::vertex_program::input_attrib_names[index];

					result.code += "uniform ";

					if (state.is_int & (1 << index))
					{
						result.code += "isamplerBuffer ";
						code_end += "ivec4 ";
					}
					else
					{
						result.code += "samplerBuffer ";
						code_end += "vec4 ";
					}

					result.code += attrib_name + "_buffer" + ";\n";

					code_end += attrib_name + ";\n";

					std::string vertex_id;

					if (state.frequency[index] == 1)
					{
						if (state.divider_op & (1 << index))
						{
							vertex_id += "0";
						}
						else
						{
							vertex_id += "gl_VertexID";
						}
					}
					else
					{
						vertex_id = "gl_VertexID";

						if (state.frequency[index])
						{
							if (state.divider_op & (1 << index))
							{
								vertex_id += " % ";
							}
							else
							{
								vertex_id += " / ";
							}

							vertex_id += std::to_string(state.frequency[index]);
						}
					}

					prepare += '\t' + attrib_name + " = texelFetch(" + attrib_name + "_buffer, " + vertex_id + ");\n";
				}
			}

			result.code += code_end;
		}

		{
			if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_FOG)
			{
				result.code += "out vec4 fog;\n";
				finalize += "\tfog = o5.xxxx;\n";
			}

			auto map_register = [&](int to, int from)
			{
				if (shader.output_attributes & (1 << from))
				{
					result.code += "out vec4 " + rsx::vertex_program::output_attrib_names[to] + ";\n";
					finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = o" + std::to_string(from) + ";\n";
				}
				else if (state.output_attributes & (1 << to))
				{
					result.code += "out vec4 " + rsx::vertex_program::output_attrib_names[to] + ";\n";

					if (to == CELL_GCM_ATTRIB_OUTPUT_BACKDIFFUSE && shader.output_attributes & (1 << 1))
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = o1;\n";
					}
					else if (to == CELL_GCM_ATTRIB_OUTPUT_BACKSPECULAR && shader.output_attributes & (1 << 2))
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = o2;\n";
					}
					else
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = vec4(0.0);\n";
					}
				}
			};

			map_register(CELL_GCM_ATTRIB_OUTPUT_FRONTDIFFUSE, 1);
			map_register(CELL_GCM_ATTRIB_OUTPUT_FRONTSPECULAR, 2);
			map_register(CELL_GCM_ATTRIB_OUTPUT_BACKDIFFUSE, 3);
			map_register(CELL_GCM_ATTRIB_OUTPUT_BACKSPECULAR, 4);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX0, 7);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX1, 8);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX2, 9);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX3, 10);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX4, 11);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX5, 12);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX6, 13);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX7, 14);
			map_register(CELL_GCM_ATTRIB_OUTPUT_TEX8, 15);

			if (shader.output_attributes & (1 << 5))
			{
				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC0)
				{
					result.code += "uniform int uc_m0 = 0;\n";
					finalize += "\tgl_ClipDistance[0] = uc_m0 * o5.y;\n";
				}

				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC1)
				{
					result.code += "uniform int uc_m1 = 0;\n";
					finalize += "\tgl_ClipDistance[1] = uc_m1 * o5.z;\n";
				}

				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC2)
				{
					result.code += "uniform int uc_m2 = 0;\n";
					finalize += "\tgl_ClipDistance[2] = uc_m2 * o5.w;\n";
				}
			}

			if (shader.output_attributes & (1 << 6))
			{
				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_POINTSIZE)
				{
					finalize += "\tgl_PointSize = o6.x;\n";
				}

				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC3)
				{
					result.code += "uniform int uc_m3 = 0;\n";
					finalize += "\tgl_ClipDistance[3] = uc_m3 * o6.y;\n";
				}

				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC4)
				{
					result.code += "uniform int uc_m4 = 0;\n";
					finalize += "\tgl_ClipDistance[4] = uc_m4 * o6.z;\n";
				}

				if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_UC4)
				{
					result.code += "uniform int uc_m5 = 0;\n";
					finalize += "\tgl_ClipDistance[5] = uc_m5 * o6.w;\n";
				}
			}
		}
		break;

	default:
		throw;
	}

	result.code += "\n";
	result.code += shader.code;

	result.code += "void main()\n{\n" + prepare + "\n\t" + shader.entry_function + "();\n\n" + finalize + "}";
	return result;
}

void* glsl_compile_shader(rsx::program_type type, const std::string &code)
{
	gl::glsl::shader *result = new gl::glsl::shader();

	result->create(type == rsx::program_type::vertex ? ::gl::glsl::shader::type::vertex : ::gl::glsl::shader::type::fragment);
	result->source(code);
	result->compile();

	return result;
}

void* glsl_make_program(const void *vertex_shader, const void *fragment_shader)
{
	gl::glsl::program *result = new gl::glsl::program();

	result->create();
	result->attach(*(gl::glsl::shader*)vertex_shader);
	result->attach(*(gl::glsl::shader*)fragment_shader);

	result->link();

	return result;
}

void glsl_remove_program(void *buf)
{
	delete (gl::glsl::program*)buf;
}

void glsl_remove_shader(void *buf)
{
	delete (gl::glsl::shader*)buf;
}

void init_glsl_cache_program_context(rsx::program_cache_context &ctxt)
{
	ctxt.compile_shader = glsl_compile_shader;
	ctxt.complete_shader = glsl_complete_shader;
	ctxt.make_program = glsl_make_program;
	ctxt.remove_program = glsl_remove_program;
	ctxt.remove_shader = glsl_remove_shader;
	ctxt.lang = rsx::decompile_language::glsl;
}
