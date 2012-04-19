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
#include <list>
#include <map>
#include "GS.h"
#include "ZZGl.h"
//#include "ZZoglVB.h"

#ifndef GL_TEXTURE_RECTANGLE
#define GL_TEXTURE_RECTANGLE GL_TEXTURE_RECTANGLE_NV
#endif

#define VB_BUFFERSIZE			   0x4000

extern void FlushIfNecesary(void* ptr);
extern bool g_bSaveZUpdate;

// all textures have this width
extern int GPU_TEXWIDTH;
extern float g_fiGPU_TEXWIDTH;
#define MASKDIVISOR		0							// Used for decrement bitwise mask texture size if 1024 is too big
#define GPU_TEXMASKWIDTH	(1024 >> MASKDIVISOR)	// bitwise mask width for region repeat mode

// managers render-to-texture targets
class CRenderTarget
{

	public:
		CRenderTarget();
		virtual ~CRenderTarget();

		virtual bool Create(const frameInfo& frame);
		virtual void Destroy();

		// set the GPU_POSXY variable, scissor rect, and current render target
		void SetTarget(int fbplocal, const Rect2& scissor, int context);
		void SetViewport();

		// copies/creates the feedback contents
		inline void CreateFeedback()
		{
			if (ptexFeedback == 0 || !(status&TS_FeedbackReady))
				_CreateFeedback();
		}

		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* pdepth);
		virtual void ConvertTo32(); // converts a psm==2 target, to a psm==0
		virtual void ConvertTo16(); // converts a psm==0 target, to a psm==2

		virtual bool IsDepth() { return false; }

		void SetRenderTarget(int targ);

		void* psys;   // system data used for comparison
		u32 ptex;

		int fbp, fbw, fbh, fbhCalc; // if fbp is negative, virtual target (not mapped to any real addr)
		int start, end; // in bytes
		u32 lastused;	// time stamp since last used
		float4 vposxy;

		u32 fbm;
		u16 status;
		u8 psm;
		u8 resv0;
		Rect scissorrect;

		u8 created;	// Check for object destruction/creating for r201.

		//int startresolve, endresolve;
		u32 nUpdateTarg; // use this target to update the texture if non 0 (one time only)

		// this is optionally used when feedback effects are used (render target is used as a texture when rendering to itself)
		u32 ptexFeedback;

		enum TargetStatus
		{
			TS_Resolved = 1,
			TS_NeedUpdate = 2,
			TS_Virtual = 4, // currently not mapped to memory
			TS_FeedbackReady = 8, // feedback effect is ready and doesn't need to be updated
			TS_NeedConvert32 = 16,
			TS_NeedConvert16 = 32
		};
		float4 DefaultBitBltPos();
		float4 DefaultBitBltTex();

	private:
		void _CreateFeedback();
		inline bool InitialiseDefaultTexture(u32 *p_ptr, int fbw, int fbh) ;
};

// manages zbuffers

class CDepthTarget : public CRenderTarget
{

	public:
		CDepthTarget();
		virtual ~CDepthTarget();

		virtual bool Create(const frameInfo& frame);
		virtual void Destroy();

		virtual void Resolve();
		virtual void Resolve(int startrange, int endrange); // resolves only in the allowed range
		virtual void Update(int context, CRenderTarget* prndr);

		virtual bool IsDepth() { return true; }

		void SetDepthStencilSurface();

		u32 pdepth;		 // 24 bit, will contain the stencil buffer if possible
		u32 pstencil;	   // if not 0, contains the stencil buffer
		int icount;		 // internal counter
};

// manages contiguous chunks of memory (width is always 1024)

class CMemoryTarget
{
	public:
		struct TEXTURE
		{
			inline TEXTURE() : tex(0), memptr(NULL), ref(0) {}
			inline ~TEXTURE() { glDeleteTextures(1, &tex); _aligned_free(memptr); }

			u32 tex;
			u8* memptr;  // GPU memory used for comparison
			int ref;
		};

		inline CMemoryTarget() : ptex(NULL), starty(0), height(0), realy(0), realheight(0), usedstamp(0), psm(0), cpsm(0), channels(0), clearminy(0), clearmaxy(0), validatecount(0), clut(NULL), clutsize(0) {}

		inline CMemoryTarget(const CMemoryTarget& r)
		{
			ptex = r.ptex;

			if (ptex != NULL) ptex->ref++;

			starty = r.starty;
			height = r.height;
			realy = r.realy;
			realheight = r.realheight;
			usedstamp = r.usedstamp;
			psm = r.psm;
			cpsm = r.cpsm;
			clut = r.clut;
			clearminy = r.clearminy;
			clearmaxy = r.clearmaxy;
			widthmult = r.widthmult;
			texH = r.texH;
			texW = r.texW;
			channels = r.channels;
			validatecount = r.validatecount;
			fmt = r.fmt;
		}

		~CMemoryTarget() { Destroy(); }

		inline void Destroy()
		{
			if (ptex != NULL && ptex->ref > 0)
			{
				if (--ptex->ref <= 0) delete ptex;
			}

			ptex = NULL;

            _aligned_free(clut);
            clut = NULL;
            clutsize = 0;
		}

		// returns true if clut data is synced
		bool ValidateClut(const tex0Info& tex0);
		// returns true if tex data is synced
		bool ValidateTex(const tex0Info& tex0, int starttex, int endtex, bool bDeleteBadTex);

