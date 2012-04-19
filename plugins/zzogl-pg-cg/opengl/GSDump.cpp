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

#include "GSDump.h"

GSDump::GSDump()
	: m_gs(NULL)
	, m_frames(0)
{
}

GSDump::~GSDump()
{
	Close();
}

void GSDump::Open(const string& fn, u32 crc, const freezeData& fd, u8* regs)
{
	if (m_gs) return;

	m_gs = fopen(fn.c_str(), "wb");

	m_frames = 0;

	if(m_gs)
	{
		fwrite(&crc, 4, 1, m_gs);
		fwrite(&fd.size, 4, 1, m_gs);
		fwrite(fd.data, fd.size, 1, m_gs);
		fwrite(regs, 0x2000, 1, m_gs);
	}
}

void GSDump::Close()
{
	if(m_gs) {
		fclose(m_gs);
		m_gs = NULL;
		fprintf(stderr, "Closing GS dump");
	}
}

void GSDump::Transfer(int index, const u8* mem, size_t size)
{
	if(m_gs && size > 0)
	{
		fputc(0, m_gs);
		fputc(index, m_gs);
		fwrite(&size, 4, 1, m_gs);
		fwrite(mem, size, 1, m_gs);
	}
}

void GSDump::ReadFIFO(u32 size)
{
	if(m_gs && size > 0)
	{
		fputc(2, m_gs);
		fwrite(&size, 4, 1, m_gs);
	}
}

void GSDump::VSync(int field, bool last, u8* regs)
{
	if(m_gs)
	{
		fputc(3, m_gs);
		fwrite(regs, 0x2000, 1, m_gs);

		fputc(1, m_gs);
		fputc(field, m_gs);

		if((++m_frames & 1) == 0 && last)
		{
			Close();
		}
	}
}
