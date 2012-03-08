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

#include "stdafx.h"
#include "GSTextureCache.h"

GSTextureCache::GSTextureCache(GSRenderer* r)
	: m_renderer(r)
{
	m_spritehack = !!theApp.GetConfig("UserHacks", 0) ? theApp.GetConfig("UserHacks_SpriteHack", 0) : 0;
	
	UserHacks_HalfPixelOffset = !!theApp.GetConfig("UserHacks", 0) && !!theApp.GetConfig("UserHacks_HalfPixelOffset", 0);

	m_paltex = !!theApp.GetConfig("paltex", 0);

	m_temp = (uint8*)_aligned_malloc(1024 * 1024 * sizeof(uint32), 32);
}

GSTextureCache::~GSTextureCache()
{
	RemoveAll();

	_aligned_free(m_temp);
}

void GSTextureCache::RemoveAll()
{
	m_src.RemoveAll();

	for(int type = 0; type < 2; type++)
	{
		for_each(m_dst[type].begin(), m_dst[type].end(), delete_object());

		m_dst[type].clear();
	}
}

GSTextureCache::Source* GSTextureCache::LookupSource(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, const GSVector4i& r)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];
	const uint32* clut = m_renderer->m_mem.m_clut;

	Source* src = NULL;

	list<Source*>& m = m_src.m_map[TEX0.TBP0 >> 5];

	for(list<Source*>::iterator i = m.begin(); i != m.end(); i++)
	{
		Source* s = *i;

		if(((TEX0.u32[0] ^ s->m_TEX0.u32[0]) | ((TEX0.u32[1] ^ s->m_TEX0.u32[1]) & 3)) != 0) // TBP0 TBW PSM TW TH
		{
			continue;
		}

		if((psm.trbpp == 16 || psm.trbpp == 24) && TEX0.TCC && TEXA != s->m_TEXA)
		{
			continue;
		}

		if(s->m_palette == NULL && psm.pal > 0 && !GSVector4i::compare64(clut, s->m_clut, psm.pal * sizeof(clut[0])))
		{
			continue;
		}

		m.splice(m.begin(), m, i);

		src = s;

		break;
	}

	Target* dst = NULL;

#ifdef DISABLE_HW_TEXTURE_CACHE
	if( 0 )
#else
	if(src == NULL)
#endif
	{
		uint32 bp = TEX0.TBP0;
		uint32 psm = TEX0.PSM;

		// Arc the Lad finds the wrong surface here when looking for a depth stencil.
		// Since we're currently not caching depth stencils (check ToDo in CreateSource) we should not look for it here.
		
		// (Simply not doing this code at all makes a lot of previsouly missing stuff show (but breaks pretty much everything
		// else.)
		
		//for(int type = 0; type < 2 && dst == NULL; type++)
		for(int type = 0; type < 1 && dst == NULL; type++) // Only look for render target, no depth stencil
		{
			for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
			{
				Target* t = *i;

				if(t->m_used && t->m_dirty.empty() && GSUtil::HasSharedBits(bp, psm, t->m_TEX0.TBP0, t->m_TEX0.PSM))
				{
					dst = t;

					break;
				}
			}
		}
	}

	if(src == NULL)
	{
		src = CreateSource(TEX0, TEXA, dst);

		if(src == NULL)
		{
			return NULL;
		}
	}

	if(psm.pal > 0)
	{
		int size = psm.pal * sizeof(clut[0]);

		if(src->m_palette)
		{
			if(src->m_initpalette || !GSVector4i::update(src->m_clut, clut, size))
			{
				src->m_palette->Update(GSVector4i(0, 0, psm.pal, 1), src->m_clut, size);
				src->m_initpalette = false;
			}
		}
	}

	src->Update(r);

	m_src.m_used = true;

	return src;
}

