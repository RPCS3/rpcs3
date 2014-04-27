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
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GSdx.h"
#include "GSCrc.h"

CRC::Game CRC::m_games[] =
{
	{0x00000000, NoTitle, NoRegion, 0},
	{0x2113EA2E, MetalSlug6, NoRegion, 0},
	{0x42E05BAF, TomoyoAfter, JP, PointListPalette},
	{0x7800DC84, Clannad, JP, PointListPalette},
	{0xA6167B59, Lamune, JP, PointListPalette},
	{0xDDB59F46, KyuuketsuKitanMoonties, JP, PointListPalette},
	{0xC8EE2562, PiaCarroteYoukosoGPGakuenPrincess, JP, PointListPalette},
	{0x6CF94A43, KazokuKeikakuKokoroNoKizuna, JP, PointListPalette},
	{0xEDAF602D, DuelSaviorDestiny, JP, PointListPalette},
	{0xA39517AB, FFX, EU, 0},
	{0xA39517AE, FFX, FR, 0},
	{0x941BB7D9, FFX, DE, 0},
	{0xA39517A9, FFX, IT, 0},
	{0x941BB7DE, FFX, ES, 0},
	{0xA80F497C, FFX, ES, 0},
	{0xB4414EA1, FFX, RU, 0},
	{0xEE97DB5B, FFX, RU, 0},
	{0xAEC495CC, FFX, RU, 0},
	{0xBB3D833A, FFX, US, 0},
	{0x6A4EFE60, FFX, JP, 0},
	{0x3866CA7E, FFX, ASIA, 0}, // int.
	{0x658597E2, FFX, JP, 0}, // int.
	{0x9AAC5309, FFX2, EU, 0},
	{0x9AAC530C, FFX2, FR, 0},
	{0x9AAC530A, FFX2, ES, 0}, 
	{0x9AAC530D, FFX2, DE, 0},
	{0x9AAC530B, FFX2, IT, 0},
	{0x48FE0C71, FFX2, US, 0},
	{0x8A6D7F14, FFX2, JP, 0},
	{0xE1FD9A2D, FFX2, JP, 0}, // int.
	{0x11624CD6, FFX2, KO, 0},
	{0x78DA0252, FFXII, EU, 0},
	{0xC1274668, FFXII, EU, 0},
	{0xDC2A467E, FFXII, EU, 0},
	{0xCA284668, FFXII, EU, 0},
	{0xC52B466E, FFXII, EU, 0}, //ES
	{0xE5E71BF9, FFXII, FR, 0},
	{0x280AD120, FFXII, JP, 0},
	{0x08C1ED4D, HauntingGround, NoRegion, 0},
	{0x2CD5794C, HauntingGround, EU, 0},
	//duplicate crc with genji.. {0x7D4EA48F, HauntingGround, EU, 0},
	{0x867BB945, HauntingGround, JP, 0},
	{0xE263BC4B, HauntingGround, JP, 0},
	{0x901AAC09, HauntingGround, US, 0},
	{0x8BE3D7B2, ShadowHearts, NoRegion, 0},
	{0xDEFA4763, ShadowHearts, US, 0},
	{0xDDFB18B0, ShadowHearts, JP, 0},
	{0x21068223, Okami, US, 0},
	{0x891F223F, Okami, FR, 0},
	{0xC5DEFEA0, Okami, JP, 0},
	{0x086273D2, MetalGearSolid3, FR, 0},
	{0x26A6E286, MetalGearSolid3, EU, 0},
	{0x9F185CE1, MetalGearSolid3, EU, 0},
	{0x98D4BC93, MetalGearSolid3, EU, 0},
	{0x79ED26AD, MetalGearSolid3, EU, 0},
	{0x5E31EA42, MetalGearSolid3, EU, 0},
	{0xD7ED797D, MetalGearSolid3, EU, 0},
	{0x053D2239, MetalGearSolid3, US, 0},
	{0xAA31B5BF, MetalGearSolid3, US, 0},
	{0x86BC3040, MetalGearSolid3, US, 0}, //Subsistance disc 1
	{0x0481AD8A, MetalGearSolid3, JP, 0},
	{0xC69ACB6F, MetalGearSolid3, KO, 0}, //Metal Gear Solid 3 SnakeEater
	{0xB0D195EF, MetalGearSolid3, KO, 0}, //Metal Gear Solid 3 Substance disc1
	{0x3EBABC9C, MetalGearSolid3, KO, 0}, //Metal Gear Solid 3 Substance disc2
	{0x8A5C25A7, MetalGearSolid3, ES, 0}, //Metal Gear Solid 3 Subsistence Spanish version
	{0x278722BF, DBZBT2, US, 0},
	{0xFE961D28, DBZBT2, US, 0},
	{0x0393B6BE, DBZBT2, EU, 0},
	{0xE2F289ED, DBZBT2, JP, 0}, // Sparking Neo!
	{0xE29C09A3, DBZBT2, KO, 0}, //DragonBall Z Sparking Neo
	{0x0BAA4387, DBZBT2, JP, 0},
	{0x35AA84D1, DBZBT2, NoRegion, 0},
	{0x428113C2, DBZBT3, US, 0},
	{0xA422BB13, DBZBT3, EU, 0},
	{0xF28D21F1, DBZBT3, JP, 0},
	{0x983C53D2, DBZBT3, NoRegion, 0},
	{0x983C53D3, DBZBT3, NoRegion, 0},
	{0x9B0E119F, DBZBT3, KO, 0}, //DragonBall Z Sparking Meteo
	{0x72B3802A, SFEX3, US, 0},
	{0x71521863, SFEX3, US, 0},
	{0x28703748, Bully, US, 0},
	{0x019CFA48, Bully, JP, 0},
	{0xC78A495D, BullyCC, US, 0},
	{0xC19A374E, SoTC, US, 0},
	{0x7D8F539A, SoTC, EU, 0},
	{0x0F0C4A9C, SoTC, EU, 0},
	{0x877F3436, SoTC, JP, 0},
	{0xA17D6AAA, SoTC, KO, 0},
	{0x877B3D35, SoTC, CH, 0},
	{0x3122B508, OnePieceGrandAdventure, US, 0},
	{0x8DF14A24, OnePieceGrandAdventure, EU, 0},
	{0xE446C9F9, OnePieceGrandAdventure, KO, 0},
	{0xCA2073B3, OnePieceGrandBattle, KO, 0},
	{0x66953267, OnePieceGrandAdventure, JP, 0},
	{0x947B933B, OnePieceGrandAdventure, US, 0},
	{0xB049DD5E, OnePieceGrandBattle, US, 0},
	{0x5D02CC5B, OnePieceGrandBattle, NoRegion, 0},
	{0x6F8545DB, ICO, US, 0},
	{0xB01A4C95, ICO, JP, 0},
	{0x2DF2C1EA, ICO, KO, 0},
	{0x5C991F4E, ICO, NoRegion, 0},
	{0x7ACF7E03, ICO, NoRegion, 0},
	{0x788D8B4F, ICO, EU, 0},
	{0x29C28734, ICO, CH, 0},
	{0xAEAD1CA3, GT4, JP, 0},
	{0x30E41D93, GT4, KO, 0},
	{0x44A61C8F, GT4, EU, 0},
	{0x0086E35B, GT4, EU, 0},
	{0x77E61C8A, GT4, US, 0},
	{0x33C6E35E, GT4, US, 0},
	{0x7ABDBB5E, GT3, CH, 0}, // cutie comment
	{0x3E9D448A, GT3, CH, 0}, // cutie comment
	{0xAD66643C, GT3, CH, 0}, // cutie comment
	{0x6810C3BC, GT3, CH, 0}, //GRAN TURISMO Concept 2002 Tokyo-Geneva
	{0x85AE91B3, GT3, US, 0},
	{0xC220951A, GT3, NoRegion, 0},
	{0x9DE5CF65, GT3, JP, 0}, //Gran Turismo 3: A-spec
	{0x60013EBD, GTConcept, EU, 0},
	{0xB590CE04, GTConcept, NoRegion, 0},
	{0x0EEF32A3, GTConcept, KO, 0}, //Gran Turismo Concept 2002 Tokyo-Seoul
	{0xC164550A, WildArms5, JPUNDUB, 0},
	{0xC1640D2C, WildArms5, US, 0},
	{0x0FCF8FE4, WildArms5, EU, 0},
	{0x2294D322, WildArms5, JP, 0},
	{0x565B6170, WildArms5, JP, 0},
	{0xBBC3EFFA, WildArms4, US, 0},
	{0xBBC396EC, WildArms4, US, 0}, //hmm such a small diff in the CRC..
	{0x7B2DE9CC, WildArms4, EU, 0},
	{0x8B029334, Manhunt2, NoRegion, 0},
	{0x09F49E37, CrashBandicootWoC, NoRegion, 0},
	{0x103B5706, CrashBandicootWoC, US, 0}, //American Greatest Hits release
	{0x75182BE5, CrashBandicootWoC, US, 0},
	{0x5188ABCA, CrashBandicootWoC, US, 0},
	{0x3A03D62F, CrashBandicootWoC, EU, 0},
	{0x013E349D, ResidentEvil4, US, 0},
	// same CRC as EU {0x6BA2F6B9, ResidentEvil4, NoRegion, 0},
	{0xDBB7A559, ResidentEvil4, US, 0},
	{0x6BA2F6B9, ResidentEvil4, EU, 0},
	{0x60FA8C69, ResidentEvil4, JP, 0},
	{0x5F254B7C, ResidentEvil4, KO, 0},
	{0x72E1E60E, Spartan, NoRegion, 0},
	{0x26689C87, Spartan, JP, 0},
	{0xA32F7CD0, AceCombat4, US, 0},
	{0x5ED8FB53, AceCombat4, JP, 0},
	{0x1B9B7563, AceCombat4, NoRegion, 0},
	{0xEC432B24, Drakengard2, EU, 0},
	{0x1648E3C9, Drakengard2, US, 0},
	{0xB7ADB13A, Drakengard2, CH, 0},
	{0xFC46EA61, Tekken5, JP, 0},
	{0x1F88EE37, Tekken5, EU, 0},
	{0x1F88BECD, Tekken5, EU, 0},	//language selector...
	{0x652050D2, Tekken5, US, 0},
	{0xEA64EF39, Tekken5, KO, 0},
	{0x9E98B8AE, IkkiTousen, JP, 0},
	{0xD6385328, GodOfWar, US, 0},
	{0xFB0E6D72, GodOfWar, EU, 0},
	{0xEB001875, GodOfWar, EU, 0},
	{0xCF148C74, GodOfWar, EU, 0},
	{0xCA052D22, GodOfWar, JP, 0},
	{0xBFCC1795, GodOfWar, KO, 0},
	{0x9567B7D6, GodOfWar, KO, 0},
	{0x9B5C97BA, GodOfWar, KO, 0},
	{0xA61A4C6D, GodOfWar, NoRegion, 0},
	{0xE23D532B, GodOfWar, NoRegion, 0},
	{0xDF1AF973, GodOfWar, NoRegion, 0},
	// same CRC as US {0xD6385328, GodOfWar, NoRegion, 0},
	{0x1A85E924, GodOfWar, NoRegion, 0}, // cutie comment
	{0x608ACBD3, GodOfWar, CH, 0}, // cutie comment
	//same crc as the US version. {0x2F123FD8, GodOfWar2, RU, 0},
	{0x2F123FD8, GodOfWar2, US, 0},
	{0x44A8A22A, GodOfWar2, EU, 0},
	{0x60BC362B, GodOfWar2, EU, 0},
	{0x4340C7C6, GodOfWar2, KO, 0},
	{0xE96E55BD, GodOfWar2, JP, 0},
	{0xF8CD3DF6, GodOfWar2, NoRegion, 0},
	{0x0B82BFF7, GodOfWar2, NoRegion, 0},
	{0x5990866F, GodOfWar2, NoRegion, 0},
	{0xC4C4FD5F, GodOfWar2, CH, 0},
	{0xDCD9A9F7, GodOfWar2, NoRegion, 0},
	{0xFA0DF523, GodOfWar2, CH, 0}, // cutie comment
	{0x9FEE3466, GodOfWar2, CH, 0}, // cutie comment
	{0x5D482F18, JackieChanAdv, NoRegion, 0},
	{0xF0A6D880, HarvestMoon, US, 0},
	{0x75C01A04, NamcoXCapcom, US, 0},
	//Same CRC also reported as EU, and we have another US crc... {0xBF6F101F, GiTS, US, 0},
	{0x95CC86EF, GiTS, US, 0},
	{0xA5768F53, GiTS, JP, 0},
	{0xA3643EB1, GiTS, KO, 0},
	{0xBF6F101F, GiTS, EU, 0},
	{0x6BF11378, Onimusha3, US, 0},
	{0x71320CA8, Onimusha3, JP, 0},
	{0xDAFFFB0D, Onimusha3, KO, 0},
	{0xF442260C, MajokkoALaMode2, JP, 0},
	{0x14FE77F7, TalesOfAbyss, US, 0},
	{0x045D77E9, TalesOfAbyss, JPUNDUB, 0},
	{0xAA5EC3A3, TalesOfAbyss, JP, 0},
	{0xFB236A46, SonicUnleashed, US, 0},
	{0x8C913264, SonicUnleashed, EU, 0},
	{0x5C1EBD61, SimpsonsGame, NoRegion, 0},
	{0x4C7BB3C8, SimpsonsGame, NoRegion, 0},
	{0x4C94B32C, SimpsonsGame, NoRegion, 0},
	{0x565B7E04, SimpsonsGame, IT, 0},
	{0x206779D8, SimpsonsGame, EU, 0},
	{0xBBE4D862, SimpsonsGame, US, 0},
	{0xD71B57F4, Genji, US, 0},
	{0xFADEBC45, Genji, EU, 0},
	{0xB4776FC1, Genji, JP, 0},
	{0x56242EC9, Genji, KO, 0},
	{0xCDAF243D, Genji, CH, 0}, 
	{0x2A5E0B61, Genji, CH, 0},
	{0x7D4EA48F, Genji, NoRegion, 0},
	{0xE04EA200, StarOcean3, EU, 0},
	{0x23A97857, StarOcean3, US, 0},
	{0xBEC32D49, StarOcean3, JP, 0},
	{0x8192A241, StarOcean3, JP, 0}, //NTSC JP special directors cut limited extra sugar on top edition (the special one :p)
	// it's the US version with speach files from JP... {0x23A97857, StarOcean3, JPUNDUB, 0},
	{0xCC96CE93, ValkyrieProfile2, US, 0},
	{0x774DE8E2, ValkyrieProfile2, JP, 0},
	{0x04CCB600, ValkyrieProfile2, EU, 0},
	{0xB65E141B, ValkyrieProfile2, EU, 0}, // PAL German
	{0xC70FC973, ValkyrieProfile2, IT, 0}, 
	{0x47B9B2FD, RadiataStories, US, 0},
	{0xAC73005E, RadiataStories, JP, 0},
	{0xE8FCF8EC, SMTNocturne, US, ZWriteMustNotClear},	// saves/reloads z buffer around shadow drawing, same issue with all the SMT games following
	{0xF0A31EE3, SMTNocturne, EU, ZWriteMustNotClear},	// SMTNocturne (Lucifers Call in EU)
	{0xAE0DE7B7, SMTNocturne, EU, ZWriteMustNotClear},	// SMTNocturne (Lucifers Call in EU)
	{0xD60DA6D4, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne
	{0x0E762E8D, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne Maniacs
	{0x47BA9034, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne Maniacs Chronicle
	{0xD3FFC263, SMTNocturne, KO, ZWriteMustNotClear},
	{0xD7273511, SMTDDS1, US, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0x1683A6BE, SMTDDS1, EU, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0x44865CE1, SMTDDS1, JP, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0xF2E397C0, SMTDDS1, KO, ZWriteMustNotClear}, // SMT Digital Devil Saga
	{0x43202D1A, SMTDDS2, KO, ZWriteMustNotClear}, // SMT Digital Devil Saga 2
	{0xD382C164, SMTDDS2, US, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0xD568B684, SMTDDS2, EU, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0xE47C1A9C, SMTDDS2, JP, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0x0B8AB37B, RozenMaidenGebetGarden, JP, 0},
	{0x1CC39DBD, SuikodenTactics, US, 0},
	{0x3E205556, SuikodenTactics, EU, 0},
	{0xB808413B, SuikodenTactics, JP, 0},
	{0x64C58FB4, TenchuFS, US, 0},
	{0xE7CCCB1E, TenchuFS, EU, 0},
	{0x1969B19A, TenchuFS, ES, 0},		//PAL Spanish
	{0x696BBEC3, TenchuFS, KO, 0},
	{0x525C1994, TenchuFS, ASIA, 0},
	{0x0D73BBCD, TenchuFS, KO, 0},
	{0xAFBFB287, TenchuWoH, KO, 0},
	{0x767E383D, TenchuWoH, US, 0},
	{0x83261085, TenchuWoH, EU, 0},		//PAL German
	{0x7FA1510D, TenchuWoH, EU, 0},		//PAL ES, IT
	{0x13DD9957, TenchuWoH, JP, 0},
	{0x8BC95883, Sly3, US, 0},
	{0x8164C614, Sly3, EU, 0},
	{0xA8CC1583, Sly3, KO, 0},
	{0x518DD841, Sly2, KO, 0},
	{0x07652DD9, Sly2, US, 0},
	{0xFDA1CBF6, Sly2, EU, 0},
	{0x15DD1F6F, Sly2, NoRegion, 0},
	{0xA9C82AB9, DemonStone, US, 0},
	{0x7C7578F3, DemonStone, EU, 0},
	{0x22425C19, DemonStone, KO, 0},
	{0x506644B3, BigMuthaTruckers, EU, 0},
	{0x90F0D852, BigMuthaTruckers, US, 0},
	{0x5CC9BF81, TimeSplitters2, EU, 0},
	{0x12532F1C, TimeSplitters2, US, 0},
	{0xA33748AA, ReZ, US, 0},
	{0xAE1152EB, ReZ, EU, 0},
	{0xD2EA890A, ReZ, JP, 0},
	{0xC818BEC2, LordOfTheRingsTwoTowers, US, 0},
	{0xDC43F2B8, LordOfTheRingsTwoTowers, EU, 0},
	{0x9ABF90FB, LordOfTheRingsTwoTowers, ES, 0},
	{0xC0E909E9, LordOfTheRingsTwoTowers, JP, 0},
	{0x6898435D, LordOfTheRingsTwoTowers, KO, 0},
	{0xDC2F9B98, LordOfTheRingsTwoTowers, CH, 0}, // cutie comment
	{0xEB198738, LordOfTheRingsThirdAge, US, 0},
	{0x614F4CF4, LordOfTheRingsThirdAge, EU, 0},
	{0x37CD4279, LordOfTheRingsThirdAge, KO, 0},
	{0xE169BAF8, RedDeadRevolver, US, 0},
	{0xE2E67E23, RedDeadRevolver, EU, 0},
	{0xEDDD6573, SpidermanWoS, US, 0},	//Web of Shadows
	{0xF14C1D82, SpidermanWoS, EU, 0},
	{0xF56C7948, HeavyMetalThunder, JP, 0},
	{0x2498951B, SilentHill3, US, 0},
	{0x5088CCDB, SilentHill3, EU, 0},
	{0x8CFE667F, SilentHill3, JP, 0},
	{0xC6CBDE91, SilentHill3, KO, 0},
	{0x8E8E384B, SilentHill2, US, 0},
	{0xFE06A030, SilentHill2, US, 0},	//greatest hits
	{0xE36E16C9, SilentHill2, JP, 0},
	{0x380D6782, SilentHill2, JP, 0},	//Saigo no uta
	{0x6DF62AEA, BleachBladeBattlers, JP, 0},
	{0x6EB71AB0, BleachBladeBattlers, JP, 0},	//2nd
	{0x3A446111, CastlevaniaCoD, US, 0},
	{0xF321BC38, CastlevaniaCoD, EU, 0},
	{0x950876FA, CastlevaniaCoD, KO, 0},
	{0x237B84D3, CastlevaniaCoD, CH, 0},
	{0x28270F7D, CastlevaniaLoI, US, 0},
	{0x306CDADA, CastlevaniaLoI, EU, 0},
	{0xA36CFF6C, CastlevaniaLoI, JP, 0},
	{0x9A93FE5D, CastlevaniaLoI, KO, 0},
	{0xA79B0491, NanoBreaker, JP, 0},
	{0x7985D894, FinalFightStreetwise, US, 0}, 
	{0xED4BF0D3, FinalFightStreetwise, US, 0}, // cutie comment
	{0x73C560BA, FinalFightStreetwise, EU, 0},
	{0xCBB87BF9, EvangelionJo, JP, 0}, // cutie comment
	{0x278A91FD, CaptainTsubasa, JP, 0}, // cutie comment
	{0xC5B75C7C, Oneechanbara2Special, JP, 0}, // cutie comment
	{0xC0659AD1, NarutimateAccel, JP, 0}, // cutie comment
	{0xF3D9DFBE, NarutimateAccel, JP, 0},
	{0x59739DDE, Naruto, JP, 0}, // cutie comment
	{0xF7786EE4, EternalPoison, JP, 0}, // cutie comment
	{0x2BE55519, EternalPoison, US, 0},
	{0xE01F57EC, LegoBatman, US, 0}, // cutie comment
	{0xE01F57ED, LegoBatman, EU, 0},
	{0xE0347841, XE3, JP, 0}, // cutie comment
	{0xA4E88698, XE3, CH, 0},
	{0x2088950A, XE3, US, 0},
	// DMC(1)? {0x79B8A95F, DevilMayCry3, US, 0},
	{0x7F3D692D, DevilMayCry3, CH, 0},
	//duplicate crc with GOW1... {0x1A85E924, DevilMayCry3, CH, 0},
	{0x0a8ef911, ArctheLad, US, 0}, // cutie comment
	{0x2C5E7DEA, ArctheLad, CH, 0},
	{0xE69E7F58, ArctheLad, US, 0}, // cutie comment
	{0xB1995E29, ShadowofRome, EU, 0}, // cutie comment
	{0x958DCA28, ShadowofRome, EU, 0},
	{0x57818AF6, ShadowofRome, US, 0}, 
	{0xF21EE6E0, CrashNburn, US, 0},
	{0x694A998E, TombRaiderUnderworld, JP, 0}, // cutie comment
	{0x8E214549, TombRaiderUnderworld, EU, 0},
	{0xB639EB17, TombRaiderAnniversary, US, 0},
	{0xB05805B6, TombRaiderAnniversary, JP, 0}, // cutie comment
	{0xA629A376, TombRaiderAnniversary, EU, 0},
	{0xBC8B3F50, TombRaiderLegend, US, 0}, // cutie comment
	{0x05177ECE, TombRaiderLegend, EU, 0},
	{0x08FFF00D, SSX3, JP, 0}, // cutie comment
	{0xCE942B2A, SSX3, EU, 0},
	{0x5C891FF1, Black, US, 0},
	{0xCAA04879, Black, EU, 0},
	{0xADDFF505, Black, EU, 0},	//?
	{0xB3A9F9ED, Black, JP, 0},
	{0x7838882F, VF4, JP, 0},
	{0xEA131B57, VF4, US, 0},
	{0x4F755D39, TyTasmanianTiger, US, 0},
	{0xD59D3252, TyTasmanianTiger, EU, 0},
	{0x5A1BB2A1, TyTasmanianTiger2, US, 0},
	{0x44A5FA15, FFVIIDoC, US, 0},
	{0x33F7D21A, FFVIIDoC, EU, 0},
	{0xAFAC88EF, FFVIIDoC, JP, 0},
	{0x568A5C78, DigimonRumbleArena2, US, 0},
	{0x785E22BB, DigimonRumbleArena2, EU, 0},
	{0x4C5CE4C3, DigimonRumbleArena2, EU, 0},
	{0x115A184D, DigimonRumbleArena2, KO, 0},
	{0x879CDA5E, StarWarsForceUnleashed, US, 0},
	{0x137C792E, StarWarsForceUnleashed, US, 0},
	{0x503BF9E1, StarWarsBattlefront, NoRegion, 0},		//EU and US versions have same CRC
	{0x02F4B541, StarWarsBattlefront2, NoRegion, 0},	//EU and US versions have same CRC
	{0xA8DB29DF, BlackHawkDown, EU, 0},
	{0x25FC361B, DevilMayCry3, US, 0},	//SE
	{0x2F7D8AD5, DevilMayCry3, US, 0},
	{0x0BED0AF9, DevilMayCry3, US, 0},
	{0x18C9343F, DevilMayCry3, EU, 0},	//SE
	{0x7ADCB24A, DevilMayCry3, EU, 0},
	{0x79C952B0, DevilMayCry3, JP, 0},	//SE
	{0x7F3DDEAB, DevilMayCry3, JP, 0},
	{0x05931990, DevilMayCry3, KO, 0},
	{0x4AD36D59, DevilMayCry3, RU, 0},
	{0xBEBF8793, BurnoutTakedown, US, 0},
	{0x75BECC18, BurnoutTakedown, EU, 0},
	{0xCE49B0DE, BurnoutTakedown, EU, 0},
	{0xD224D348, BurnoutRevenge, US, 0},
	{0x7E83CC5B, BurnoutRevenge, EU, 0},
	{0xEEA60511, BurnoutRevenge, KO, 0},
	{0x8C9576A1, BurnoutDominator, US, 0},
	{0x8C9576B4, BurnoutDominator, EU, 0},
	{0x4A0E5B3A, MidnightClub3, US, 0},	//dub
	{0xEBE1972D, MidnightClub3, EU, 0},	//dub
	{0x60A42FF5, MidnightClub3, US, 0},	//remix
	{0x4B1A0FFA, XmenOriginsWolverine, US, 0},
	{0xBFF3DBCB, CallofDutyFinalFronts, US, 0},
	{0xB78A5F5A, CallofDutyFinalFronts, EU, 0},
	{0xD03D4C77, SpyroNewBeginning, US, 0},
	{0x0EE5646B, SpyroNewBeginning, EU, 0},
	//duplicate crc with ico... {0x7ACF7E03, SpyroNewBeginning, NoRegion, 0},
	{0xB80CE8EC, SpyroEternalNight, US, 0},
	{0x8AE9536D, SpyroEternalNight, EU, 0},
	{0xC95F0198, SpyroEternalNight, NoRegion, 0},
	{0x43AB7214, TalesOfLegendia, US, 0},
	{0x1F8640E0, TalesOfLegendia, JP, 0},
	{0xE4F5DA2B, TalesOfLegendia, KO, 0},
	{0x98C7B76D, NanoBreaker, US, 0},
	{0x7098BE76, NanoBreaker, KO, 0},
	{0x9B89F425, NanoBreaker, EU, 0},
	{0x519E816B, Kunoichi, US, 0},	//Nightshade
	{0x3FB419FD, Kunoichi, JP, 0},
	{0x086D198E, Kunoichi, CH, 0},
	{0x3B470BBD, Kunoichi, EU, 0},
	{0x6BA65DD8, Kunoichi, KO, 0},
	{0XD3F182A3, Yakuza, EU, 0},
	{0x6F9F99F8, Yakuza, EU, 0},
	{0x388F687B, Yakuza, US, 0},
	{0xB7B3800A, Yakuza, JP, 0},
	{0xA60C2E65, Yakuza2, EU, 0},
	{0x800E3E5A, Yakuza2, EU, 0},
	{0x97E9C87E, Yakuza2, US, 0},
	{0xC6B95C48, Yakuza2, JP, 0},
	{0x9000252A, SkyGunner, JP, 0},
	{0x93092623, SkyGunner, JP, 0},
	{0xA9461CB2, SkyGunner, US, 0},
	{0xB799A60C, SkyGunner, NoRegion, 0},
	{0x6848699B, JamesBondEverythingOrNothing, US, 0},
	{0x5FFFDE40, JamesBondEverythingOrNothing, EU, 0},
	{0xF7FB054C, Siren, CH, 0}, // cutie comment
	{0x47C2C34A, Siren, KO, 0},
	{0xB083CCC2, Siren, EU, 0}, // Spanish
	{0x90F4B057, ZettaiZetsumeiToshi2, CH, 0},
	{0xC988ECBB, ZettaiZetsumeiToshi2, JP, 0},
	{0x81CA29BE, VF4EVO, EU, 0},
	{0xC9DEF513, VF4EVO, US, 0},
	{0x7B402694, VF4EVO, KO, 0},
	{0xAB01411F, VF4EVO, JP, 0},
	{0xE11DFA28, Dororo, CH, 0},
	{0x89954774, Dororo, US, 0},
	{0xFDA2F2DF, Dororo, KO, 0},
	{0xBD17248E, ShinOnimusha, JP, 0},
	{0xBE17248E, ShinOnimusha, JP, 0},
	{0xB817248E, ShinOnimusha, JP, 0},
	{0x812C5A96, ShinOnimusha, EU, 0},
	{0xFE44479E, ShinOnimusha, US, 0},
	{0xFFDE85E9, ShinOnimusha, US, 0},
	{0xE21404E2, GetaWay, US, 0},
	{0xE78971DF, GetaWayBlackMonday, US, 0},
	{0x1130BF23, SakuraTaisen, CH, 0}, // cutie comment
	{0x4FAE8B83, SakuraTaisen, KO, 0},
	{0xEF06DBD6, SakuraWarsSoLongMyLove, JP, 0}, // cutie comment
	{0xDD41054D, SakuraWarsSoLongMyLove, US, 0}, // cutie comment
	{0xC2E3A7A4, SakuraWarsSoLongMyLove, KO, 0},
	{0x4A4B623A, FightingBeautyWulong, JP,0}, // cutie comment
	{0x5AC7E79C, TouristTrophy, CH, 0}, // cutie comment
	{0xFF9C0E93, TouristTrophy, US, 0},
	{0xCA9AA903, TouristTrophy, EU, 0}, //crc hack not fully working on PAL, still needs brightness =0
	{0xA1B3F232, GTASanAndreas, EU, 0}, // cutie comment
	{0x399A49CA, GTASanAndreas, US, 0}, 
	{0x60FE139C, GTASanAndreas, JP, 0}, 
	{0x2615F542, FrontMission5, JP, 0}, 
	{0xF60255AC, FrontMission5, JP, 0},
	{0xCB783836, FrontMission5, JP, 0},
	{0xAEDAEE99, GodHand, JP, 0}, 
	{0x6FB69282, GodHand, US, 0},
	{0x924C4AA6, GodHand, KO, 0},
	{0x9637D496, KnightsOfTheTemple2, JP, 0}, // cutie comment
	{0x4E811100, UltramanFightingEvolution, JP, 0}, // cutie comment
	{0xF7F181C3, DeathByDegreesTekkenNinaWilliams, CH, 0}, // cutie comment
	{0xF088FA5B, DeathByDegreesTekkenNinaWilliams, KO, 0},
	{0x59683BB0, DeathByDegreesTekkenNinaWilliams, EU, 0},
	{0x771C3B47, AlpineRacer3, JP, 0}, // cutie comment
	{0x7367D841, AlpineRacer3, EU, 0},
	{0x449E1F6B, HummerBadlands, US, 0}, 
	{0xAEA1B3AD, SengokuBasara, JP, 0},
	{0x5B659BED, Grandia3, JP, 0},
	{0x5B657DAD, Grandia3, US, 0},
	{0x830B6FB1, TalesofSymphonia, JP, 0},
	{0x8409FD51, TalesofDestiny, JP, 0}, // cutie comment
	{0xA90CD846, TalesofDestiny, JP, 0},
	{0xC4D0FACC, SDGundamGGeneration, JP, 0}, // cutie comment
	{0xBBDE6926, SDGundamGGeneration, JP, 0}, // cutie comment
	{0x49D60A00, SDGundamGGeneration, JP, 0}, //NEO
	{0x83AFB38A, SoulCalibur2, KO, 0},
	{0xE1B01308, SoulCalibur2, US, 0},
	{0xFB8554A0, SoulCalibur3, JP, 0},
	{0x027C604C, SoulCalibur3, US, 0},
	{0x24090A12, SoulCalibur3, EU, 0},
	{0x37B99B14, SoulCalibur3, KO, 0},
	{0xBC5480A3, SoulCalibur3, EU, 0},
	{0xFC0F8A5B, Simple2000Vol114, JP, 0},
	{0x0098F740, SeintoSeiya, NoRegion, 0}, // cutie comment
	{0xBDD9BAAD, UrbanReign, US, 0}, // cutie comment
	{0xAE4BEBD3, UrbanReign, EU, 0},
	{0x9F391882, SteambotChronicles, US, 0},
	{0x06A7506A, SacredBlaze, JP, 0},
};

hash_map<uint32, CRC::Game*> CRC::m_map;

string ToLower( string str )
{
	transform( str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

// The exclusions list is a comma separated list of: the word "all" and/or CRCs in standard hex notation (0x and 8 digits with leading 0's if required).
// The list is case insensitive and order insensitive.
// E.g. Disable all CRC hacks:          CrcHacksExclusions=all
// E.g. Disable hacks for these CRCs:   CrcHacksExclusions=0x0F0C4A9C, 0x0EE5646B, 0x7ACF7E03
bool IsCrcExcluded(string exclusionList, uint32 crc)
{
	string target = format( "0x%08x", crc );
	exclusionList = ToLower( exclusionList );
	return ( exclusionList.find( target ) != string::npos || exclusionList.find( "all" ) != string::npos );
}

CRC::Game CRC::Lookup(uint32 crc)
{
	if(m_map.empty())
	{
		string exclusions = theApp.GetConfig( "CrcHacksExclusions", "" );
		if (exclusions.length() != 0)
			printf( "GSdx: CrcHacksExclusions: %s\n", exclusions.c_str() );

		int crcDups = 0;
		for(size_t i = 0; i < countof(m_games); i++)
		{
			if( !IsCrcExcluded( exclusions, m_games[i].crc ) ){
				if(m_map[m_games[i].crc]){
					printf("[FIXME] GSdx: Duplicate CRC: 0x%x: (game-id/region-id) %d/%d overrides %d/%d\n"
						, m_games[i].crc, m_games[i].title, m_games[i].region, m_map[m_games[i].crc]->title, m_map[m_games[i].crc]->region);
					crcDups++;
				}
				
				m_map[m_games[i].crc] = &m_games[i];
			}
			//else
			//	printf( "GSdx: excluding CRC hack for 0x%08x\n", m_games[i].crc );
		}
		if(crcDups)
			printf("[FIXME] GSdx: Duplicate CRC: Overall: %d\n", crcDups);
	}

	hash_map<uint32, Game*>::iterator i = m_map.find(crc);

	if(i != m_map.end())
	{
		return *i->second;
	}

	return m_games[0];
}
