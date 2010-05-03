/*  GSnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

 #ifndef __GIF_TRANSFER_H__
 #define __GIF_TRANSFER_H__

#include <assert.h>
#include <algorithm>

 struct GSRegSIGBLID
{
	u32 SIGID;
	u32 LBLID;
};

enum GIF_FLG
{
	GIF_FLG_PACKED		= 0,
	GIF_FLG_REGLIST		= 1,
	GIF_FLG_IMAGE		= 2,
	GIF_FLG_IMAGE2		= 3
};

enum GIF_PATH
{
	GIF_PATH_1 = 0,
	GIF_PATH_2,
	GIF_PATH_3,
};

enum GIF_REG
{
	GIF_REG_PRIM		= 0x00,
	GIF_REG_RGBA		= 0x01,
	GIF_REG_STQ		= 0x02,
	GIF_REG_UV			= 0x03,
	GIF_REG_XYZF2		= 0x04,
	GIF_REG_XYZ2		= 0x05,
	GIF_REG_TEX0_1		= 0x06,
	GIF_REG_TEX0_2		= 0x07,
	GIF_REG_CLAMP_1	= 0x08,
	GIF_REG_CLAMP_2	= 0x09,
	GIF_REG_FOG		= 0x0a,
	GIF_REG_XYZF3		= 0x0c,
	GIF_REG_XYZ3		= 0x0d,
	GIF_REG_A_D			= 0x0e,
	GIF_REG_NOP		= 0x0f,
};

enum GIF_A_D_REG
{
	GIF_A_D_REG_PRIM		= 0x00,
	GIF_A_D_REG_RGBAQ		= 0x01,
	GIF_A_D_REG_ST			= 0x02,
	GIF_A_D_REG_UV			= 0x03,
	GIF_A_D_REG_XYZF2		= 0x04,
	GIF_A_D_REG_XYZ2		= 0x05,
	GIF_A_D_REG_TEX0_1		= 0x06,
	GIF_A_D_REG_TEX0_2		= 0x07,
	GIF_A_D_REG_CLAMP_1		= 0x08,
	GIF_A_D_REG_CLAMP_2		= 0x09,
	GIF_A_D_REG_FOG			= 0x0a,
	GIF_A_D_REG_XYZF3		= 0x0c,
	GIF_A_D_REG_XYZ3		= 0x0d,
	GIF_A_D_REG_NOP			= 0x0f,
	GIF_A_D_REG_TEX1_1		= 0x14,
	GIF_A_D_REG_TEX1_2		= 0x15,
	GIF_A_D_REG_TEX2_1		= 0x16,
	GIF_A_D_REG_TEX2_2		= 0x17,
	GIF_A_D_REG_XYOFFSET_1	= 0x18,
	GIF_A_D_REG_XYOFFSET_2	= 0x19,
	GIF_A_D_REG_PRMODECONT	= 0x1a,
	GIF_A_D_REG_PRMODE		= 0x1b,
	GIF_A_D_REG_TEXCLUT		= 0x1c,
	GIF_A_D_REG_SCANMSK		= 0x22,
	GIF_A_D_REG_MIPTBP1_1	= 0x34,
	GIF_A_D_REG_MIPTBP1_2	= 0x35,
	GIF_A_D_REG_MIPTBP2_1	= 0x36,
	GIF_A_D_REG_MIPTBP2_2	= 0x37,
	GIF_A_D_REG_TEXA		= 0x3b,
	GIF_A_D_REG_FOGCOL		= 0x3d,
	GIF_A_D_REG_TEXFLUSH	= 0x3f,
	GIF_A_D_REG_SCISSOR_1	= 0x40,
	GIF_A_D_REG_SCISSOR_2	= 0x41,
	GIF_A_D_REG_ALPHA_1		= 0x42,
	GIF_A_D_REG_ALPHA_2		= 0x43,
	GIF_A_D_REG_DIMX		= 0x44,
	GIF_A_D_REG_DTHE		= 0x45,
	GIF_A_D_REG_COLCLAMP	= 0x46,
	GIF_A_D_REG_TEST_1		= 0x47,
	GIF_A_D_REG_TEST_2		= 0x48,
	GIF_A_D_REG_PABE		= 0x49,
	GIF_A_D_REG_FBA_1		= 0x4a,
	GIF_A_D_REG_FBA_2		= 0x4b,
	GIF_A_D_REG_FRAME_1		= 0x4c,
	GIF_A_D_REG_FRAME_2		= 0x4d,
	GIF_A_D_REG_ZBUF_1		= 0x4e,
	GIF_A_D_REG_ZBUF_2		= 0x4f,
	GIF_A_D_REG_BITBLTBUF	= 0x50,
	GIF_A_D_REG_TRXPOS		= 0x51,
	GIF_A_D_REG_TRXREG		= 0x52,
	GIF_A_D_REG_TRXDIR		= 0x53,
	GIF_A_D_REG_HWREG		= 0x54,
	GIF_A_D_REG_SIGNAL		= 0x60,
	GIF_A_D_REG_FINISH		= 0x61,
	GIF_A_D_REG_LABEL		= 0x62,
};

struct GIFTAG
{
	u32 nloop : 15;
	u32 eop : 1;
	u32 dummy0 : 16;
	u32 dummy1 : 14;
	u32 pre : 1;
	u32 prim : 11;
	u32 flg : 2;
	u32 nreg : 4;
	u32 regs[2];

	GIFTAG() {}
};

struct GIFPath
{
	GIFTAG tag;
	u32 curreg;
	u32 _pad[3];
	u8 regs[16];

	__forceinline void PrepRegs();
	void SetTag(const void* mem);
	u32 GetReg();
	bool StepReg();
};

extern GIFPath m_path[3];
extern bool Path3transfer;

extern void _GSgifTransfer1(u32 *pMem, u32 addr);
extern void _GSgifTransfer2(u32 *pMem, u32 size);
extern void _GSgifTransfer3(u32 *pMem, u32 size);

#endif