GSTextureCache::Target* GSTextureCache::LookupTarget(const GIFRegTEX0& TEX0, int w, int h, int type, bool used)
{
	uint32 bp = TEX0.TBP0;

	Target* dst = NULL;

	for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); i++)
	{
		Target* t = *i;

		if(bp == t->m_TEX0.TBP0)
		{
			m_dst[type].splice(m_dst[type].begin(), m_dst[type], i);

			dst = t;

			dst->m_TEX0 = TEX0;

			break;
		}
	}

	if(dst == NULL)
	{
		dst = CreateTarget(TEX0, w, h, type);

		if(dst == NULL)
		{
			return NULL;
		}
	}
	else
	{
		dst->Update();
	}

	if(m_renderer->CanUpscale())
	{
		int multiplier = m_renderer->GetUpscaleMultiplier();

		if(multiplier > 1) // it's limited to a maximum of 4 on reading the config
		{

#if 0 //#ifdef ENABLE_UPSCALE_HACKS //not happy with this yet..

			float x = 1.0f;
			float y = 1.0f;

			switch(multiplier)
			{
				case 2: x = 1.9375; y = 2.0f; break; // x res get's rid of vertical lines in many games
				case 3: x = 2.9375f; y = 2.9375f; break; // not helping much
				case 4: x = 3.875f; y = 3.875f; break; // not helping much
				default: __assume(0);
			}

			dst->m_texture->SetScale(GSVector2::_(x, y));
#else
			dst->m_texture->SetScale(GSVector2((float)multiplier, (float)multiplier));
#endif
		}
		else
		{
			GSVector4i fr = m_renderer->GetFrameRect();

			int ww = (int)(fr.left + m_renderer->GetDisplayRect().width());
			int hh = (int)(fr.top + m_renderer->GetDisplayRect().height());

			if(hh <= m_renderer->GetDeviceSize().y / 2)
			{
				hh *= 2;
			}

			// This vp2 fix doesn't work most of the time

			if(hh < 512 && m_renderer->m_context->SCISSOR.SCAY1 == 511) // vp2
			{
				hh = 512;
			}

			if(ww > 0 && hh > 0)
			{
				dst->m_texture->SetScale(GSVector2((float)w / ww, (float)h / hh));
			}
		}
	}

	if(used)
	{
		dst->m_used = true;
	}

	return dst;
}

GSTextureCache::Target* GSTextureCache::LookupTarget(const GIFRegTEX0& TEX0, int w, int h)
{
	uint32 bp = TEX0.TBP0;

	Target* dst = NULL;

	for(list<Target*>::iterator i = m_dst[RenderTarget].begin(); i != m_dst[RenderTarget].end(); i++)
	{
		Target* t = *i;

		if(bp == t->m_TEX0.TBP0)
		{
			dst = t;

			break;
		}
		else
		{
			// HACK: try to find something close to the base pointer

			if(t->m_TEX0.TBP0 <= bp && bp < t->m_TEX0.TBP0 + 0x700 && (!dst || t->m_TEX0.TBP0 >= dst->m_TEX0.TBP0))
			{
				dst = t;
			}
		}
	}

	if(dst == NULL)
	{
		dst = CreateTarget(TEX0, w, h, RenderTarget);

		if(dst == NULL)
		{
			return NULL;
		}

		m_renderer->m_dev->ClearRenderTarget(dst->m_texture, 0); // new frame buffers after reset should be cleared, don't display memory garbage
	}
	else
	{
		dst->Update();
	}

	dst->m_used = true;

	return dst;
}

void GSTextureCache::InvalidateVideoMem(GSOffset* o, const GSVector4i& rect, bool target)
{
	if(!o) return; // Fixme. Crashes Dual Hearts, maybe others as well. Was fine before r1549.

	uint32 bp = o->bp;
	uint32 bw = o->bw;
	uint32 psm = o->psm;

	if(!target)
	{
		const list<Source*>& m = m_src.m_map[bp >> 5];

		for(list<Source*>::const_iterator i = m.begin(); i != m.end(); )
		{
			list<Source*>::const_iterator j = i++;

			Source* s = *j;

			if(GSUtil::HasSharedBits(bp, psm, s->m_TEX0.TBP0, s->m_TEX0.PSM))
			{
				m_src.RemoveAt(s);
			}
		}
	}

	GSVector4i r;

	uint32* pages = (uint32*)m_temp;
	
	o->GetPages(rect, pages, &r);

	bool found = false;

	for(const uint32* p = pages; *p != GSOffset::EOP; p++)
	{
		uint32 page = *p;

		const list<Source*>& m = m_src.m_map[page];

		for(list<Source*>::const_iterator i = m.begin(); i != m.end(); )
		{
			list<Source*>::const_iterator j = i++;

			Source* s = *j;

			if(GSUtil::HasSharedBits(psm, s->m_TEX0.PSM))
			{
				uint32* RESTRICT valid = s->m_valid;

				bool b = bp == s->m_TEX0.TBP0;

				if(!s->m_target)
				{
					if(s->m_repeating)
					{
						vector<GSVector2i>& l = s->m_p2t[page];
						
						for(vector<GSVector2i>::iterator k = l.begin(); k != l.end(); k++)
						{
							valid[k->x] &= k->y;
						}
					}
					else
					{
						valid[page] = 0;
					}

					s->m_complete = false;

					found = b;
				}
				else
				{
					// TODO

					if(b)
					{
						m_src.RemoveAt(s);
					}
				}
			}
		}
	}

	if(!target) return;

	for(int type = 0; type < 2; type++)
	{
		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); )
		{
			list<Target*>::iterator j = i++;

			Target* t = *j;

			if(GSUtil::HasSharedBits(bp, psm, t->m_TEX0.TBP0, t->m_TEX0.PSM))
			{
				if(!found && GSUtil::HasCompatibleBits(psm, t->m_TEX0.PSM))
				{
					t->m_dirty.push_back(GSDirtyRect(r, psm));
					t->m_TEX0.TBW = bw;
				}
				else
				{
					m_dst[type].erase(j);
					delete t;
					continue;
				}
			}

			if(GSUtil::HasSharedBits(psm, t->m_TEX0.PSM) && bp < t->m_TEX0.TBP0)
			{
				uint32 rowsize = bw * 8192;
				uint32 offset = (uint32)((t->m_TEX0.TBP0 - bp) * 256);

				if(rowsize > 0 && offset % rowsize == 0)
				{
					int y = GSLocalMemory::m_psm[psm].pgs.y * offset / rowsize;

					if(r.bottom > y)
					{
						// TODO: do not add this rect above too
						t->m_dirty.push_back(GSDirtyRect(GSVector4i(r.left, r.top - y, r.right, r.bottom - y), psm));
						t->m_TEX0.TBW = bw;
						continue;
					}
				}
			}
		}
	}
}

