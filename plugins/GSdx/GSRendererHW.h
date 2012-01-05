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

class GSRendererHW : public GSRenderer
{
private:
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

	bool OI_FFXII(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_FFX(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_MetalSlug6(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_GodOfWar2(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_SimpsonsGame(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_RozenMaidenGebetGarden(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_SpidermanWoS(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_TyTasmanianTiger(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_DigimonRumbleArena2(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_BlackHawkDown(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_StarWarsForceUnleashed(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_XmenOriginsWolverine(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_CallofDutyFinalFronts(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_SpyroNewBeginning(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_SpyroEternalNight(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_TalesOfLegendia(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	bool OI_PointListPalette(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* t);
	void OO_DBZBT2();
	void OO_MajokkoALaMode2();

	bool CU_DBZBT2();
	bool CU_MajokkoALaMode2();
	bool CU_TalesOfAbyss();

	class Hacks
	{
		template<class T> class HackEntry
		{
		public:
			CRC::Title title;
			CRC::Region region;
			T func;

			HackEntry(CRC::Title t, CRC::Region r, T f)
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

				for(typename list<HackEntry<T> >::iterator i = m_tbl.begin(); i != m_tbl.end(); i++)
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

		Hacks();

		void SetGameCRC(const CRC::Game& game);

	} m_hacks;

	virtual int GetPosX(const void* vertex) const = 0;
	virtual int GetPosY(const void* vertex) const = 0;
	virtual uint32 GetColor(const void* vertex) const = 0;
	virtual void SetColor(void* vertex, uint32 c) const = 0;

	#pragma endregion

protected:
	GSTextureCache* m_tc;

	virtual void DrawPrims(GSTexture* rt, GSTexture* ds, GSTextureCache::Source* tex) = 0;

public:
	GSRendererHW(GSVertexTrace* vt, size_t vertex_stride, GSTextureCache* tc);
	virtual ~GSRendererHW();

	void SetGameCRC(uint32 crc, int options);
	bool CanUpscale();
	int GetUpscaleMultiplier();

	void Reset();
	void VSync(int field);
	void ResetDevice();
	GSTexture* GetOutput(int i);
	void InvalidateVideoMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r);
	void InvalidateLocalMem(const GIFRegBITBLTBUF& BITBLTBUF, const GSVector4i& r, bool clut = false);
	void Draw();
};
