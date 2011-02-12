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
#include "GSCodeBuffer.h"

GSCodeBuffer::GSCodeBuffer(size_t blocksize)
	: m_ptr(NULL)
	, m_blocksize(blocksize)
	, m_pos(0)
	, m_reserved(0)
{
}

GSCodeBuffer::~GSCodeBuffer()
{
	for(list<void*>::iterator i = m_buffers.begin(); i != m_buffers.end(); i++)
	{
		VirtualFree(*i, 0, MEM_RELEASE);
	}
}

void* GSCodeBuffer::GetBuffer(size_t size)
{
	ASSERT(size < m_blocksize);
	ASSERT(m_reserved == 0);

	size = (size + 15) & ~15;

	if(m_ptr == NULL || m_pos + size > m_blocksize)
	{
		m_ptr = (uint8*)VirtualAlloc(NULL, m_blocksize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

		m_pos = 0;

		m_buffers.push_back(m_ptr);
	}

	uint8* ptr = &m_ptr[m_pos];

	m_reserved = size;

	return ptr;
}

void GSCodeBuffer::ReleaseBuffer(size_t size)
{
	ASSERT(size <= m_reserved);

	m_pos = ((m_pos + size) + 15) & ~15;

	ASSERT(m_pos < m_blocksize);

	m_reserved = 0;
}
