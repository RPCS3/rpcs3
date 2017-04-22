#include "stdafx.h"
#include "Emu/System.h"

#include "GLVertexProgram.h"
#include "GLCommonDecompiler.h"
#include "../GCM.h"

#include <algorithm>

std::string GLVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImpl(elementCount);
}

std::string GLVertexDecompilerThread::getIntTypeName(size_t elementCount)
{
	return "ivec4";
}


std::string GLVertexDecompilerThread::getFunction(FUNCTION f)
{
	return getFunctionImpl(f);
}

std::string GLVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImpl(f, Op0, Op1);
}

void GLVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 430" << std::endl << std::endl;
	OS << "layout(std140, binding = 0) uniform ScaleOffsetBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	mat4 scaleOffsetMat;" << std::endl;
	OS << "	vec4 userClip[2];" << std::endl;
	OS << "};" << std::endl;
}

void GLVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	std::vector<std::tuple<size_t, std::string>> input_data;
	for (const ParamType &PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
		{
			input_data.push_back(std::make_tuple(PI.location, PI.name));
		}
	}

	/**
	 * Its is important that the locations are in the order that vertex attributes are expected.
	 * If order is not adhered to, channels may be swapped leading to corruption
	*/

	std::sort(input_data.begin(), input_data.end());

	int location = 1;
	for (const std::tuple<size_t, std::string>& item : input_data)
	{
		for (const ParamType &PT : inputs)
		{
			for (const ParamItem &PI : PT.items)
			{
				if (PI.name == std::get<1>(item))
				{
					bool is_int = false;
					for (const auto &attrib : rsx_vertex_program.rsx_vertex_inputs)
					{
						if (attrib.location == std::get<0>(item))
						{
							if (attrib.int_type || attrib.flags & GL_VP_SINT_MASK) is_int = true;
							break;
						}
					}

					std::string samplerType = is_int ? "isamplerBuffer" : "samplerBuffer";
					OS << "layout(location=" << location++ << ")" << "	uniform " << samplerType << " " << PI.name << "_buffer;" << std::endl;
				}
			}
		}
	}

	for (int i = 0; i <= 5; i++)
	{
		OS << "uniform int uc_m" + std::to_string(i) + "= 0;\n";
	}
}

void GLVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "layout(std140, binding = 1) uniform VertexConstantsBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	vec4 vc[468];" << std::endl;
	OS << "	uint transform_branch_bits;" << std::endl;
	OS << "};" << std::endl << std::endl;

	for (const ParamType &PT: constants)
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name == "vc[468]")
				continue;

			OS << "uniform " << PT.type << " " << PI.name << ";" << std::endl;
		}
	}
}

struct reg_info
{
	std::string name;
	bool need_declare;
	std::string src_reg;
	std::string src_reg_mask;
	bool need_cast;
};

static const reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fog_c", true, "dst_reg5", ".xxxx", true },
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y * userClip[0].x", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z * userClip[0].y", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w * userClip[0].z", false },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y * userClip[0].w", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z * userClip[1].x", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w * userClip[1].y", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void GLVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	bool insert_front_diffuse = (rsx_vertex_program.output_mask & 1);
	bool insert_back_diffuse = (rsx_vertex_program.output_mask & 4);

	bool insert_front_specular = (rsx_vertex_program.output_mask & 2);
	bool insert_back_specular = (rsx_vertex_program.output_mask & 8);

	bool front_back_diffuse = (insert_back_diffuse && insert_front_diffuse);
	bool front_back_specular = (insert_back_specular && insert_front_specular);

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg) && i.need_declare)
		{
			if (i.name == "front_diff_color")
				insert_front_diffuse = false;

			if (i.name == "front_spec_color")
				insert_front_specular = false;

			std::string name = i.name;

			if (front_back_diffuse && name == "diff_color")
				name = "back_diff_color";

			if (front_back_specular && name == "spec_color")
				name = "back_spec_color";

			OS << "out vec4 " << name << ";" << std::endl;
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		OS << "out vec4 front_diff_color;" << std::endl;

	if (insert_back_specular && insert_front_specular)
		OS << "out vec4 front_spec_color;" << std::endl;
}

void add_input(std::stringstream & OS, const ParamItem &PI, const std::vector<rsx_vertex_input> &inputs)
{
	for (const auto &real_input : inputs)
	{
		if (real_input.location != PI.location)
			continue;

		std::string vecType = "	vec4 ";
		if (real_input.int_type)
			vecType = "	ivec4 ";

		std::string scale = "";
		if (real_input.flags & GL_VP_SINT_MASK)
		{
			if (real_input.flags & GL_VP_ATTRIB_S16_INT)
				scale = " / 32767.";
			else
				scale = " / 2147483647.";
		}

		if (!real_input.is_array)
		{
			OS << vecType << PI.name << " = texelFetch(" << PI.name << "_buffer, 0)" << scale << ";" << std::endl;
			return;
		}

		if (real_input.frequency > 1)
		{
			if (real_input.is_modulo)
			{
				OS << vecType << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexID %" << real_input.frequency << ")" << scale << ";" << std::endl;
				return;
			}

			OS << vecType << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexID /" << real_input.frequency << ")" << scale << ";" << std::endl;
			return;
		}

		OS << vecType << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexID)" << scale << ";" << std::endl;
		return;
	}

	LOG_WARNING(RSX, "Vertex input %s does not have a matching vertex_input declaration", PI.name.c_str());
	
	OS << "	vec4 " << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexID);" << std::endl;
}

void GLVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	insert_glsl_legacy_function(OS);

	std::string parameters = "";
	for (int i = 0; i < 16; ++i)
	{
		std::string reg_name = "dst_reg" + std::to_string(i);
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", reg_name))
		{
			if (parameters.length())
				parameters += ", ";

			parameters += "inout vec4 " + reg_name;
		}
	}

	OS << "void vs_main(" << parameters << ")" << std::endl;
	OS << "{" << std::endl;

	//Declare temporary registers, ignoring those mapped to outputs
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name.substr(0, 7) == "dst_reg")
				continue;

			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;

			OS << ";" << std::endl;
		}
	}

	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
			add_input(OS, PI, rsx_vertex_program.rsx_vertex_inputs);
	}

	for (const ParamType &PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D")
		{
			for (const ParamItem &PI : PT.items)
			{
				OS << "	vec2 " << PI.name << "_coord_scale = vec2(1.);" << std::endl;
			}
		}
	}
}

void GLVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}" << std::endl << std::endl;

	OS << "void main ()" << std::endl;
	OS << "{" << std::endl;

	std::string parameters = "";
	
	if (ParamType *vec4Types = m_parr.SearchParam(PF_PARAM_NONE, "vec4"))
	{
		for (int i = 0; i < 16; ++i)
		{
			std::string reg_name = "dst_reg" + std::to_string(i);
			for (auto &PI : vec4Types->items)
			{
				if (reg_name == PI.name)
				{
					if (parameters.length())
						parameters += ", ";

					parameters += reg_name;
					OS << "	vec4 " << reg_name;

					if (!PI.value.empty())
						OS << "= " << PI.value;

					OS << ";" << std::endl;
				}
			}
		}
	}

	OS << std::endl << "	vs_main(" << parameters << ");" << std::endl << std::endl;

	bool insert_front_diffuse = (rsx_vertex_program.output_mask & 1);
	bool insert_front_specular = (rsx_vertex_program.output_mask & 2);

	bool insert_back_diffuse = (rsx_vertex_program.output_mask & 4);
	bool insert_back_specular = (rsx_vertex_program.output_mask & 8);

	bool front_back_diffuse = (insert_back_diffuse && insert_front_diffuse);
	bool front_back_specular = (insert_back_specular && insert_front_specular);

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
		{
			if (i.name == "front_diff_color")
				insert_front_diffuse = false;

			if (i.name == "front_spec_color")
				insert_front_specular = false;

			std::string name = i.name;

			if (front_back_diffuse && name == "diff_color")
				name = "back_diff_color";

			if (front_back_specular && name == "spec_color")
				name = "back_spec_color";

			OS << "	" << name << " = " << i.src_reg << i.src_reg_mask << ";" << std::endl;
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg1"))
			OS << "	front_diff_color = dst_reg1;\n";

	if (insert_back_specular && insert_front_specular)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg2"))
			OS << "	front_spec_color = dst_reg2;\n";

	OS << "	gl_Position = gl_Position * scaleOffsetMat;" << std::endl;
	OS << "}" << std::endl;
}


void GLVertexDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLVertexProgram::GLVertexProgram()
{
}

GLVertexProgram::~GLVertexProgram()
{
	Delete();
}

void GLVertexProgram::Decompile(const RSXVertexProgram& prog)
{
	GLVertexDecompilerThread decompiler(prog, shader, parr);
	decompiler.Task();
}

void GLVertexProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.c_str();
	const int strlen = ::narrow<int>(shader.length());

	fs::create_path(fs::get_config_dir() + "/shaderlog");
	fs::file(fs::get_config_dir() + "shaderlog/VertexProgram" + std::to_string(id) + ".glsl", fs::rewrite).write(str);

	glShaderSource(id, 1, &str, &strlen);
	glCompileShader(id);

	GLint r = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &r);
	if (r != GL_TRUE)
	{
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &r);

		if (r)
		{
			char* buf = new char[r + 1]();
			GLsizei len;
			glGetShaderInfoLog(id, r, &len, buf);
			LOG_ERROR(RSX, "Failed to compile vertex shader: %s", buf);
			delete[] buf;
		}

		LOG_NOTICE(RSX, "%s", shader.c_str());
		Emu.Pause();
	}
}

void GLVertexProgram::Delete()
{
	shader.clear();

	if (id)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLVertexProgram::Delete(): glDeleteShader(%d) avoided", id);
		}
		else
		{
			glDeleteShader(id);
		}
		id = 0;
	}
}
