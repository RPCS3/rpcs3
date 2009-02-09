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
	: m_fp(NULL)
{
}

GSDump::~GSDump()
{
	if(m_fp)
	{
		fclose(m_fp);
	}
}

void GSDump::Open(LPCTSTR fn, DWORD crc, const GSFreezeData& fd, const void* regs)
{
	m_fp = _tfopen(fn, _T("wb"));
	m_vsyncs = 0;

	if(m_fp)
	{
		fwrite(&crc, 4, 1, m_fp);
		fwrite(&fd.size, 4, 1, m_fp);
		fwrite(fd.data, fd.size, 1, m_fp);
		fwrite(regs, 0x2000, 1, m_fp);
	}
}

void GSDump::Transfer(int index, BYTE* mem, size_t size)
{
	if(m_fp && size > 0)
	{
		fputc(0, m_fp);
		fputc(index, m_fp);
		fwrite(&size, 4, 1, m_fp);
		fwrite(mem, size, 1, m_fp);
	}
}

void GSDump::ReadFIFO(UINT32 size)
{
	if(m_fp && size > 0)
	{
		fputc(2, m_fp);
		fwrite(&size, 4, 1, m_fp);
	}
}

void GSDump::VSync(int field, bool last, const void* regs)
{
	if(m_fp)
	{
		fputc(3, m_fp);
		fwrite(regs, 0x2000, 1, m_fp);

		fputc(1, m_fp);
		fputc(field, m_fp);

		if((++m_vsyncs & 1) == 0 && last)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
	}
}