void GSTextureCache::InvalidateLocalMem(GSOffset* o, const GSVector4i& r)
{
	uint32 bp = o->bp;
	uint32 psm = o->psm;
	uint32 bw = o->bw;

	// No depth handling please.
	if (psm == PSM_PSMZ32 || psm == PSM_PSMZ24 || psm == PSM_PSMZ16 || psm == PSM_PSMZ16S)
		return;

	// This is a shorter but potentially slower version of the below, commented out code.
	// It works for all the games mentioned below and fixes a couple of other ones as well
	// (Busen0: Wizardry and Chaos Legion).
	// Also in a few games the below code ran the Grandia3 case when it shouldn't :p
	for(list<Target*>::iterator i = m_dst[RenderTarget].begin(); i != m_dst[RenderTarget].end(); )
	{
		list<Target*>::iterator j = i++;

		Target* t = *j;

		if (t->m_TEX0.PSM != PSM_PSMZ32 && t->m_TEX0.PSM != PSM_PSMZ24 && t->m_TEX0.PSM != PSM_PSMZ16 && t->m_TEX0.PSM != PSM_PSMZ16S)
		{
			if(GSUtil::HasSharedBits(bp, psm, t->m_TEX0.TBP0, t->m_TEX0.PSM))
			{
				// note: r.rintersect breaks Wizardry and Chaos Legion
				// Read(t, t->m_valid) works in all tested games but is very slow in GUST titles ><
				if (r.x == 0 && r.y == 0) // Full screen read?
					Read(t, t->m_valid);
				else // Block level read?
					Read(t, r.rintersect(t->m_valid));
			}
		}
	}
	
	//GSTextureCache::Target* rt2 = NULL;
	//int ymin = INT_MAX;
	//for(list<Target*>::iterator i = m_dst[RenderTarget].begin(); i != m_dst[RenderTarget].end(); )
	//{
	//	list<Target*>::iterator j = i++;

	//	Target* t = *j;

	//	if (t->m_TEX0.PSM != PSM_PSMZ32 && t->m_TEX0.PSM != PSM_PSMZ24 && t->m_TEX0.PSM != PSM_PSMZ16 && t->m_TEX0.PSM != PSM_PSMZ16S)
	//	{
	//		if(GSUtil::HasSharedBits(bp, psm, t->m_TEX0.TBP0, t->m_TEX0.PSM))
	//		{
	//			if(GSUtil::HasCompatibleBits(psm, t->m_TEX0.PSM))
	//			{
	//				Read(t, r.rintersect(t->m_valid));
	//				return;
	//			}
	//			else if(psm == PSM_PSMCT32 && (t->m_TEX0.PSM == PSM_PSMCT16 || t->m_TEX0.PSM == PSM_PSMCT16S))
	//			{
	//				// ffx-2 riku changing to her default (shoots some reflecting glass at the end), 16-bit rt read as 32-bit
	//				Read(t, GSVector4i(r.left, r.top, r.right, r.top + (r.bottom - r.top) * 2).rintersect(t->m_valid));
	//				return;
	//			}
	//			else
	//			{
	//				if (psm == PSM_PSMT4HH && t->m_TEX0.PSM == PSM_PSMCT32)
	//				{
	//					// Silent Hill Origins shadows: Read 8 bit using only the HIGH bits (4 bit) texture as 32 bit.
	//					Read(t, r.rintersect(t->m_valid));
	//					return;
	//				}
	//				else
	//				{
	//					//printf("Trashing render target. We have a %d type texture and we are trying to write into a %d type texture\n", t->m_TEX0.PSM, psm);
	//					m_dst[RenderTarget].erase(j);
	//					delete t;
	//				}
	//			}
	//		}

	//		// Grandia3, FFX, FFX-2 pause menus. t->m_TEX0.TBP0 magic number checks because otherwise kills xs2 videos
	//		if( (GSUtil::HasSharedBits(psm, t->m_TEX0.PSM) && (bp > t->m_TEX0.TBP0) )
	//			&& ((t->m_TEX0.TBP0 == 0) || (t->m_TEX0.TBP0==3328) || (t->m_TEX0.TBP0==3584) ))
	//		{
	//			//printf("first : %d-%d child : %d-%d\n", psm, bp, t->m_TEX0.PSM, t->m_TEX0.TBP0);
	//			uint32 rowsize = bw * 8192;
	//			uint32 offset = (uint32)((bp - t->m_TEX0.TBP0) * 256);

	//			if(rowsize > 0 && offset % rowsize == 0)
	//			{
	//				int y = GSLocalMemory::m_psm[psm].pgs.y * offset / rowsize;

	//				if(y < ymin && y < 512)
	//				{
	//					rt2 = t;
	//					ymin = y;
	//				}
	//			}
	//		}
	//	}
	//}
	//if(rt2)
	//{
	//	Read(rt2, GSVector4i(r.left, r.top + ymin, r.right, r.bottom + ymin));
	//}

	
	// TODO: ds
}

