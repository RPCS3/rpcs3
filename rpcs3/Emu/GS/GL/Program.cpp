#include "stdafx.h"
#include "Program.h"
#include "GLGSRender.h"

Program::Program() : id(0)
{
}

bool Program::IsCreated() const
{
	return id > 0;
}

void Program::Create(const u32 vp, const u32 fp)
{
	if(IsCreated()) Delete();
	id = glCreateProgram();

	glAttachShader(id, vp);
	glAttachShader(id, fp);

	glBindFragDataLocation(id, 0, "r0");
	static const wxString reg_table[] = 
	{
		"in_pos", "in_weight", "in_normal",
		"in_col0", "in_col1",
		"in_fogc",
		"in_6", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	for(u32 i=0; i<WXSIZEOF(reg_table); ++i)
	{
		glBindAttribLocation(id, i, reg_table[i]);
		checkForGlError("glBindAttribLocation");
	}

	glLinkProgram(id);

	GLint linkStatus = GL_FALSE;
	glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
	if(linkStatus != GL_TRUE)
	{
		GLint bufLength = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &bufLength);

		if (bufLength)
		{
			char* buf = new char[bufLength+1];
			memset(buf, 0, bufLength+1);
			glGetProgramInfoLog(id, bufLength, NULL, buf);
			ConLog.Error("Could not link program: %s", buf);
			delete[] buf;
		}
	}
	//else ConLog.Write("program linked!");

	glGetProgramiv(id, GL_VALIDATE_STATUS, &linkStatus);
	if(linkStatus != GL_TRUE)
	{
		GLint bufLength = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &bufLength);

		if (bufLength)
		{
			char* buf = new char[bufLength];
			memset(buf, 0, bufLength);
			glGetProgramInfoLog(id, bufLength, NULL, buf);
			ConLog.Error("Could not link program: %s", buf);
			delete[] buf;
		}
	}
}

void Program::Use()
{
	glUseProgram(id);
	checkForGlError("glUseProgram");
}

void Program::SetTex(u32 index)
{
	glUniform1i(glGetUniformLocation(id, wxString::Format("tex_%d", index)), index);
	checkForGlError(wxString::Format("SetTex(%d)", index));
}

void Program::Delete()
{
	if(!IsCreated()) return;
	glDeleteProgram(id);
	id = 0;
}