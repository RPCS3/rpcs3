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
 *	Special Notes: 
 *
 *	Based on Page.c from GSSoft
 *	Copyright (C) 2002-2004 GSsoft Team
 *
 */
 
#include "StdAfx.h"
#include "GSLocalMemory.h"

#define ASSERT_BLOCK(r, w, h) \
	ASSERT((r).width() >= w && (r).height() >= h && !((r).left&(w-1)) && !((r).top&(h-1)) && !((r).right&(w-1)) && !((r).bottom&(h-1))); \

#define FOREACH_BLOCK_START(w, h, bpp) \
	uint32 bp = TEX0.TBP0; \
	uint32 bw = TEX0.TBW; \
	int offset = dstpitch * h - r.width() * bpp / 8; \
	for(int y = r.top, ye = r.bottom; y < ye; y += h, dst += offset) \
	{ ASSERT_BLOCK(r, w, h); \
		for(int x = r.left, xe = r.right; x < xe; x += w, dst += w * bpp / 8) \
		{ \

#define FOREACH_BLOCK_END }}

//

uint32 GSLocalMemory::pageOffset32[32][32][64];
uint32 GSLocalMemory::pageOffset32Z[32][32][64];
uint32 GSLocalMemory::pageOffset16[32][64][64];
uint32 GSLocalMemory::pageOffset16S[32][64][64];
uint32 GSLocalMemory::pageOffset16Z[32][64][64];
uint32 GSLocalMemory::pageOffset16SZ[32][64][64];
uint32 GSLocalMemory::pageOffset8[32][64][128];
uint32 GSLocalMemory::pageOffset4[32][128][128];

int GSLocalMemory::rowOffset32[2048];
int GSLocalMemory::rowOffset32Z[2048];
int GSLocalMemory::rowOffset16[2048];
int GSLocalMemory::rowOffset16S[2048];
int GSLocalMemory::rowOffset16Z[2048];
int GSLocalMemory::rowOffset16SZ[2048];
int GSLocalMemory::rowOffset8[2][2048];
int GSLocalMemory::rowOffset4[2][2048];

int GSLocalMemory::blockOffset32[256];
int GSLocalMemory::blockOffset32Z[256];
int GSLocalMemory::blockOffset16[256];
int GSLocalMemory::blockOffset16S[256];
int GSLocalMemory::blockOffset16Z[256];
int GSLocalMemory::blockOffset16SZ[256];
int GSLocalMemory::blockOffset8[256];
int GSLocalMemory::blockOffset4[256];

//

GSLocalMemory::psm_t GSLocalMemory::m_psm[64];

//

GSLocalMemory::GSLocalMemory()
	: m_clut(this)
{
	m_vm8 = (uint8*)VirtualAlloc(NULL, m_vmsize * 2, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	memset(m_vm8, 0, m_vmsize);

	for(int bp = 0; bp < 32; bp++)
	{
		for(int y = 0; y < 32; y++) for(int x = 0; x < 64; x++)
		{
			pageOffset32[bp][y][x] = PixelAddressOrg32(x, y, bp, 0);
			pageOffset32Z[bp][y][x] = PixelAddressOrg32Z(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 64; x++) 
		{
			pageOffset16[bp][y][x] = PixelAddressOrg16(x, y, bp, 0);
			pageOffset16S[bp][y][x] = PixelAddressOrg16S(x, y, bp, 0);
			pageOffset16Z[bp][y][x] = PixelAddressOrg16Z(x, y, bp, 0);
			pageOffset16SZ[bp][y][x] = PixelAddressOrg16SZ(x, y, bp, 0);
		}

		for(int y = 0; y < 64; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset8[bp][y][x] = PixelAddressOrg8(x, y, bp, 0);
		}

		for(int y = 0; y < 128; y++) for(int x = 0; x < 128; x++)
		{
			pageOffset4[bp][y][x] = PixelAddressOrg4(x, y, bp, 0);
		}
	}

	for(int x = 0; x < countof(rowOffset32); x++)
	{
		rowOffset32[x] = (int)PixelAddress32(x, 0, 0, 32) - (int)PixelAddress32(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset32Z); x++)
	{
		rowOffset32Z[x] = (int)PixelAddress32Z(x, 0, 0, 32) - (int)PixelAddress32Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16); x++)
	{
		rowOffset16[x] = (int)PixelAddress16(x, 0, 0, 32) - (int)PixelAddress16(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16S); x++)
	{
		rowOffset16S[x] = (int)PixelAddress16S(x, 0, 0, 32) - (int)PixelAddress16S(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16Z); x++)
	{
		rowOffset16Z[x] = (int)PixelAddress16Z(x, 0, 0, 32) - (int)PixelAddress16Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset16SZ); x++)
	{
		rowOffset16SZ[x] = (int)PixelAddress16SZ(x, 0, 0, 32) - (int)PixelAddress16SZ(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset8[0]); x++)
	{
		rowOffset8[0][x] = (int)PixelAddress8(x, 0, 0, 32) - (int)PixelAddress8(0, 0, 0, 32);
		rowOffset8[1][x] = (int)PixelAddress8(x, 2, 0, 32) - (int)PixelAddress8(0, 2, 0, 32);
	}

	for(int x = 0; x < countof(rowOffset4[0]); x++)
	{
		rowOffset4[0][x] = (int)PixelAddress4(x, 0, 0, 32) - (int)PixelAddress4(0, 0, 0, 32);
		rowOffset4[1][x] = (int)PixelAddress4(x, 2, 0, 32) - (int)PixelAddress4(0, 2, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset32); x++)
	{
		blockOffset32[x] = (int)BlockNumber32(x << 3, 0, 0, 32) - (int)BlockNumber32(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset32Z); x++)
	{
		blockOffset32Z[x] = (int)BlockNumber32Z(x << 3, 0, 0, 32) - (int)BlockNumber32Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset16); x++)
	{
		blockOffset16[x] = (int)BlockNumber16(x << 3, 0, 0, 32) - (int)BlockNumber16(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset16S); x++)
	{
		blockOffset16S[x] = (int)BlockNumber16S(x << 3, 0, 0, 32) - (int)BlockNumber16S(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset16Z); x++)
	{
		blockOffset16Z[x] = (int)BlockNumber16Z(x << 3, 0, 0, 32) - (int)BlockNumber16Z(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset16SZ); x++)
	{
		blockOffset16SZ[x] = (int)BlockNumber16SZ(x << 3, 0, 0, 32) - (int)BlockNumber16SZ(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset8); x++)
	{
		blockOffset8[x] = (int)BlockNumber8(x << 3, 0, 0, 32) - (int)BlockNumber8(0, 0, 0, 32);
	}

	for(int x = 0; x < countof(blockOffset4); x++)
	{
		blockOffset4[x] = (int)BlockNumber4(x << 3, 0, 0, 32) - (int)BlockNumber4(0, 0, 0, 32);
	}

	for(int i = 0; i < countof(m_psm); i++)
	{
		m_psm[i].pa = &GSLocalMemory::PixelAddress32;
		m_psm[i].bn = &GSLocalMemory::BlockNumber32;
		m_psm[i].rp = &GSLocalMemory::ReadPixel32;
		m_psm[i].rpa = &GSLocalMemory::ReadPixel32;
		m_psm[i].wp = &GSLocalMemory::WritePixel32;
		m_psm[i].wpa = &GSLocalMemory::WritePixel32;
		m_psm[i].rt = &GSLocalMemory::ReadTexel32;
		m_psm[i].rtNP = &GSLocalMemory::ReadTexel32;
		m_psm[i].rta = &GSLocalMemory::ReadTexel32;
		m_psm[i].wfa = &GSLocalMemory::WritePixel32;
		m_psm[i].wi = &GSLocalMemory::WriteImage<PSM_PSMCT32, 8, 8, 32>;
		m_psm[i].ri = &GSLocalMemory::ReadImageX; // TODO
		m_psm[i].rtx = &GSLocalMemory::ReadTexture32;
		m_psm[i].rtxNP = &GSLocalMemory::ReadTexture32;
		m_psm[i].rtxP = &GSLocalMemory::ReadTexture32;
		m_psm[i].rtxb = &GSLocalMemory::ReadTextureBlock32;
		m_psm[i].rtxbP = &GSLocalMemory::ReadTextureBlock32;
		m_psm[i].bpp = m_psm[i].trbpp = 32;
		m_psm[i].pal = 0;
		m_psm[i].bs = GSVector2i(8, 8);
		m_psm[i].pgs = GSVector2i(64, 32);
		for(int j = 0; j < 8; j++) m_psm[i].rowOffset[j] = rowOffset32;
		m_psm[i].blockOffset = blockOffset32;
	}

	m_psm[PSM_PSMCT16].pa = &GSLocalMemory::PixelAddress16;
	m_psm[PSM_PSMCT16S].pa = &GSLocalMemory::PixelAddress16S;
	m_psm[PSM_PSMT8].pa = &GSLocalMemory::PixelAddress8;
	m_psm[PSM_PSMT4].pa = &GSLocalMemory::PixelAddress4;
	m_psm[PSM_PSMZ32].pa = &GSLocalMemory::PixelAddress32Z;
	m_psm[PSM_PSMZ24].pa = &GSLocalMemory::PixelAddress32Z;
	m_psm[PSM_PSMZ16].pa = &GSLocalMemory::PixelAddress16Z;
	m_psm[PSM_PSMZ16S].pa = &GSLocalMemory::PixelAddress16SZ;

	m_psm[PSM_PSMCT16].bn = &GSLocalMemory::BlockNumber16;
	m_psm[PSM_PSMCT16S].bn = &GSLocalMemory::BlockNumber16S;
	m_psm[PSM_PSMT8].bn = &GSLocalMemory::BlockNumber8;
	m_psm[PSM_PSMT4].bn = &GSLocalMemory::BlockNumber4;
	m_psm[PSM_PSMZ32].bn = &GSLocalMemory::BlockNumber32Z;
	m_psm[PSM_PSMZ24].bn = &GSLocalMemory::BlockNumber32Z;
	m_psm[PSM_PSMZ16].bn = &GSLocalMemory::BlockNumber16Z;
	m_psm[PSM_PSMZ16S].bn = &GSLocalMemory::BlockNumber16SZ;

	m_psm[PSM_PSMCT24].rp = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMCT16].rp = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMCT16S].rp = &GSLocalMemory::ReadPixel16S;
	m_psm[PSM_PSMT8].rp = &GSLocalMemory::ReadPixel8;
	m_psm[PSM_PSMT4].rp = &GSLocalMemory::ReadPixel4;
	m_psm[PSM_PSMT8H].rp = &GSLocalMemory::ReadPixel8H;
	m_psm[PSM_PSMT4HL].rp = &GSLocalMemory::ReadPixel4HL;
	m_psm[PSM_PSMT4HH].rp = &GSLocalMemory::ReadPixel4HH;
	m_psm[PSM_PSMZ32].rp = &GSLocalMemory::ReadPixel32Z;
	m_psm[PSM_PSMZ24].rp = &GSLocalMemory::ReadPixel24Z;
	m_psm[PSM_PSMZ16].rp = &GSLocalMemory::ReadPixel16Z;
	m_psm[PSM_PSMZ16S].rp = &GSLocalMemory::ReadPixel16SZ;

	m_psm[PSM_PSMCT24].rpa = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMCT16].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMCT16S].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMT8].rpa = &GSLocalMemory::ReadPixel8;
	m_psm[PSM_PSMT4].rpa = &GSLocalMemory::ReadPixel4;
	m_psm[PSM_PSMT8H].rpa = &GSLocalMemory::ReadPixel8H;
	m_psm[PSM_PSMT4HL].rpa = &GSLocalMemory::ReadPixel4HL;
	m_psm[PSM_PSMT4HH].rpa = &GSLocalMemory::ReadPixel4HH;
	m_psm[PSM_PSMZ32].rpa = &GSLocalMemory::ReadPixel32;
	m_psm[PSM_PSMZ24].rpa = &GSLocalMemory::ReadPixel24;
	m_psm[PSM_PSMZ16].rpa = &GSLocalMemory::ReadPixel16;
	m_psm[PSM_PSMZ16S].rpa = &GSLocalMemory::ReadPixel16;

	m_psm[PSM_PSMCT32].wp = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMCT24].wp = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wp = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMCT16S].wp = &GSLocalMemory::WritePixel16S;
	m_psm[PSM_PSMT8].wp = &GSLocalMemory::WritePixel8;
	m_psm[PSM_PSMT4].wp = &GSLocalMemory::WritePixel4;
	m_psm[PSM_PSMT8H].wp = &GSLocalMemory::WritePixel8H;
	m_psm[PSM_PSMT4HL].wp = &GSLocalMemory::WritePixel4HL;
	m_psm[PSM_PSMT4HH].wp = &GSLocalMemory::WritePixel4HH;
	m_psm[PSM_PSMZ32].wp = &GSLocalMemory::WritePixel32Z;
	m_psm[PSM_PSMZ24].wp = &GSLocalMemory::WritePixel24Z;
	m_psm[PSM_PSMZ16].wp = &GSLocalMemory::WritePixel16Z;
	m_psm[PSM_PSMZ16S].wp = &GSLocalMemory::WritePixel16SZ;

	m_psm[PSM_PSMCT32].wpa = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMCT24].wpa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMCT16S].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMT8].wpa = &GSLocalMemory::WritePixel8;
	m_psm[PSM_PSMT4].wpa = &GSLocalMemory::WritePixel4;
	m_psm[PSM_PSMT8H].wpa = &GSLocalMemory::WritePixel8H;
	m_psm[PSM_PSMT4HL].wpa = &GSLocalMemory::WritePixel4HL;
	m_psm[PSM_PSMT4HH].wpa = &GSLocalMemory::WritePixel4HH;
	m_psm[PSM_PSMZ32].wpa = &GSLocalMemory::WritePixel32;
	m_psm[PSM_PSMZ24].wpa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMZ16].wpa = &GSLocalMemory::WritePixel16;
	m_psm[PSM_PSMZ16S].wpa = &GSLocalMemory::WritePixel16;

	m_psm[PSM_PSMCT24].rt = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMCT16].rt = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMCT16S].rt = &GSLocalMemory::ReadTexel16S;
	m_psm[PSM_PSMT8].rt = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rt = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rt = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rt = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rt = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ32].rt = &GSLocalMemory::ReadTexel32Z;
	m_psm[PSM_PSMZ24].rt = &GSLocalMemory::ReadTexel24Z;
	m_psm[PSM_PSMZ16].rt = &GSLocalMemory::ReadTexel16Z;
	m_psm[PSM_PSMZ16S].rt = &GSLocalMemory::ReadTexel16SZ;

	m_psm[PSM_PSMCT24].rta = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMCT16].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMCT16S].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMT8].rta = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rta = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rta = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rta = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rta = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ24].rta = &GSLocalMemory::ReadTexel24;
	m_psm[PSM_PSMZ16].rta = &GSLocalMemory::ReadTexel16;
	m_psm[PSM_PSMZ16S].rta = &GSLocalMemory::ReadTexel16;

	m_psm[PSM_PSMCT24].wfa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMCT16].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMCT16S].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMZ24].wfa = &GSLocalMemory::WritePixel24;
	m_psm[PSM_PSMZ16].wfa = &GSLocalMemory::WriteFrame16;
	m_psm[PSM_PSMZ16S].wfa = &GSLocalMemory::WriteFrame16;

	m_psm[PSM_PSMCT16].rtNP = &GSLocalMemory::ReadTexel16NP;
	m_psm[PSM_PSMCT16S].rtNP = &GSLocalMemory::ReadTexel16SNP;
	m_psm[PSM_PSMT8].rtNP = &GSLocalMemory::ReadTexel8;
	m_psm[PSM_PSMT4].rtNP = &GSLocalMemory::ReadTexel4;
	m_psm[PSM_PSMT8H].rtNP = &GSLocalMemory::ReadTexel8H;
	m_psm[PSM_PSMT4HL].rtNP = &GSLocalMemory::ReadTexel4HL;
	m_psm[PSM_PSMT4HH].rtNP = &GSLocalMemory::ReadTexel4HH;
	m_psm[PSM_PSMZ32].rtNP = &GSLocalMemory::ReadTexel32Z;
	m_psm[PSM_PSMZ24].rtNP = &GSLocalMemory::ReadTexel24Z;
	m_psm[PSM_PSMZ16].rtNP = &GSLocalMemory::ReadTexel16ZNP;
	m_psm[PSM_PSMZ16S].rtNP = &GSLocalMemory::ReadTexel16SZNP;

	m_psm[PSM_PSMCT24].wi = &GSLocalMemory::WriteImage24; // TODO
	m_psm[PSM_PSMCT16].wi = &GSLocalMemory::WriteImage<PSM_PSMCT16, 16, 8, 16>;
	m_psm[PSM_PSMCT16S].wi = &GSLocalMemory::WriteImage<PSM_PSMCT16S, 16, 8, 16>;
	m_psm[PSM_PSMT8].wi = &GSLocalMemory::WriteImage<PSM_PSMT8, 16, 16, 8>;
	m_psm[PSM_PSMT4].wi = &GSLocalMemory::WriteImage<PSM_PSMT4, 32, 16, 4>;
	m_psm[PSM_PSMT8H].wi = &GSLocalMemory::WriteImage8H; // TODO
	m_psm[PSM_PSMT4HL].wi = &GSLocalMemory::WriteImage4HL; // TODO
	m_psm[PSM_PSMT4HH].wi = &GSLocalMemory::WriteImage4HH; // TODO
	m_psm[PSM_PSMZ32].wi = &GSLocalMemory::WriteImage<PSM_PSMZ32, 8, 8, 32>;
	m_psm[PSM_PSMZ24].wi = &GSLocalMemory::WriteImage24Z; // TODO
	m_psm[PSM_PSMZ16].wi = &GSLocalMemory::WriteImage<PSM_PSMZ16, 16, 8, 16>;
	m_psm[PSM_PSMZ16S].wi = &GSLocalMemory::WriteImage<PSM_PSMZ16S, 16, 8, 16>;

	m_psm[PSM_PSMCT24].rtx = &GSLocalMemory::ReadTexture24;
	m_psm[PSM_PSMCT16].rtx = &GSLocalMemory::ReadTexture16;
	m_psm[PSM_PSMCT16S].rtx = &GSLocalMemory::ReadTexture16S;
	m_psm[PSM_PSMT8].rtx = &GSLocalMemory::ReadTexture8;
	m_psm[PSM_PSMT4].rtx = &GSLocalMemory::ReadTexture4;
	m_psm[PSM_PSMT8H].rtx = &GSLocalMemory::ReadTexture8H;
	m_psm[PSM_PSMT4HL].rtx = &GSLocalMemory::ReadTexture4HL;
	m_psm[PSM_PSMT4HH].rtx = &GSLocalMemory::ReadTexture4HH;
	m_psm[PSM_PSMZ32].rtx = &GSLocalMemory::ReadTexture32Z;
	m_psm[PSM_PSMZ24].rtx = &GSLocalMemory::ReadTexture24Z;
	m_psm[PSM_PSMZ16].rtx = &GSLocalMemory::ReadTexture16Z;
	m_psm[PSM_PSMZ16S].rtx = &GSLocalMemory::ReadTexture16SZ;

	m_psm[PSM_PSMCT16].rtxNP = &GSLocalMemory::ReadTexture16NP;
	m_psm[PSM_PSMCT16S].rtxNP = &GSLocalMemory::ReadTexture16SNP;
	m_psm[PSM_PSMT8].rtxNP = &GSLocalMemory::ReadTexture8NP;
	m_psm[PSM_PSMT4].rtxNP = &GSLocalMemory::ReadTexture4NP;
	m_psm[PSM_PSMT8H].rtxNP = &GSLocalMemory::ReadTexture8HNP;
	m_psm[PSM_PSMT4HL].rtxNP = &GSLocalMemory::ReadTexture4HLNP;
	m_psm[PSM_PSMT4HH].rtxNP = &GSLocalMemory::ReadTexture4HHNP;
	m_psm[PSM_PSMZ32].rtxNP = &GSLocalMemory::ReadTexture32Z;
	m_psm[PSM_PSMZ24].rtxNP = &GSLocalMemory::ReadTexture24Z;
	m_psm[PSM_PSMZ16].rtxNP = &GSLocalMemory::ReadTexture16ZNP;
	m_psm[PSM_PSMZ16S].rtxNP = &GSLocalMemory::ReadTexture16SZNP;

	m_psm[PSM_PSMCT24].rtxP = &GSLocalMemory::ReadTexture24;
	m_psm[PSM_PSMCT16].rtxP = &GSLocalMemory::ReadTexture16;
	m_psm[PSM_PSMCT16S].rtxP = &GSLocalMemory::ReadTexture16S;
	m_psm[PSM_PSMT8].rtxP = &GSLocalMemory::ReadTexture8P;
	m_psm[PSM_PSMT4].rtxP = &GSLocalMemory::ReadTexture4P;
	m_psm[PSM_PSMT8H].rtxP = &GSLocalMemory::ReadTexture8HP;
	m_psm[PSM_PSMT4HL].rtxP = &GSLocalMemory::ReadTexture4HLP;
	m_psm[PSM_PSMT4HH].rtxP = &GSLocalMemory::ReadTexture4HHP;
	m_psm[PSM_PSMZ32].rtxP = &GSLocalMemory::ReadTexture32Z;
	m_psm[PSM_PSMZ24].rtxP = &GSLocalMemory::ReadTexture24Z;
	m_psm[PSM_PSMZ16].rtxP = &GSLocalMemory::ReadTexture16Z;
	m_psm[PSM_PSMZ16S].rtxP = &GSLocalMemory::ReadTexture16SZ;

	m_psm[PSM_PSMCT24].rtxb = &GSLocalMemory::ReadTextureBlock24;
	m_psm[PSM_PSMCT16].rtxb = &GSLocalMemory::ReadTextureBlock16;
	m_psm[PSM_PSMCT16S].rtxb = &GSLocalMemory::ReadTextureBlock16S;
	m_psm[PSM_PSMT8].rtxb = &GSLocalMemory::ReadTextureBlock8;
	m_psm[PSM_PSMT4].rtxb = &GSLocalMemory::ReadTextureBlock4;
	m_psm[PSM_PSMT8H].rtxb = &GSLocalMemory::ReadTextureBlock8H;
	m_psm[PSM_PSMT4HL].rtxb = &GSLocalMemory::ReadTextureBlock4HL;
	m_psm[PSM_PSMT4HH].rtxb = &GSLocalMemory::ReadTextureBlock4HH;
	m_psm[PSM_PSMZ32].rtxb = &GSLocalMemory::ReadTextureBlock32Z;
	m_psm[PSM_PSMZ24].rtxb = &GSLocalMemory::ReadTextureBlock24Z;
	m_psm[PSM_PSMZ16].rtxb = &GSLocalMemory::ReadTextureBlock16Z;
	m_psm[PSM_PSMZ16S].rtxb = &GSLocalMemory::ReadTextureBlock16SZ;

	m_psm[PSM_PSMCT24].rtxbP = &GSLocalMemory::ReadTextureBlock24;
	m_psm[PSM_PSMCT16].rtxbP = &GSLocalMemory::ReadTextureBlock16;
	m_psm[PSM_PSMCT16S].rtxbP = &GSLocalMemory::ReadTextureBlock16S;
	m_psm[PSM_PSMT8].rtxbP = &GSLocalMemory::ReadTextureBlock8P;
	m_psm[PSM_PSMT4].rtxbP = &GSLocalMemory::ReadTextureBlock4P;
	m_psm[PSM_PSMT8H].rtxbP = &GSLocalMemory::ReadTextureBlock8HP;
	m_psm[PSM_PSMT4HL].rtxbP = &GSLocalMemory::ReadTextureBlock4HLP;
	m_psm[PSM_PSMT4HH].rtxbP = &GSLocalMemory::ReadTextureBlock4HHP;
	m_psm[PSM_PSMZ32].rtxbP = &GSLocalMemory::ReadTextureBlock32Z;
	m_psm[PSM_PSMZ24].rtxbP = &GSLocalMemory::ReadTextureBlock24Z;
	m_psm[PSM_PSMZ16].rtxbP = &GSLocalMemory::ReadTextureBlock16Z;
	m_psm[PSM_PSMZ16S].rtxbP = &GSLocalMemory::ReadTextureBlock16SZ;

	m_psm[PSM_PSMT8].pal = m_psm[PSM_PSMT8H].pal = 256;
	m_psm[PSM_PSMT4].pal = m_psm[PSM_PSMT4HL].pal = m_psm[PSM_PSMT4HH].pal = 16;

	m_psm[PSM_PSMCT16].bpp = m_psm[PSM_PSMCT16S].bpp = 16;
	m_psm[PSM_PSMT8].bpp = 8;
	m_psm[PSM_PSMT4].bpp = 4;
	m_psm[PSM_PSMZ16].bpp = m_psm[PSM_PSMZ16S].bpp = 16;

	m_psm[PSM_PSMCT24].trbpp = 24;
	m_psm[PSM_PSMCT16].trbpp = m_psm[PSM_PSMCT16S].trbpp = 16;
	m_psm[PSM_PSMT8].trbpp = m_psm[PSM_PSMT8H].trbpp = 8;
	m_psm[PSM_PSMT4].trbpp = m_psm[PSM_PSMT4HL].trbpp = m_psm[PSM_PSMT4HH].trbpp = 4;
	m_psm[PSM_PSMZ24].trbpp = 24;
	m_psm[PSM_PSMZ16].trbpp = m_psm[PSM_PSMZ16S].trbpp = 16;

	m_psm[PSM_PSMCT16].bs = m_psm[PSM_PSMCT16S].bs = GSVector2i(16, 8);
	m_psm[PSM_PSMT8].bs = GSVector2i(16, 16);
	m_psm[PSM_PSMT4].bs = GSVector2i(32, 16);
	m_psm[PSM_PSMZ16].bs = m_psm[PSM_PSMZ16S].bs = GSVector2i(16, 8);

	m_psm[PSM_PSMCT16].pgs = m_psm[PSM_PSMCT16S].pgs = GSVector2i(64, 64);
	m_psm[PSM_PSMT8].pgs = GSVector2i(128, 64);
	m_psm[PSM_PSMT4].pgs = GSVector2i(128, 128);
	m_psm[PSM_PSMZ16].pgs = m_psm[PSM_PSMZ16S].pgs = GSVector2i(64, 64);

	for(int i = 0; i < 8; i++) m_psm[PSM_PSMCT16].rowOffset[i] = rowOffset16;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMCT16S].rowOffset[i] = rowOffset16S;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMT8].rowOffset[i] = rowOffset8[((i + 2) >> 2) & 1];
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMT4].rowOffset[i] = rowOffset4[((i + 2) >> 2) & 1];
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ32].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ24].rowOffset[i] = rowOffset32Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ16].rowOffset[i] = rowOffset16Z;
	for(int i = 0; i < 8; i++) m_psm[PSM_PSMZ16S].rowOffset[i] = rowOffset16SZ;

	m_psm[PSM_PSMCT16].blockOffset = blockOffset16;
	m_psm[PSM_PSMCT16S].blockOffset = blockOffset16S;
	m_psm[PSM_PSMT8].blockOffset = blockOffset8;
	m_psm[PSM_PSMT4].blockOffset = blockOffset4;
	m_psm[PSM_PSMZ32].blockOffset = blockOffset32Z;
	m_psm[PSM_PSMZ24].blockOffset = blockOffset32Z;
	m_psm[PSM_PSMZ16].blockOffset = blockOffset16Z;
	m_psm[PSM_PSMZ16S].blockOffset = blockOffset16SZ;
}

