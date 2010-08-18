/*  ZZ Open GL graphics plugin
 *  Copyright (c)2010 gregory.hainaut@gmail.com
 *  Based on GSdx Copyright (C) 2007-2009 Gabest
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

/* This file is a collection of hack for removing the blur effect on some games
 * The blur renders very badly on high screen flat panel.
 *
 * To avoid severals combo-box, the hack detects the game based on crc
 */

#ifndef ZZOGL_FLUSH_HACK_H_INCLUDED
#define ZZOGL_FLUSH_HACK_H_INCLUDED

#include "GS.h"
#include "zerogs.h"

extern int g_SkipFlushFrame;

struct GSFrameInfo
{
	u32  FBP;
	u32  FPSM;
	u32  FBMSK;
	u32  TBP0;
	u32  TPSM;
	u32  TZTST;
	bool TME;
};

typedef bool (*GetSkipCount)(const GSFrameInfo& fi, int& skip);

extern GetSkipCount GetSkipCount_Handler;

bool GSC_Null(const GSFrameInfo& fi, int& skip);
bool GSC_Okami(const GSFrameInfo& fi, int& skip);
bool GSC_MetalGearSolid3(const GSFrameInfo& fi, int& skip);
bool GSC_DBZBT2(const GSFrameInfo& fi, int& skip);
bool GSC_DBZBT3(const GSFrameInfo& fi, int& skip);
bool GSC_SFEX3(const GSFrameInfo& fi, int& skip);
bool GSC_Bully(const GSFrameInfo& fi, int& skip);
bool GSC_BullyCC(const GSFrameInfo& fi, int& skip);
bool GSC_SoTC(const GSFrameInfo& fi, int& skip);
bool GSC_OnePieceGrandAdventure(const GSFrameInfo& fi, int& skip);
bool GSC_OnePieceGrandBattle(const GSFrameInfo& fi, int& skip);
bool GSC_ICO(const GSFrameInfo& fi, int& skip);
bool GSC_GT4(const GSFrameInfo& fi, int& skip);
bool GSC_WildArms4(const GSFrameInfo& fi, int& skip);
bool GSC_WildArms5(const GSFrameInfo& fi, int& skip);
bool GSC_Manhunt2(const GSFrameInfo& fi, int& skip);
bool GSC_CrashBandicootWoC(const GSFrameInfo& fi, int& skip);
bool GSC_ResidentEvil4(const GSFrameInfo& fi, int& skip);
bool GSC_Spartan(const GSFrameInfo& fi, int& skip);
bool GSC_AceCombat4(const GSFrameInfo& fi, int& skip);
bool GSC_Drakengard2(const GSFrameInfo& fi, int& skip);
bool GSC_Tekken5(const GSFrameInfo& fi, int& skip);
bool GSC_IkkiTousen(const GSFrameInfo& fi, int& skip);
bool GSC_GodOfWar(const GSFrameInfo& fi, int& skip);
bool GSC_GodOfWar2(const GSFrameInfo& fi, int& skip);
bool GSC_GiTS(const GSFrameInfo& fi, int& skip);
bool GSC_Onimusha3(const GSFrameInfo& fi, int& skip);
bool GSC_TalesOfAbyss(const GSFrameInfo& fi, int& skip);
bool GSC_SonicUnleashed(const GSFrameInfo& fi, int& skip);
bool GSC_Genji(const GSFrameInfo& fi, int& skip);
bool GSC_StarOcean3(const GSFrameInfo& fi, int& skip);
bool GSC_ValkyrieProfile2(const GSFrameInfo& fi, int& skip);
bool GSC_RadiataStories(const GSFrameInfo& fi, int& skip);

bool IsBadFrame(ZeroGS::VB& curvb);
#endif
