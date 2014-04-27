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
 
 #include "Util.h"
 #include "ZZHacks.h"
 #include "ZZLog.h"
 
int CurrentHack = 0;

// A list of what bit controls each of the current hacks.
u32 hackList[HACK_NUMBER] = 
{
	0, // No hack
	1, //GAME_TEXTURETARGS,
	2, //GAME_AUTORESET,
	3, //GAME_INTERLACE2X,
	4, //GAME_TEXAHACK,
	5, //GAME_NOTARGETRESOLVE,
	6, //GAME_EXACTCOLOR,
	//7 //GAME_NOCOLORCLAMP,
	//8 //GAME_FFXHACK,
	9, //GAME_NOALPHAFAIL,
	10, //GAME_NODEPTHUPDATE,
	11, //GAME_QUICKRESOLVE1,
	12, //GAME_NOQUICKRESOLVE,
	13, //GAME_NOTARGETCLUT,
	14, //GAME_NOSTENCIL,
	15, //GAME_NODEPTHRESOLVE,
	16, //GAME_FULL16BITRES,
	17, //GAME_RESOLVEPROMOTED,
	18, //GAME_FASTUPDATE,
	19, //GAME_NOALPHATEST,
	20, //GAME_DISABLEMRTDEPTH,
	//21 //GAME_32BITTARGS,
	//22 //GAME_PATH3HACK,
	//23 //GAME_DOPARALLELCTX,
	24, //GAME_XENOSPECHACK,
	//25 //GAME_PARTIALPOINTERS,
	26, //GAME_PARTIALDEPTH,
	27, //GAME_REGETHACK,
	28, //GAME_GUSTHACK,
	29, //GAME_NOLOGZ,
	30, //GAME_AUTOSKIPDRAW
};


char hackDesc[32][64] = 
{
	"No hack",
	"Texture targs",
	"Auto reset",
	"Interlace 2x",
	"Texa",
	"No target resolve",
	"Exact color",
	"No color clamp",
	"Final Fantasy X",
	"No alpha fail",
	"No depth update",
	"Quick resolve 1",
	"No Quick resolve",
	"No target clut",
	"No stencil",
	"VSS",
	"No depth resolve",
	"Full 16 bit resolution",
	"Resolve promoted",
	"Fast update",
	"No alpha test",
	"Disable mrt depth",
	"Args 32 bit",
	"",
	"Parallel context",
	"Xenosaga spec",
	"Partial pointers",
	"Partial depth",
	"Reget",
	"Gust",
	"No logz",
	"Automatic skip draw"	
};

struct hacks
{
	bool enabled;
	char shortDesc[64];
	char longDesc[256];
};

hacks hack_list[32] = 
{
	{ true, "No hack", "No hack" },
	{ true, "Texture targs", "Tex Target checking - 00000001\nLego Racers" },
	{ true, "Auto reset", "Auto reset targs - 00000002\nUse when game is slow and toggling AA fixes it. Samurai Warriors. (Automatically on for Shadow Hearts)" },
	{ true, "Interlace 2x", "Interlace 2X - 00000004\nFixes 2x bigger screen. Gradius 3." },
	{ false, "Texa", "" },
	{ true, "No target resolve", "No target resolves - 00000010\nStops all resolving of targets.  Try this first for really slow games. (Automatically on for Dark Cloud 1.)" },
	{ true,  "Exact color", "Exact color testing - 00000020\nFixes overbright or shadow/black artifacts. Crash 'n Burn." },
	{ false, "No color clamp", "No color clamping - 00000040\nSpeeds up games, but might be too bright or too dim." },
	{ false, "Final Fantasy X", "" },
	{ false, "No alpha fail", "Alpha Fail hack - 00000100\nRemove vertical stripes or other coloring artifacts. Breaks Persona 4 and MGS3. (Automatically on for Sonic Unleashed, Shadow the Hedgehog, & Ghost in the Shell.)" },
	{ true, "No depth update", "Disable depth updates - 00000200" },
	{ true, "Quick resolve 1", "Resolve Hack #1 - 00000400\n Speeds some games. Kingdom Hearts."},
	{ true, "No Quick resolve", "Resolve Hack #2 - 00000800\nShadow Hearts, Urbz. Destroys FFX."},
	{ true, "No target clut", "No target CLUT - 00001000\nResident Evil 4, or foggy scenes." },
	{ true, "No stencil", "Disable stencil buffer - 00002000\nUsually safe to do for simple scenes. Harvest Moon." },
	{ false, "VSS", "" },
	{ true, "No depth resolve", "No depth resolve - 00008000\nMight give z buffer artifacts." },
	{ true, "Full 16 bit resolution", "Full 16 bit resolution - 00010000\nUse when half the screen is missing." },
	{ true, "Resolve promoted", "Resolve Hack #3 - 00020000\nNeopets" },
	{ true, "Fast update", "Fast Update - 00040000\n Speeds some games. Needed for Sonic Unleashed. Okami." },
	{ true, "No alpha test", "Disable alpha testing - 00080000" },
	{ true, "Disable mrt depth", "Enable Multiple RTs - 00100000" },
	{ false, "Args 32 bit", "" },
	{ false, "Path3", "" },
	{ false, "Parallel context", "" },
	{ true, "Xenosaga spec", "Specular Highlights - 01000000\nMakes graphics faster by removing highlights. (Automatically on for Xenosaga, Okami, & Okage.)" },
	{ false, "Partial pointers", "Partial targets - 02000000" },
	{ true, "Partial depth", "Partial depth - 04000000" },
	{ false, "Reget", "" },
	{ true, "Gust", "Gust fix - 10000000. Makes gust games cleaner and faster. (Automatically on for most Gust games)" },
	{ true, "No logz", "No logarithmic Z - 20000000. Could decrease number of Z-artifacts." },
	{ true, "Automatic skip draw", "Remove blur effect on some games\nSlow games." }
};

void ReportHacks(gameHacks hacks)
{
	for(int i = 0; i < 32; i++)
	{
		if (hacks._u32 & (1 << i)) 
		{
			ZZLog::WriteLn("'%s' hack enabled.", hackDesc[i+1]);
		}
	}
}

void ListHacks()
{
	if ((!conf.disableHacks) && (conf.def_hacks._u32 != 0))
	{
		ZZLog::WriteLn("Auto-enabling these hacks:");
		ReportHacks(conf.def_hacks);
	}
	
	if (conf.hacks._u32 != 0)
	{
		ZZLog::WriteLn("You've manually enabled these hacks:");
		ReportHacks(conf.hacks);
	}
}

void DisplayHack(int hack)
{
	ZZLog::WriteToScreen2("***%d %s", hack, hackDesc[hackList[hack]]);
}

void ChangeCurrentHack(int hack)
{
	FUNCLOG
	
	conf.hacks._u32 &= !(hackList[CurrentHack]);
	conf.hacks._u32 |= hackList[hack];
	
	DisplayHack(hack);
		
	CurrentHack = hack;
	SaveConfig();
	
}

