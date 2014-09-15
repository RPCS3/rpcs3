#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "GLProgram.h"
#include "GLGSRender.h"

GLProgram::GLProgram() : id(0)
{
}

int GLProgram::GetLocation(const std::string& name)
{
	for(u32 i=0; i<m_locations.size(); ++i)
	{
		if(!m_locations[i].name.compare(name))
		{
			return m_locations[i].loc;
		}
	}
	
	m_locations.emplace_back();
	u32 pos = m_locations.size()-1;
	m_locations[pos].name = name;

	m_locations[pos].loc = glGetUniformLocation(id, name.c_str());
	checkForGlError(fmt::Format("glGetUniformLocation(0x%x, %s)", id, name.c_str()));
	return m_locations[pos].loc;
}

bool GLProgram::IsCreated() const
{
	return id > 0;
}

void GLProgram::Create(const u32 vp, const u32 fp)
{
	if(IsCreated()) Delete();
	id = glCreateProgram();

	glAttachShader(id, vp);
	glAttachShader(id, fp);

	glLinkProgram(id);

	GLint linkStatus = GL_FALSE;
	glGetProgramiv(id, GL_LINK_STATUS, &linkStatus);
	if(linkStatus != GL_TRUE)
	{
		GLint bufLength = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &bufLength);

		if (bufLength)
		{
			char* buf = new char[bufLength+1]();
			glGetProgramInfoLog(id, bufLength, NULL, buf);
			LOG_ERROR(RSX, "Could not link program: %s", buf);
			delete[] buf;

			return;
		}
	}
	//else LOG_NOTICE(HLE, "program linked!");

	glGetProgramiv(id, GL_VALIDATE_STATUS, &linkStatus);
	if(linkStatus != GL_TRUE)
	{
		GLint bufLength = 0;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &bufLength);

		if (bufLength)
		{
			char* buf = new char[bufLength]();
			glGetProgramInfoLog(id, bufLength, NULL, buf);
			LOG_ERROR(RSX, "Could not link program: %s", buf);
			delete[] buf;

			return;
		}
	}
}

void GLProgram::UnUse()
{
	id = 0;
	m_locations.clear();
}

void GLProgram::Use()
{
	glUseProgram(id);
	checkForGlError("glUseProgram");
}

void GLProgram::SetTex(u32 index)
{
	int loc = GetLocation(fmt::Format("tex%u", index));
	glProgramUniform1i(id, loc, index);
	checkForGlError(fmt::Format("SetTex(%u - %d - %d)", id, index, loc));
}

void GLProgram::Delete()
{
	if(!IsCreated()) return;
	glDeleteProgram(id);
	id = 0;
	m_locations.clear();
}