void GSTextureCache::IncAge()
{
	int maxage = m_src.m_used ? 3 : 30;

	for(hash_set<Source*>::iterator i = m_src.m_surfaces.begin(); i != m_src.m_surfaces.end(); )
	{
		hash_set<Source*>::iterator j = i++;

		Source* s = *j;

		if(++s->m_age > maxage)
		{
			m_src.RemoveAt(s);
		}
	}

	m_src.m_used = false;

	// Clearing of Rendertargets causes flickering in many scene transitions.
	// Sigh, this seems to be used to invalidate surfaces. So set a huge maxage to avoid flicker,
	// but still invalidate surfaces. (Disgaea 2 fmv when booting the game through the BIOS)
	// Original maxage was 4 here, Xenosaga 2 needs at least 240, else it flickers on scene transitions.
	maxage = 400; // ffx intro scene changes leave the old image untouched for a couple of frames and only then start using it

	for(int type = 0; type < 2; type++)
	{
		for(list<Target*>::iterator i = m_dst[type].begin(); i != m_dst[type].end(); )
		{
			list<Target*>::iterator j = i++;

			Target* t = *j;

			if(++t->m_age > maxage)
			{
				m_dst[type].erase(j);

				delete t;
			}
		}
	}
}

//Fixme: Several issues in here. Not handling depth stencil, pitch conversion doesnt work.
GSTextureCache::Source* GSTextureCache::CreateSource(const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, Target* dst)
{
	Source* src = new Source(m_renderer, TEX0, TEXA, m_temp);

	int tw = 1 << TEX0.TW;
	int th = 1 << TEX0.TH;
	int tp = TEX0.TBW << 6;

	bool hack = false;

	if(dst == NULL)
	{
		if(m_spritehack && (TEX0.PSM == PSM_PSMT8 || TEX0.PSM == PSM_PSMT8H)) 
		{
			src->m_spritehack_t = true;
			
			if(m_spritehack == 2 && TEX0.CPSM != PSM_PSMCT16) 
				src->m_spritehack_t = false;		
		}			
		else
			src->m_spritehack_t = false;
		
		if(m_paltex && GSLocalMemory::m_psm[TEX0.PSM].pal > 0)
		{
			src->m_fmt = FMT_8;

			src->m_texture = m_renderer->m_dev->CreateTexture(tw, th, Get8bitFormat());
			src->m_palette = m_renderer->m_dev->CreateTexture(256, 1);
		}
		else
		{
			src->m_fmt = FMT_32;

			src->m_texture = m_renderer->m_dev->CreateTexture(tw, th);
		}
	}
	else
	{
		// TODO: clean up this mess

		src->m_target = true;

		if(dst->m_type != RenderTarget)
		{
			// TODO
			delete src;
			return NULL;
		}

		dst->Update();

		GSTexture* tmp = NULL;

		if(dst->m_texture->IsMSAA())
		{
			tmp = dst->m_texture;

			dst->m_texture = m_renderer->m_dev->Resolve(dst->m_texture);
		}

		// do not round here!!! if edge becomes a black pixel and addressing mode is clamp => everything outside the clamped area turns into black (kh2 shadows)

		int w = (int)(dst->m_texture->GetScale().x * tw);
		int h = (int)(dst->m_texture->GetScale().y * th);

		GSVector2i dstsize = dst->m_texture->GetSize();

		// pitch conversion

		if(dst->m_TEX0.TBW != TEX0.TBW) // && dst->m_TEX0.PSM == TEX0.PSM
		{
			// This is so broken :p
			////Better not do the code below, "fixes" like every game that ever gets here..
			////Edit: Ratchet and Clank needs this to show most of it's graphics at all.
			////Someone else fix this please, I can't :p
			////delete src; return NULL;

			//// sfex3 uses this trick (bw: 10 -> 5, wraps the right side below the left)

			//ASSERT(dst->m_TEX0.TBW > TEX0.TBW); // otherwise scale.x need to be reduced to make the larger texture fit (TODO)

			//src->m_texture = m_renderer->m_dev->CreateRenderTarget(dstsize.x, dstsize.y, false);

			//GSVector4 size = GSVector4(dstsize).xyxy();
			//GSVector4 scale = GSVector4(dst->m_texture->GetScale()).xyxy();

			//int blockWidth  = 64;
			//int blockHeight = TEX0.PSM == PSM_PSMCT32 || TEX0.PSM == PSM_PSMCT24 ? 32 : 64;

			//GSVector4i br(0, 0, blockWidth, blockHeight);

			//int sw = (int)dst->m_TEX0.TBW << 6;

			//int dw = (int)TEX0.TBW << 6;
			//int dh = 1 << TEX0.TH;

			//if(sw != 0)
			//for(int dy = 0; dy < dh; dy += blockHeight)
			//{
			//	for(int dx = 0; dx < dw; dx += blockWidth)
			//	{
			//		int o = dy * dw / blockHeight + dx;

			//		int sx = o % sw;
			//		int sy = o / sw;

			//		GSVector4 sr = GSVector4(GSVector4i(sx, sy).xyxy() + br) * scale / size;
			//		GSVector4 dr = GSVector4(GSVector4i(dx, dy).xyxy() + br) * scale;

			//		m_renderer->m_dev->StretchRect(dst->m_texture, sr, src->m_texture, dr);

			//		// TODO: this is quite a lot of StretchRect, do it with one Draw
			//	}
			//}
		}
		else if(tw < 1024)
		{
			// FIXME: timesplitters blurs the render target by blending itself over a couple of times
			hack = true;
			//if(tw == 256 && th == 128 && (TEX0.TBP0 == 0 || TEX0.TBP0 == 0x00e00))
			//{
			//	delete src;
			//	return NULL;
			//}
		}
		// width/height conversion

		GSVector2 scale = dst->m_texture->GetScale();

		GSVector4 dr(0, 0, w, h);

		if(w > dstsize.x)
		{
			scale.x = (float)dstsize.x / tw;
			dr.z = (float)dstsize.x * scale.x / dst->m_texture->GetScale().x;
			w = dstsize.x;
		}

		if(h > dstsize.y)
		{
			scale.y = (float)dstsize.y / th;
			dr.w = (float)dstsize.y * scale.y / dst->m_texture->GetScale().y;
			h = dstsize.y;
		}

		GSVector4 sr(0, 0, w, h);

		GSTexture* st = src->m_texture ? src->m_texture : dst->m_texture;
		GSTexture* dt = m_renderer->m_dev->CreateRenderTarget(w, h, false);

		if(!src->m_texture)
		{
			src->m_texture = dt;
		}

		if((sr == dr).alltrue())
		{
			m_renderer->m_dev->CopyRect(st, dt, GSVector4i(0, 0, w, h));
		}
		else
		{
			sr.z /= st->GetWidth();
			sr.w /= st->GetHeight();

			m_renderer->m_dev->StretchRect(st, sr, dt, dr);
		}

		if(dt != src->m_texture)
		{
			m_renderer->m_dev->Recycle(src->m_texture);

			src->m_texture = dt;
		}

		if( src->m_texture )
			src->m_texture->SetScale(scale);
		else
			ASSERT(0);

		switch(TEX0.PSM)
		{
		default:
			// Note: this assertion triggers in Xenosaga2 after the first intro scenes, when
			// gameplay first begins (in the city).
			ASSERT(0);
		case PSM_PSMCT32:
			src->m_fmt = FMT_32;
			break;
		case PSM_PSMCT24:
			src->m_fmt = FMT_24;
			break;
		case PSM_PSMCT16:
		case PSM_PSMCT16S:
			src->m_fmt = FMT_16;
			break;
		case PSM_PSMT8H:
			src->m_fmt = FMT_8H;
			src->m_palette = m_renderer->m_dev->CreateTexture(256, 1);
			break;
		case PSM_PSMT8:
			//Not sure, this wasn't handled at all.
			//Xenosaga 2 and 3 use it, Tales of Legendia as well.
			//It's always used for fog like effects.
			src->m_fmt = FMT_8;
			src->m_palette = m_renderer->m_dev->CreateTexture(256, 1);
			break;
		case PSM_PSMT4HL:
			src->m_fmt = FMT_4HL;
			src->m_palette = m_renderer->m_dev->CreateTexture(256, 1);
			break;
		case PSM_PSMT4HH:
			src->m_fmt = FMT_4HH;
			src->m_palette = m_renderer->m_dev->CreateTexture(256, 1);
			break;
		}

		if(tmp != NULL)
		{
			m_renderer->m_dev->Recycle(dst->m_texture);

			dst->m_texture = tmp;
		}

		// Offset hack. Can be enabled via GSdx options.
		// The offset will be used in Draw().

		float modx = 0.0f;
		float mody = 0.0f;

		if(UserHacks_HalfPixelOffset && hack)
		{
			switch(m_renderer->GetUpscaleMultiplier())
			{
			case 2:  modx = 2.2f; mody = 2.2f; dst->m_texture->LikelyOffset = true;  break;
			case 3:  modx = 3.1f; mody = 3.1f; dst->m_texture->LikelyOffset = true;  break;
			case 4:  modx = 4.2f; mody = 4.2f; dst->m_texture->LikelyOffset = true;  break;
			case 5:  modx = 5.3f; mody = 5.3f; dst->m_texture->LikelyOffset = true;  break;
			case 6:  modx = 6.2f; mody = 6.2f; dst->m_texture->LikelyOffset = true;  break;
			default: modx = 0.0f; mody = 0.0f; dst->m_texture->LikelyOffset = false; break;
			}
		}

		dst->m_texture->OffsetHack_modx = modx;
		dst->m_texture->OffsetHack_mody = mody;
	}

	if(src->m_texture == NULL)
	{
		ASSERT(0);
		delete src;
		return NULL;
	}

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	if(psm.pal > 0)
	{
		memcpy(src->m_clut, (const uint32*)m_renderer->m_mem.m_clut, psm.pal * sizeof(uint32));
	}

	m_src.Add(src, TEX0, m_renderer->m_context->offset.tex);

	return src;
}