GSLocalMemory::~GSLocalMemory()
{
	VirtualFree(m_vm8, 0, MEM_RELEASE);

	for(hash_map<uint32, Offset*>::iterator i = m_omap.begin(); i != m_omap.end(); i++)
	{
		Offset* o = (*i).second;
		
		_aligned_free(o->col[0]);

		_aligned_free(o);
	}

	for(hash_map<uint32, Offset4*>::iterator i = m_o4map.begin(); i != m_o4map.end(); i++)
	{
		_aligned_free((*i).second);
	}
}

GSLocalMemory::Offset* GSLocalMemory::GetOffset(uint32 bp, uint32 bw, uint32 psm)
{
	if(bw == 0) {ASSERT(0); return NULL;}

	ASSERT(m_psm[psm].bpp > 8); // only for 16/24/32/8h/4hh/4hl formats where all columns are the same

	uint32 hash = bp | (bw << 14) | (psm << 20);

	hash_map<uint32, Offset*>::iterator i = m_omap.find(hash);

	if(i != m_omap.end())
	{
		return (*i).second;
	}

	Offset* o = (Offset*)_aligned_malloc(sizeof(Offset), 16);

	o->hash = hash;

	pixelAddress pa = m_psm[psm].pa;

	for(int i = 0; i < 2048; i++)
	{
		o->row[i] = GSVector4i((int)pa(0, i, bp, bw));
	}

	int* p = (int*)_aligned_malloc(sizeof(int) * (2048 + 3) * 4, 16);

	for(int i = 0; i < 4; i++)
	{
		o->col[i] = &p[2048 * i + ((4 - (i & 3)) & 3)];

		memcpy(o->col[i], m_psm[psm].rowOffset[0], sizeof(int) * 2048);
	}

	m_omap[hash] = o;

	return o;
}

