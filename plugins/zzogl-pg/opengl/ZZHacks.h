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

#ifndef ZZHACKS_H_INCLUDED
#define ZZHACKS_H_INCLUDED

#include "PS2Edefs.h"

// This is a list of the various hacks, and what bit controls them. 
// Changing these is not advised unless you know what you are doing.
enum GAME_HACK_OPTIONS
{
	GAME_TEXTURETARGS		=	0x00000001,
	GAME_AUTORESET			=	0x00000002,
	GAME_INTERLACE2X		=	0x00000004,
	GAME_TEXAHACK			=	0x00000008, // apply texa to non textured polys
	GAME_NOTARGETRESOLVE	=	0x00000010,
	GAME_EXACTCOLOR			=	0x00000020,
	GAME_NOCOLORCLAMP		=	0x00000040,
	GAME_FFXHACK			=	0x00000080,
	GAME_NOALPHAFAIL		=	0x00000100,
	GAME_NODEPTHUPDATE		=	0x00000200,
	GAME_QUICKRESOLVE1		=	0x00000400,
	GAME_NOQUICKRESOLVE		=	0x00000800,
	GAME_NOTARGETCLUT		=	0x00001000, // full 16 bit resolution
	GAME_NOSTENCIL			=	0x00002000,
	GAME_VSSHACKOFF			=	0x00004000, // vertical stripe syndrome
	GAME_NODEPTHRESOLVE		=	0x00008000,
	GAME_FULL16BITRES		=	0x00010000,
	GAME_RESOLVEPROMOTED	=	0x00020000,
	GAME_FASTUPDATE			=	0x00040000,
	GAME_NOALPHATEST		=	0x00080000,
	GAME_DISABLEMRTDEPTH	=	0x00100000,
	GAME_32BITTARGS			=	0x00200000,
	GAME_PATH3HACK			=	0x00400000,
	GAME_DOPARALLELCTX		=	0x00800000, // tries to parallelize both contexts so that render calls are reduced (xenosaga)
	// makes the game faster, but can be buggy
	GAME_XENOSPECHACK		=	0x01000000, // xenosaga specularity hack (ignore any zmask=1 draws)
	GAME_PARTIALPOINTERS	=	0x02000000, // whenver the texture or render target are small, tries to look for bigger ones to read from
	GAME_PARTIALDEPTH		=	0x04000000, // tries to save depth targets as much as possible across height changes
	GAME_REGETHACK			=	0x08000000, // some sort of weirdness in ReGet() code
	GAME_GUSTHACK			=	0x10000000, // Needed for Gustgames fast update.
	GAME_NOLOGZ				=	0x20000000, // Intended for linux -- not logarithmic Z.
	GAME_AUTOSKIPDRAW		=	0x40000000, // Remove blur effect on some games
	GAME_RESERVED_HACK		=	0x80000000
};
	
#define USEALPHATESTING (!(conf.settings().no_alpha_test))

typedef union 
{
	struct
	{
		u32 texture_targs : 1;
		u32 auto_reset : 1;
		u32 interlace_2x : 1;
		u32 texa : 1; // apply texa to non textured polys
		u32 no_target_resolve : 1;
		u32 exact_color : 1;
		u32 no_color_clamp : 1;
		u32 ffx : 1;
		u32 no_alpha_fail : 1;
		u32 no_depth_update : 1;
		u32 quick_resolve_1 : 1;
		u32 no_quick_resolve : 1;
		u32 no_target_clut : 1; // full 16 bit resolution
		u32 no_stencil : 1;
		u32 vss_hack_off : 1; // vertical stripe syndrome
		u32 no_depth_resolve : 1;
		u32 full_16_bit_res : 1;
		u32 resolve_promoted : 1;
		u32 fast_update : 1;
		u32 no_alpha_test : 1;
		u32 disable_mrt_depth : 1;
		u32 args_32_bit : 1;
		u32 path3 : 1;
		u32 parallel_context : 1; // tries to parallelize both contexts so that render calls are reduced (xenosaga)
									// makes the game faster, but can be buggy
		u32 xenosaga_spec : 1; // xenosaga specularity hack (ignore any zmask=1 draws)
		u32 partial_pointers : 1; // whenver the texture or render target are small, tries to look for bigger ones to read from
		u32 partial_depth : 1; // tries to save depth targets as much as possible across height changes
		u32 reget : 1; // some sort of weirdness in ReGet() code
		u32 gust : 1; // Needed for Gustgames fast update.
		u32 no_logz : 1; // Intended for linux -- not logarithmic Z.
		u32 automatic_skip_draw :1; // allow debug of the automatic skip draw option
		u32 reserved2 :1;
	};
	u32 _u32;
} gameHacks;

#define HACK_NUMBER 25
extern u32 hackList[HACK_NUMBER];
extern char hackDesc[32][64];
extern int CurrentHack;

extern void ReportHacks(gameHacks hacks);
extern void ListHacks();

extern void DisplayHack(int hack);
extern void ChangeCurrentHack(int hack);

#endif // ZZHACKS_H_INCLUDED
