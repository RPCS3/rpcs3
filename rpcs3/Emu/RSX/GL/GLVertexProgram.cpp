#include "stdafx.h"
#include "Emu/System.h"

#include "GLVertexProgram.h"
#include "GLCommonDecompiler.h"
#include "GLHelpers.h"
#include "../GCM.h"

#include <algorithm>

std::string GLVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string GLVertexDecompilerThread::getIntTypeName(size_t elementCount)
{
	return "ivec4";
}

std::string GLVertexDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string GLVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void GLVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 430\n";
	OS << "layout(std140, binding = 0) uniform VertexContextBuffer\n";
	OS << "{\n";
	OS << "	mat4 scale_offset_mat;\n";
	OS << "	ivec4 user_clip_enabled[2];\n";
	OS << "	vec4 user_clip_factor[2];\n";
	OS << "	uint transform_branch_bits;\n";
	OS << "	uint vertex_base_index;\n";
	OS << "	ivec4 input_attributes[16];\n";
	OS << "};\n\n";
}

void GLVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "layout(location=0) uniform usamplerBuffer persistent_input_stream;\n";    //Data stream with persistent vertex data (cacheable)
	OS << "layout(location=1) uniform usamplerBuffer volatile_input_stream;\n";      //Data stream with per-draw data (registers and immediate draw data)
}

void GLVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "layout(std140, binding = 1) uniform VertexConstantsBuffer\n";
	OS << "{\n";
	OS << "	vec4 vc[468];\n";
	OS << "};\n\n";

	for (const ParamType &PT: constants)
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name == "vc[468]")
				continue;

			OS << "uniform " << PT.type << " " << PI.name << ";\n";
		}
	}
}

static const vertex_reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE },
	{ "spec_color", true, "dst_reg2", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR },
	//These are only present when back variants are specified, otherwise the default diff/spec color vars are for both front and back
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	//Fog output shares a data source register with clip planes 0-2 so only declare when specified
	{ "fog_c", true, "dst_reg5", ".xxxx", true, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FOG },
	//Warning: Always define all 3 clip plane groups together to avoid flickering with openGL
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y * user_clip_factor[0].x", false, "user_clip_enabled[0].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z * user_clip_factor[0].y", false, "user_clip_enabled[0].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w * user_clip_factor[0].z", false, "user_clip_enabled[0].z > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y * user_clip_factor[0].w", false, "user_clip_enabled[0].w > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z * user_clip_factor[1].x", false, "user_clip_enabled[1].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w * user_clip_factor[1].y", false, "user_clip_enabled[1].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 | CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
	{ "tc0", true, "dst_reg7", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX0 },
	{ "tc1", true, "dst_reg8", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX1 },
	{ "tc2", true, "dst_reg9", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX2 },
	{ "tc3", true, "dst_reg10", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX3 },
	{ "tc4", true, "dst_reg11", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX4 },
	{ "tc5", true, "dst_reg12", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX5 },
	{ "tc6", true, "dst_reg13", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX6 },
	{ "tc7", true, "dst_reg14", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX7 },
	{ "tc8", true, "dst_reg15", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX8 },
	{ "tc9", true, "dst_reg6", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX9 }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void GLVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	bool insert_front_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE) != 0;
	bool insert_back_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE) != 0;

	bool insert_front_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR) != 0;
	bool insert_back_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR) != 0;

	bool front_back_diffuse = (insert_back_diffuse && insert_front_diffuse);
	bool front_back_specular = (insert_back_specular && insert_front_specular);

	std::vector<std::string> outputs_to_declare;

	for (auto &i : reg_table)
	{
		if (i.need_declare)
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

			outputs_to_declare.push_back(name);
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		outputs_to_declare.push_back("front_diff_color");

	if (insert_back_specular && insert_front_specular)
		outputs_to_declare.push_back("front_spec_color");

	for (auto &name: outputs_to_declare)
	{
		OS << "layout(location=" << gl::get_varying_register_location(name) << ") out vec4 " << name << ";\n";
	}
}

void GLVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	insert_glsl_legacy_function(OS, glsl::glsl_vertex_program);
	glsl::insert_vertex_input_fetch(OS, glsl::glsl_rules_opengl4, gl::get_driver_caps().vendor_INTEL==false);

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

	OS << "void vs_main(" << parameters << ")\n";
	OS << "{\n";

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

			OS << ";\n";
		}
	}

	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	vec4 " << PI.name << "= read_location(" << std::to_string(PI.location) << ");\n";
		}
	}

	for (const ParamType &PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D")
		{
			for (const ParamItem &PI : PT.items)
			{
				OS << "	vec2 " << PI.name << "_coord_scale = vec2(1.);\n";
			}
		}
	}
}

void GLVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main ()\n";
	OS << "{\n";

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

					OS << ";\n";
				}
			}
		}
	}

	OS << "\n" << "	vs_main(" << parameters << ");\n\n";

	bool insert_front_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE) != 0;
	bool insert_front_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR) != 0;

	bool insert_back_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE) != 0;
	bool insert_back_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR) != 0;

	bool front_back_diffuse = (insert_back_diffuse && insert_front_diffuse);
	bool front_back_specular = (insert_back_specular && insert_front_specular);

	for (auto &i : reg_table)
	{
		std::string name = i.name;

		if (front_back_diffuse && name == "diff_color")
			name = "back_diff_color";

		if (front_back_specular && name == "spec_color")
			name = "back_spec_color";

		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
		{
			if (i.check_mask && (rsx_vertex_program.output_mask & i.check_mask_value) == 0)
				continue;

			if (i.name == "front_diff_color")
				insert_front_diffuse = false;

			if (i.name == "front_spec_color")
				insert_front_specular = false;

			std::string condition = (!i.cond.empty()) ? "(" + i.cond + ") " : "";
			if (condition.empty() || i.default_val.empty())
			{
				if (!condition.empty()) condition = "if " + condition;
				OS << "	" << condition << name << " = " << i.src_reg << i.src_reg_mask << ";\n";
			}
			else
			{
				//Insert if-else condition
				OS << "	" << name << " = " << condition << "? " << i.src_reg << i.src_reg_mask << ": " << i.default_val << ";\n";
			}
		}
		else if (i.need_declare && (rsx_vertex_program.output_mask & i.check_mask_value) > 0)
		{
			//An output was declared but nothing was written to it
			//Set it to all ones (Atelier Escha)
			OS << "	" << name << " = vec4(1.);\n";
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg1"))
			OS << "	front_diff_color = dst_reg1;\n";

	if (insert_back_specular && insert_front_specular)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg2"))
			OS << "	front_spec_color = dst_reg2;\n";

	OS << "	gl_Position = gl_Position * scale_offset_mat;\n";

	//Since our clip_space is symetrical [-1, 1] we map it to linear space using the eqn:
	//ln = (clip * 2) - 1 to fully utilize the 0-1 range of the depth buffer
	//RSX matrices passed already map to the [0, 1] range but mapping to classic OGL requires that we undo this step
	//This can be made unnecessary using the call glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE).
	//However, ClipControl only made it to opengl core in ver 4.5 though, so this is a workaround.
	
	//NOTE: It is completely valid for games to use very large w values, causing the post-multiplied z to be in the hundreds
	//It is therefore critical that this step is done post-transform and the result re-scaled by w
	//SEE Naruto: UNS
	
	//NOTE: On GPUs, poor fp32 precision means dividing z by w, then multiplying by w again gives slightly incorrect results
	//This equation is simplified algebraically to an addition and subreaction which gives more accurate results (Fixes flickering skybox in Dark Souls 2)
	//OS << "	float ndc_z = gl_Position.z / gl_Position.w;\n";
	//OS << "	ndc_z = (ndc_z * 2.) - 1.;\n";
	//OS << "	gl_Position.z = ndc_z * gl_Position.w;\n";
	OS << "	gl_Position.z = (gl_Position.z + gl_Position.z) - gl_Position.w;\n";
	OS << "}\n";
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
