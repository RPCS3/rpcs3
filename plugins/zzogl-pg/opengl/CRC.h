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

#ifndef CRC_H_INCLUDED
#define CRC_H_INCLUDED

// don't change these values!
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
	GAME_AUTOSKIPDRAW		=	0x40000000 // Remove blur effect on some games
};

#define USEALPHATESTING (!(conf.settings().no_alpha_test))

// CRC Information
enum Title_Info
{
	Unknown_Title,
	MetalSlug6,
	TomoyoAfter,
	Clannad,
	Lamune,
	KyuuketsuKitanMoonties,
	PiaCarroteYoukosoGPGakuenPrincess,
	KazokuKeikakuKokoroNoKizuna,
	DuelSaviorDestiny,
	FFX,
	FFX2,
	FFXII,
	ShadowHearts,
	Okami,
	MetalGearSolid3,
	DBZBT2,
	DBZBT3,
	SFEX3,
	Bully,
	BullyCC,
	SoTC,
	OnePieceGrandAdventure,
	OnePieceGrandBattle,
	ICO,
	GT4,
	WildArms4,
	WildArms5,
	Manhunt2,
	CrashBandicootWoC,
	ResidentEvil4,
	Spartan,
	AceCombat4,
	Drakengard2,
	Tekken5,
	IkkiTousen,
	GodOfWar,
	GodOfWar2,
	JackieChanAdv,
	HarvestMoon,
	NamcoXCapcom,
	GiTS,
	Onimusha3,
	MajokkoALaMode2,
	TalesOfAbyss,
	SonicUnleashed,
	SimpsonsGame,
	Genji,
	StarOcean3,
	ValkyrieProfile2,
	RadiataStories,
	SMTNocturne,
	SMTDDS1,
	SMTDDS2,
	RozenMaidenGebetGarden,
	Xenosaga,
	Espgaluda,
	OkageShadowKing,
	ShadowTheHedgehog,
	AtelierIris1,
	AtelierIris2,
	AtelierIris3,
	AtelierJudie,
	AtelierLilie,
	AtelierViorate,
	ArTonelico1,
	ArTonelico2,
	ManaKhemia1,
	ManaKhemia2,
	DarkCloud1,
	DarkCloud2,
	GhostInTheShell,
	TitleCount,
	Disgaea,
	Disgaea2,
	Gradius,
	KingdomHearts,
	KingdomHeartsFM,
	KingdomHearts2,
	KingdomHearts2FM,
	KingdomHeartsCOM,
	Tekken4,
	Kaena,
	Sims_The_Urbz,
	MarvelxCapcom2,
	NeoPets_Darkest_Faerie,
	CrashnBurn,
	Xenosaga2,
	HauntingGround,
	NightmareBeforeChristmas,
	PowershotPinball,
	BioHazard4,
	NUMBER_OF_TITLES
};

enum Region_Info
{
	Unknown_Region,
	US,
	EU,
	JP,
	JPUNDUB,
	RU,
	FR,
	DE,
	IT,
	ES,
	ASIA,
	RegionCount,
};

struct Game_Info
{
	u32 crc;
	Title_Info title;
	Region_Info region;
	u32 flags;
	s32 v_thresh, t_thresh;
};

