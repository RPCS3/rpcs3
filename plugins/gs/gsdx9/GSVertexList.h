/* 
 *	Copyright (C) 2003-2005 Gabest
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

template <class VERTEX> class GSVertexList
{
	VERTEX* m_v;
	int m_head, m_tail, m_count;

public:
	GSVertexList()
	{
		m_v = (VERTEX*)_aligned_malloc(sizeof(VERTEX)*4, 16);
		RemoveAll();
	}

	~GSVertexList()
	{
		_aligned_free(m_v);
	}

	void RemoveAll()
	{
		m_head = m_tail = m_count = 0;
	}

	VERTEX& AddTail()
	{
		ASSERT(m_count < 4);
		VERTEX& v = m_v[m_tail];
		m_tail = (m_tail+1)&3;
		m_count++;
		return v;
	}

	void AddTail(VERTEX& v)
	{
		ASSERT(m_count < 4);
		m_v[m_tail] = v;
		m_tail = (m_tail+1)&3;
		m_count++;
	}

	void RemoveAt(int i, VERTEX& v)
	{
		GetAt(i, v);
		i = (m_head+i)&3;
		if(i == m_head) m_head = (m_head+1)&3;
		else for(m_tail = (m_tail+4-1)&3; i != m_tail; i = (i+1)&3) m_v[i] = m_v[(i+1)&3];
		m_count--;
	}

	void GetAt(int i, VERTEX& v)
	{
		ASSERT(m_count > 0); 
		v = m_v[(m_head+i)&3];
	}

	int GetCount()
	{
		return m_count;
	}
};