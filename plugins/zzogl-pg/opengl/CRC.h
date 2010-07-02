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
	GAME_NOLOGZ				=	0x20000000 // Intended for linux -- not logarithmic Z.
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
	BioHazard4
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

	//{0xC164550A, WildArms5, JPUNDUB, 0, -1, -1},
	//{0xC1640D2C, WildArms5, US, 0, -1, -1},
	//{0x0FCF8FE4, WildArms5, EU, 0, -1, -1},
	//{0x2294D322, WildArms5, JP, 0, -1, -1},
	//{0x565B6170, WildArms5, JP, 0, -1, -1},
	//{0xD7273511, SMTDDS1, US, 0, -1, -1},          // SMT Digital Devil Saga
	//{0x1683A6BE, SMTDDS1, EU, 0, -1, -1},          // SMT Digital Devil Saga
	//{0x44865CE1, SMTDDS1, JP, 0, -1, -1},          // SMT Digital Devil Saga
	//{0xD382C164, SMTDDS2, US, 0, -1, -1},          // SMT Digital Devil Saga 2
	//{0xE47C1A9C, SMTDDS2, JP, 0, -1, -1},          // SMT Digital Devil Saga 2
	
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
