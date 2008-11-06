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

#include "stdafx.h"
#include "GSState.h"

void GSState::WriteStep()
{
//	if(m_y == m_de.TRXREG.RRH && m_x == m_de.TRXPOS.DSAX) ASSERT(0);

	if(++m_x == m_de.TRXREG.RRW)
	{
		m_x = m_de.TRXPOS.DSAX;
		m_y++;
	}
}

void GSState::ReadStep()
{
//	if(m_y == m_de.TRXREG.RRH && m_x == m_de.TRXPOS.SSAX) ASSERT(0);

	if(++m_x == m_de.TRXREG.RRW)
	{
		m_x = m_de.TRXPOS.SSAX;
		m_y++;
	}
}

void GSState::WriteTransfer(BYTE* pMem, int len)
{
	LOG(_T("*TC2 WriteTransfer %d,%d (psm=%d rr=%dx%d len=%d)\n"), m_x, m_y, m_de.BITBLTBUF.DPSM, m_de.TRXREG.RRW, m_de.TRXREG.RRH, len);

	if(len == 0) return;

	// TODO: hmmmm
	if(m_pPRIM->TME && (m_de.BITBLTBUF.DBP == m_ctxt->TEX0.TBP0 || m_de.BITBLTBUF.DBP == m_ctxt->TEX0.CBP))
		FlushPrim();

	int bpp = GSLocalMemory::m_psmtbl[m_de.BITBLTBUF.DPSM].trbpp;
	int pitch = (m_de.TRXREG.RRW - m_de.TRXPOS.DSAX)*bpp>>3;
	int height = len / pitch;

	if(m_nTrBytes > 0 || height < m_de.TRXREG.RRH - m_de.TRXPOS.DSAY)
	{
		LOG(_T("*TC2 WriteTransfer delayed\n"));

		ASSERT(len <= m_nTrMaxBytes); // transferring more than 4mb into a 4mb local mem doesn't make any sense

		len = min(m_nTrMaxBytes, len);

		if(m_nTrBytes + len > m_nTrMaxBytes)
			FlushWriteTransfer();

		memcpy(&m_pTrBuff[m_nTrBytes], pMem, len);
		m_nTrBytes += len;
	}
	else
	{
		int x = m_x, y = m_y;

		(m_lm.*GSLocalMemory::m_psmtbl[m_de.BITBLTBUF.DPSM].st)(m_x, m_y, pMem, len, m_de.BITBLTBUF, m_de.TRXPOS, m_de.TRXREG);

		m_perfmon.IncCounter(GSPerfMon::c_swizzle, len);

		//ASSERT(m_de.TRXREG.RRH >= m_y - y);

		CRect r(m_de.TRXPOS.DSAX, y, m_de.TRXREG.RRW, min(m_x == m_de.TRXPOS.DSAX ? m_y : m_y+1, m_de.TRXREG.RRH));
		InvalidateTexture(m_de.BITBLTBUF, r);

		m_lm.InvalidateCLUT();
	}
}

void GSState::FlushWriteTransfer()
{
	if(!m_nTrBytes) return;

	int x = m_x, y = m_y;

	LOG(_T("*TC2 FlushWriteTransfer %d,%d-%d,%d (psm=%d rr=%dx%d len=%d)\n"), x, y, m_x, m_y, m_de.BITBLTBUF.DPSM, m_de.TRXREG.RRW, m_de.TRXREG.RRH, m_nTrBytes);

	(m_lm.*GSLocalMemory::m_psmtbl[m_de.BITBLTBUF.DPSM].st)(m_x, m_y, m_pTrBuff, m_nTrBytes, m_de.BITBLTBUF, m_de.TRXPOS, m_de.TRXREG);

	m_perfmon.IncCounter(GSPerfMon::c_swizzle, m_nTrBytes);

	m_nTrBytes = 0;

	//ASSERT(m_de.TRXREG.RRH >= m_y - y);

	CRect r(m_de.TRXPOS.DSAX, y, m_de.TRXREG.RRW, min(m_x == m_de.TRXPOS.DSAX ? m_y : m_y+1, m_de.TRXREG.RRH));
	InvalidateTexture(m_de.BITBLTBUF, r);

	m_lm.InvalidateCLUT();
}

void GSState::ReadTransfer(BYTE* pMem, int len)
{
	BYTE* pb = (BYTE*)pMem;
	WORD* pw = (WORD*)pMem;
	DWORD* pd = (DWORD*)pMem;

	if(m_y >= (int)m_de.TRXREG.RRH) {ASSERT(0); return;}

	if(m_x == m_de.TRXPOS.SSAX && m_y == m_de.TRXPOS.SSAY)
	{
		CRect r(m_de.TRXPOS.SSAX, m_de.TRXPOS.SSAY, m_de.TRXREG.RRW, m_de.TRXREG.RRH);
		InvalidateLocalMem(m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW, m_de.BITBLTBUF.SPSM, r);
	}

	switch(m_de.BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMCT16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMCT16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16S(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMT8:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4(m_x+1, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT8H:
		for(; len-- > 0; ReadStep(), pb++)
			*pb = (BYTE)m_lm.readPixel8H(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMT4HL:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HL(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HL(m_x+1, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMT4HH:
		for(; len-- > 0; ReadStep(), ReadStep(), pb++)
			*pb = (BYTE)(m_lm.readPixel4HH(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)&0x0f)
				| (BYTE)(m_lm.readPixel4HH(m_x+1, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW)<<4);
		break;
	case PSM_PSMZ32:
		for(len /= 4; len-- > 0; ReadStep(), pd++)
			*pd = m_lm.readPixel32Z(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ24:
		for(len /= 3; len-- > 0; ReadStep(), pb+=3)
		{
			DWORD dw = m_lm.readPixel24Z(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
			pb[0] = ((BYTE*)&dw)[0]; pb[1] = ((BYTE*)&dw)[1]; pb[2] = ((BYTE*)&dw)[2];
		}
		break;
	case PSM_PSMZ16:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16Z(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	case PSM_PSMZ16S:
		for(len /= 2; len-- > 0; ReadStep(), pw++)
			*pw = (WORD)m_lm.readPixel16SZ(m_x, m_y, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW);
		break;
	}
}

void GSState::MoveTransfer()
{
	GSLocalMemory::readPixel rp = GSLocalMemory::m_psmtbl[m_de.BITBLTBUF.SPSM].rp;
	GSLocalMemory::writePixel wp = GSLocalMemory::m_psmtbl[m_de.BITBLTBUF.DPSM].wp;

	int sx = m_de.TRXPOS.SSAX;
	int dx = m_de.TRXPOS.DSAX;
	int sy = m_de.TRXPOS.SSAY;
	int dy = m_de.TRXPOS.DSAY;
	int w = m_de.TRXREG.RRW;
	int h = m_de.TRXREG.RRH;
	int xinc = 1;
	int yinc = 1;

	if(sx < dx) sx += w-1, dx += w-1, xinc = -1;
	if(sy < dy) sy += h-1, dy += h-1, yinc = -1;

	for(int y = 0; y < h; y++, sy += yinc, dy += yinc, sx -= xinc*w, dx -= xinc*w)
		for(int x = 0; x < w; x++, sx += xinc, dx += xinc)
			(m_lm.*wp)(dx, dy, (m_lm.*rp)(sx, sy, m_de.BITBLTBUF.SBP, m_de.BITBLTBUF.SBW), m_de.BITBLTBUF.DBP, m_de.BITBLTBUF.DBW);
}


