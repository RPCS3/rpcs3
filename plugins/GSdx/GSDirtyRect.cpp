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
	: psm(PSM_PSMCT32)
{
	left = top = right = bottom = 0;
}

GSDirtyRect::GSDirtyRect(const GSVector4i& r, uint32 psm)
	: psm(psm)
{
	left = r.left;
	top = r.top;
	right = r.right;
	bottom = r.bottom;
}

GSVector4i GSDirtyRect::GetDirtyRect(const GIFRegTEX0& TEX0)
{
	GSVector4i r;

	GSVector2i src = GSLocalMemory::m_psm[psm].bs;

	if(psm != TEX0.PSM)
	{
		GSVector2i dst = GSLocalMemory::m_psm[TEX0.PSM].bs;

		r.left = MulDiv(left, dst.x, src.x);
		r.top = MulDiv(top, dst.y, src.y);
		r.right = MulDiv(right, dst.x, src.x);
		r.bottom = MulDiv(bottom, dst.y, src.y);
	}
	else
	{
		r = GSVector4i(left, top, right, bottom).ralign<GSVector4i::Outside>(src);
	}

	return r;
}

//

GSVector4i GSDirtyRectList::GetDirtyRectAndClear(const GIFRegTEX0& TEX0, const GSVector2i& size)
{
	if(!empty())
	{
		GSVector4i r(INT_MAX, INT_MAX, 0, 0);

		for(list<GSDirtyRect>::iterator i = begin(); i != end(); i++)
		{
			r = r.runion(i->GetDirtyRect(TEX0));
		}

		clear();

		GSVector2i bs = GSLocalMemory::m_psm[TEX0.PSM].bs;

		return r.ralign<GSVector4i::Outside>(bs).rintersect(GSVector4i(0, 0, size.x, size.y));
	}

	return GSVector4i::zero();
}
