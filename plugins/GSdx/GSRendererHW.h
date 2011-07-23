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

#pragma once

#include "GSRenderer.h"
#include "GSTextureCache.h"
#include "GSCrc.h"
#include "GSFunctionMap.h"


template<class Vertex>
class GSRendererHW : public GSRendererT<Vertex>
{
	int m_width;
	int m_height;
	int m_skip;
	bool m_reset;
	bool m_nativeres;
	int m_upscale_multiplier;
	int m_userhacks_skipdraw;

	#pragma region hacks

	typedef bool (GSRendererHW::*OI_Ptr)(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	typedef void (GSRendererHW::*OO_Ptr)();
	typedef bool (GSRendererHW::*CU_Ptr)();

	bool OI_FFXII(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		static uint32* video = NULL;
		static int lines = 0;

		if(lines == 0)
		{
			if(m_vt.m_primclass == GS_LINE_CLASS && (m_count == 448 * 2 || m_count == 512 * 2))
			{
				lines = m_count / 2;
			}
		}
		else
		{
			if(m_vt.m_primclass == GS_POINT_CLASS)
			{
				if(m_count >= 16 * 512)
				{
					// incoming pixels are stored in columns, one column is 16x512, total res 448x512 or 448x454

					if(!video) video = new uint32[512 * 512];

					int ox = m_context->XYOFFSET.OFX;
					int oy = m_context->XYOFFSET.OFY;

					for(int i = 0; i < m_count; i++)
					{
						int x = ((int)m_vertices[i].p.x - ox) >> 4;
						int y = ((int)m_vertices[i].p.y - oy) >> 4;

						// video[y * 448 + x] = m_vertices[i].c0;
						video[(y << 8) + (y << 7) + (y << 6) + x] = m_vertices[i]._c0();
					}

					return false;
				}
				else
				{
					lines = 0;
				}
			}
			else if(m_vt.m_primclass == GS_LINE_CLASS)
			{
				if(m_count == lines * 2)
				{
					// normally, this step would copy the video onto screen with 512 texture mapped horizontal lines,
					// but we use the stored video data to create a new texture, and replace the lines with two triangles

					m_dev->Recycle(t->m_texture);

					t->m_texture = m_dev->CreateTexture(512, 512);

					t->m_texture->Update(GSVector4i(0, 0, 448, lines), video, 448 * 4);

					m_vertices[0] = m_vertices[0];
					m_vertices[1] = m_vertices[1];
					m_vertices[2] = m_vertices[m_count - 2];
					m_vertices[3] = m_vertices[1];
					m_vertices[4] = m_vertices[2];
					m_vertices[5] = m_vertices[m_count - 1];

					m_count = 6;

					m_vt.Update(m_vertices, m_count, GS_TRIANGLE_CLASS);
				}
				else
				{
					lines = 0;
				}
			}
		}

		return true;
	}

	bool OI_FFX(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 ZBP = m_context->ZBUF.Block();
		uint32 TBP = m_context->TEX0.TBP0;

		if((FBP == 0x00d00 || FBP == 0x00000) && ZBP == 0x02100 && PRIM->TME && TBP == 0x01a00 && m_context->TEX0.PSM == PSM_PSMCT16S)
		{
			// random battle transition (z buffer written directly, clear it now)

			m_dev->ClearDepth(ds, 0);
		}

		return true;
	}

	bool OI_MetalSlug6(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		// missing red channel fix

		for(int i = 0, j = m_count; i < j; i++)
		{
			if(m_vertices[i]._r() == 0 && m_vertices[i]._g() != 0 && m_vertices[i]._b() != 0)
			{
				m_vertices[i]._r() = (m_vertices[i]._g() + m_vertices[i]._b()) / 2;
			}
		}

		m_vt.Update(m_vertices, m_count, m_vt.m_primclass);

		return true;
	}

	bool OI_GodOfWar2(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FBW = m_context->FRAME.FBW;
		uint32 FPSM = m_context->FRAME.PSM;

		if((FBP == 0x00f00 || FBP == 0x00100 || FBP == 0x01280) && FPSM == PSM_PSMZ24) // ntsc 0xf00, pal 0x100, ntsc "HD" 0x1280
		{
			// z buffer clear

			GIFRegTEX0 TEX0;

			TEX0.TBP0 = FBP;
			TEX0.TBW = FBW;
			TEX0.PSM = FPSM;

			if(GSTextureCache::Target* ds = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::DepthStencil, true))
			{
				m_dev->ClearDepth(ds->m_texture, 0);
			}

			return false;
		}

		return true;
	}

