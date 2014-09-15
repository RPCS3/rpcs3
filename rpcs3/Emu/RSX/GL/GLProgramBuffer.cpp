#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"

#include "GLProgramBuffer.h"

int GLProgramBuffer::SearchFp(const RSXShaderProgram& rsx_fp, GLShaderProgram& gl_fp)
{
	for(u32 i=0; i<m_buf.size(); ++i)
	{
		if(memcmp(&m_buf[i].fp_data[0], vm::get_ptr<void>(rsx_fp.addr), m_buf[i].fp_data.size()) != 0) continue;

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
		LOG_NOTICE(RSX, "Get program (%d):", fp);
		LOG_NOTICE(RSX, "*** prog id = %d", m_buf[fp].prog_id);
		LOG_NOTICE(RSX, "*** vp id = %d", m_buf[fp].vp_id);
		LOG_NOTICE(RSX, "*** fp id = %d", m_buf[fp].fp_id);

		LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[fp].vp_shader.wx_str());
		LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[fp].fp_shader.wx_str());
		*/
		return m_buf[fp].prog_id;
	}

	for(u32 i=0; i<m_buf.size(); ++i)
	{
		if(i == fp || i == vp) continue;

		if(CmpVP(vp, i) && CmpFP(fp, i))
		{
			/*
			LOG_NOTICE(RSX, "Get program (%d):", i);
			LOG_NOTICE(RSX, "*** prog id = %d", m_buf[i].prog_id);
			LOG_NOTICE(RSX, "*** vp id = %d", m_buf[i].vp_id);
			LOG_NOTICE(RSX, "*** fp id = %d", m_buf[i].fp_id);

			LOG_NOTICE(RSX, "*** vp shader = \n%s", m_buf[i].vp_shader.wx_str());
			LOG_NOTICE(RSX, "*** fp shader = \n%s", m_buf[i].fp_shader.wx_str());
			*/
			return m_buf[i].prog_id;
		}
	}

	return 0;
}

void GLProgramBuffer::Add(GLProgram& prog, GLShaderProgram& gl_fp, RSXShaderProgram& rsx_fp, GLVertexProgram& gl_vp, RSXVertexProgram& rsx_vp)
{
	GLBufferInfo new_buf;

	LOG_NOTICE(RSX, "Add program (%d):", m_buf.size());
	LOG_NOTICE(RSX, "*** prog id = %d", prog.id);
	LOG_NOTICE(RSX, "*** vp id = %d", gl_vp.id);
	LOG_NOTICE(RSX, "*** fp id = %d", gl_fp.GetId());
	LOG_NOTICE(RSX, "*** vp data size = %d", rsx_vp.data.size() * 4);
	LOG_NOTICE(RSX, "*** fp data size = %d", rsx_fp.size);

	LOG_NOTICE(RSX, "*** vp shader = \n%s", gl_vp.shader.c_str());
	LOG_NOTICE(RSX, "*** fp shader = \n%s", gl_fp.GetShaderText().c_str());
	

	new_buf.prog_id = prog.id;
	new_buf.vp_id = gl_vp.id;
	new_buf.fp_id = gl_fp.GetId();

	new_buf.fp_data.insert(new_buf.fp_data.end(), vm::get_ptr<u8>(rsx_fp.addr), vm::get_ptr<u8>(rsx_fp.addr + rsx_fp.size));
	new_buf.vp_data = rsx_vp.data;

	new_buf.vp_shader = gl_vp.shader;
	new_buf.fp_shader = gl_fp.GetShaderText();

	m_buf.push_back(new_buf);
}

void GLProgramBuffer::Clear()
{
	for(u32 i=0; i<m_buf.size(); ++i)
	{
		glDetachShader(m_buf[i].prog_id, m_buf[i].fp_id);
		glDetachShader(m_buf[i].prog_id, m_buf[i].vp_id);
		glDeleteShader(m_buf[i].fp_id);
		glDeleteShader(m_buf[i].vp_id);
		glDeleteProgram(m_buf[i].prog_id);

		m_buf[i].fp_data.clear();
		m_buf[i].vp_data.clear();

		m_buf[i].vp_shader.clear();
		m_buf[i].fp_shader.clear();
	}

	m_buf.clear();
}
