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

#pragma once

template <class Vertex> class GSVertexList
{
	void* m_base;
	Vertex* m_v[3];
	int m_count;

public:
	GSVertexList()
		: m_count(0)
	{
		m_base = _aligned_malloc(sizeof(Vertex) * countof(m_v), 32);

		for(uint i = 0; i < countof(m_v); i++)
		{
			m_v[i] = &((Vertex*)m_base)[i];
		}
	}

	virtual ~GSVertexList()
	{
		_aligned_free(m_base);
	}

	void RemoveAll()
	{
		m_count = 0;
	}

	__forceinline Vertex& AddTail()
	{
		ASSERT(m_count < 3);

		return *m_v[m_count++];
	}

	__forceinline void RemoveAt(int pos, int keep)
	{
		if(keep == 1)
		{
			Vertex* tmp = m_v[pos + 0];
			m_v[pos + 0] = m_v[pos + 1];
			m_v[pos + 1] = tmp;
		}
		else if(keep == 2)
		{
			Vertex* tmp = m_v[pos + 0];
			m_v[pos + 0] = m_v[pos + 1];
			m_v[pos + 1] = m_v[pos + 2];
			m_v[pos + 2] = tmp;
		}

		m_count = pos + keep;
	}

	__forceinline void GetAt(int i, Vertex& v)
	{
		v = *m_v[i];
	}

	int GetCount()
	{
		return m_count;
	}
};