	bool OI_SimpsonsGame(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FBW = m_context->FRAME.FBW;
		uint32 FPSM = m_context->FRAME.PSM;

		if((FBP == 0x01500 || FBP == 0x01800) && FPSM == PSM_PSMZ24)	//0x1800 pal, 0x1500 ntsc
		{
			// instead of just simply drawing a full height 512x512 sprite to clear the z buffer,
			// it uses a 512x256 sprite only, yet it is still able to fill the whole surface with zeros,
			// how? by using a render target that overlaps with the lower half of the z buffer...

			m_dev->ClearDepth(ds, 0);

			return false;
		}

		return true;
	}

	bool OI_RozenMaidenGebetGarden(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		if(!PRIM->TME)
		{
			uint32 FBP = m_context->FRAME.Block();
			uint32 ZBP = m_context->ZBUF.Block();

			if(FBP == 0x008c0 && ZBP == 0x01a40)
			{
				//  frame buffer clear, atst = fail, afail = write z only, z buffer points to frame buffer

				GIFRegTEX0 TEX0;

				TEX0.TBP0 = ZBP;
				TEX0.TBW = m_context->FRAME.FBW;
				TEX0.PSM = m_context->FRAME.PSM;

				if(GSTextureCache::Target* rt = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::RenderTarget, true))
				{
					m_dev->ClearRenderTarget(rt->m_texture, 0);
				}

				return false;
			}
			else if(FBP == 0x00000 && m_context->ZBUF.Block() == 0x01180)
			{
				// z buffer clear, frame buffer now points to the z buffer (how can they be so clever?)

				GIFRegTEX0 TEX0;

				TEX0.TBP0 = FBP;
				TEX0.TBW = m_context->FRAME.FBW;
				TEX0.PSM = m_context->ZBUF.PSM;

				if(GSTextureCache::Target* ds = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::DepthStencil, true))
				{
					m_dev->ClearDepth(ds->m_texture, 0);
				}

				return false;
			}
		}

		return true;
	}

	bool OI_SpidermanWoS(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FPSM = m_context->FRAME.PSM;

		if((FBP == 0x025a0 || FBP == 0x02800) && FPSM == PSM_PSMCT32)	//0x2800 pal, 0x25a0 ntsc
		{
			//only top half of the screen clears
			m_dev->ClearDepth(ds, 0);
		}

		return true;
	}

	bool OI_TyTasmanianTiger(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FBW = m_context->FRAME.FBW;
		uint32 FPSM = m_context->FRAME.PSM;

		if((FBP == 0x02800 || FBP == 0x02BC0) && FPSM == PSM_PSMCT24)	//0x2800 pal, 0x2bc0 ntsc
		{
			//half height buffer clear
			m_dev->ClearDepth(ds, 0);

			return false;
		}

		return true;
	}

	bool OI_DigimonRumbleArena2(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)	
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FPSM = m_context->FRAME.PSM;

		if(!PRIM->TME)
		{
			if((FBP == 0x02300 || FBP == 0x03fc0) && FPSM == PSM_PSMCT32)
			{
				//half height buffer clear
				m_dev->ClearDepth(ds, 0);
			}
		}

		return true;
	}

	bool OI_BlackHawkDown(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)	
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FPSM = m_context->FRAME.PSM;

		if(FBP == 0x02000 && FPSM == PSM_PSMZ24)
		{
			//half height buffer clear
			m_dev->ClearDepth(ds, 0);

			return false;
		}

		return true;
	}

	bool OI_StarWarsForceUnleashed(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)	
	{
		uint32 FBP = m_context->FRAME.Block();
		uint32 FPSM = m_context->FRAME.PSM;
		
		if(FPSM == PSM_PSMCT24 && FBP == 0x2bc0)
		{
			m_dev->ClearDepth(ds, 0);

			return false;
		}
		else if((FBP == 0x36e0 || FBP == 0x34a0) && FPSM == PSM_PSMCT24)
		{
			m_dev->ClearDepth(ds, 0);
		}
		
		return true;
	}


	bool OI_PointListPalette(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		if(m_vt.m_primclass == GS_POINT_CLASS && !PRIM->TME)
		{
			uint32 FBP = m_context->FRAME.Block();
			uint32 FBW = m_context->FRAME.FBW;

			if(FBP >= 0x03f40 && (FBP & 0x1f) == 0)
			{
				if(m_count == 16)
				{
					for(int i = 0; i < 16; i++)
					{
						uint8 a = m_vertices[i]._a();

						m_vertices[i]._a() = a >= 0x80 ? 0xff : a * 2;

						m_mem.WritePixel32(i & 7, i >> 3, m_vertices[i]._c0(), FBP, FBW);
					}

					m_mem.m_clut.Invalidate();

					return false;
				}
				else if(m_count == 256)
				{
					for(int i = 0; i < 256; i++)
					{
						uint8 a = m_vertices[i]._a();
						
						m_vertices[i]._a() = a >= 0x80 ? 0xff : a * 2;

						m_mem.WritePixel32(i & 15, i >> 4, m_vertices[i]._c0(), FBP, FBW);
					}

					m_mem.m_clut.Invalidate();

					return false;
				}
				else
				{
					ASSERT(0);
				}
			}
		}

		return true;
	}

	void OO_DBZBT2()
	{
		// palette readback (cannot detect yet, when fetching the texture later)

		uint32 FBP = m_context->FRAME.Block();
		uint32 TBP0 = m_context->TEX0.TBP0;

		if(PRIM->TME && (FBP == 0x03c00 && TBP0 == 0x03c80 || FBP == 0x03ac0 && TBP0 == 0x03b40))
		{
			GIFRegBITBLTBUF BITBLTBUF;

			BITBLTBUF.SBP = FBP;
			BITBLTBUF.SBW = 1;
			BITBLTBUF.SPSM = PSM_PSMCT32;

			InvalidateLocalMem(BITBLTBUF, GSVector4i(0, 0, 64, 64));
		}
	}

	void OO_MajokkoALaMode2()
	{
		// palette readback

		uint32 FBP = m_context->FRAME.Block();

		if(!PRIM->TME && FBP == 0x03f40)
		{
			GIFRegBITBLTBUF BITBLTBUF;

			BITBLTBUF.SBP = FBP;
			BITBLTBUF.SBW = 1;
			BITBLTBUF.SPSM = PSM_PSMCT32;

			InvalidateLocalMem(BITBLTBUF, GSVector4i(0, 0, 16, 16));
		}
	}

	bool CU_DBZBT2()
	{
		// palette should stay 64 x 64

		uint32 FBP = m_context->FRAME.Block();

		return FBP != 0x03c00 && FBP != 0x03ac0;
	}

	bool CU_MajokkoALaMode2()
	{
		// palette should stay 16 x 16

		uint32 FBP = m_context->FRAME.Block();

		return FBP != 0x03f40;
	}

	bool CU_TalesOfAbyss()
	{
		// full image blur and brightening

		uint32 FBP = m_context->FRAME.Block();

		return FBP != 0x036e0 && FBP != 0x03560 && FBP != 0x038e0;
	}

	class Hacks
	{
		template<class T> struct HackEntry
		{
			CRC::Title title;
			CRC::Region region;
			T func;

			struct HackEntry(CRC::Title t, CRC::Region r, T f)
			{
				title = t;
				region = r;
				func = f;
			}
		};

		template<class T> class FunctionMap : public GSFunctionMap<uint32, T>
		{
			list<HackEntry<T> >& m_tbl;

			T GetDefaultFunction(uint32 key)
			{
				CRC::Title title = (CRC::Title)(key & 0xffffff);
				CRC::Region region = (CRC::Region)(key >> 24);

				for(list<HackEntry<T> >::iterator i = m_tbl.begin(); i != m_tbl.end(); i++)
				{
					if(i->title == title && (i->region == CRC::RegionCount || i->region == region))
					{
						return i->func;
					}
				}

				return NULL;
			}

		public:
			FunctionMap(list<HackEntry<T> >& tbl) : m_tbl(tbl) {}
		};

		list<HackEntry<OI_Ptr> > m_oi_list;
		list<HackEntry<OO_Ptr> > m_oo_list;
		list<HackEntry<CU_Ptr> > m_cu_list;

		FunctionMap<OI_Ptr> m_oi_map;
		FunctionMap<OO_Ptr> m_oo_map;
		FunctionMap<CU_Ptr> m_cu_map;

	public:
		OI_Ptr m_oi;
		OO_Ptr m_oo;
		CU_Ptr m_cu;

		Hacks()
			: m_oi_map(m_oi_list)
			, m_oo_map(m_oo_list)
			, m_cu_map(m_cu_list)
			, m_oi(NULL)
			, m_oo(NULL)
			, m_cu(NULL)
		{
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::FFXII, CRC::EU, &GSRendererHW::OI_FFXII));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::FFX, CRC::RegionCount, &GSRendererHW::OI_FFX));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::MetalSlug6, CRC::RegionCount, &GSRendererHW::OI_MetalSlug6));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::GodOfWar2, CRC::RegionCount, &GSRendererHW::OI_GodOfWar2));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::SimpsonsGame, CRC::RegionCount, &GSRendererHW::OI_SimpsonsGame));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::RozenMaidenGebetGarden, CRC::RegionCount, &GSRendererHW::OI_RozenMaidenGebetGarden));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::SpidermanWoS, CRC::RegionCount, &GSRendererHW::OI_SpidermanWoS));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::TyTasmanianTiger, CRC::RegionCount, &GSRendererHW::OI_TyTasmanianTiger));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::DigimonRumbleArena2, CRC::RegionCount, &GSRendererHW::OI_DigimonRumbleArena2));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::StarWarsForceUnleashed, CRC::RegionCount, &GSRendererHW::OI_StarWarsForceUnleashed));
			m_oi_list.push_back(HackEntry<OI_Ptr>(CRC::BlackHawkDown, CRC::RegionCount, &GSRendererHW::OI_BlackHawkDown));

			m_oo_list.push_back(HackEntry<OO_Ptr>(CRC::DBZBT2, CRC::RegionCount, &GSRendererHW::OO_DBZBT2));
			m_oo_list.push_back(HackEntry<OO_Ptr>(CRC::MajokkoALaMode2, CRC::RegionCount, &GSRendererHW::OO_MajokkoALaMode2));

			m_cu_list.push_back(HackEntry<CU_Ptr>(CRC::DBZBT2, CRC::RegionCount, &GSRendererHW::CU_DBZBT2));
			m_cu_list.push_back(HackEntry<CU_Ptr>(CRC::MajokkoALaMode2, CRC::RegionCount, &GSRendererHW::CU_MajokkoALaMode2));
			m_cu_list.push_back(HackEntry<CU_Ptr>(CRC::TalesOfAbyss, CRC::RegionCount, &GSRendererHW::CU_TalesOfAbyss));
		}

		void SetGame(const CRC::Game& game)
		{
			uint32 hash = (uint32)((game.region << 24) | game.title);

			m_oi = m_oi_map[hash];
			m_oo = m_oo_map[hash];
			m_cu = m_cu_map[hash];

			if(game.flags & CRC::PointListPalette)
			{
				ASSERT(m_oi == NULL);

				m_oi = &GSRendererHW::OI_PointListPalette;
			}
		}

	} m_hacks;

	#pragma endregion

