#include "stdafx.h"
#include "ProgramBuffer.h"

int ProgramBuffer::SearchFp(ShaderProgram& fp)
{
	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(memcmp(&m_buf[i].fp_data[0], &Memory[fp.addr], m_buf[i].fp_data.GetCount()) != 0) continue;

		fp.id = m_buf[i].fp_id;
		fp.shader = m_buf[i].fp_shader.GetPtr();

		return i;
	}

	return -1;
}

int ProgramBuffer::SearchVp(VertexProgram& vp)
{
	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(m_buf[i].vp_data.GetCount() != vp.data.GetCount()) continue;
		if(memcmp(m_buf[i].vp_data.GetPtr(), vp.data.GetPtr(), vp.data.GetCount() * 4) != 0) continue;

		vp.id = m_buf[i].vp_id;
		vp.shader = m_buf[i].vp_shader.GetPtr();

		return i;
	}

	return -1;
}

bool ProgramBuffer::CmpVP(const u32 a, const u32 b) const
{
	if(m_buf[a].vp_data.GetCount() != m_buf[b].vp_data.GetCount()) return false;
	return memcmp(m_buf[a].vp_data.GetPtr(), m_buf[b].vp_data.GetPtr(), m_buf[a].vp_data.GetCount() * 4) == 0;
}

bool ProgramBuffer::CmpFP(const u32 a, const u32 b) const
{
	if(m_buf[a].fp_data.GetCount() != m_buf[b].fp_data.GetCount()) return false;
	return memcmp(m_buf[a].fp_data.GetPtr(), m_buf[b].fp_data.GetPtr(), m_buf[a].fp_data.GetCount()) == 0;
}

u32 ProgramBuffer::GetProg(u32 fp, u32 vp) const
{
	if(fp == vp)
	{
		/*
		ConLog.Write("Get program (%d):", fp);
		ConLog.Write("*** prog id = %d", m_buf[fp].prog_id);
		ConLog.Write("*** vp id = %d", m_buf[fp].vp_id);
		ConLog.Write("*** fp id = %d", m_buf[fp].fp_id);

		ConLog.Write("*** vp shader = \n%s", m_buf[fp].vp_shader);
		ConLog.Write("*** fp shader = \n%s", m_buf[fp].fp_shader);
		*/
		return m_buf[fp].prog_id;
	}

	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(i == fp || i == vp) continue;

		if(CmpVP(vp, i) && CmpFP(fp, i))
		{
			/*
			ConLog.Write("Get program (%d):", i);
			ConLog.Write("*** prog id = %d", m_buf[i].prog_id);
			ConLog.Write("*** vp id = %d", m_buf[i].vp_id);
			ConLog.Write("*** fp id = %d", m_buf[i].fp_id);

			ConLog.Write("*** vp shader = \n%s", m_buf[i].vp_shader);
			ConLog.Write("*** fp shader = \n%s", m_buf[i].fp_shader);
			*/
			return m_buf[i].prog_id;
		}
	}

	return 0;
}

void ProgramBuffer::Add(Program& prog, ShaderProgram& fp, VertexProgram& vp)
{
	BufferInfo& new_buf = *new BufferInfo();

	ConLog.Write("Add program (%d):", m_buf.GetCount());
	ConLog.Write("*** prog id = %d", prog.id);
	ConLog.Write("*** vp id = %d", vp.id);
	ConLog.Write("*** fp id = %d", fp.id);
	ConLog.Write("*** vp data size = %d", vp.data.GetCount() * 4);
	ConLog.Write("*** fp data size = %d", fp.size);

	ConLog.Write("*** vp shader = \n%s", vp.shader);
	ConLog.Write("*** fp shader = \n%s", fp.shader);

	new_buf.prog_id = prog.id;
	new_buf.vp_id = vp.id;
	new_buf.fp_id = fp.id;

	new_buf.fp_data.AddCpy(&Memory[fp.addr], fp.size);
	new_buf.vp_data.CopyFrom(vp.data);

	new_buf.vp_shader = vp.shader;
	new_buf.fp_shader = fp.shader;

	m_buf.Move(&new_buf);
}

void ProgramBuffer::Clear()
{
	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		glDeleteProgram(m_buf[i].prog_id);
		glDeleteShader(m_buf[i].fp_id);
		glDeleteShader(m_buf[i].vp_id);

		m_buf[i].fp_data.Clear();
		m_buf[i].vp_data.Clear();

		m_buf[i].vp_shader.Clear();
		m_buf[i].fp_shader.Clear();
	}

	m_buf.Clear();
}