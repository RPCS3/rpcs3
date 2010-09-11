/*  ZZ Open GL graphics plugin
 *  Copyright (c)2010 gregory.hainaut@gmail.com, zeydlitz@gmail.com
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

#include "ZZoglFlushHack.h"

inline bool GABEST_HAS_SHARED_BITS (int fbp, int fpsm, int tbp, int tpsm)
{
    if ( !PSMT_HAS_SHARED_BITS (fpsm, tpsm) )
        return ((fbp ^ tbp) == 0);
    else
        return false;
}


// GSC_... function has been imported from GSdx
void GSC_Okami(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT32)
			skip = 1000;
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSMT4)
			skip = 0;
	}
}

void GSC_MetalGearSolid3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSMCT24)
			skip = 1000; // 76, 79
		else if(fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSMCT24 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01000) && fi.TPSM == PSMCT32)
			skip = 1000; // 69
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01000) && fi.FPSM == PSMCT32)
			skip = 0;
	}
}

void GSC_DBZBT2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && /*fi.FBP == 0x00000 && fi.FPSM == PSMCT16 &&*/ fi.TBP0 == 0x02000 && fi.TPSM == PSMT16Z)
			skip = 27;
		else if(!fi.TME && fi.FBP == 0x03000 && fi.FPSM == PSMCT16)
			skip = 10;
	}
}

void GSC_DBZBT3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00) && fi.TPSM == PSMT8H)
			skip = 24; // blur
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && fi.FPSM == PSMCT32 && fi.TPSM == PSMT8H)
			skip = 28; // outline
	}
}

void GSC_SFEX3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00500 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x00f00 && fi.TPSM == PSMCT16)
			skip = 2; // blur
	}
}

void GSC_Bully(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
        // Test is useless !
		// if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.FPSM == fi.TPSM)
		// 	return; // allowed
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSMCT16S && fi.TBP0 == 0x02300 && fi.TPSM == PSMT16SZ)
			skip = 6;
	}
	else
	{
		if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && fi.FPSM == PSMCT32)
			skip = 0;
	}
}

void GSC_BullyCC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
        // Test is useless !
		// if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01180) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01180) && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.FPSM == fi.TPSM)
		// 	return; // allowed

		if(!fi.TME && fi.FBP == 0x02800 && fi.FPSM == PSMCT24)
			skip = 9;
	}
}

void GSC_SoTC(const GSFrameInfo& fi, int& skip)
{
	// Not needed anymore? What did it fix anyway? (rama)
	/*if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02b80 && fi.FPSM == PSMCT24 && fi.TBP0 == 0x01e80 && fi.TPSM == PSMCT24)
			skip = 9;
		else if(fi.TME && fi.FBP == 0x01c00 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x03800 && fi.TPSM == PSMCT32)
			skip = 8;
		else if(fi.TME && fi.FBP == 0x01e80 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x03880 && fi.TPSM == PSMCT32)
			skip = 8;
	}*/
}

void GSC_OnePieceGrandAdventure(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x00f00) && fi.TPSM == PSMCT16)
			skip = 4;
	}
}

void GSC_OnePieceGrandBattle(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02d00 && fi.FPSM == PSMCT16 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00f00) && fi.TPSM == PSMCT16)
			skip = 4;
	}
}

void GSC_ICO(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x03d00 && fi.TPSM == PSMCT32)
			skip = 3;
		else if(fi.TME && fi.FBP == 0x00800 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x02800 && fi.TPSM == PSMT8H)
			skip = 1;
	}
	else
	{
		if(fi.TME && fi.TBP0 == 0x00800 && fi.TPSM == PSMCT32)
			skip = 0;
	}
}

void GSC_GT4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x03440 || fi.FBP >= 0x03e00) && fi.FPSM == PSMCT32 && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x01400) && fi.TPSM == PSMT8)
			skip = 880;
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x01400) && fi.FPSM == PSMCT24 && fi.TBP0 >= 0x03420 && fi.TPSM == PSMT8)
		{
			// TODO: removes gfx from where it is not supposed to (garage)
			// skip = 58;
		}
	}
}

void GSC_WildArms4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSMT32Z && fi.TBP0 == 0x01c00 && fi.TPSM == PSMT32Z)
			skip = 100;
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSMCT32)
			skip = 1;
	}
}

void GSC_WildArms5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSMT32Z && fi.TBP0 == 0x01c00 && fi.TPSM == PSMT32Z)
			skip = 100;
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00e00 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x02a00 && fi.TPSM == PSMCT32)
			skip = 1;
	}
}

void GSC_Manhunt2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03c20 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x01400 && fi.TPSM == PSMT8)
			skip = 640;
	}
}

