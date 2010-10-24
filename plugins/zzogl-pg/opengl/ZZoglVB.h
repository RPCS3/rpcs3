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

// Zerogs:VB implementation.
// VB stands for Visual Buffer, as I think

#ifndef ZZOGLVB_H_INCLUDED
#define ZZOGLVB_H_INCLUDED

#include "targets.h"

extern const GLenum primtype[8];

class VB
{
	public:
		VB();
		~VB();

		void Destroy();

		inline bool CheckPrim()
		{
			static const int PRIMMASK = 0x0e;   // for now ignore 0x10 (AA)

			if ((PRIMMASK & prim->_val) != (PRIMMASK & curprim._val) || primtype[prim->prim] != primtype[curprim.prim])
				return nCount > 0;

			return false;
		}

		void CheckFrame(int tbp);

		// context specific state
		Point offset;
		Rect2 scissor;
		tex0Info tex0;
		tex1Info tex1;
		miptbpInfo miptbp0;
		miptbpInfo miptbp1;
		alphaInfo alpha;
		fbaInfo fba;
		clampInfo clamp;
		pixTest test;
		u32 ptexClamp[2]; // textures for x and y dir region clamping

		void FlushTexData();
		inline int CheckFrameAddConstraints(int tbp);
		inline void CheckScissors(int maxpos);
		inline void CheckFrame32bitRes(int maxpos);
		inline int FindMinimalMemoryConstrain(int tbp, int maxpos);
		inline int FindZbufferMemoryConstrain(int tbp, int maxpos);
		inline int FindMinimalHeightConstrain(int maxpos);

		inline int CheckFrameResolveRender(int tbp);
		inline void CheckFrame16vs32Conversion();
		inline int CheckFrameResolveDepth(int tbp);

		inline void FlushTexUnchangedClutDontUpdate() ;
		inline void FlushTexClutDontUpdate() ;
		inline void FlushTexClutting() ;
		inline void FlushTexSetNewVars(u32 psm) ;

		// Increase the size of pbuf
		void IncreaseVertexBuffer()
		{
			assert(pBufferData != NULL && nCount > nNumVertices);
			nNumVertices *= 2;
			VertexGPU* ptemp = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU) * nNumVertices, 256);
			memcpy_amd(ptemp, pBufferData, sizeof(VertexGPU) * nCount);
			assert(nCount <= nNumVertices);
			_aligned_free(pBufferData);
			pBufferData = ptemp;
		}

		void Init(int nVerts)
		{
			if (pBufferData == NULL && nVerts > 0)
			{
				pBufferData = (VertexGPU*)_aligned_malloc(sizeof(VertexGPU) * nVerts, 256);
				nNumVertices = nVerts;
			}

			nCount = 0;
		}

		u8 bNeedFrameCheck;
		u8 bNeedZCheck;
		u8 bNeedTexCheck;
		u8 dummy0;

		union
		{
			struct
			{
				u8 bTexConstsSync; // only pixel shader constants that context owns
				u8 bVarsTexSync; // texture info
				u8 bVarsSetTarg;
				u8 dummy1;
			};

			u32 bSyncVars;
		};

		int ictx;
		VertexGPU* pBufferData; // current allocated data

		int nNumVertices;   // size of pBufferData in terms of VertexGPU objects
		int nCount;
		primInfo curprim;	// the previous prim the current buffers are set to

		zbufInfo zbuf;
		frameInfo gsfb; // the real info set by FRAME cmd
		frameInfo frame;
		int zprimmask; // zmask for incoming points

		union
		{
			u32 uCurTex0Data[2]; // current tex0 data
			GIFRegTEX0 uCurTex0;	
		};
		u32 uNextTex0Data[2]; // tex0 data that has to be applied if bNeedTexCheck is 1

		//int nFrameHeights[8];	// frame heights for the past frame changes
		int nNextFrameHeight;

		CMemoryTarget* pmemtarg; // the current mem target set
		CRenderTarget* prndr;
		CDepthTarget* pdepth;

};

// VB variables
extern VB vb[2];

#endif // ZZOGLVB_H_INCLUDED
