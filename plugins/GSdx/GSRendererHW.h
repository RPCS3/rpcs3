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

	void MinMaxUV(int w, int h, GSVector4i& r)
	{
		int wms = m_context->CLAMP.WMS;
		int wmt = m_context->CLAMP.WMT;

		int minu = (int)m_context->CLAMP.MINU;
		int minv = (int)m_context->CLAMP.MINV;
		int maxu = (int)m_context->CLAMP.MAXU;
		int maxv = (int)m_context->CLAMP.MAXV;

		GSVector4i vr = GSVector4i(0, 0, w, h);

		GSVector4i wm[3];

		if(wms + wmt < 6)
		{
			GSVector4 mm;

			if(m_count < 100)
			{
				Vertex* v = m_vertices;

				GSVector4 minv(+1e10f);
				GSVector4 maxv(-1e10f);

				int i = 0;

				if(PRIM->FST)
				{
					for(int j = m_count - 3; i < j; i += 4)
					{
						GSVector4 v0 = v[i + 0].vf[0];
						GSVector4 v1 = v[i + 1].vf[0];
						GSVector4 v2 = v[i + 2].vf[0];
						GSVector4 v3 = v[i + 3].vf[0];

						minv = minv.minv((v0.minv(v1)).minv(v2.minv(v3)));
						maxv = maxv.maxv((v0.maxv(v1)).maxv(v2.maxv(v3)));
					}

					for(int j = m_count; i < j; i++)
					{
						GSVector4 v0 = v[i + 0].vf[0];

						minv = minv.minv(v0);
						maxv = maxv.maxv(v0);
					}

					mm = minv.xyxy(maxv) * GSVector4(16 << m_context->TEX0.TW, 16 << m_context->TEX0.TH).xyxy().rcpnr();
				}
				else
				{
					/*
					for(int j = m_count - 3; i < j; i += 4)
					{
						GSVector4 v0 = GSVector4(v[i + 0].m128[0]) / GSVector4(v[i + 0].GetQ());
						GSVector4 v1 = GSVector4(v[i + 1].m128[0]) / GSVector4(v[i + 1].GetQ());
						GSVector4 v2 = GSVector4(v[i + 2].m128[0]) / GSVector4(v[i + 2].GetQ());
						GSVector4 v3 = GSVector4(v[i + 3].m128[0]) / GSVector4(v[i + 3].GetQ());

						minv = minv.minv((v0.minv(v1)).minv(v2.minv(v3)));
						maxv = maxv.maxv((v0.maxv(v1)).maxv(v2.maxv(v3)));
					}

					for(int j = m_count; i < j; i++)
					{
						GSVector4 v0 = GSVector4(v[i + 0].m128[0]) / GSVector4(v[i + 0].GetQ());;

						minv = minv.minv(v0);
						maxv = maxv.maxv(v0);
					}

					mm = minv.xyxy(maxv);
					*/

					// just can't beat the compiler generated scalar sse code with packed div or rcp

					mm.x = mm.y = +1e10;
					mm.z = mm.w = -1e10;

					for(int j = m_count; i < j; i++)
					{
						float w = 1.0f / v[i].GetQ();

						float x = v[i].t.x * w;

						if(x < mm.x) mm.x = x;
						if(x > mm.z) mm.z = x;
						
						float y = v[i].t.y * w;

						if(y < mm.y) mm.y = y;
						if(y > mm.w) mm.w = y;
					}
				}
			}
			else
			{
				mm = GSVector4(0.0f, 0.0f, 1.0f, 1.0f);
			}

			GSVector4 v0 = GSVector4(vr);
			GSVector4 v1 = v0.zwzw();

			GSVector4 mmf = mm.floor();
			GSVector4 mask = mmf.xyxy() == mmf.zwzw();

			wm[0] = GSVector4i(v0.blend8((mm - mmf) * v1, mask));

			mm *= v1;

			wm[1] = GSVector4i(mm.sat(GSVector4::zero(), v1));
			wm[2] = GSVector4i(mm.sat(GSVector4(minu, minv, maxu, maxv)));
		}

		GSVector4i v;

		switch(wms)
		{
		case CLAMP_REPEAT:
			v = wm[0];
			if(v.x == 0 && v.z != w) v.z = w; // FIXME
			vr.x = v.x;
			vr.z = v.z;
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			v = wm[wms];
			if(v.x > v.z) v.x = v.z;
			vr.x = v.x;
			vr.z = v.z;
			break;
		case CLAMP_REGION_REPEAT:
			vr.x = maxu; 
			vr.z = vr.x + (minu + 1);
			break;
		default: 
			__assume(0);
		}

		switch(wmt)
		{
		case CLAMP_REPEAT:
			v = wm[0];
			if(v.y == 0 && v.w != h) v.w = h; // FIXME
			vr.y = v.y;
			vr.w = v.w;
			break;
		case CLAMP_CLAMP:
		case CLAMP_REGION_CLAMP:
			v = wm[wmt];
			if(v.y > v.w) v.y = v.w;
			vr.y = v.y;
			vr.w = v.w;
			break;
		case CLAMP_REGION_REPEAT:
			vr.y = maxv; 
			vr.w = vr.y + (minv + 1);
			break;
		default:
			__assume(0);
		}

		r = vr + GSVector4i(-1, -1, 1, 1); // one more pixel because of bilinear filtering

		GSVector2i bs = GSLocalMemory::m_psm[m_context->TEX0.PSM].bs;

		r = r.ralign<GSVector4i::Outside>(bs).rintersect(GSVector4i(0, 0, w, h));
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

		if(GSTextureCache::GSRenderTarget* rt = m_tc->GetRenderTarget(TEX0, m_width, m_height, true))
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
		if(IsBadFrame(m_skip))
		{
			return;
		}

		GSDrawingEnvironment& env = m_env;
		GSDrawingContext* context = m_context;

		GIFRegTEX0 TEX0;

		TEX0.TBP0 = context->FRAME.Block();
		TEX0.TBW = context->FRAME.FBW;
		TEX0.PSM = context->FRAME.PSM;

		GSTextureCache::GSRenderTarget* rt = m_tc->GetRenderTarget(TEX0, m_width, m_height);

		TEX0.TBP0 = context->ZBUF.Block();
		TEX0.TBW = context->FRAME.FBW;
		TEX0.PSM = context->ZBUF.PSM;

		GSTextureCache::GSDepthStencil* ds = m_tc->GetDepthStencil(TEX0, m_width, m_height);

		GSTextureCache::GSCachedTexture* tex = NULL;

		if(PRIM->TME)
		{
			tex = m_tc->GetTexture();

			if(!tex) return;
		}

		if(s_dump)
		{
			uint64 frame = m_perfmon.GetFrame();

			string s;
			
			if(s_save && PRIM->TME) 
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

		Draw(prim, rt->m_texture, ds->m_texture, tex);

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

		m_tc->InvalidateTextures(context->FRAME, context->ZBUF);
	}

	virtual void Draw(int prim, GSTexture* rt, GSTexture* ds, GSTextureCache::GSCachedTexture* tex) = 0;

	virtual bool OverrideInput(int& prim, GSTexture* rt, GSTexture* ds, GSTextureCache::GSCachedTexture* t)
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

		#pragma region tomoyo after, clannad, lamune (palette uploaded in a point list, pure genius...)

		if(m_game.title == CRC::TomoyoAfter || m_game.title == CRC::Clannad || m_game.title == CRC::Lamune)
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

				if(GSTextureCache::GSDepthStencil* ds = m_tc->GetDepthStencil(TEX0, m_width, m_height))
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