void GSC_CrashBandicootWoC(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
        // Test is useless !
		// if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && (fi.TBP0 == 0x00000 || fi.TBP0 == 0x00a00) && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.FPSM == fi.TPSM)
		// 	return false; // allowed

		if(fi.TME && fi.FBP == 0x02200 && fi.FPSM == PSMT24Z && fi.TBP0 == 0x01400 && fi.TPSM == PSMT24Z)
			skip = 41;
	}
	else
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00) && fi.FPSM == PSMCT32 && fi.TBP0 == 0x03c00 && fi.TPSM == PSMCT32)
			skip = 0;
		else if(!fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00a00))
			skip = 0;
	}
}

void GSC_ResidentEvil4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x03100 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x01c00 && fi.TPSM == PSMT24Z)
			skip = 176;
	}
}

void GSC_Spartan(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02000 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT32)
			skip = 107;
	}
}

void GSC_AceCombat4(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02a00 && fi.FPSM == PSMT24Z && fi.TBP0 == 0x01600 && fi.TPSM == PSMT24Z)
			skip = 71; // clouds (z, 16-bit)
		else if(fi.TME && fi.FBP == 0x02900 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT24)
			skip = 28; // blur
	}
}

void GSC_Drakengard2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x026c0 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00a00 && fi.TPSM == PSMCT32)
			skip = 64;
	}
}

void GSC_Tekken5(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02ea0 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT32)
			skip = 95;
	}
}

void GSC_IkkiTousen(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00a80 && fi.FPSM == PSMT24Z && fi.TBP0 == 0x01180 && fi.TPSM == PSMT24Z)
			skip = 1000; // shadow (result is broken without depth copy, also includes 16 bit)
		else if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSMT24Z && fi.TBP0 == 0x01180 && fi.TPSM == PSMT24Z)
			skip = 11; // blur
	}
	else if(skip > 7)
	{
		if(fi.TME && fi.FBP == 0x00700 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x00700 && fi.TPSM == PSMCT16)
			skip = 7; // the last steps of shadow drawing
	}
}

void GSC_GodOfWar(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT16)
		{
			// skip = 30; //GSdx
			skip = 4; // 23 or 4 need more testing
		}
		else if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSMCT32 && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT32 && fi.FBMSK == 0xff000000)
			skip = 1; // blur
        else if(fi.FBP == 0x00000 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT8
                && ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
			skip = 1; // wall of fog
	}
}

void GSC_GodOfWar2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME)
		{
            if((fi.FBP == 0x00100 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x00100 && fi.TPSM == PSMCT16) // ntsc
                    || (fi.FBP == 0x02100 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x02100 && fi.TPSM == PSMCT16)) // pal
                skip = 29; // shadows
            if(fi.FBP == 0x00100 && fi.FPSM == PSMCT32 && (fi.TBP0 & 0x03000) == 0x03000
                    && (fi.TPSM == PSMT8 || fi.TPSM == PSMT4)
                    && ((fi.TZTST == 2 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 1 && fi.FBMSK == 0x00FFFFFF) || (fi.TZTST == 3 && fi.FBMSK == 0xFF000000)))
                skip = 1; // wall of fog
        }
	}
}

void GSC_GiTS(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01400 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x02e40 && fi.TPSM == PSMCT16)
			skip = 1315;
	}
}

void GSC_Onimusha3(const GSFrameInfo& fi, int& skip)
{
	if(fi.TME /*&& (fi.FBP == 0x00000 || fi.FBP == 0x00700)*/ && (fi.TBP0 == 0x01180 || fi.TBP0 == 0x00e00 || fi.TBP0 == 0x01000 || fi.TBP0 == 0x01200) && (fi.TPSM == PSMCT32 || fi.TPSM == PSMCT24))
		skip = 1;
}

void GSC_TalesOfAbyss(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && fi.TBP0 == 0x01c00 && fi.TPSM == PSMT8) // copies the z buffer to the alpha channel of the fb
			skip = 1000;
		else if(fi.TME && (fi.FBP == 0x00000 || fi.FBP == 0x00e00) && (fi.TBP0 == 0x03560 || fi.TBP0 == 0x038e0) && fi.TPSM == PSMCT32)
			skip = 1;
	}
	else
	{
		if(fi.TME && fi.TPSM != PSMT8)
			skip = 0;
	}
}

void GSC_SonicUnleashed(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x02200 && fi.FPSM == PSMCT16S && fi.TBP0 == 0x00000 && fi.TPSM == PSMCT16)
			skip = 1000; // shadow
	}
	else
	{
		if(fi.TME && fi.FBP == 0x00000 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x02200 && fi.TPSM == PSMCT16S)
			skip = 2;
	}
}

void GSC_Genji(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == 0x01500 && fi.FPSM == PSMCT16 && fi.TBP0 == 0x00e00 && fi.TPSM == PSMT16Z)
			skip = 6; //
	}
}