GSTextureCache::Target* GSTextureCache::CreateTarget(const GIFRegTEX0& TEX0, int w, int h, int type)
{
	Target* t = new Target(m_renderer, TEX0, m_temp);

	// FIXME: initial data should be unswizzled from local mem in Update() if dirty

	t->m_type = type;

	if(type == RenderTarget)
	{
		t->m_texture = m_renderer->m_dev->CreateRenderTarget(w, h, true);

		t->m_used = true; // FIXME
	}
	else if(type == DepthStencil)
	{
		t->m_texture = m_renderer->m_dev->CreateDepthStencil(w, h, true);
	}

	if(t->m_texture == NULL)
	{
		ASSERT(0);
		delete t;
		return NULL;
	}

	m_dst[type].push_front(t);

	return t;
}

// GSTextureCache::Surface

GSTextureCache::Surface::Surface(GSRenderer* r, uint8* temp)
	: m_renderer(r)
	, m_temp(temp)
	, m_texture(NULL)
	, m_age(0)
{
	m_TEX0.TBP0 = 0x3fff;
}

GSTextureCache::Surface::~Surface()
{
	m_renderer->m_dev->Recycle(m_texture);
}

void GSTextureCache::Surface::Update()
{
	m_age = 0;
}

// GSTextureCache::Source