protected:
	GSTextureCache* m_tc;

	void Reset()
	{
		// TODO: GSreset can come from the main thread too => crash
		// m_tc->RemoveAll();

		m_reset = true;

		__super::Reset();
	}

	void VSync(int field)
	{
		__super::VSync(field);

		m_tc->IncAge();
		m_dev->AgePool();

		m_skip = 0;

		if(m_reset)
		{
			m_tc->RemoveAll();

			m_reset = false;
		}
	}

	void ResetDevice()
	{
		m_tc->RemoveAll();

		__super::ResetDevice();
	}

	GSTexture* GetOutput(int i)
	{
		const GSRegDISPFB& DISPFB = m_regs->DISP[i].DISPFB;

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = DISPFB.Block();
		TEX0.TBW = DISPFB.FBW;
		TEX0.PSM = DISPFB.PSM;

		// TRACE(_T("[%d] GetOutput %d %05x (%d)\n"), (int)m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM);

		GSTexture* t = NULL;

		if(GSTextureCache::Target* rt = m_tc->LookupTarget(TEX0, m_width, m_height))
		{
			t = rt->m_texture;

			if(s_dump)
			{
				if(s_save && s_n >= s_saven)
				{
					t->Save(format("c:\\temp2\\_%05d_f%lld_fr%d_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM));
				}

				s_n++;
			}
		}

		return t;
	}

	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
	{
		// printf("[%d] InvalidateVideoMem %d,%d - %d,%d %05x (%d)\n", (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.DBP, (int)BITBLTBUF.DPSM);

		m_tc->InvalidateVideoMem(m_mem.GetOffset(BITBLTBUF.DBP, BITBLTBUF.DBW, BITBLTBUF.DPSM), r);
	}

	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
	{
		// printf("[%d] InvalidateLocalMem %d,%d - %d,%d %05x (%d)\n", (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.SBP, (int)BITBLTBUF.SPSM);

		m_tc->InvalidateLocalMem(m_mem.GetOffset(BITBLTBUF.SBP, BITBLTBUF.SBW, BITBLTBUF.SPSM), r);
	}

	void Draw()
	{
#ifndef NO_CRC_HACKS
		if(IsBadFrame(m_skip, m_userhacks_skipdraw)) return;
#endif

		GSDrawingEnvironment& env = m_env;
		GSDrawingContext* context = m_context;

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = context->FRAME.Block();
		TEX0.TBW = context->FRAME.FBW;
		TEX0.PSM = context->FRAME.PSM;

		GSTextureCache::Target* rt = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::RenderTarget, true);

		TEX0.TBP0 = context->ZBUF.Block();
		TEX0.TBW = context->FRAME.FBW;
		TEX0.PSM = context->ZBUF.PSM;

		GSTextureCache::Target* ds = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::DepthStencil, m_context->DepthWrite());

		GSTextureCache::Source* tex = NULL;

		if(PRIM->TME)
		{
			m_mem.m_clut.Read32(context->TEX0, env.TEXA);

			GSVector4i r;

			GetTextureMinMax(r, context->TEX0, context->CLAMP, m_vt.IsLinear());

			tex = m_tc->LookupSource(context->TEX0, env.TEXA, r);

			if(!tex) return;
		}

		if(s_dump)
		{
			uint64 frame = m_perfmon.GetFrame();

			string s;

			if(s_save && s_n >= s_saven && tex)
			{
				s = format("c:\\temp2\\_%05d_f%lld_tex_%05x_%d_%d%d_%02x_%02x_%02x_%02x.dds",
					s_n, frame, (int)context->TEX0.TBP0, (int)context->TEX0.PSM,
					(int)context->CLAMP.WMS, (int)context->CLAMP.WMT,
					(int)context->CLAMP.MINU, (int)context->CLAMP.MAXU,
					(int)context->CLAMP.MINV, (int)context->CLAMP.MAXV);

				tex->m_texture->Save(s, true);

				if(tex->m_palette)
				{
					s = format("c:\\temp2\\_%05d_f%lld_tpx_%05x_%d.dds", s_n, frame, context->TEX0.CBP, context->TEX0.CPSM);

					tex->m_palette->Save(s, true);
				}
			}

			s_n++;

			if(s_save && s_n >= s_saven)
			{
				s = format("c:\\temp2\\_%05d_f%lld_rt0_%05x_%d.bmp", s_n, frame, context->FRAME.Block(), context->FRAME.PSM);

				rt->m_texture->Save(s);
			}

			if(s_savez && s_n >= s_saven)
			{
				s = format("c:\\temp2\\_%05d_f%lld_rz0_%05x_%d.bmp", s_n, frame, context->ZBUF.Block(), context->ZBUF.PSM);

				ds->m_texture->Save(s);
			}

			s_n++;
		}

		if(m_hacks.m_oi && !(this->*m_hacks.m_oi)(rt->m_texture, ds->m_texture, tex))
		{
			return;
		}

		// skip alpha test if possible

		GIFRegTEST TEST = context->TEST;
		GIFRegFRAME FRAME = context->FRAME;
		GIFRegZBUF ZBUF = context->ZBUF;

		uint32 fm = context->FRAME.FBMSK;
		uint32 zm = context->ZBUF.ZMSK || context->TEST.ZTE == 0 ? 0xffffffff : 0;

		if(context->TEST.ATE && context->TEST.ATST != ATST_ALWAYS)
		{
			if(TryAlphaTest(fm, zm))
			{
				context->TEST.ATST = ATST_ALWAYS;
			}
		}

		context->FRAME.FBMSK = fm;
		context->ZBUF.ZMSK = zm != 0;

		//

		Draw(rt->m_texture, ds->m_texture, tex);

		//

		context->TEST = TEST;
		context->FRAME = FRAME;
		context->ZBUF = ZBUF;

		//

		GSVector4i r = GSVector4i(m_vt.m_min.p.xyxy(m_vt.m_max.p)).rintersect(GSVector4i(m_context->scissor.in));

		if(fm != 0xffffffff)
		{
			rt->m_valid = rt->m_valid.runion(r);

			m_tc->InvalidateVideoMem(m_context->offset.fb, r, false);
		}

		if(zm != 0xffffffff)
		{
			ds->m_valid = ds->m_valid.runion(r);

			m_tc->InvalidateVideoMem(m_context->offset.zb, r, false);
		}

		//

		if(m_hacks.m_oo)
		{
			(this->*m_hacks.m_oo)();
		}

		if(s_dump)
		{
			uint64 frame = m_perfmon.GetFrame();

			string s;

			if(s_save && s_n >= s_saven)
			{
				s = format("c:\\temp2\\_%05d_f%lld_rt1_%05x_%d.bmp", s_n, frame, context->FRAME.Block(), context->FRAME.PSM);

				rt->m_texture->Save(s);
			}

			if(s_savez && s_n >= s_saven)
			{
				s = format("c:\\temp2\\_%05d_f%lld_rz1_%05x_%d.bmp", s_n, frame, context->ZBUF.Block(), context->ZBUF.PSM);

				ds->m_texture->Save(s);
			}

			s_n++;
		}
#ifdef HW_NO_TEXTURE_CACHE
		m_tc->Read(rt, r);
#endif
	}

	virtual void Draw(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) = 0;

	bool CanUpscale()
	{
		if(m_hacks.m_cu && !(this->*m_hacks.m_cu)())
		{
			return false;
		}

		return !m_nativeres && m_regs->PMODE.EN != 0; // upscale ratio depends on the display size, with no output it may not be set correctly (ps2 logo to game transition)
	}

	int GetUpscaleMultiplier()
	{
		return m_upscale_multiplier;
	}