void GSC_StarOcean3(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH)
			skip = 1000; //
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH))
			skip = 0;
	}
}

void GSC_ValkyrieProfile2(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH)
			skip = 1000; //
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH))
			skip = 0;
	}
}

void GSC_RadiataStories(const GSFrameInfo& fi, int& skip)
{
	if(skip == 0)
	{
		if(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH)
			skip = 1000; // Shadows
		else if (fi.TME && fi.FBP == fi.TBP0 && (fi.TBP0 == 0x3700 || fi.TBP0 == 0x3400) && fi.TZTST == 1) 
			skip = 1; // Start manu issue;
	}
	else
	{
		if(!(fi.TME && fi.FBP == fi.TBP0 && fi.FPSM == PSMCT32 && fi.TPSM == PSMT4HH))
			skip = 0;
	}
}

bool GSC_HauntingGround(const GSFrameInfo& fi, int& skip)
{
    // Note GSdx seems to use invert somewhere FBMSK. So values were inverted
	if(skip == 0)
    {
        if(fi.TME && fi.FPSM == fi.TPSM && fi.TPSM == PSMCT16S && fi.FBMSK == ~(0x03FFF))
            skip = 1;
        else if(fi.TME && fi.FBP == 0x3000 && fi.TBP0 == 0x3380)
            skip = 1; // bloom
        else if(fi.TME && fi.FBP == fi.TBP0 && fi.TBP0 == 0x3000 && fi.FBMSK == ~(0xFFFFFF) &&
                GABEST_HAS_SHARED_BITS(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM))
            skip = 1; 
    }

	return true;
}

// Record skipped frame to allow better analysis
// #define FRAME_RECORDING_ON 1
#ifdef FRAME_RECORDING_ON
static const u32 MAX_FRAMES = 500;
static GSFrameInfo FrameAppear[MAX_FRAMES];
static u32 Rec_Numbers = 0;

void RecordNewFrames(ZeroGS::VB& curvb, GSFrameInfo fi) {
    if (Rec_Numbers >= MAX_FRAMES)
        return;

    u32 i;
    bool was_recorded = false;
    for (i = 0; i < Rec_Numbers; i++ ) {
        if (FrameAppear[i].FBP == fi.FBP && FrameAppear[i].FPSM == fi.FPSM
                && FrameAppear[i].TBP0 == fi.TBP0 && FrameAppear[i].TPSM == fi.TPSM) {
            was_recorded = true;
            break;
        }
    }
    if (!was_recorded) {
        FrameAppear[Rec_Numbers] = fi;
        Rec_Numbers++;
        ZZLog::Print( "New frame %d, skip %d | fpb: %x fpsm: %d fpmsk: %x tme: %x tbp0: %x tpsm: %d tztst: %x | bits %d\n", \
                Rec_Numbers, g_SkipFlushFrame, fi.FBP, fi.FPSM, fi.FBMSK, fi.TME, fi.TBP0, fi.TPSM, fi.TZTST, GABEST_HAS_SHARED_BITS(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM) );

        // Dump a nice picture of the frame
        char filename[255];
        sprintf(filename, "SkipFlushFrame_%d__%d.tga", g_SkipFlushFrame, Rec_Numbers);
        ZeroGS::SaveRenderTarget(filename, curvb.prndr->fbw, curvb.prndr->fbh, 0);
    }
}
#endif

__forceinline bool IsBadFrame(ZeroGS::VB& curvb)
{
	GSFrameInfo fi;

    // Keep GSdx naming convention to ease sharing code
	fi.FBP = curvb.frame.fbp;
	fi.FPSM = curvb.frame.psm;
	fi.FBMSK = ~curvb.frame.fbm;
	fi.TME = curvb.curprim.tme;
	fi.TBP0 = curvb.tex0.tbp0;
	fi.TPSM = curvb.tex0.psm;
	fi.TZTST = curvb.test.ztst;

	if (GetSkipCount_Handler && conf.settings().automatic_skip_draw)
        GetSkipCount_Handler(fi, g_SkipFlushFrame);

    if(g_SkipFlushFrame == 0 && (conf.SkipDraw > 0))
    {
        if(fi.TME)
        {
            // depth textures (bully, mgs3s1 intro, Front Mission 5)
            // Or General, often problematic post processing
            if (PSMT_ISZTEX(fi.TPSM) || (GABEST_HAS_SHARED_BITS(fi.FBP, fi.FPSM, fi.TBP0, fi.TPSM)))
                g_SkipFlushFrame = conf.SkipDraw;
        }
	}

	if(g_SkipFlushFrame > 0)
	{
#ifdef FRAME_RECORDING_ON
        RecordNewFrames(curvb, fi);
#endif
		g_SkipFlushFrame--;
		return true;
	}

	return false;
}