GSTextureCache::Source::Source(GSRenderer* r, const GIFRegTEX0& TEX0, const GIFRegTEXA& TEXA, uint8* temp)
	: Surface(r, temp)
	, m_palette(NULL)
	, m_initpalette(true)
	, m_fmt(0)
	, m_target(false)
	, m_complete(false)
	, m_p2t(NULL)
{
	m_TEX0 = TEX0;
	m_TEXA = TEXA;

	memset(m_valid, 0, sizeof(m_valid));

	m_clut = (uint32*)_aligned_malloc(256 * sizeof(uint32), 32);

	memset(m_clut, 0, sizeof(m_clut));

	m_write.rect = (GSVector4i*)_aligned_malloc(3 * sizeof(GSVector4i), 32);
	m_write.count = 0;

	m_repeating = m_TEX0.IsRepeating();

	if(m_repeating)
	{
		m_p2t = r->m_mem.GetPage2TileMap(m_TEX0);
	}
}

GSTextureCache::Source::~Source()
{
	m_renderer->m_dev->Recycle(m_palette);

	_aligned_free(m_clut);

	_aligned_free(m_write.rect);
}

void GSTextureCache::Source::Update(const GSVector4i& rect)
{
	Surface::Update();

	if(m_complete || m_target)
	{
		return;
	}

	GSVector2i bs = GSLocalMemory::m_psm[m_TEX0.PSM].bs;

	int tw = std::max<int>(1 << m_TEX0.TW, bs.x);
	int th = std::max<int>(1 << m_TEX0.TH, bs.y);

	GSVector4i r = rect.ralign<Align_Outside>(bs);

	if(r.eq(GSVector4i(0, 0, tw, th)))
	{
		m_complete = true; // lame, but better than nothing
	}

	const GSOffset* o = m_renderer->m_context->offset.tex;

	uint32 blocks = 0;

	if(m_repeating)
	{
		for(int y = r.top; y < r.bottom; y += bs.y)
		{
			uint32 base = o->block.row[y >> 3];

			for(int x = r.left, i = (y << 7) + x; x < r.right; x += bs.x, i += bs.x)
			{
				uint32 block = base + o->block.col[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 addr = i >> 3;

					uint32 row = addr >> 5;
					uint32 col = 1 << (addr & 31);

					if((m_valid[row] & col) == 0)
					{
						m_valid[row] |= col;

						Write(GSVector4i(x, y, x + bs.x, y + bs.y));

						blocks++;
					}
				}
			}
		}
	}
	else
	{
		for(int y = r.top; y < r.bottom; y += bs.y)
		{
			uint32 base = o->block.row[y >> 3];

			for(int x = r.left; x < r.right; x += bs.x)
			{
				uint32 block = base + o->block.col[x >> 3];

				if(block < MAX_BLOCKS)
				{
					uint32 row = block >> 5;
					uint32 col = 1 << (block & 31);

					if((m_valid[row] & col) == 0)
					{
						m_valid[row] |= col;

						Write(GSVector4i(x, y, x + bs.x, y + bs.y));

						blocks++;
					}
				}
			}
		}
	}

	if(blocks > 0)
	{
		m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, bs.x * bs.y * blocks << (m_fmt == FMT_32 ? 2 : 0));

		Flush(m_write.count);
	}
}

