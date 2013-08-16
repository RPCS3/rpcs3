#include "stdafx.h"
#include "Program.h"
#include "GLGSRender.h"

Program::Program() : id(0)
{
}

int Program::GetLocation(const wxString& name)
{
	for(u32 i=0; i<m_locations.GetCount(); ++i)
	{
		if(!m_locations[i].name.Cmp(name))
		{
			return m_locations[i].loc;
		}
	}
	
	u32 pos = m_locations.Move(new Location());
	m_locations[pos].name = name;

	m_locations[pos].loc = glGetUniformLocation(id, name);
	checkForGlError(wxString::Format("glGetUniformLocation(0x%x, %s)", id, name));
	return m_locations[pos].loc;
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

void Program::UnUse()
{
	id = 0;
}

void Program::Use()
{
	glUseProgram(id);
	checkForGlError("glUseProgram");
}

void Program::SetTex(u32 index)
{
	int loc = GetLocation(wxString::Format("tex%d", index));
	checkForGlError(wxString::Format("GetLocation(tex%d)", index));
	glProgramUniform1i(id, loc, index);
	GLenum err = glGetError();
	if(err != 0x502)
	{
		printGlError(err, wxString::Format("SetTex(%d - %d - %d)", id, index, loc));
	}
}

void Program::Delete()
{
	if(!IsCreated()) return;
	glDeleteProgram(id);
	id = 0;
}