		// realy is offset in pixels from start of valid region
		// so texture in memory is [realy,starty+height]
		// valid texture is [starty,starty+height]
		// offset in mem [starty-realy, height]
		TEXTURE* ptex; // can be 16bit

		int starty, height; // assert(starty >= realy)
		int realy, realheight; // this is never touched once allocated
		// realy is start pointer of data in 4M data block (start) and size (end-start).
		
		u32 usedstamp;
		u8 psm, cpsm; // texture and clut format. For psm, only 16bit/32bit differentiation matters

		u32 fmt;

		int widthmult;	// Either 1 or 2.
		int channels;	// The number of pixels per PSM format word. channels == PIXELS_PER_WORD(psm)
						// This is the real drawing size in pixels of the texture in renderbuffer.
		int texW;		// (realheight + widthmult - 1)/widthmult == realheight or [(realheight+1)/2]
		int texH;		//  GPU_TEXWIDTH *widthmult * channels;			

		int clearminy, clearmaxy;	// when maxy > 0, need to check for clearing

		int validatecount; // count how many times has been validated, if too many, destroy

		u8* clut;        // Clut texture data. Null otherwise
        int clutsize;    // size of the clut array. 0 otherwise 
};

inline u32 GetFrameKey(int fbp, int fbw);

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
		
		bool isFound(const frameInfo& frame, MAPTARGETS::iterator& it, u32 opts, u32 key, int maxposheight);
		
		CRenderTarget* GetTarg(const frameInfo& frame, u32 Options, int maxposheight);
		inline CRenderTarget* GetTarg(int fbp, int fbw)
		{
			MAPTARGETS::iterator it = mapTargets.find(GetFrameKey(fbp, fbw));

			/*			if (fbp == 0x3600 && fbw == 0x100 && it == mapTargets.end())
						{
							ZZLog::Debug_Log("%x", GetFrameKey(fbp, fbw)) ;
							ZZLog::Debug_Log("%x %x", fbp, fbw);
							for(MAPTARGETS::iterator it1 = mapTargets.begin(); it1 != mapTargets.end(); ++it1)
								ZZLog::Debug_Log("\t %x %x %x %x", it1->second->fbw, it1->second->fbh, it1->second->psm, it1->second->fbp);
						}*/
			return it != mapTargets.end() ? it->second : NULL;
		}

		// gets all targets with a range
		void GetTargs(int start, int end, list<CRenderTarget*>& listTargets) const;

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

		void DestroyTarg(CRenderTarget* ptarg);
		void PrintTargets();
		MAPTARGETS mapTargets, mapDummyTargs;
	private:
		
		void DestroyAllTargetsHelper(void* ptr);
};

class CMemoryTargetMngr
{
	public:
		CMemoryTargetMngr() : curstamp(0) {}

		CMemoryTarget* GetMemoryTarget(const tex0Info& tex0, int forcevalidate); // pcbp is pointer to start of clut
		CMemoryTarget* SearchExistTarget(int start, int end, int clutsize, const tex0Info& tex0, int forcevalidate);
		CMemoryTarget* ClearedTargetsSearch(int fmt, int widthmult, int channels, int height);
		int CompareTarget(list<CMemoryTarget>::iterator& it, const tex0Info& tex0, int clutsize);

		void Destroy(); // destroy all targs

		void ClearRange(int starty, int endy); // set all targets to cleared
		void DestroyCleared(); // flush all cleared targes
		void DestroyOldest();

		list<CMemoryTarget> listTargets, listClearedTargets;
		u32 curstamp;

	private:
		list<CMemoryTarget>::iterator DestroyTargetIter(list<CMemoryTarget>::iterator& it);
		void GetClutVariables(int& clutsize, const tex0Info& tex0);
		void GetMemAddress(int& start, int& end,  const tex0Info& tex0);
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
extern CRangeManager s_RangeMngr; // manages overwritten memory

//extern u8 s_AAx, s_AAy;
extern Point AA;

// Real rendered width, depends on AA.
inline int RW(int tbw)
{
    return (tbw << AA.x);
}

// Real rendered height, depends on AA.
inline int RH(int tbh)
{
    return (tbh << AA.y);
}

/*	inline void CreateTargetsList(int start, int end, list<CRenderTarget*>& listTargs) {
		s_DepthRTs.GetTargs(start, end, listTargs);
		s_RTs.GetTargs(start, end, listTargs);
	}*/

// This pattern of functions is called 3 times, so I add creating Targets list into one.
inline list<CRenderTarget*> CreateTargetsList(int start, int end)
{
	list<CRenderTarget*> listTargs;
	s_DepthRTs.GetTargs(start, end, listTargs);
	s_RTs.GetTargs(start, end, listTargs);
	return listTargs;
}

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

inline u32 GetFrameKey(int fbp, int fbw)
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

//------------------------ Inlines -------------------------

// Calculate maximum height for target
inline int get_maxheight(int fbp, int fbw, int psm)
{
	int ret;

	if (fbw == 0) return 0;

	ret = (((0x00100000 - 64 * fbp) / fbw) & ~0x1f);
	if (PSMT_ISHALF(psm)) ret *= 2;

	return ret;
}

// memory size for one row of texture. It depends on width of texture and number of bytes
// per pixel
inline u32 Pitch(int fbw) { return (RW(fbw) * 4) ; }

// memory size of whole texture. It is number of rows multiplied by memory size of row
inline u32 Tex_Memory_Size(int fbw, int fbh) { return (RH(fbh) * Pitch(fbw)); }


#endif