void GSTextureCache::Source::Write(const GSVector4i& r)
{
	m_write.rect[m_write.count++] = r;

	while(m_write.count >= 2)
	{
		GSVector4i& a = m_write.rect[m_write.count - 2];
		GSVector4i& b = m_write.rect[m_write.count - 1];

		if((a == b.zyxw()).mask() == 0xfff0)
		{
			a.right = b.right; // extend right

			m_write.count--;
		}
		else if((a == b.xwzy()).mask() == 0xff0f)
		{
			a.bottom = b.bottom; // extend down

			m_write.count--;
		}
		else
		{
			break;
		}
	}

	if(m_write.count > 2)
	{
		Flush(1);
	}
}

void GSTextureCache::Source::Flush(uint32 count)
{
	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[m_TEX0.PSM];

	int tw = 1 << m_TEX0.TW;
	int th = 1 << m_TEX0.TH;

	GSVector4i tr(0, 0, tw, th);

	int pitch = max(tw, psm.bs.x) * sizeof(uint32);

	GSLocalMemory& mem = m_renderer->m_mem;

	const GSOffset* o = m_renderer->m_context->offset.tex;

	GSLocalMemory::readTexture rtx = psm.rtx;

	if(m_fmt == FMT_8)
	{
		pitch >>= 2;
		rtx = psm.rtxP;
	}

	uint8* buff = m_temp;

	for(uint32 i = 0; i < count; i++)
	{
		GSVector4i r = m_write.rect[i];

		if((r > tr).mask() & 0xff00)
		{
			(mem.*rtx)(o, r, buff, pitch, m_TEXA);

			m_texture->Update(r.rintersect(tr), buff, pitch);
		}
		else
		{
			GSTexture::GSMap m;

			if(m_texture->Map(m, &r))
			{
				(mem.*rtx)(o, r, m.bits, m.pitch, m_TEXA);

				m_texture->Unmap();
			}
			else
			{
				(mem.*rtx)(o, r, buff, pitch, m_TEXA);

				m_texture->Update(r, buff, pitch);
			}
		}
	}

	if(count < m_write.count)
	{
		memcpy(&m_write.rect[0], &m_write.rect[count], (m_write.count - count) * sizeof(m_write.rect[0]));
	}

	m_write.count -= count;
}

