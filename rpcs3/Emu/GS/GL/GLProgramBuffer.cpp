#include "stdafx.h"
#include "GLProgramBuffer.h"

int GLProgramBuffer::SearchFp(const RSXShaderProgram& rsx_fp, GLShaderProgram& gl_fp)
{
	for(u32 i=0; i<m_buf.size(); ++i)
	{
		if(memcmp(&m_buf[i].fp_data[0], &Memory[rsx_fp.addr], m_buf[i].fp_data.size()) != 0) continue;

		gl_fp.SetId(m_buf[i].fp_id);
		gl_fp.SetShaderText(m_buf[i].fp_shader);

		return i;
	}

	return -1;
}

int GLProgramBuffer::SearchVp(const RSXVertexProgram& rsx_vp, GLVertexProgram& gl_vp)
{
	for(u32 i=0; i<m_buf.size(); ++i)
	{
		if(m_buf[i].vp_data.size() != rsx_vp.data.size()) continue;
		if(memcmp(m_buf[i].vp_data.data(), rsx_vp.data.data(), rsx_vp.data.size() * 4) != 0) continue;

		gl_vp.id = m_buf[i].vp_id;
		gl_vp.shader = m_buf[i].vp_shader.c_str();

		return i;
	}

	return -1;
}

bool GLProgramBuffer::CmpVP(const u32 a, const u32 b) const
{
	if(m_buf[a].vp_data.size() != m_buf[b].vp_data.size()) return false;
	return memcmp(m_buf[a].vp_data.data(), m_buf[b].vp_data.data(), m_buf[a].vp_data.size() * 4) == 0;
}

bool GLProgramBuffer::CmpFP(const u32 a, const u32 b) const
{
	if(m_buf[a].fp_data.size() != m_buf[b].fp_data.size()) return false;
	return memcmp(m_buf[a].fp_data.data(), m_buf[b].fp_data.data(), m_buf[a].fp_data.size()) == 0;
}

u32 GLProgramBuffer::GetProg(u32 fp, u32 vp) const
{
	if(fp == vp)
	{
		/*
		ConLog.Write("Get program (%d):", fp);
		ConLog.Write("*** prog id = %d", m_buf[fp].prog_id);
		ConLog.Write("*** vp id = %d", m_buf[fp].vp_id);
		ConLog.Write("*** fp id = %d", m_buf[fp].fp_id);

		ConLog.Write("*** vp shader = \n%s", m_buf[fp].vp_shader.wx_str());
		ConLog.Write("*** fp shader = \n%s", m_buf[fp].fp_shader.wx_str());
		*/
		return m_buf[fp].prog_id;
	}

	for(u32 i=0; i<m_buf.size(); ++i)
	{
		if(i == fp || i == vp) continue;

		if(CmpVP(vp, i) && CmpFP(fp, i))
		{
			/*
			ConLog.Write("Get program (%d):", i);
			ConLog.Write("*** prog id = %d", m_buf[i].prog_id);
			ConLog.Write("*** vp id = %d", m_buf[i].vp_id);
			ConLog.Write("*** fp id = %d", m_buf[i].fp_id);

			ConLog.Write("*** vp shader = \n%s", m_buf[i].vp_shader.wx_str());
			ConLog.Write("*** fp shader = \n%s", m_buf[i].fp_shader.wx_str());
			*/
			return m_buf[i].prog_id;
		}
	}

	return 0;
}

void GLProgramBuffer::Add(GLProgram& prog, GLShaderProgram& gl_fp, RSXShaderProgram& rsx_fp, GLVertexProgram& gl_vp, RSXVertexProgram& rsx_vp)
{
	GLBufferInfo new_buf;

	ConLog.Write("Add program (%d):", m_buf.size());
	ConLog.Write("*** prog id = %d", prog.id);
	ConLog.Write("*** vp id = %d", gl_vp.id);
	ConLog.Write("*** fp id = %d", gl_fp.GetId());
	ConLog.Write("*** vp data size = %d", rsx_vp.data.size() * 4);
	ConLog.Write("*** fp data size = %d", rsx_fp.size);

	ConLog.Write("*** vp shader = \n%s", gl_vp.shader.c_str());
	ConLog.Write("*** fp shader = \n%s", gl_fp.GetShaderText().c_str());
	

	new_buf.prog_id = prog.id;
	new_buf.vp_id = gl_vp.id;
	new_buf.fp_id = gl_fp.GetId();

	new_buf.fp_data.insert(new_buf.fp_data.end(),&Memory[rsx_fp.addr], &Memory[rsx_fp.addr] + rsx_fp.size);
	new_buf.vp_data = rsx_vp.data;

	new_buf.vp_shader = gl_vp.shader;
	new_buf.fp_shader = gl_fp.GetShaderText();

	m_buf.push_back(new_buf);
}

void GLProgramBuffer::Clear()
{
	for(u32 i=0; i<m_buf.size(); ++i)
	{
		glDeleteProgram(m_buf[i].prog_id);
		glDeleteShader(m_buf[i].fp_id);
		glDeleteShader(m_buf[i].vp_id);

		m_buf[i].fp_data.clear();
		m_buf[i].vp_data.clear();

		m_buf[i].vp_shader.clear();
		m_buf[i].fp_shader.clear();
	}

	m_buf.clear();
}
