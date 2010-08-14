/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// Save and Load.

//------------------ Includes
#include "zerogs.h"
#include "targets.h"

//----------------------- Defines

#define VBSAVELIMIT ((u32)((u8*)&vb[0].nNextFrameHeight-(u8*)&vb[0]))
#define ZEROGS_SAVEVER 0xaa000005

//------------------ Variables

// Hack for save game compatible!
#ifdef _DEBUG
char *libraryNameX	 = "ZeroGS-Pg OpenGL (Debug) ";
#elif defined(ZEROGS_DEVBUILD)
char *libraryNameX	 = "ZeroGS-Pg OpenGL (Dev) ";
#else
char *libraryNameX	 = "ZeroGS Playground OpenGL ";
#endif

//------------------ Code

extern char *libraryName;
extern u32 s_uTex1Data[2][2], s_uClampData[2];

int ZeroGS::Save(s8* pbydata)
{
	if (pbydata == NULL)
		return 40 + MEMORY_END + sizeof(gs) + 2*VBSAVELIMIT + 2*sizeof(frameInfo) + 4 + 256*4;

	s_RTs.ResolveAll();
	s_DepthRTs.ResolveAll();

	strcpy((char*)pbydata, libraryNameX);

	*(u32*)(pbydata + 16) = ZEROGS_SAVEVER;

	pbydata += 32;
	*(int*)pbydata = icurctx;

	pbydata += 4;
	*(int*)pbydata = VBSAVELIMIT;

	pbydata += 4;

	memcpy(pbydata, g_pbyGSMemory, MEMORY_END);
	pbydata += MEMORY_END;

	memcpy(pbydata, g_pbyGSClut, 256*4);
	pbydata += 256 * 4;

	*(int*)pbydata = sizeof(gs);
	pbydata += 4;

	memcpy(pbydata, &gs, sizeof(gs));
	pbydata += sizeof(gs);

	for (int i = 0; i < 2; ++i)
	{
		memcpy(pbydata, &vb[i], VBSAVELIMIT);
		pbydata += VBSAVELIMIT;
	}

	return 0;
}

bool ZeroGS::Load(s8* pbydata)
{
	memset(s_uTex1Data, 0, sizeof(s_uTex1Data));
	memset(s_uClampData, 0, sizeof(s_uClampData));

	g_nCurVBOIndex = 0;

	// first 32 bytes are the id
	u32 savever = *(u32*)(pbydata + 16);

	if (strncmp((char*)pbydata, libraryNameX, 6) == 0 && (savever == ZEROGS_SAVEVER || savever == 0xaa000004))
	{
		g_MemTargs.Destroy();

		GSStateReset();
		pbydata += 32;

		//int context = *(int*)pbydata;
		pbydata += 4;
		u32 savelimit = VBSAVELIMIT;

		savelimit = *(u32*)pbydata;
		pbydata += 4;

		memcpy(g_pbyGSMemory, pbydata, MEMORY_END);
		pbydata += MEMORY_END;

		memcpy(g_pbyGSClut, pbydata, 256*4);
		pbydata += 256 * 4;

		memset(&gs, 0, sizeof(gs));

		int savedgssize;

		if (savever == 0xaa000004)
		{
			savedgssize = 0x1d0;
		}
		else
		{
			savedgssize = *(int*)pbydata;
			pbydata += 4;
		}

		memcpy(&gs, pbydata, savedgssize);

		pbydata += savedgssize;
		prim = &gs._prim[gs.prac];

		vb[0].Destroy();
		memcpy(&vb[0], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[0].pBufferData = NULL;

		vb[1].Destroy();
		memcpy(&vb[1], pbydata, min(savelimit, VBSAVELIMIT));
		pbydata += savelimit;
		vb[1].pBufferData = NULL;

		for (int i = 0; i < 2; ++i)
		{
			vb[i].Init(VB_BUFFERSIZE);
			vb[i].bNeedZCheck = vb[i].bNeedFrameCheck = 1;

			vb[i].bSyncVars = 0;
			vb[i].bNeedTexCheck = 1;
			memset(vb[i].uCurTex0Data, 0, sizeof(vb[i].uCurTex0Data));
		}

		icurctx = -1;

		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, s_uFramebuffer);   // switch to the backbuffer
		SetFogColor(gs.fogcol);

		GL_REPORT_ERRORD();
		return true;
	}

	return false;
}

