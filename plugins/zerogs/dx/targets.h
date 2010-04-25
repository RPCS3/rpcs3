#define TARGET_VIRTUAL_KEY 0x80000000

namespace ZeroGS
{
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

		CRenderTarget* GetTarg(const frameInfo& frame, DWORD Options, int maxposheight);
		inline CRenderTarget* GetTarg(int fbp, int fbw) {
			MAPTARGETS::iterator it = mapTargets.find(fbp|(fbw<<16));
			return it != mapTargets.end() ? it->second : NULL;
		}

		// gets all targets with a range
		void GetTargs(int start, int end, list<ZeroGS::CRenderTarget*>& listTargets) const;

		virtual void DestroyChildren(CRenderTarget* ptarg);

		// resolves all targets within a range
		__forceinline void Resolve(int start, int end);
		__forceinline void ResolveAll() {
			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end(); ++it )
				it->second->Resolve();
		}

		void DestroyTarg(CRenderTarget* ptarg) {
			for(int i = 0; i < 2; ++i) {
				if( ptarg == vb[i].prndr ) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
				if( ptarg == vb[i].pdepth ) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
			}

			DestroyChildren(ptarg);
			delete ptarg;
		}

		inline void DestroyAll(int start, int end, int fbw) {
			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();) {
				if( it->second->start < end && start < it->second->end ) {
					// if is depth, only resolve if fbw is the same
					if( !it->second->IsDepth() ) {
						// only resolve if the widths are the same or it->second has bit outside the range
						// shadow of colossus swaps between fbw=256,fbh=256 and fbw=512,fbh=448. This kills the game if doing || it->second->end > end

						// kh hack, sometimes kh movies do this to clear the target, so have a static count that periodically checks end
						static int count = 0;

						if( it->second->fbw == fbw || (it->second->fbw != fbw && (it->second->start < start || ((count++&0xf)?0:it->second->end > end) )) )
							it->second->Resolve();
						else {
							if( vb[0].prndr == it->second || vb[0].pdepth == it->second ) Flush(0);
							if( vb[1].prndr == it->second || vb[1].pdepth == it->second ) Flush(1);

							it->second->status |= CRenderTarget::TS_Resolved;
						}
					}
					else {
						if( it->second->fbw == fbw )
							it->second->Resolve();
						else {
							if( vb[0].prndr == it->second || vb[0].pdepth == it->second ) Flush(0);
							if( vb[1].prndr == it->second || vb[1].pdepth == it->second ) Flush(1);

							it->second->status |= CRenderTarget::TS_Resolved;
						}
					}

					for(int i = 0; i < 2; ++i) {
						if( it->second == vb[i].prndr ) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
						if( it->second == vb[i].pdepth ) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
					}

					u32 dummykey = (it->second->fbw<<16)|it->second->fbh;
					if( it->second->pmimicparent != NULL && mapDummyTargs.find(dummykey) == mapDummyTargs.end() ) {
						DestroyChildren(it->second);
						mapDummyTargs[dummykey] = it->second;
					}
					else {
						DestroyChildren(it->second);
						delete it->second;
					}
					it = mapTargets.erase(it);
				}
				else ++it;
			}
		}

		inline void DestroyIntersecting(CRenderTarget* prndr)
		{
			assert( prndr != NULL );

			int start, end;
			GetRectMemAddress(start, end, prndr->psm, 0, 0, prndr->fbw, prndr->fbh, prndr->fbp, prndr->fbw);

			for(MAPTARGETS::iterator it = mapTargets.begin(); it != mapTargets.end();) {
				if( it->second != prndr && it->second->start < end && start < it->second->end ) {
					it->second->Resolve();

					for(int i = 0; i < 2; ++i) {
						if( it->second == vb[i].prndr ) { vb[i].prndr = NULL; vb[i].bNeedFrameCheck = 1; }
						if( it->second == vb[i].pdepth ) { vb[i].pdepth = NULL; vb[i].bNeedZCheck = 1; }
					}

					u32 dummykey = (it->second->fbw<<16)|it->second->fbh;
					if( it->second->pmimicparent != NULL && mapDummyTargs.find(dummykey) == mapDummyTargs.end() ) {
						DestroyChildren(it->second);
						mapDummyTargs[dummykey] = it->second;
					}
					else {
						DestroyChildren(it->second);
						delete it->second;
					}

					it = mapTargets.erase(it);
				}
				else ++it;
			}
		}

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
		__forceinline LPD3DTEX GetTex(u32 bitvalue, LPD3DTEX ptexDoNotDelete) {
			map<u32, LPD3DTEX>::iterator it = mapTextures.find(bitvalue);
			if( it != mapTextures.end() )
				return it->second;
			return GetTexInt(bitvalue, ptexDoNotDelete);
		}

	private:
		LPD3DTEX GetTexInt(u32 bitvalue, LPD3DTEX ptexDoNotDelete);

		map<u32, LPD3DTEX> mapTextures;
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
