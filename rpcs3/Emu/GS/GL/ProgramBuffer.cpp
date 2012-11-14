#include "stdafx.h"
#include "ProgramBuffer.h"

int ProgramBuffer::SearchFp(ShaderProgram& fp)
{
	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(memcmp(&m_buf[i].fp_data[0], &Memory[fp.addr], m_buf[i].fp_data.GetCount()) != 0) continue;

		fp.id = m_buf[i].fp_id;
		fp.shader = m_buf[i].fp_shader;

		return i;
	}

	return -1;
}

int ProgramBuffer::SearchVp(VertexProgram& vp)
{
	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(m_buf[i].vp_data.GetCount() != vp.data.GetCount()) continue;
		if(memcmp(&m_buf[i].vp_data[0], &vp.data[0], vp.data.GetCount() * 4) != 0) continue;

		vp.id = m_buf[i].vp_id;
		vp.shader = m_buf[i].vp_shader;

		return i;
	}

	return -1;
}

bool ProgramBuffer::CmpVP(const u32 a, const u32 b) const
{
	if(m_buf[a].vp_data.GetCount() != m_buf[b].vp_data.GetCount()) return false;
	return memcmp(&m_buf[a].vp_data[0], &m_buf[b].vp_data[0], m_buf[a].vp_data.GetCount() * 4) == 0;
}

bool ProgramBuffer::CmpFP(const u32 a, const u32 b) const
{
	if(m_buf[a].fp_data.GetCount() != m_buf[b].fp_data.GetCount()) return false;
	return memcmp(&m_buf[a].fp_data[0], &m_buf[b].fp_data[0], m_buf[a].fp_data.GetCount()) == 0;
}

u32 ProgramBuffer::GetProg(u32 fp, u32 vp) const
{
	if(fp == vp) return m_buf[fp].prog_id;

	for(u32 i=0; i<m_buf.GetCount(); ++i)
	{
		if(i == fp || i == vp) continue;

		if(CmpVP(vp, i) && CmpFP(fp, i))
		{
			ConLog.Write("Get program (%d):", i);
			ConLog.Write("*** prog id = %d", m_buf[i].prog_id);
			ConLog.Write("*** vp id = %d", m_buf[i].vp_id);
			ConLog.Write("*** fp id = %d", m_buf[i].fp_id);

			ConLog.Write("*** vp shader = %s\n", m_buf[i].vp_shader);
			ConLog.Write("*** fp shader = %s\n", m_buf[i].fp_shader);
			return m_buf[i].prog_id;
		}
	}

	return 0;
}

void ProgramBuffer::Add(Program& prog, ShaderProgram& fp, VertexProgram& vp)
{
	const u32 pos = m_buf.GetCount();
	m_buf.Add(new BufferInfo());
	BufferInfo& new_buf = m_buf[pos];

	ConLog.Write("Add program (%d):", pos);
	ConLog.Write("*** prog id = %d", prog.id);
	ConLog.Write("*** vp id = %d", vp.id);
	ConLog.Write("*** fp id = %d", fp.id);
	ConLog.Write("*** vp data size = %d", vp.data.GetCount() * 4);
	ConLog.Write("*** fp data size = %d", fp.size);

	ConLog.Write("*** vp shader = %s\n", vp.shader);
	ConLog.Write("*** fp shader = %s\n", fp.shader);

	new_buf.prog_id = prog.id;
	new_buf.vp_id = vp.id;
	new_buf.fp_id = fp.id;

	new_buf.fp_data.SetCount(fp.size);
	memcpy(&new_buf.fp_data[0], &Memory[fp.addr], fp.size);

	new_buf.vp_data.SetCount(vp.data.GetCount());
	memcpy(&new_buf.vp_data[0], &vp.data[0], vp.data.GetCount() * 4);

	new_buf.vp_shader = vp.shader;
	new_buf.fp_shader = fp.shader;
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