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

#ifndef __ZEROGS_TARGETS_H__
#define __ZEROGS_TARGETS_H__

#define TARGET_VIRTUAL_KEY 0x80000000
#include "PS2Edefs.h"

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE GL_TEXTURE_RECTANGLE_NV
#endif

namespace ZeroGS
{

inline u32 GetFrameKey(int fbp, int fbw, VB& curvb);

// manages render targets

class CRenderTargetMngr
{
	public:
		typedef map<u32, CRenderTarget*> MAPTARGETS;

		enum TargetOptions
		{
			TO_DepthBuffer = 1,
			TO_StrictHeight = 2, // height returned has to be the same as requested
			TO_Virtual = 4
		};

		~CRenderTargetMngr() { Destroy(); }

		void Destroy();
		static MAPTARGETS::iterator GetOldestTarg(MAPTARGETS& m);

		CRenderTarget* GetTarg(const frameInfo& frame, u32 Options, int maxposheight);
		inline CRenderTarget* GetTarg(int fbp, int fbw, VB& curvb)
		{
			MAPTARGETS::iterator it = mapTargets.find(GetFrameKey(fbp, fbw, curvb));

			/*			if (fbp == 0x3600 && fbw == 0x100 && it == mapTargets.end())
						{
							printf("%x\n", GetFrameKey(fbp, fbw, curvb)) ;
							printf("%x %x\n", fbp, fbw);
							for(MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
								printf ("\t %x %x %x %x\n", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
						}*/
			return it != mapTargets.end() ? it->second : NULL;
		}

		// gets all targets with a range
		void GetTargs(int start, int end, list<ZeroGS::CRenderTarget*>& listTargets) const;

		// resolves all targets within a range
		__forceinline void Resolve(int start, int end);
		__forceinline void ResolveAll()
		{
			for (MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it)
				it->second->Resolve();
		}

		void DestroyAllTargs(int start, int end, int fbw);
		void DestroyIntersecting(CRenderTarget* prndr);

		// promotes a target from virtual to real
		inline CRenderTarget* Promote(u32 key)
		{
			assert(!(key & TARGET_VIRTUAL_KEY));

			// promote to regular targ
			CRenderTargetMngr::MAPTARGETS::iterator it = mapTargets.find(key | TARGET_VIRTUAL_KEY);
			assert(it != mapTargets.end());

			CRenderTarget* ptarg = it->second;
			mapTargets.erase(it);

			DestroyIntersecting(ptarg);

			it = mapTargets.find(key);

			if (it != mapTargets.end())
			{
				DestroyTarg(it->second);
				it->second = ptarg;
			}
			else
				mapTargets[key] = ptarg;

			if (conf.settings().resolve_promoted)
				ptarg->status = CRenderTarget::TS_Resolved;
			else
				ptarg->status = CRenderTarget::TS_NeedUpdate;

			return ptarg;
		}

		static void DestroyTarg(CRenderTarget* ptarg);
		void PrintTargets();
		MAPTARGETS mapTargets, mapDummyTargs;
};

class CMemoryTargetMngr
{

	public:
		CMemoryTargetMngr() : curstamp(0) {}

		CMemoryTarget* GetMemoryTarget(const tex0Info& tex0, int forcevalidate); // pcbp is pointer to start of clut
		CMemoryTarget* MemoryTarget_SearchExistTarget(int start, int end, int nClutOffset, int clutsize, const tex0Info& tex0, int forcevalidate);
		CMemoryTarget* MemoryTarget_ClearedTargetsSearch(int fmt, int widthmult, int channels, int height);

		void Destroy(); // destroy all targs

		void ClearRange(int starty, int endy); // set all targets to cleared
		void DestroyCleared(); // flush all cleared targes
		void DestroyOldest();

		list<CMemoryTarget> listTargets, listClearedTargets;
		u32 curstamp;

	private:
		list<CMemoryTarget>::iterator DestroyTargetIter(list<CMemoryTarget>::iterator& it);
};

class CBitwiseTextureMngr
{
	public:
		~CBitwiseTextureMngr() { Destroy(); }

		void Destroy();

		// since GetTex can delete textures to free up mem, it is dangerous if using that texture, so specify at least one other tex to save
		__forceinline u32 GetTex(u32 bitvalue, u32 ptexDoNotDelete)
		{
			map<u32, u32>::iterator it = mapTextures.find(bitvalue);

			if (it != mapTextures.end()) return it->second;

			return GetTexInt(bitvalue, ptexDoNotDelete);
		}

	private:
		u32 GetTexInt(u32 bitvalue, u32 ptexDoNotDelete);

		map<u32, u32> mapTextures;
};

// manages

class CRangeManager
{
	public:
		CRangeManager()
		{
			ranges.reserve(16);
		}

		// [start, end)

		struct RANGE
		{
			RANGE() {}

			inline RANGE(int start, int end) : start(start), end(end) {}

			int start, end;
		};

		// works in semi logN
		void Insert(int start, int end);
		void RangeSanityCheck();
		inline void Clear()
		{
			ranges.resize(0);
		}

