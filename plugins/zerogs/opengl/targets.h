/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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

#define TARGET_VIRTUAL_KEY 0x80000000

namespace ZeroGS {

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
		inline CRenderTarget* GetTarg(int fbp, int fbw) {
			MAPTARGETS::iterator it = mapTargets.find(fbp|(fbw<<16));
			return it != mapTargets.end() ? it->second : NULL;
		}

		// gets all targets with a range
		void GetTargs(int start, int end, list<ZeroGS::CRenderTarget*>& listTargets) const;

		// resolves all targets within a range
		__forceinline void Resolve(int start, int end);
		__forceinline void ResolveAll() {
			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it )
				it->second->Resolve();
		}

		void DestroyAllTargs(int start, int end, int fbw);
		void DestroyIntersecting(CRenderTarget* prndr);

		// promotes a target from virtual to real
		inline CRenderTarget* Promote(u32 key) {
			assert( !(key & TARGET_VIRTUAL_KEY) );

			// promote to regular targ
			CRenderTargetMngr::MAPTARGETS::iterator it = mapTargets.find(key|TARGET_VIRTUAL_KEY);
			assert( it != mapTargets.end() );

			CRenderTarget* ptarg = it->second;
			mapTargets.erase(it);

			DestroyIntersecting(ptarg);

			it = mapTargets.find(key);
			if( it != mapTargets.end() ) {
				DestroyTarg(it->second);
				it->second = ptarg;
			}
			else
				mapTargets[key] = ptarg;

            if( g_GameSettings & GAME_RESOLVEPROMOTED )
                ptarg->status = CRenderTarget::TS_Resolved;
            else
                ptarg->status = CRenderTarget::TS_NeedUpdate;
			return ptarg;
		}

		static void DestroyTarg(CRenderTarget* ptarg);

		MAPTARGETS mapTargets, mapDummyTargs;
	};

	class CMemoryTargetMngr
	{
	public:
		CMemoryTargetMngr() : curstamp(0) {}
		CMemoryTarget* GetMemoryTarget(const tex0Info& tex0, int forcevalidate); // pcbp is pointer to start of clut

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
		__forceinline u32 GetTex(u32 bitvalue, u32 ptexDoNotDelete) {
			map<u32, u32>::iterator it = mapTextures.find(bitvalue);
			if( it != mapTextures.end() )
				return it->second;
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
		inline void Clear() { ranges.resize(0); }

		vector<RANGE> ranges; // organized in ascending order, non-intersecting
	};

	extern CRenderTargetMngr s_RTs, s_DepthRTs;
	extern CBitwiseTextureMngr s_BitwiseTextures;
	extern CMemoryTargetMngr g_MemTargs;
}
