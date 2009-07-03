/* 
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "GSDump.h"

GSDump::GSDump()
	: m_gs(NULL)
	, m_obj(NULL)
	, m_frames(0)
	, m_objects(0)
	, m_vertices(0)
{
}

GSDump::~GSDump()
{
	Close();
}

void GSDump::Open(const string& fn, uint32 crc, const GSFreezeData& fd, const GSPrivRegSet* regs)
{
	m_gs = fopen((fn + ".gs").c_str(), "wb");
	m_obj = fopen((fn + ".obj").c_str(), "wt");

	m_frames = 0;
	m_objects = 0;
	m_vertices = 0;

	if(m_gs)
	{
		fwrite(&crc, 4, 1, m_gs);
		fwrite(&fd.size, 4, 1, m_gs);
		fwrite(fd.data, fd.size, 1, m_gs);
		fwrite(regs, sizeof(*regs), 1, m_gs);
	}
}

void GSDump::Close()
{
	if(m_gs) {fclose(m_gs); m_gs = NULL;}
	if(m_obj) {fclose(m_obj); m_obj = NULL;}
}

void GSDump::Transfer(int index, uint8* mem, size_t size)
{
	if(m_gs && size > 0)
	{
		fputc(0, m_gs);
		fputc(index, m_gs);
		fwrite(&size, 4, 1, m_gs);
		fwrite(mem, size, 1, m_gs);
	}
}

void GSDump::ReadFIFO(uint32 size)
{
	if(m_gs && size > 0)
	{
		fputc(2, m_gs);
		fwrite(&size, 4, 1, m_gs);
	}
}

void GSDump::VSync(int field, bool last, const GSPrivRegSet* regs)
{
	if(m_gs)
	{
		fputc(3, m_gs);
		fwrite(regs, sizeof(*regs), 1, m_gs);

		fputc(1, m_gs);
		fputc(field, m_gs);

		if((++m_frames & 1) == 0 && last)
		{
			Close();
		}
	}
}

void GSDump::Object(GSVertexSW* vertices, int count, GS_PRIM_CLASS primclass)
{
	if(m_obj)
	{
		switch(primclass)
		{
		case GS_POINT_CLASS:

			// TODO

			break;

		case GS_LINE_CLASS:

			// TODO

			break;

		case GS_TRIANGLE_CLASS:

			for(int i = 0; i < count; i++)
			{
				float x = vertices[i].p.x;
				float y = vertices[i].p.y;
				float z = vertices[i].p.z;

				fprintf(m_obj, "v %f %f %f\n", x, y, z);
			}

			for(int i = 0; i < count; i++)
			{
				fprintf(m_obj, "vt %f %f %f\n", vertices[i].t.x, vertices[i].t.y, vertices[i].t.z);
			}

			for(int i = 0; i < count; i++)
			{
				fprintf(m_obj, "vn %f %f %f\n", 0.0f, 0.0f, 0.0f);
			}

			fprintf(m_obj, "g f%d_o%d_p%d_v%d\n", m_frames, m_objects, primclass, count);

			for(int i = 0; i < count; i += 3)
			{
				int a = m_vertices + i + 1;
				int b = m_vertices + i + 2;
				int c = m_vertices + i + 3;

				fprintf(m_obj, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", a, a, a, b, b, b, c, c, c);
			}

			m_vertices += count;
			m_objects++;

			break;
	
		case GS_SPRITE_CLASS:

			// TODO

			break;
		}
	}
}