public:
	GSRendererHW(GSTextureCache* tc)
		: GSRendererT<Vertex>()
		, m_tc(tc)
		, m_width(1024)
		, m_height(1024)
		, m_skip(0)
		, m_reset(false)
		, m_upscale_multiplier(1)
	{
		m_nativeres = !!theApp.GetConfig("nativeres", 0);
		m_upscale_multiplier = theApp.GetConfig("upscale_multiplier", 1);
		m_userhacks_skipdraw = theApp.GetConfig("UserHacks_SkipDraw", 0);

		if(!m_nativeres)
		{
			m_width = theApp.GetConfig("resx", m_width);
			m_height = theApp.GetConfig("resy", m_height);

			m_upscale_multiplier = theApp.GetConfig("upscale_multiplier", m_upscale_multiplier);

			if(m_upscale_multiplier > 6)
			{
				m_upscale_multiplier = 1; // use the normal upscale math
			}
			else if(m_upscale_multiplier > 1)
			{
				m_width = 640 * m_upscale_multiplier; // 512 is also common, but this is not always detected right.
				m_height = 512 * m_upscale_multiplier; // 448 is also common, but this is not always detected right.
			}
		}
		else m_upscale_multiplier = 1;
	}

	virtual ~GSRendererHW()
	{
		delete m_tc;
	}

	void SetGameCRC(uint32 crc, int options)
	{
		__super::SetGameCRC(crc, options);

		m_hacks.SetGame(m_game);

		if(m_game.title == CRC::JackieChanAdv)
		{
			m_width = 1280; // TODO: uses a 1280px wide 16 bit render target, but this only fixes half of the problem
		}
	}
};
