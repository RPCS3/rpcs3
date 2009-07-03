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

template<class Vertex> 
class GSRendererHW : public GSRendererT<Vertex>
{
	int m_width;
	int m_height;
	int m_skip;
	bool m_reset;

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

		if(GSTextureCache::Target* rt = m_tc->LookupTarget(TEX0, m_width, m_height, GSTextureCache::RenderTarget, true, true))
		{
			t = rt->m_texture;

			if(s_dump)
			{
				if(s_save) 
				{
					t->Save(format("c:\\temp2\\_%05d_f%I64d_fr%d_%05x_%d.bmp", s_n, m_perfmon.GetFrame(), i, (int)TEX0.TBP0, (int)TEX0.PSM));
				}

				s_n++;
			}
		}

		return t;
	}

	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
	{
		// printf("[%d] InvalidateVideoMem %d,%d - %d,%d %05x (%d)\n", (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.DBP, (int)BITBLTBUF.DPSM);

		m_tc->InvalidateVideoMem(BITBLTBUF, r);
	}

	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r)
	{
		// printf("[%d] InvalidateLocalMem %d,%d - %d,%d %05x (%d)\n", (int)m_perfmon.GetFrame(), r.left, r.top, r.right, r.bottom, (int)BITBLTBUF.SBP, (int)BITBLTBUF.SPSM);

		m_tc->InvalidateLocalMem(BITBLTBUF, r);
	}

	void Draw()
	{
		if(IsBadFrame(m_skip)) return;

		m_vt.Update(m_vertices, m_count, GSUtil::GetPrimClass(PRIM->PRIM), PRIM, m_context);

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

			GetTextureMinMax(r, IsLinear());

			tex = m_tc->LookupSource(context->TEX0, env.TEXA, r);

			if(!tex) return;
		}

		if(s_dump)
		{
			uint64 frame = m_perfmon.GetFrame();

			string s;
			
			if(s_save && tex) 
			{
				s = format("c:\\temp2\\_%05d_f%I64d_tex_%05x_%d_%d%d_%02x_%02x_%02x_%02x.dds", 
					s_n, frame, (int)context->TEX0.TBP0, (int)context->TEX0.PSM,
					(int)context->CLAMP.WMS, (int)context->CLAMP.WMT, 
					(int)context->CLAMP.MINU, (int)context->CLAMP.MAXU, 
					(int)context->CLAMP.MINV, (int)context->CLAMP.MAXV);

				tex->m_texture->Save(s, true);

				if(tex->m_palette)
				{
					s = format("c:\\temp2\\_%05d_f%I64d_tpx_%05x_%d.dds", s_n, frame, context->TEX0.CBP, context->TEX0.CPSM);

					tex->m_palette->Save(s, true);
				}
			}

			s_n++;

			if(s_save)
			{
				s = format("c:\\temp2\\_%05d_f%I64d_rt0_%05x_%d.bmp", s_n, frame, context->FRAME.Block(), context->FRAME.PSM);

				rt->m_texture->Save(s);
			}

			if(s_savez)
			{
				s = format("c:\\temp2\\_%05d_f%I64d_rz0_%05x_%d.bmp", s_n, frame, context->ZBUF.Block(), context->ZBUF.PSM);

				ds->m_texture->Save(s);
			}

			s_n++;
		}

		int prim = PRIM->PRIM;

		if(!OverrideInput(prim, rt->m_texture, ds->m_texture, tex))
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
				context->TEST.ATE = 0;
			}
		}

		context->FRAME.FBMSK = fm;
		context->ZBUF.ZMSK = zm != 0;

		//

		Draw(GSUtil::GetPrimClass(prim), rt->m_texture, ds->m_texture, tex);

		//

		context->TEST = TEST;
		context->FRAME = FRAME;
		context->ZBUF = ZBUF;

		//

		GSVector4i r = GSVector4i(m_vt.m_min.p.xyxy(m_vt.m_max.p)).rintersect(GSVector4i(m_context->scissor.in));

		GIFRegBITBLTBUF BITBLTBUF;

		BITBLTBUF.DBW = context->FRAME.FBW;

		if(fm != 0xffffffff)
		{
			BITBLTBUF.DBP = context->FRAME.Block();
			BITBLTBUF.DPSM = context->FRAME.PSM;

			m_tc->InvalidateVideoMem(BITBLTBUF, r, false);
		}

		if(zm != 0xffffffff)
		{
			BITBLTBUF.DBP = context->ZBUF.Block();
			BITBLTBUF.DPSM = context->ZBUF.PSM;

			m_tc->InvalidateVideoMem(BITBLTBUF, r, false);
		}

		//

		OverrideOutput();

		if(s_dump)
		{
			uint64 frame = m_perfmon.GetFrame();

			string s;

			if(s_save)
			{
				s = format("c:\\temp2\\_%05d_f%I64d_rt1_%05x_%d.bmp", s_n, frame, context->FRAME.Block(), context->FRAME.PSM);

				rt->m_texture->Save(s);
			}

			if(s_savez)
			{
				s = format("c:\\temp2\\_%05d_f%I64d_rz1_%05x_%d.bmp", s_n, frame, context->ZBUF.Block(), context->ZBUF.PSM);

				ds->m_texture->Save(s);
			}

			s_n++;
		}
	}

	virtual void Draw(GS_PRIM_CLASS primclass, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) = 0;

	virtual bool OverrideInput(int& prim, GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t)
	{
		#pragma region ffxii pal video conversion

		if(m_game.title == CRC::FFXII && m_game.region == CRC::EU)
		{
			static uint32* video = NULL;
			static bool ok = false;

			if(prim == GS_POINTLIST && m_count >= 448 * 448 && m_count <= 448 * 512)
			{
				// incoming pixels are stored in columns, one column is 16x512, total res 448x512 or 448x454

				if(!video) video = new uint32[512 * 512];

				for(int x = 0, i = 0, rows = m_count / 448; x < 448; x += 16)
				{
					uint32* dst = &video[x];

					for(int y = 0; y < rows; y++, dst += 512)
					{
						for(int j = 0; j < 16; j++, i++)
						{
							dst[j] = m_vertices[i].c0;
						}
					}
				}

				ok = true;

				return false;
			}
			else if(prim == GS_LINELIST && m_count == 512 * 2 && ok)
			{
				// normally, this step would copy the video onto screen with 512 texture mapped horizontal lines,
				// but we use the stored video data to create a new texture, and replace the lines with two triangles

				ok = false;

				m_dev->Recycle(t->m_texture);

				t->m_texture = m_dev->CreateTexture(512, 512);

				t->m_texture->Update(GSVector4i(0, 0, 448, 512), video, 512 * 4);

				m_vertices[0] = m_vertices[0];
				m_vertices[1] = m_vertices[1];
				m_vertices[2] = m_vertices[m_count - 2];
				m_vertices[3] = m_vertices[1];
				m_vertices[4] = m_vertices[2];
				m_vertices[5] = m_vertices[m_count - 1];

				prim = GS_TRIANGLELIST;
				m_count = 6;

				return true;
			}
		}

		#pragma endregion

		#pragma region ffx random battle transition (z buffer written directly, clear it now)

		if(m_game.title == CRC::FFX)
		{
			uint32 FBP = m_context->FRAME.Block();
			uint32 ZBP = m_context->ZBUF.Block();
			uint32 TBP = m_context->TEX0.TBP0;

			if((FBP == 0x00d00 || FBP == 0x00000) && ZBP == 0x02100 && PRIM->TME && TBP == 0x01a00 && m_context->TEX0.PSM == PSM_PSMCT16S)
			{
				m_dev->ClearDepth(ds, 0);
			}

			return true;
		}

		#pragma endregion

		#pragma region metal slug missing red channel fix
		
		if(m_game.title == CRC::MetalSlug6)
		{
			for(int i = 0, j = m_count; i < j; i++)
			{
				if(m_vertices[i].r == 0 && m_vertices[i].g != 0 && m_vertices[i].b != 0)
				{
					m_vertices[i].r = (m_vertices[i].g + m_vertices[i].b) / 2;
				}
			}

			return true;
		}

		#pragma endregion

		#pragma region palette uploaded in a point list, pure genius...

		if(m_game.flags & CRC::PointListPalette)
		{
			if(prim == GS_POINTLIST && !PRIM->TME)
			{
				uint32 bp = m_context->FRAME.Block();
				uint32 bw = m_context->FRAME.FBW;

				if(bp >= 0x03f40 && (bp & 0x1f) == 0)
				{
					if(m_count == 16)
					{
						for(int i = 0; i < 16; i++)
						{
							m_vertices[i].a = m_vertices[i].a >= 0x80 ? 0xff : m_vertices[i].a * 2;

							m_mem.WritePixel32(i & 7, i >> 3, m_vertices[i].c0, bp, bw);
						}

						m_mem.m_clut.Invalidate();

						return false;
					}
					else if(m_count == 256)
					{
						for(int i = 0; i < 256; i++)
						{
							m_vertices[i].a = m_vertices[i].a >= 0x80 ? 0xff : m_vertices[i].a * 2;

							m_mem.WritePixel32(i & 15, i >> 4, m_vertices[i].c0, bp, bw);
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

		#pragma endregion

		#pragma region GoW2 z buffer clear

		if(m_game.title == CRC::GodOfWar2)
		{
			uint32 FBP = m_context->FRAME.Block();
			uint32 FBW = m_context->FRAME.FBW;
			uint32 FPSM = m_context->FRAME.PSM;

			if((FBP == 0x00f00 || FBP == 0x00100) && FPSM == PSM_PSMZ24) // ntsc 0xf00, pal 0x100
			{
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

		#pragma endregion

		#pragma region Simpsons Game z buffer clear

		if(m_game.title == CRC::SimpsonsGame)
		{
			uint32 FBP = m_context->FRAME.Block();
			uint32 FBW = m_context->FRAME.FBW;
			uint32 FPSM = m_context->FRAME.PSM;

			if(FBP == 0x01800 && FPSM == PSM_PSMZ24)
			{
				// instead of just simply drawing a full height 512x512 sprite to clear the z buffer,
				// it uses a 512x256 sprite only, yet it is still able to fill the whole surface with zeros,
				// how? by using a render target that overlaps with the lower half of the z buffer...

				m_dev->ClearDepth(ds, 0);

				return false;
			}

			return true;
		}

		#pragma endregion

		return true;
	}

	virtual void OverrideOutput()
	{
		#pragma region dbzbt2 palette readback (cannot detect yet, when fetching the texture later)

		if(m_game.title == CRC::DBZBT2)
		{
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

		#pragma endregion

		#pragma region MajokkoALaMode2 palette readback

		if(m_game.title == CRC::MajokkoALaMode2)
		{
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

		#pragma endregion
	}

	bool CanUpscale()
	{
		#pragma region dbzbt2 palette should stay 64 x 64

		if(m_game.title == CRC::DBZBT2)
		{
			uint32 FBP = m_context->FRAME.Block();

			if(FBP == 0x03c00 || FBP == 0x03ac0)
			{
				return false;
			}
		}

		#pragma endregion

		#pragma region MajokkoALaMode2 palette should stay 16 x 16

		if(m_game.title == CRC::MajokkoALaMode2)
		{
			uint32 FBP = m_context->FRAME.Block();

			if(FBP == 0x03f40)
			{
				return false;
			}
		}

		#pragma endregion

		#pragma region TalesOfAbyss full image blur and brightening

		if(m_game.title == CRC::TalesOfAbyss)
		{
			uint32 FBP = m_context->FRAME.Block();

			if(FBP == 0x036e0 || FBP == 0x03560 || FBP == 0x038e0)
			{
				return false;
			}
		}

		#pragma endregion

		return __super::CanUpscale();
	}

public:
	GSRendererHW(uint8* base, bool mt, void (*irq)(), GSDevice* dev, GSTextureCache* tc)
		: GSRendererT<Vertex>(base, mt, irq, dev)
		, m_tc(tc)
		, m_width(1024)
		, m_height(1024)
		, m_skip(0)
		, m_reset(false)
	{
		if(!m_nativeres)
		{
			m_width = theApp.GetConfig("resx", m_width);
			m_height = theApp.GetConfig("resy", m_height);
		}
	}

	virtual ~GSRendererHW()
	{
		delete m_tc;
	}

	void SetGameCRC(uint32 crc, int options)
	{
		__super::SetGameCRC(crc, options);

		if(m_game.title == CRC::JackieChanAdv)
		{
			m_width = 1280; // TODO: uses a 1280px wide 16 bit render target, but this only fixes half of the problem
		}
	}
};
