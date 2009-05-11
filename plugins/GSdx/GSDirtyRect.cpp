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
#include "GSDirtyRect.h"

GSDirtyRect::GSDirtyRect() 
	: m_psm(PSM_PSMCT32)
	, m_rect(0, 0, 0, 0)
{
}

GSDirtyRect::GSDirtyRect(DWORD psm, CRect rect)
{
	m_psm = psm;
	m_rect = rect;
}

CRect GSDirtyRect::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	CRect r = m_rect;

	CSize src = GSLocalMemory::m_psm[m_psm].bs;

	r.left = (r.left) & ~(src.cx-1);
	r.right = (r.right + (src.cx-1) /* + 1 */) & ~(src.cx-1);
	r.top = (r.top) & ~(src.cy-1);
	r.bottom = (r.bottom + (src.cy-1) /* + 1 */) & ~(src.cy-1);

	if(m_psm != TEX0.PSM)
	{
		CSize dst = GSLocalMemory::m_psm[TEX0.PSM].bs;

		r.left = MulDiv(m_rect.left, dst.cx, src.cx);
		r.right = MulDiv(m_rect.right, dst.cx, src.cx);
		r.top = MulDiv(m_rect.top, dst.cy, src.cy);
		r.bottom = MulDiv(m_rect.bottom, dst.cy, src.cy);
	}

	return r;
}

//

CRect GSDirtyRectList::GetDirtyRect(const GIFRegTEX0& TEX0, CSize size)
{
	if(empty())
	{
		return CRect(0, 0, 0, 0);
	}
	
	CRect r(INT_MAX, INT_MAX, 0, 0);

	for(list<GSDirtyRect>::iterator i = begin(); i != end(); i++)
	{
		r |= i->GetDirtyRect(TEX0);
	}

	return r & CRect(0, 0, size.cx, size.cy);
}
