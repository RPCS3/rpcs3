#include "stdafx.h"
#include "rsx_gl_cache.h"
#include "gl_helpers.h"
#include "../GCM.h"

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

	for (const rsx::texture_info& texture : shader.textures)
	{
		result.code += "uniform vec4 " + texture.name + "_cm = vec4(1.0);\n";
		result.code += "uniform sampler2D " + texture.name + ";\n";
	}

	std::string prepare;
	std::string finalize;
	int location = 1;

	switch (shader.raw->type)
	{
	case rsx::program_type::fragment:
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
		break;

	case rsx::program_type::vertex:

		result.code += "out vec4 wpos;\n";
		
		// TODO
		if (0)
		{
			finalize += "\tgl_Position = o0;\n";
		}
		else
		{
			finalize +=
				"	wpos = window_matrix * viewport_matrix * vec4(o0.xyz, 1.0);\n"
				"	gl_Position = normalize_matrix * vec4(wpos.xyz, 1.0);\n"
				"	gl_Position.w = o0.w;\n";
		}

		{
			std::string code_end;

			for (std::size_t index = 0; index < 16; ++index)
			{
				if (shader.input_attributes & (1 << index))
				{
					// result.code += "in vec4 " + rsx::vertex_program::input_attrib_names[index] + ";\n";

					// TODO: use actual information about vertex inputs
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

					if (state.is_array & (1 << index))
					{
						vertex_id = "gl_VertexID";

						if (state.frequency[index] > 1)
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
					else
					{
						vertex_id = "0";
					}

					prepare += '\t' + attrib_name + " = texelFetch(" + attrib_name + "_buffer, " + vertex_id + ");\n";
				}
			}

			result.code += code_end;
		}

		{
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

					if ((1 << to) == CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE && shader.output_attributes & (1 << 1))
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = o1;\n";
					}
					else if ((1 << to) == CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR && shader.output_attributes & (1 << 2))
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = o2;\n";
					}
					else
					{
						finalize += "\t" + rsx::vertex_program::output_attrib_names[to] + " = vec4(0.0);\n";
					}
				}
			};

			map_register(0, 1);
			map_register(1, 2);
			map_register(2, 3);
			map_register(3, 4);
			map_register(14, 7);
			map_register(15, 8);
			map_register(16, 9);
			map_register(17, 10);
			map_register(18, 11);
			map_register(19, 12);
			map_register(20, 13);
			map_register(21, 14);
			map_register(12, 15);

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

			if (state.output_attributes & CELL_GCM_ATTRIB_OUTPUT_MASK_FOG)
			{
				//TODO
			}
		}
		break;

	default:
		throw;
	}

	result.code += "\n";
	result.code += shader.code;

	result.code += "void main()\n{\n" + prepare + "\t" + shader.entry_function + "();\n" + finalize + "}";
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
	result->validate();

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