		vector<RANGE> ranges; // organized in ascending order, non-intersecting
};

extern CRenderTargetMngr s_RTs, s_DepthRTs;
extern CBitwiseTextureMngr s_BitwiseTextures;
extern CMemoryTargetMngr g_MemTargs;

extern u8 s_AAx, s_AAy;

// Real rendered width, depends on AA and AAneg.
inline int RW(int tbw)
{
    return (tbw << s_AAx);
}

// Real rendered height, depends on AA and AAneg.
inline int RH(int tbh)
{
    return (tbh << s_AAy);
}

/*	inline void CreateTargetsList(int start, int end, list<ZeroGS::CRenderTarget*>& listTargs) {
		s_DepthRTs.GetTargs(start, end, listTargs);
		s_RTs.GetTargs(start, end, listTargs);
	}*/

// This pattern of functions is called 3 times, so I add creating Targets list into one.
inline list<ZeroGS::CRenderTarget*> CreateTargetsList(int start, int end)
{
	list<ZeroGS::CRenderTarget*> listTargs;
	s_DepthRTs.GetTargs(start, end, listTargs);
	s_RTs.GetTargs(start, end, listTargs);
	return listTargs;
}

extern Vector g_vdepth;
extern int icurctx;
extern GLuint vboRect;

// Unworking
#define PSMPOSITION 28

// Code width and height of frame into key, that used in targetmanager
// This is 3 variants of one function, Key dependant on fbp and fbw.
inline u32 GetFrameKey(const frameInfo& frame)
{
	return (((frame.fbw) << 16) | (frame.fbp));
}

inline u32 GetFrameKey(CRenderTarget* frame)
{
	return (((frame->fbw) << 16) | (frame->fbp));
}

inline u32 GetFrameKey(int fbp, int fbw,  VB& curvb)
{
	return (((fbw) << 16) | (fbp));
}

inline u16 ShiftHeight(int fbh, int fbp, int fbhCalc)
{
	return fbh;
}

//#define FRAME_KEY_BY_FBH

//FIXME: this code is for P4 and KH1. It should not be so strange!
//Dummy targets was deleted from mapTargets, but not erased.
inline u32 GetFrameKeyDummy(int fbp, int fbw, int fbh, int psm)
{
//	if (fbp > 0x2000 && ZZOgl_fbh_Calc(fbp, fbw, psm) < 0x400 && ZZOgl_fbh_Calc(fbp, fbw, psm) != fbh)
//		ZZLog::Debug_Log("Z %x %x %x %x\n", fbh, fbhCalc, fbp, ZZOgl_fbh_Calc(fbp, fbw, psm));
	// height over 1024 would shrink to 1024, so dummy targets with calculated size more than 0x400 should be
	// distinct by real height. But in FFX there is 3e0 height target, so I put 0x300 as limit.

#ifndef FRAME_KEY_BY_FBH	
	int calc = ZZOgl_fbh_Calc(fbp, fbw, psm);
	if (/*fbp > 0x2000 && */calc < /*0x300*/0x2E0)
		return ((fbw << 16) | calc);
	else
#endif
		return ((fbw << 16) | fbh);
}

inline u32 GetFrameKeyDummy(const frameInfo& frame)
{
	return GetFrameKeyDummy(frame.fbp, frame.fbw, frame.fbh, frame.psm);
}

inline u32 GetFrameKeyDummy(CRenderTarget* frame)
{
	return GetFrameKeyDummy(frame->fbp, frame->fbw, frame->fbh, frame->psm);
}

} // End of namespace

#include "Mem.h"

static __forceinline void DrawTriangleArray()
{
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL_REPORT_ERRORD();
}

static __forceinline void DrawBuffers(GLenum *buffer)
{
	if (glDrawBuffers != NULL) 
	{
		glDrawBuffers(1, buffer);
	}

	GL_REPORT_ERRORD();
}

static __forceinline void FBTexture(int attach, int id = 0)
{
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + attach, GL_TEXTURE_RECTANGLE_NV, id, 0);
	GL_REPORT_ERRORD();
}

static __forceinline void Texture2D(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage2D(GL_TEXTURE_2D, 0, iFormat, width, height, 0, format, type, pixels);
}

static __forceinline void Texture2D(GLint iFormat, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage2D(GL_TEXTURE_2D, 0, iFormat, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, 0, format, type, pixels);
}

static __forceinline void Texture3D(GLint iFormat, GLint width, GLint height, GLint depth, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage3D(GL_TEXTURE_3D, 0, iFormat, width, height, depth, 0, format, type, pixels);
}
	
static __forceinline void TextureRect(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, iFormat, width, height, 0, format, type, pixels);
}

static __forceinline void TextureRect2(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, iFormat, width, height, 0, format, type, pixels);
}

static __forceinline void TextureRect(GLenum attach, GLuint id = 0)
{
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, attach, GL_RENDERBUFFER_EXT, id);
}

static __forceinline void setTex2DFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setTex2DWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, type);
}

static __forceinline void setTex3DFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setTex3DWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, type);
}

static __forceinline void setRectFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setRectWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, type);
}

static __forceinline void setRectWrap2(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, type);
}
	
#endif