// GSTextureCache::Target

GSTextureCache::Target::Target(GSRenderer* r, const GIFRegTEX0& TEX0, uint8* temp)
	: Surface(r, temp)
	, m_type(-1)
	, m_used(false)
{
	m_TEX0 = TEX0;

	m_valid = GSVector4i::zero();
}

void GSTextureCache::Target::Update()
{
	Surface::Update();

	// FIXME: the union of the rects may also update wrong parts of the render target (but a lot faster :)

	GSVector4i r = m_dirty.GetDirtyRectAndClear(m_TEX0, m_texture->GetSize());

	if(r.rempty()) return;

	if(m_type == RenderTarget)
	{
		int w = r.width();
		int h = r.height();

		if(GSTexture* t = m_renderer->m_dev->CreateTexture(w, h))
		{
			const GSOffset* o = m_renderer->m_mem.GetOffset(m_TEX0.TBP0, m_TEX0.TBW, m_TEX0.PSM);

			GIFRegTEXA TEXA;

			TEXA.AEM = 1;
			TEXA.TA0 = 0;
			TEXA.TA1 = 0x80;

			GSTexture::GSMap m;

			if(t->Map(m))
			{
				m_renderer->m_mem.ReadTexture(o, r, m.bits,  m.pitch, TEXA);

				t->Unmap();
			}
			else
			{
				int pitch = ((w + 3) & ~3) * 4;

				m_renderer->m_mem.ReadTexture(o, r, m_temp, pitch, TEXA);

				t->Update(r.rsize(), m_temp, pitch);
			}

			// m_renderer->m_perfmon.Put(GSPerfMon::Unswizzle, w * h * 4);

			m_renderer->m_dev->StretchRect(t, m_texture, GSVector4(r) * GSVector4(m_texture->GetScale()).xyxy());

			m_renderer->m_dev->Recycle(t);
		}
	}
	else if(m_type == DepthStencil)
	{
		// do the most likely thing a direct write would do, clear it

		if((m_renderer->m_game.flags & CRC::ZWriteMustNotClear) == 0)
		{
			m_renderer->m_dev->ClearDepth(m_texture, 0);
		}
	}
}

// GSTextureCache::SourceMap

void GSTextureCache::SourceMap::Add(Source* s, const GIFRegTEX0& TEX0, const GSOffset* o)
{
	m_surfaces.insert(s);

	if(s->m_target)
	{
		// TODO

		m_map[TEX0.TBP0 >> 5].push_front(s);

		return;
	}

	const GSLocalMemory::psm_t& psm = GSLocalMemory::m_psm[TEX0.PSM];

	GSVector2i bs = (TEX0.TBP0 & 31) == 0 ? psm.pgs : psm.bs;

	int tw = 1 << TEX0.TW;
	int th = 1 << TEX0.TH;

	for(int y = 0; y < th; y += bs.y)
	{
		uint32 base = o->block.row[y >> 3];

		for(int x = 0; x < tw; x += bs.x)
		{
			uint32 page = (base + o->block.col[x >> 3]) >> 5;

			if(page < MAX_PAGES)
			{
				m_pages[page >> 5] |= 1 << (page & 31);
			}
		}
	}

	for(int i = 0; i < countof(m_pages); i++)
	{
		if(uint32 p = m_pages[i])
		{
			m_pages[i] = 0;

			list<Source*>* m = &m_map[i << 5];

			unsigned long j;

			while(_BitScanForward(&j, p))
			{
				p ^= 1 << j;

				m[j].push_front(s);
			}
		}
	}
}

void GSTextureCache::SourceMap::RemoveAll()
{
	for_each(m_surfaces.begin(), m_surfaces.end(), delete_object());

	m_surfaces.clear();

	for(uint32 i = 0; i < countof(m_map); i++)
	{
		m_map[i].clear();
	}
}

void GSTextureCache::SourceMap::RemoveAt(Source* s)
{
	m_surfaces.erase(s);

	for(uint32 start = s->m_TEX0.TBP0 >> 5, end = s->m_target ? start : countof(m_map) - 1; start <= end; start++)
	{
		list<Source*>& m = m_map[start];

		for(list<Source*>::iterator i = m.begin(); i != m.end(); )
		{
			list<Source*>::iterator j = i++;

			if(*j == s) {m.erase(j); break;}
		}
	}

	delete s;
}