GSLocalMemory::Offset4* GSLocalMemory::GetOffset4(const GIFRegFRAME& FRAME, const GIFRegZBUF& ZBUF)
{
	uint32 fbp = FRAME.Block();
	uint32 zbp = ZBUF.Block();
	uint32 fpsm = FRAME.PSM;
	uint32 zpsm = ZBUF.PSM;
	uint32 bw = FRAME.FBW;

	ASSERT(m_psm[fpsm].trbpp > 8 || m_psm[zpsm].trbpp > 8);

	// "(psm & 0x0f) ^ ((psm & 0xf0) >> 2)" creates 4 bit unique identifiers for render target formats (only)

	uint32 fpsm_hash = (fpsm & 0x0f) ^ ((fpsm & 0x30) >> 2);
	uint32 zpsm_hash = (zpsm & 0x0f) ^ ((zpsm & 0x30) >> 2);

	uint32 hash = (FRAME.FBP << 0) | (ZBUF.ZBP << 9) | (bw << 18) | (fpsm_hash << 24) | (zpsm_hash << 28);

	hash_map<uint32, Offset4*>::iterator i = m_o4map.find(hash);

	if(i != m_o4map.end())
	{
		return (*i).second;
	}

	Offset4* o = (Offset4*)_aligned_malloc(sizeof(Offset4), 16);

	o->hash = hash;

	pixelAddress fpa = m_psm[fpsm].pa;
	pixelAddress zpa = m_psm[zpsm].pa;

	int fs = m_psm[fpsm].bpp >> 5;
	int zs = m_psm[zpsm].bpp >> 5;

	for(int i = 0; i < 2048; i++)
	{
		o->row[i].x = (int)fpa(0, i, fbp, bw) << fs;
		o->row[i].y = (int)zpa(0, i, zbp, bw) << zs;
	}

	for(int i = 0; i < 512; i++)
	{
		o->col[i].x = m_psm[fpsm].rowOffset[0][i * 4] << fs;
		o->col[i].y = m_psm[zpsm].rowOffset[0][i * 4] << zs;
	}

	m_o4map[hash] = o;

	return o;
}