static const Game_Info crc_game_list[] =
{
	// This section is straight from GSdx. Ones that also have ZZOgl hacks are commented out.
	
	{0x00000000, Unknown_Title, Unknown_Region, 0, -1, -1},
	{0x2113EA2E, MetalSlug6, Unknown_Region, 0, -1, -1},
	{0x42E05BAF, TomoyoAfter, JP, 0, -1, -1},
	{0x7800DC84, Clannad, JP, 0, -1, -1},
	{0xA6167B59, Lamune, JP, 0, -1, -1},
	{0xDDB59F46, KyuuketsuKitanMoonties, JP, 0, -1, -1},
	{0xC8EE2562, PiaCarroteYoukosoGPGakuenPrincess, JP, 0, -1, -1},
	{0x6CF94A43, KazokuKeikakuKokoroNoKizuna, JP, 0, -1, -1},
	{0xEDAF602D, DuelSaviorDestiny, JP, 0, -1, -1},
	{0xa39517ab, FFX, EU, 0, -1, -1},
	{0xa39517ae, FFX, FR, 0, -1, -1},
	{0x941bb7d9, FFX, DE, 0, -1, -1},
	{0xa39517a9, FFX, IT, 0, -1, -1},
	{0x941bb7de, FFX, ES, 0, -1, -1},
	{0xb4414ea1, FFX, RU, 0, -1, -1},
	{0xee97db5b, FFX, RU, 0, -1, -1},
	{0xaec495cc, FFX, RU, 0, -1, -1},
	{0xbb3d833a, FFX, US, 0, -1, -1},
	{0x6a4efe60, FFX, JP, 0, -1, -1},
	{0x3866ca7e, FFX, ASIA, 0, -1, -1}, // int.
	{0x658597e2, FFX, JP, 0, -1, -1}, // int.
	{0x9aac5309, FFX2, EU, 0, -1, -1},
	{0x9aac530c, FFX2, FR, 0, -1, -1},
	{0x9aac530a, FFX2, FR, 0, -1, -1}, // ?
	{0x9aac530d, FFX2, DE, 0, -1, -1},
	{0x9aac530b, FFX2, IT, 0, -1, -1},
	{0x48fe0c71, FFX2, US, 0, -1, -1},
	{0xe1fd9a2d, FFX2, JP, 0, -1, -1}, // int.
	{0x78da0252, FFXII, EU, 0, -1, -1},
	{0xc1274668, FFXII, EU, 0, -1, -1},
	{0xdc2a467e, FFXII, EU, 0, -1, -1},
	{0xca284668, FFXII, EU, 0, -1, -1},
	{0x280AD120, FFXII, JP, 0, -1, -1},
	{0x08C1ED4D, HauntingGround, Unknown_Region, 0, -1, -1},
	{0x2CD5794C, HauntingGround, EU, 0, -1, -1},
	{0x867BB945, HauntingGround, JP, 0, -1, -1},
	{0xE263BC4B, HauntingGround, JP, 0, -1, -1},
	{0x901AAC09, HauntingGround, US, 0, -1, -1},
	{0x8BE3D7B2, ShadowHearts, Unknown_Region, 0, -1, -1},
	{0xDEFA4763, ShadowHearts, US, 0, -1, -1},
	//{0x21068223, Okami, US, 0, -1, -1},
	//{0x891f223f, Okami, FR, 0, -1, -1},
	//{0xC5DEFEA0, Okami, JP, 0, -1, -1},
	{0x053D2239, MetalGearSolid3, US, 0, -1, -1},
	{0x086273D2, MetalGearSolid3, FR, 0, -1, -1},
	{0x26A6E286, MetalGearSolid3, EU, 0, -1, -1},
	{0xAA31B5BF, MetalGearSolid3, Unknown_Region, 0, -1, -1},
	{0x9F185CE1, MetalGearSolid3, Unknown_Region, 0, -1, -1},
	{0x98D4BC93, MetalGearSolid3, EU, 0, -1, -1},
	{0x86BC3040, MetalGearSolid3, US, 0, -1, -1}, //Subsistance disc 1
	{0x0481AD8A, MetalGearSolid3, JP, 0, -1, -1},
	{0x79ED26AD, MetalGearSolid3, EU, 0, -1, -1},
	{0x5E31EA42, MetalGearSolid3, EU, 0, -1, -1},
	{0xD7ED797D, MetalGearSolid3, EU, 0, -1, -1},
	{0x278722BF, DBZBT2, US, 0, -1, -1},
	{0xFE961D28, DBZBT2, US, 0, -1, -1},
	{0x0393B6BE, DBZBT2, EU, 0, -1, -1},
	{0xE2F289ED, DBZBT2, JP, 0, -1, -1},  // Sparking Neo!
	{0x35AA84D1, DBZBT2, Unknown_Region, 0, -1, -1},
	{0x428113C2, DBZBT3, US, 0, -1, -1},
	{0xA422BB13, DBZBT3, EU, 0, -1, -1},
	{0x983C53D2, DBZBT3, Unknown_Region, 0, -1, -1},
	{0x983C53D3, DBZBT3, Unknown_Region, 0, -1, -1},
	{0x72B3802A, SFEX3, US, 0, -1, -1},
	{0x71521863, SFEX3, US, 0, -1, -1},
	{0x28703748, Bully, US, 0, -1, -1},
	{0xC78A495D, BullyCC, US, 0, -1, -1},
	{0xC19A374E, SoTC, US, 0, -1, -1},
	{0x7D8F539A, SoTC, EU, 0, -1, -1},
	{0x3122B508, OnePieceGrandAdventure, US, 0, -1, -1},
	{0x8DF14A24, OnePieceGrandAdventure, EU, 0, -1, -1},
	{0xB049DD5E, OnePieceGrandBattle, US, 0, -1, -1},
	{0x5D02CC5B, OnePieceGrandBattle, Unknown_Region, 0, -1, -1},
	{0x6F8545DB, ICO, US, 0, -1, -1},
	{0xB01A4C95, ICO, JP, 0, -1, -1},
	{0x5C991F4E, ICO, Unknown_Region, 0, -1, -1},
    // FIXME multiple CRC
	{0x7ACF7E03, ICO, Unknown_Region, 0, -1, -1},
	{0xAEAD1CA3, GT4, JP, 0, -1, -1},
	{0x44A61C8F, GT4, Unknown_Region, 0, -1, -1},
	{0x0086E35B, GT4, Unknown_Region, 0, -1, -1},
	{0x77E61C8A, GT4, Unknown_Region, 0, -1, -1},
	{0xC164550A, WildArms5, JPUNDUB, 0, -1, -1},
	{0xC1640D2C, WildArms5, US, 0, -1, -1},
	{0x0FCF8FE4, WildArms5, EU, 0, -1, -1},
	{0x2294D322, WildArms5, JP, 0, -1, -1},
	{0x565B6170, WildArms5, JP, 0, -1, -1},
	{0xBBC3EFFA, WildArms4, US, 0, -1, -1},
	{0xBBC396EC, WildArms4, US, 0, -1, -1}, //hmm such a small diff in the CRC..
	{0x8B029334, Manhunt2, Unknown_Region, 0, -1, -1},
	{0x09F49E37, CrashBandicootWoC, Unknown_Region, 0, -1, -1},
	{0x013E349D, ResidentEvil4, US, 0, -1, -1},
	{0x6BA2F6B9, ResidentEvil4, Unknown_Region, 0, -1, -1},
	{0x60FA8C69, ResidentEvil4, JP, 0, -1, -1},
	{0x72E1E60E, Spartan, Unknown_Region, 0, -1, -1},
	{0x5ED8FB53, AceCombat4, JP, 0, -1, -1},
	{0x1B9B7563, AceCombat4, Unknown_Region, 0, -1, -1},
	{0xEC432B24, Drakengard2, Unknown_Region, 0, -1, -1},
	{0xFC46EA61, Tekken5, JP, 0, -1, -1},
	{0x1F88EE37, Tekken5, Unknown_Region, 0, -1, -1},
	{0x652050D2, Tekken5, Unknown_Region, 0, -1, -1},
	{0x9E98B8AE, IkkiTousen, JP, 0, -1, -1},
	//{0xD6385328, GodOfWar, US, 0, -1, -1},
	//{0xFB0E6D72, GodOfWar, EU, 0, -1, -1},
	//{0xEB001875, GodOfWar, EU, 0, -1, -1},
	//{0xA61A4C6D, GodOfWar, Unknown_Region, 0, -1, -1},
	//{0xE23D532B, GodOfWar, Unknown_Region, 0, -1, -1},
	//{0xDF1AF973, GodOfWar, Unknown_Region, 0, -1, -1},
	//{0xD6385328, GodOfWar, Unknown_Region, 0, -1, -1},
	{0x2F123FD8, GodOfWar2, RU, 0, -1, -1},
	{0x2F123FD8, GodOfWar2, US, 0, -1, -1},
	{0x44A8A22A, GodOfWar2, EU, 0, -1, -1},
	{0x4340C7C6, GodOfWar2, Unknown_Region, 0, -1, -1},
	{0xF8CD3DF6, GodOfWar2, Unknown_Region, 0, -1, -1},
	{0x0B82BFF7, GodOfWar2, Unknown_Region, 0, -1, -1},
	{0x5D482F18, JackieChanAdv, Unknown_Region, 0, -1, -1},
	//{0xf0a6d880, HarvestMoon, US, 0, -1, -1},
	{0x75c01a04, NamcoXCapcom, US, 0, -1, -1},
	{0xBF6F101F, GiTS, US, 0, -1, -1},
	{0xA5768F53, GiTS, JP, 0, -1, -1},
	{0x6BF11378, Onimusha3, US, 0, -1, -1},
	{0xF442260C, MajokkoALaMode2, JP, 0, -1, -1},
	{0x14FE77F7, TalesOfAbyss, US, 0, -1, -1},
	{0x045D77E9, TalesOfAbyss, JPUNDUB, 0, -1, -1},
	{0xAA5EC3A3, TalesOfAbyss, JP, 0, -1, -1},
	//{0xFB236A46, SonicUnleashed, US, 0, -1, -1},
	{0x4C7BB3C8, SimpsonsGame, Unknown_Region, 0, -1, -1},
	{0x4C94B32C, SimpsonsGame, Unknown_Region, 0, -1, -1},
	{0xD71B57F4, Genji, Unknown_Region, 0, -1, -1},
	{0x23A97857, StarOcean3, US, 0, -1, -1},
	{0xBEC32D49, StarOcean3, JP, 0, -1, -1},
	{0x8192A241, StarOcean3, JP, 0, -1, -1}, //NTSC JP special directors cut limited extra sugar on top edition (the special one :p)
	{0x23A97857, StarOcean3, JPUNDUB, 0, -1, -1},
	{0xCC96CE93, ValkyrieProfile2, US, 0, -1, -1},
	{0x774DE8E2, ValkyrieProfile2, JP, 0, -1, -1},
	{0x04CCB600, ValkyrieProfile2, EU, 0, -1, -1},
	{0xB65E141B, ValkyrieProfile2, EU, 0, -1, -1}, // PAL German
	{0x47B9B2FD, RadiataStories, US, 0, -1, -1},
	{0xE8FCF8EC, SMTNocturne, US, 0, -1, -1},	// GSdx saves/reloads z buffer around shadow drawing, same issue with all the SMT games following
	{0xF0A31EE3, SMTNocturne, EU, 0, -1, -1},	// SMTNocturne (Lucifers Call in EU)
	{0xAE0DE7B7, SMTNocturne, EU, 0, -1, -1},	// SMTNocturne (Lucifers Call in EU)
	{0xD60DA6D4, SMTNocturne, JP, 0, -1, -1},	// SMTNocturne
	{0x0e762e8d, SMTNocturne, JP, 0, -1, -1},	// SMTNocturne Maniacs
	{0x47BA9034, SMTNocturne, JP, 0, -1, -1},	// SMTNocturne Maniacs Chronicle
	{0xD7273511, SMTDDS1, US, 0, -1, -1},		// SMT Digital Devil Saga
	{0x1683A6BE, SMTDDS1, EU, 0, -1, -1},		// SMT Digital Devil Saga
	{0x44865CE1, SMTDDS1, JP, 0, -1, -1},		// SMT Digital Devil Saga
	{0xD382C164, SMTDDS2, US, 0, -1, -1},		// SMT Digital Devil Saga 2
	{0xD568B684, SMTDDS2, EU, 0, -1, -1},	// SMT Digital Devil Saga 2
	{0xE47C1A9C, SMTDDS2, JP, 0, -1, -1},		// SMT Digital Devil Saga 2
	{0x0B8AB37B, RozenMaidenGebetGarden, JP, 0, -1, -1},
	
	// And these are here for ZZogl hacks.
	{0xA3D63039, Xenosaga, JP, GAME_DOPARALLELCTX, 64, 32},
	{0x0E7807B2, Xenosaga, US, GAME_DOPARALLELCTX, 64, 32},
	{0x7D2FE035, Espgaluda, JP, 0/*GAME_BIGVALIDATE*/, 24, -1},
	{0x21068223, Okami, US, GAME_XENOSPECHACK, -1, -1},
	{0x891f223f, Okami, FR, GAME_XENOSPECHACK, -1, -1},
	{0xC5DEFEA0, Okami, JP, GAME_XENOSPECHACK, -1, -1},
	{0xe0426fc6, OkageShadowKing, Unknown_Region, GAME_XENOSPECHACK, -1, -1},

	{0xD6385328, GodOfWar, US, GAME_FULL16BITRES, -1, -1},
	{0xFB0E6D72, GodOfWar, EU, GAME_FULL16BITRES, -1, -1},
	{0xEB001875, GodOfWar, EU, GAME_FULL16BITRES, -1, -1},
	{0xA61A4C6D, GodOfWar, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	{0xE23D532B, GodOfWar, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	{0xDF1AF973, GodOfWar, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	{0xD6385328, GodOfWar, Unknown_Region, GAME_FULL16BITRES, -1, -1},

	//{0x2F123FD8, GodOfWar2, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	//{0x44A8A22A, GodOfWar2, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	//{0x4340C7C6, GodOfWar2, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	//{0xF8CD3DF6, GodOfWar2, Unknown_Region, GAME_FULL16BITRES, -1, -1},
	//{0x0B82BFF7, GodOfWar2, Unknown_Region, GAME_FULL16BITRES, -1, -1},

	{0xF0A6D880, HarvestMoon, US, GAME_NOSTENCIL, -1, -1},
	//{0x304C115C, HarvestMoon, Unknown, GAME_NOSTENCIL, -1, -1},
	{0xFB236A46, SonicUnleashed, US, GAME_FASTUPDATE | GAME_NOALPHAFAIL, -1, -1},
	{0xa5d29941, ShadowTheHedgehog, US, GAME_FASTUPDATE | GAME_NOALPHAFAIL, -1, -1},

	{0x7acf7e03, AtelierIris1, Unknown_Region, GAME_GUSTHACK, -1, -1},
	{0xF0457CEF, AtelierIris1, Unknown_Region, GAME_GUSTHACK, -1, -1},
	{0xE3981DBB, AtelierIris1, US, GAME_GUSTHACK, -1, -1},
	{0x9AC65D6A, AtelierIris2, US, GAME_GUSTHACK, -1, -1},
	{0x4CCC9212, AtelierIris3, US, GAME_GUSTHACK, -1, -1},
	{0xCA295E61, AtelierIris3, JP, GAME_GUSTHACK, -1, -1},
	//{0x4437F4B1, ArTonelico1, US, GAME_GUSTHACK, -1, -1},
	{0xF95F37EE, ArTonelico2, US, GAME_GUSTHACK, -1, -1},
	{0xF46142D3, ArTonelico2, JPUNDUB, GAME_GUSTHACK, -1, -1},
	{0x77b0236f, ManaKhemia1, US, GAME_GUSTHACK , -1, -1},
	{0x433951e7, ManaKhemia2, US, GAME_GUSTHACK, -1, -1},
	//{0xda11c6d4, AtelierJudie, JP, GAME_GUSTHACK, -1, -1},
	//{0x3e72c085, AtelierLilie, JP, GAME_GUSTHACK, -1, -1},
	//{0x6eac076b, AtelierViorate, JP, GAME_GUSTHACK, -1, -1},

	{0xbaa8dd8, DarkCloud1, US, GAME_NOTARGETRESOLVE, -1, -1},
	{0xA5C05C78, DarkCloud1, Unknown_Region, GAME_NOTARGETRESOLVE, -1, -1},
	//{0x1DF41F33, DarkCloud2, US, 0, -1, -1},
	{0x95cc86ef, GhostInTheShell, Unknown_Region, GAME_NOALPHAFAIL, -1, -1}
	
//	Game settings that used to be in the Patches folder. Commented out until I decide what to do with them.
//	{0x951555A0, Disgaea2, US, GAME_NODEPTHRESOLVE, -1, -1},
//	{0x4334E17D, Disgaea2, JP, GAME_NODEPTHRESOLVE, -1, -1},
//	
//	{0x5EB127E7, Gradius, JP, GAME_INTERLACE2X, -1, -1},
//	{0x6ADBC24B, Gradius, EU, GAME_INTERLACE2X, -1, -1},
//	{0xF22CDDAF, Gradius, US, GAME_INTERLACE2X, -1, -1},
//	
//	{0xF52FB2BE, KingdomHearts, EU, GAME_QUICKRESOLVE1, -1, -1},
//	{0xAE3EAA05, KingdomHearts, DE, GAME_QUICKRESOLVE1, -1, -1},
//	{0xF6DC728D, KingdomHearts, FR, GAME_QUICKRESOLVE1, -1, -1},
//	{0x0F6B6315, KingdomHearts, US, GAME_QUICKRESOLVE1, -1, -1},
//	{0x3E68955A, KingdomHeartsFM, JP, GAME_QUICKRESOLVE1, -1, -1},
//	
//	{0xC398F477, KingdomHearts2, EU, GAME_NODEPTHRESOLVE, -1, -1},
//	{0xDA0535FD, KingdomHearts2, US, GAME_NODEPTHRESOLVE, -1, -1},
//	{0x93F8A60B, KingdomHearts2, JP, GAME_NODEPTHRESOLVE, -1, -1},
//	{0xF266B00B, KingdomHearts2FM, JP, GAME_NODEPTHRESOLVE, -1, -1},
//	
//	//The patch claimed to stop characters appearing as wigs on GeForce 8x00 series cards (Disable Alpha Testing)
//	{0x2251E14D, Tekken4, EU, GAME_NOALPHATEST, -1, -1},
//	
//	// This one is supposed to fix a refresh bug.
//	{0x51F91783, Kaena, JP, GAME_NOTARGETRESOLVE, -1, -1},
//	
//	{0xDEFA4763, ShadowHearts, EU, 	GAME_NODEPTHRESOLVE | GAME_NOQUICKRESOLVE | GAME_NOTARGETRESOLVE | GAME_AUTORESET, -1, -1},
//	{0x8BE3D7B2, ShadowHearts, US, GAME_NODEPTHUPDATE | GAME_AUTORESET | GAME_NOQUICKRESOLVE, -1, -1},
//	
//	{0x015314A2, Sims_The_Urbz, US, GAME_NOQUICKRESOLVE, -1, -1},
//	
//	// "Required fixes to visuals"
//	{0x086273D2, MetalGearSolid3, US, GAME_FULL16BITRES | GAME_NODEPTHRESOLVE, -1, -1},
//	
//	{0x4D228733, MarvelxCapcom2, US, GAME_QUICKRESOLVE1, -1, -1},
//	{0x934F9081, NeoPets_Darkest_Faerie, US, GAME_RESOLVEPROMOTED | GAME_FULL16BITRES | GAME_NODEPTHRESOLVE, -1, -1},
//	
//	{0x21068223, Okami, US, GAME_FULL16BITRES|GAME_NODEPTHRESOLVE|GAME_FASTUPDATE, -1, -1},
//	{0xC5DEFEA0, Okami, JP, GAME_FULL16BITRES|GAME_NODEPTHRESOLVE|GAME_FASTUPDATE, -1, -1},
//	
//	// Speed up
//	{0x6BA2F6B9, ResidentEvil4, EU, GAME_NOTARGETRESOLVE | GAME_32BITTARGS, -1, -1},
//	{0x013E349D, ResidentEvil4, US, GAME_NOTARGETCLUT, -1, -1},
//	
//	{0x2088950A, Xenosaga2, JP, GAME_FULL16BITRES | GAME_NODEPTHRESOLVE, -1, -1},
//	{0x901AAC09, HauntingGround, US, GAME_FULL16BITRES | GAME_NODEPTHRESOLVE, -1, -1},
//	{0x625AF967, NightmareBeforeChristmas, JP, GAME_TEXAHACK, -1, -1},
//	
//	{0x3CFE3B37, PowershotPinball, EU, GAME_AUTORESET, -1, -1},
//	{0x60FA8C69, BioHazard4, JP, GAME_NOTARGETCLUT, -1, -1}
//  End of game settings from the patch folder.
};

#define GAME_INFO_INDEX (sizeof(crc_game_list)/sizeof(Game_Info))


#endif // CRC_H_INCLUDED