////////////////////

template<int psm, int bsx, int bsy, bool aligned>
void GSLocalMemory::WriteImageColumn(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	const int csy = bsy / 4;

	for(int offset = srcpitch * csy; h >= csy; h -= csy, y += csy, src += offset)
	{
		for(int x = l; x < r; x += bsx)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WriteColumn32<aligned, 0xffffffff>(y, BlockPtr32(x, y, bp, bw), &src[x * 4], srcpitch); break;
			case PSM_PSMCT16: WriteColumn16<aligned>(y, BlockPtr16(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMCT16S: WriteColumn16<aligned>(y, BlockPtr16S(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMT8: WriteColumn8<aligned>(y, BlockPtr8(x, y, bp, bw), &src[x], srcpitch); break;
			case PSM_PSMT4: WriteColumn4<aligned>(y, BlockPtr4(x, y, bp, bw), &src[x >> 1], srcpitch); break;
			case PSM_PSMZ32: WriteColumn32<aligned, 0xffffffff>(y, BlockPtr32Z(x, y, bp, bw), &src[x * 4], srcpitch); break;
			case PSM_PSMZ16: WriteColumn16<aligned>(y, BlockPtr16Z(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMZ16S: WriteColumn16<aligned>(y, BlockPtr16SZ(x, y, bp, bw), &src[x * 2], srcpitch); break;
			// TODO
			default: __assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy, bool aligned>
void GSLocalMemory::WriteImageBlock(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	for(int offset = srcpitch * bsy; h >= bsy; h -= bsy, y += bsy, src += offset)
	{
		for(int x = l; x < r; x += bsx)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WriteBlock32<aligned, 0xffffffff>(BlockPtr32(x, y, bp, bw), &src[x * 4], srcpitch); break;
			case PSM_PSMCT16: WriteBlock16<aligned>(BlockPtr16(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMCT16S: WriteBlock16<aligned>(BlockPtr16S(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMT8: WriteBlock8<aligned>(BlockPtr8(x, y, bp, bw), &src[x], srcpitch); break;
			case PSM_PSMT4: WriteBlock4<aligned>(BlockPtr4(x, y, bp, bw), &src[x >> 1], srcpitch); break;
			case PSM_PSMZ32: WriteBlock32<aligned, 0xffffffff>(BlockPtr32Z(x, y, bp, bw), &src[x * 4], srcpitch); break;
			case PSM_PSMZ16: WriteBlock16<aligned>(BlockPtr16Z(x, y, bp, bw), &src[x * 2], srcpitch); break;
			case PSM_PSMZ16S: WriteBlock16<aligned>(BlockPtr16SZ(x, y, bp, bw), &src[x * 2], srcpitch); break;
			// TODO
			default: __assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy>
void GSLocalMemory::WriteImageLeftRight(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	for(; h > 0; y++, h--, src += srcpitch)
	{
		for(int x = l; x < r; x++)
		{
			switch(psm)
			{
			case PSM_PSMCT32: WritePixel32(x, y, *(uint32*)&src[x * 4], bp, bw); break;
			case PSM_PSMCT16: WritePixel16(x, y, *(uint16*)&src[x * 2], bp, bw); break;
			case PSM_PSMCT16S: WritePixel16S(x, y, *(uint16*)&src[x * 2], bp, bw); break;
			case PSM_PSMT8: WritePixel8(x, y, src[x], bp, bw); break;
			case PSM_PSMT4: WritePixel4(x, y, src[x >> 1] >> ((x & 1) << 2), bp, bw); break;
			case PSM_PSMZ32: WritePixel32Z(x, y, *(uint32*)&src[x * 4], bp, bw); break;
			case PSM_PSMZ16: WritePixel16Z(x, y, *(uint16*)&src[x * 2], bp, bw); break;
			case PSM_PSMZ16S: WritePixel16SZ(x, y, *(uint16*)&src[x * 2], bp, bw); break;
			// TODO
			default: __assume(0);
			}			
		}
	}
}

template<int psm, int bsx, int bsy, int trbpp>
void GSLocalMemory::WriteImageTopBottom(int l, int r, int y, int h, uint8* src, int srcpitch, const GIFRegBITBLTBUF& BITBLTBUF)
{
	__declspec(align(16)) uint8 buff[64]; // merge buffer for one column

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	const int csy = bsy / 4;

	// merge incomplete column

	int y2 = y & (csy - 1);

	if(y2 > 0)
	{
		int h2 = min(h, csy - y2);

		for(int x = l; x < r; x += bsx)
		{
			uint8* dst = NULL;

			switch(psm)
			{
			case PSM_PSMCT32: dst = BlockPtr32(x, y, bp, bw); break;
			case PSM_PSMCT16: dst = BlockPtr16(x, y, bp, bw); break;
			case PSM_PSMCT16S: dst = BlockPtr16S(x, y, bp, bw); break;
			case PSM_PSMT8: dst = BlockPtr8(x, y, bp, bw); break;
			case PSM_PSMT4: dst = BlockPtr4(x, y, bp, bw); break;
			case PSM_PSMZ32: dst = BlockPtr32Z(x, y, bp, bw); break;
			case PSM_PSMZ16: dst = BlockPtr16Z(x, y, bp, bw); break;
			case PSM_PSMZ16S: dst = BlockPtr16SZ(x, y, bp, bw); break;
			// TODO
			default: __assume(0);
			}

			switch(psm)
			{
			case PSM_PSMCT32:
			case PSM_PSMZ32: 
				ReadColumn32<true>(y, dst, buff, 32);
				memcpy(&buff[32], &src[x * 4], 32);
				WriteColumn32<true, 0xffffffff>(y, dst, buff, 32);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
			case PSM_PSMZ16:
			case PSM_PSMZ16S:
				ReadColumn16<true>(y, dst, buff, 32);
				memcpy(&buff[32], &src[x * 2], 32);
				WriteColumn16<true>(y, dst, buff, 32);
				break;
			case PSM_PSMT8: 
				ReadColumn8<true>(y, dst, buff, 16);
				for(int i = 0, j = y2; i < h2; i++, j++) memcpy(&buff[j * 16], &src[i * srcpitch + x], 16);
				WriteColumn8<true>(y, dst, buff, 16);
				break;
			case PSM_PSMT4: 
				ReadColumn4<true>(y, dst, buff, 16);
				for(int i = 0, j = y2; i < h2; i++, j++) memcpy(&buff[j * 16], &src[i * srcpitch + (x >> 1)], 16);
				WriteColumn4<true>(y, dst, buff, 16);
				break;
			// TODO
			default: 
				__assume(0);
			}
		}

		src += srcpitch * h2;
		y += h2;
		h -= h2;
	}

	// write whole columns

	{
		int h2 = h & ~(csy - 1);

		if(h2 > 0)
		{
			if(((DWORD_PTR)&src[l * trbpp >> 3] & 15) == 0 && (srcpitch & 15) == 0)
			{
				WriteImageColumn<psm, bsx, bsy, true>(l, r, y, h2, src, srcpitch, BITBLTBUF);
			}
			else
			{
				WriteImageColumn<psm, bsx, bsy, false>(l, r, y, h2, src, srcpitch, BITBLTBUF);
			}

			src += srcpitch * h2;
			y += h2;
			h -= h2;
		}
	}

	// merge incomplete column

	if(h >= 1)
	{
		for(int x = l; x < r; x += bsx)
		{
			uint8* dst = NULL;

			switch(psm)
			{
			case PSM_PSMCT32: dst = BlockPtr32(x, y, bp, bw); break;
			case PSM_PSMCT16: dst = BlockPtr16(x, y, bp, bw); break;
			case PSM_PSMCT16S: dst = BlockPtr16S(x, y, bp, bw); break;
			case PSM_PSMT8: dst = BlockPtr8(x, y, bp, bw); break;
			case PSM_PSMT4: dst = BlockPtr4(x, y, bp, bw); break;
			case PSM_PSMZ32: dst = BlockPtr32Z(x, y, bp, bw); break;
			case PSM_PSMZ16: dst = BlockPtr16Z(x, y, bp, bw); break;
			case PSM_PSMZ16S: dst = BlockPtr16SZ(x, y, bp, bw); break;
			// TODO
			default: __assume(0);
			}

			switch(psm)
			{
			case PSM_PSMCT32:
			case PSM_PSMZ32: 
				ReadColumn32<true>(y, dst, buff, 32);
				memcpy(&buff[0], &src[x * 4], 32);
				WriteColumn32<true, 0xffffffff>(y, dst, buff, 32);
				break;
			case PSM_PSMCT16:
			case PSM_PSMCT16S:
			case PSM_PSMZ16:
			case PSM_PSMZ16S:
				ReadColumn16<true>(y, dst, buff, 32);
				memcpy(&buff[0], &src[x * 2], 32);
				WriteColumn16<true>(y, dst, buff, 32);
				break;
			case PSM_PSMT8: 
				ReadColumn8<true>(y, dst, buff, 16);
				for(int i = 0; i < h; i++) memcpy(&buff[i * 16], &src[i * srcpitch + x], 16);
				WriteColumn8<true>(y, dst, buff, 16);
				break;
			case PSM_PSMT4: 
				ReadColumn4<true>(y, dst, buff, 16);
				for(int i = 0; i < h; i++) memcpy(&buff[i * 16], &src[i * srcpitch + (x >> 1)], 16);
				WriteColumn4<true>(y, dst, buff, 16);
				break;
			// TODO
			default: 
				__assume(0);
			}
		}
	}
}

template<int psm, int bsx, int bsy, int trbpp>
void GSLocalMemory::WriteImage(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	int l = (int)TRXPOS.DSAX;
	int r = l + (int)TRXREG.RRW;

	// finish the incomplete row first

	if(tx != l) 
	{
		int n = min(len, (r - tx) * trbpp >> 3);
		WriteImageX(tx, ty, src, n, BITBLTBUF, TRXPOS, TRXREG);
		src += n;
		len -= n;
	}

	int la = (l + (bsx - 1)) & ~(bsx - 1);
	int ra = r & ~(bsx - 1);
	int srcpitch = (r - l) * trbpp >> 3;
	int h = len / srcpitch;

	if(ra - la >= bsx && h > 0) // "transfer width" >= "block width" && there is at least one full row
	{
		uint8* s = &src[-l * trbpp >> 3];

		src += srcpitch * h;
		len -= srcpitch * h;

		// left part

		if(l < la)
		{
			WriteImageLeftRight<psm, bsx, bsy>(l, la, ty, h, s, srcpitch, BITBLTBUF);
		}

		// right part

		if(ra < r)
		{
			WriteImageLeftRight<psm, bsx, bsy>(ra, r, ty, h, s, srcpitch, BITBLTBUF);
		}

		// horizontally aligned part

		if(la < ra)
		{
			// top part

			{
				int h2 = min(h, bsy - (ty & (bsy - 1)));

				if(h2 < bsy)
				{
					WriteImageTopBottom<psm, bsx, bsy, trbpp>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);

					s += srcpitch * h2;
					ty += h2;
					h -= h2;
				}
			}

			// horizontally and vertically aligned part

			{
				int h2 = h & ~(bsy - 1);

				if(h2 > 0)
				{
					if(((DWORD_PTR)&s[la * trbpp >> 3] & 15) == 0 && (srcpitch & 15) == 0)
					{
						WriteImageBlock<psm, bsx, bsy, true>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);
					}
					else
					{
						WriteImageBlock<psm, bsx, bsy, false>(la, ra, ty, h2, s, srcpitch, BITBLTBUF);
					}

					s += srcpitch * h2;
					ty += h2;
					h -= h2;
				}
			}

			// bottom part

			if(h > 0)
			{
				WriteImageTopBottom<psm, bsx, bsy, trbpp>(la, ra, ty, h, s, srcpitch, BITBLTBUF);

				// s += srcpitch * h;
				ty += h;
				// h -= h;
			}
		}
	}

	// the rest

	if(len > 0)
	{
		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
}


#define IsTopLeftAligned(dsax, tx, ty, bw, bh) \
	((((int)dsax) & ((bw)-1)) == 0 && ((tx) & ((bw)-1)) == 0 && ((int)dsax) == (tx) && ((ty) & ((bh)-1)) == 0)

void GSLocalMemory::WriteImage24(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	int tw = TRXPOS.DSAX + TRXREG.RRW, srcpitch = TRXREG.RRW * 3;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock24(src + (x - tx) * 3, srcpitch, BlockPtr32(x, y, bp, bw));
			}
		}

		ty = th;
	}
}

void GSLocalMemory::WriteImage8H(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	int tw = TRXPOS.DSAX + TRXREG.RRW, srcpitch = TRXREG.RRW;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock8H(src + (x - tx), srcpitch, BlockPtr32(x, y, bp, bw));
			}
		}

		ty = th;
	}
}

void GSLocalMemory::WriteImage4HL(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	int tw = TRXPOS.DSAX + TRXREG.RRW, srcpitch = TRXREG.RRW / 2;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock4HL(src + (x - tx) / 2, srcpitch, BlockPtr32(x, y, bp, bw));
			}
		}

		ty = th;
	}
}

void GSLocalMemory::WriteImage4HH(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	int tw = TRXPOS.DSAX + TRXREG.RRW, srcpitch = TRXREG.RRW / 2;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock4HH(src + (x - tx) / 2, srcpitch, BlockPtr32(x, y, bp, bw));
			}
		}

		ty = th;
	}
}
void GSLocalMemory::WriteImage24Z(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(TRXREG.RRW == 0) return;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;

	int tw = TRXPOS.DSAX + TRXREG.RRW, srcpitch = TRXREG.RRW * 3;
	int th = len / srcpitch;

	bool aligned = IsTopLeftAligned(TRXPOS.DSAX, tx, ty, 8, 8);

	if(!aligned || (tw & 7) || (th & 7) || (len % srcpitch))
	{
		// TODO

		WriteImageX(tx, ty, src, len, BITBLTBUF, TRXPOS, TRXREG);
	}
	else
	{
		th += ty;

		for(int y = ty; y < th; y += 8, src += srcpitch * 8)
		{
			for(int x = tx; x < tw; x += 8)
			{
				UnpackAndWriteBlock24(src + (x - tx) * 3, srcpitch, BlockPtr32Z(x, y, bp, bw));
			}
		}

		ty = th;
	}
}
void GSLocalMemory::WriteImageX(int& tx, int& ty, uint8* src, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG)
{
	if(len <= 0) return;

	uint8* pb = (uint8*)src;
	uint16* pw = (uint16*)src;
	uint32* pd = (uint32*)src;

	uint32 bp = BITBLTBUF.DBP;
	uint32 bw = BITBLTBUF.DBW;
	psm_t* psm = &m_psm[BITBLTBUF.DPSM];

	int x = tx;
	int y = ty;
	int sx = (int)TRXPOS.DSAX;
	int ex = sx + (int)TRXREG.RRW;

	switch(BITBLTBUF.DPSM)
	{
	case PSM_PSMCT32:
	case PSM_PSMZ32:

		len /= 4;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pd++)
			{
				WritePixel32(addr + offset[x], *pd);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT24:
	case PSM_PSMZ24:

		len /= 3;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb += 3)
			{
				WritePixel24(addr + offset[x], *(uint32*)pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:

		len /= 2;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pw++)
			{
				WritePixel16(addr + offset[x], *pw);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				WritePixel8(addr + offset[x], *pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4(addr + offset[x + 0], *pb & 0xf);
				WritePixel4(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8H:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				WritePixel8H(addr + offset[x], *pb);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HL:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4HL(addr + offset[x + 0], *pb & 0xf);
				WritePixel4HL(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HH:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				WritePixel4HH(addr + offset[x + 0], *pb & 0xf);
				WritePixel4HH(addr + offset[x + 1], *pb >> 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	}

	tx = x;
	ty = y;
}

//

void GSLocalMemory::ReadImageX(int& tx, int& ty, uint8* dst, int len, GIFRegBITBLTBUF& BITBLTBUF, GIFRegTRXPOS& TRXPOS, GIFRegTRXREG& TRXREG) const
{
	if(len <= 0) return;

	uint8* pb = (uint8*)dst;
	uint16* pw = (uint16*)dst;
	uint32* pd = (uint32*)dst;

	uint32 bp = BITBLTBUF.SBP;
	uint32 bw = BITBLTBUF.SBW;
	psm_t* psm = &m_psm[BITBLTBUF.SPSM];

	int x = tx;
	int y = ty;
	int sx = (int)TRXPOS.SSAX;
	int ex = sx + (int)TRXREG.RRW;

	switch(BITBLTBUF.SPSM)
	{
	case PSM_PSMCT32:
	case PSM_PSMZ32:

		len /= 4;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pd++)
			{
				*pd = ReadPixel32(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT24:
	case PSM_PSMZ24:

		len /= 3;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb += 3)
			{
				uint32 c = ReadPixel32(addr + offset[x]);
				
				pb[0] = ((uint8*)&c)[0]; 
				pb[1] = ((uint8*)&c)[1]; 
				pb[2] = ((uint8*)&c)[2];
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMCT16:
	case PSM_PSMCT16S:
	case PSM_PSMZ16:
	case PSM_PSMZ16S:

		len /= 2;

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pw++)
			{
				*pw = ReadPixel16(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				*pb = ReadPixel8(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4(addr + offset[x + 0]) | (ReadPixel4(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT8H:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x++, pb++)
			{
				*pb = ReadPixel8H(addr + offset[x]);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;

	case PSM_PSMT4HL:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4HL(addr + offset[x + 0]) | (ReadPixel4HL(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	
	case PSM_PSMT4HH:

		while(len > 0)
		{
			uint32 addr = psm->pa(0, y, bp, bw);
			int* offset = psm->rowOffset[y & 7];

			for(; len > 0 && x < ex; len--, x += 2, pb++)
			{
				*pb = ReadPixel4HH(addr + offset[x + 0]) | (ReadPixel4HH(addr + offset[x + 1]) << 4);
			}

			if(x == ex) {x = sx; y++;}
		}

		break;
	}

	tx = x;
	ty = y;
}

///////////////////

void GSLocalMemory::ReadTexture32(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadBlock32<true>(BlockPtr32(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture24(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	if(TEXA.AEM)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<true>(BlockPtr32(x, y, bp, bw), dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<false>(BlockPtr32(x, y, bp, bw), dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>(BlockPtr16(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16S(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>(BlockPtr16S(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	FOREACH_BLOCK_START(16, 16, 32)
	{
		ReadAndExpandBlock8_32(BlockPtr8(x, y, bp, bw), dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint64* pal = m_clut;

	FOREACH_BLOCK_START(32, 16, 32)
	{
		ReadAndExpandBlock4_32(BlockPtr4(x, y, bp, bw), dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8H(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock8H_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HL(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock4HL_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HH(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadAndExpandBlock4HH_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture32Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 32)
	{
		ReadBlock32<true>(BlockPtr32Z(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture24Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	if(TEXA.AEM)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<true>(BlockPtr32Z(x, y, bp, bw), dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock24<false>(BlockPtr32Z(x, y, bp, bw), dst, dstpitch, TEXA);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16Z(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>(BlockPtr16Z(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SZ(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	FOREACH_BLOCK_START(16, 8, 32)
	{
		ReadBlock16<true>(BlockPtr16SZ(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

		ExpandBlock16(block, dst, dstpitch, TEXA);
	}
	FOREACH_BLOCK_END
}

///////////////////

void GSLocalMemory::ReadTextureBlock32(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock32<true>(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock24(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	if(TEXA.AEM)
	{
		ReadAndExpandBlock24<true>(BlockPtr(bp), dst, dstpitch, TEXA);
	}
	else
	{
		ReadAndExpandBlock24<false>(BlockPtr(bp), dst, dstpitch, TEXA);
	}
}

void GSLocalMemory::ReadTextureBlock16(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	ReadBlock16<true>(BlockPtr(bp), (uint8*)block, sizeof(block) / 8);

	ExpandBlock16(block, dst, dstpitch, TEXA);
}

void GSLocalMemory::ReadTextureBlock16S(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	ReadBlock16<true>(BlockPtr(bp), (uint8*)block, sizeof(block) / 8);

	ExpandBlock16(block, dst, dstpitch, TEXA);
}

void GSLocalMemory::ReadTextureBlock8(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadAndExpandBlock8_32(BlockPtr(bp), dst, dstpitch, m_clut);
}

void GSLocalMemory::ReadTextureBlock4(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadAndExpandBlock4_32(BlockPtr(bp), dst, dstpitch, m_clut);
}

void GSLocalMemory::ReadTextureBlock8H(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadAndExpandBlock8H_32(BlockPtr(bp), dst, dstpitch, m_clut);
}

void GSLocalMemory::ReadTextureBlock4HL(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadAndExpandBlock4HL_32(BlockPtr(bp), dst, dstpitch, m_clut);
}

void GSLocalMemory::ReadTextureBlock4HH(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadAndExpandBlock4HH_32(BlockPtr(bp), dst, dstpitch, m_clut);
}

void GSLocalMemory::ReadTextureBlock32Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock32<true>(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock24Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	if(TEXA.AEM)
	{
		ReadAndExpandBlock24<true>(BlockPtr(bp), dst, dstpitch, TEXA);
	}
	else
	{
		ReadAndExpandBlock24<false>(BlockPtr(bp), dst, dstpitch, TEXA);
	}
}

void GSLocalMemory::ReadTextureBlock16Z(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	ReadBlock16<true>(BlockPtr(bp), (uint8*)block, sizeof(block) / 8);

	ExpandBlock16(block, dst, dstpitch, TEXA);
}

void GSLocalMemory::ReadTextureBlock16SZ(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	__declspec(align(16)) uint16 block[16 * 8];

	ReadBlock16<true>(BlockPtr(bp), (uint8*)block, sizeof(block) / 8);

	ExpandBlock16(block, dst, dstpitch, TEXA);
}

///////////////////

void GSLocalMemory::ReadTexture(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
{
	readTexture rtx = m_psm[TEX0.PSM].rtx;
	readTexel rt = m_psm[TEX0.PSM].rt;
	GSVector2i bs = m_psm[TEX0.PSM].bs;

	if(r.width() < bs.x || r.height() < bs.y 
	|| (r.left & (bs.x - 1)) || (r.top & (bs.y - 1)) 
	|| (r.right & (bs.x - 1)) || (r.bottom & (bs.y - 1)))
	{
		ReadTexture<uint32>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}
///////////////////

void GSLocalMemory::ReadTexture16NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>(BlockPtr16(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>(BlockPtr16S(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(16, 16, 32)
		{
			ReadAndExpandBlock8_32(BlockPtr8(x, y, bp, bw), dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) uint8 block[16 * 16];

		FOREACH_BLOCK_START(16, 16, 16)
		{
			ReadBlock8<true>(BlockPtr8(x, y, bp, bw), (uint8*)block, sizeof(block) / 16);

			ExpandBlock8_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4NP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint64* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(32, 16, 32)
		{
			ReadAndExpandBlock4_32(BlockPtr4(x, y, bp, bw), dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) uint8 block[(32 / 2) * 16];

		FOREACH_BLOCK_START(32, 16, 16)
		{
			ReadBlock4<true>(BlockPtr4(x, y, bp, bw), (uint8*)block, sizeof(block) / 16);

			ExpandBlock4_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture8HNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock8H_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) uint32 block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>(BlockPtr32(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

			ExpandBlock8H_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4HLNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock4HL_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) uint32 block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>(BlockPtr32(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

			ExpandBlock4HL_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture4HHNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	const uint32* pal = m_clut;

	if(TEX0.CPSM == PSM_PSMCT32 || TEX0.CPSM == PSM_PSMCT24)
	{
		FOREACH_BLOCK_START(8, 8, 32)
		{
			ReadAndExpandBlock4HH_32(BlockPtr32(x, y, bp, bw), dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
	else
	{
		ASSERT(TEX0.CPSM == PSM_PSMCT16 || TEX0.CPSM == PSM_PSMCT16S);

		__declspec(align(16)) uint32 block[8 * 8];

		FOREACH_BLOCK_START(8, 8, 16)
		{
			ReadBlock32<true>(BlockPtr32(x, y, bp, bw), (uint8*)block, sizeof(block) / 8);

			ExpandBlock4HH_16(block, dst, dstpitch, pal);
		}
		FOREACH_BLOCK_END
	}
}

void GSLocalMemory::ReadTexture16ZNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>(BlockPtr16Z(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture16SZNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 8, 16)
	{
		ReadBlock16<true>(BlockPtr16SZ(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

///////////////////

void GSLocalMemory::ReadTextureNP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA)
{
	readTexture rtx = m_psm[TEX0.PSM].rtxNP;
	readTexel rt = m_psm[TEX0.PSM].rtNP;
	GSVector2i bs = m_psm[TEX0.PSM].bs;

	if(r.width() < bs.x || r.height() < bs.y 
	|| (r.left & (bs.x - 1)) || (r.top & (bs.y - 1)) 
	|| (r.right & (bs.x - 1)) || (r.bottom & (bs.y - 1)))
	{
		uint32 psm = TEX0.PSM;

		switch(psm)
		{
		case PSM_PSMT8:
		case PSM_PSMT8H:
		case PSM_PSMT4:
		case PSM_PSMT4HL:
		case PSM_PSMT4HH:
			psm = TEX0.CPSM;
			break;
		}

		switch(psm)
		{
		default:
		case PSM_PSMCT32:
		case PSM_PSMCT24:
			ReadTexture<uint32>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			ReadTexture<uint16>(r, dst, dstpitch, TEX0, TEXA, rt, rtx);
			break;
		}
	}
	else
	{
		(this->*rtx)(r, dst, dstpitch, TEX0, TEXA);
	}
}

// 32/8

void GSLocalMemory::ReadTexture8P(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(16, 16, 8)
	{
		ReadBlock8<true>(BlockPtr8(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4P(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(32, 16, 8)
	{
		ReadBlock4P(BlockPtr4(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture8HP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock8HP(BlockPtr32(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HLP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock4HLP(BlockPtr32(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

void GSLocalMemory::ReadTexture4HHP(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA) const
{
	FOREACH_BLOCK_START(8, 8, 8)
	{
		ReadBlock4HHP(BlockPtr32(x, y, bp, bw), dst, dstpitch);
	}
	FOREACH_BLOCK_END
}

//

void GSLocalMemory::ReadTextureBlock8P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ReadBlock8<true>(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock4P(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock4P(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock8HP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock8HP(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock4HLP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock4HLP(BlockPtr(bp), dst, dstpitch);
}

void GSLocalMemory::ReadTextureBlock4HHP(uint32 bp, uint8* dst, int dstpitch, const GIFRegTEXA& TEXA) const
{
	ALIGN_STACK(16);

	ReadBlock4HHP(BlockPtr(bp), dst, dstpitch);
}

//

template<typename T> 
void GSLocalMemory::ReadTexture(const GSVector4i& r, uint8* dst, int dstpitch, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, readTexel rt, readTexture rtx)
{
	GSVector4i cr = r.ralign<GSVector4i::Inside>(m_psm[TEX0.PSM].bs);

	bool aligned = ((DWORD_PTR)(dst + (cr.left - r.left) * sizeof(T)) & 0xf) == 0;

	if(cr.rempty() || !aligned)
	{
		// TODO: expand r to block size, read into temp buffer, copy to r (like above)

if(!aligned) 
printf("unaligned memory pointer passed to ReadTexture\n");

		for(int y = r.top; y < r.bottom; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
	}
	else
	{
		for(int y = r.top; y < cr.top; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);

		if(!cr.rempty())
			(this->*rtx)(cr, dst + (cr.left - r.left) * sizeof(T), dstpitch, TEX0, TEXA);

		for(int y = cr.top; y < cr.bottom; y++, dst += dstpitch)
		{
			for(int x = r.left, i = 0; x < cr.left; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
			for(int x = cr.right, i = x - r.left; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
		}

		for(int y = cr.bottom; y < r.bottom; y++, dst += dstpitch)
			for(int x = r.left, i = 0; x < r.right; x++, i++)
				((T*)dst)[i] = (T)(this->*rt)(x, y, TEX0, TEXA);
	}
}

HRESULT GSLocalMemory::SaveBMP(const string& fn, uint32 bp, uint32 bw, uint32 psm, int w, int h)
{
	int pitch = w * 4;
	int size = pitch * h;
	void* bits = ::_aligned_malloc(size, 16);

	GIFRegTEX0 TEX0;

	TEX0.TBP0 = bp;
	TEX0.TBW = bw;
	TEX0.PSM = psm;

	GIFRegTEXA TEXA;

	TEXA.AEM = 0;
	TEXA.TA0 = 0;
	TEXA.TA1 = 0x80;

	readPixel rp = m_psm[psm].rp;

	uint8* p = (uint8*)bits;

	for(int j = h-1; j >= 0; j--, p += pitch)
		for(int i = 0; i < w; i++)
			((uint32*)p)[i] = (this->*rp)(i, j, TEX0.TBP0, TEX0.TBW);

	if(FILE* fp = fopen(fn.c_str(), "wb"))
	{
		BITMAPINFOHEADER bih;
		memset(&bih, 0, sizeof(bih));
        bih.biSize = sizeof(bih);
        bih.biWidth = w;
        bih.biHeight = h;
        bih.biPlanes = 1;
        bih.biBitCount = 32;
        bih.biCompression = BI_RGB;
        bih.biSizeImage = size;

		BITMAPFILEHEADER bfh;
		memset(&bfh, 0, sizeof(bfh));
		bfh.bfType = 'MB';
		bfh.bfOffBits = sizeof(bfh) + sizeof(bih);
		bfh.bfSize = bfh.bfOffBits + size;
		bfh.bfReserved1 = bfh.bfReserved2 = 0;

		fwrite(&bfh, 1, sizeof(bfh), fp);
		fwrite(&bih, 1, sizeof(bih), fp);
		fwrite(bits, 1, size, fp);

		fclose(fp);
	}

	::_aligned_free(bits);

	return true;
}
