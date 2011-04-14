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
	{0xa39517ab, FFX, EU, 0},
	{0xa39517ae, FFX, FR, 0},
	{0x941bb7d9, FFX, DE, 0},
	{0xa39517a9, FFX, IT, 0},
	{0x941bb7de, FFX, ES, 0},
	{0xb4414ea1, FFX, RU, 0},
	{0xee97db5b, FFX, RU, 0},
	{0xaec495cc, FFX, RU, 0},
	{0xbb3d833a, FFX, US, 0},
	{0x6a4efe60, FFX, JP, 0},
	{0x3866ca7e, FFX, ASIA, 0}, // int.
	{0x658597e2, FFX, JP, 0}, // int.
	{0x9aac5309, FFX2, EU, 0},
	{0x9aac530c, FFX2, FR, 0},
	{0x9aac530a, FFX2, FR, 0}, // ?
	{0x9aac530d, FFX2, DE, 0},
	{0x9aac530b, FFX2, IT, 0},
	{0x48fe0c71, FFX2, US, 0},
	{0xe1fd9a2d, FFX2, JP, 0}, // int.
	{0x78da0252, FFXII, EU, 0},
	{0xc1274668, FFXII, EU, 0},
	{0xdc2a467e, FFXII, EU, 0},
	{0xca284668, FFXII, EU, 0},
	{0x280AD120, FFXII, JP, 0},
	{0x08C1ED4D, HauntingGround, NoRegion, 0},
	{0x2CD5794C, HauntingGround, EU, 0},
	{0x867BB945, HauntingGround, JP, 0},
	{0xE263BC4B, HauntingGround, JP, 0},
	{0x901AAC09, HauntingGround, US, 0},
	{0x8BE3D7B2, ShadowHearts, NoRegion, 0},
	{0xDEFA4763, ShadowHearts, US, 0},
	{0x21068223, Okami, US, 0},
	{0x891f223f, Okami, FR, 0},
	{0xC5DEFEA0, Okami, JP, 0},
	{0x053D2239, MetalGearSolid3, US, 0},
	{0x086273D2, MetalGearSolid3, FR, 0},
	{0x26A6E286, MetalGearSolid3, EU, 0},
	{0xAA31B5BF, MetalGearSolid3, NoRegion, 0},
	{0x9F185CE1, MetalGearSolid3, NoRegion, 0},
	{0x98D4BC93, MetalGearSolid3, EU, 0},
	{0x86BC3040, MetalGearSolid3, US, 0}, //Subsistance disc 1
	{0x0481AD8A, MetalGearSolid3, JP, 0},
	{0x79ED26AD, MetalGearSolid3, EU, 0},
	{0x5E31EA42, MetalGearSolid3, EU, 0},
	{0xD7ED797D, MetalGearSolid3, EU, 0},
	{0x278722BF, DBZBT2, US, 0},
	{0xFE961D28, DBZBT2, US, 0},
	{0x0393B6BE, DBZBT2, EU, 0},
	{0xE2F289ED, DBZBT2, JP, 0}, // Sparking Neo!
	{0x35AA84D1, DBZBT2, NoRegion, 0},
	{0x428113C2, DBZBT3, US, 0},
	{0xA422BB13, DBZBT3, EU, 0},
	{0x983C53D2, DBZBT3, NoRegion, 0},
	{0x983C53D3, DBZBT3, NoRegion, 0},
	{0x72B3802A, SFEX3, US, 0},
	{0x71521863, SFEX3, US, 0},
	{0x28703748, Bully, US, 0},
	{0xC78A495D, BullyCC, US, 0},
	{0xC19A374E, SoTC, US, 0},
	{0x7D8F539A, SoTC, EU, 0},
	{0x3122B508, OnePieceGrandAdventure, US, 0},
	{0x8DF14A24, OnePieceGrandAdventure, EU, 0},
	{0xB049DD5E, OnePieceGrandBattle, US, 0},
	{0x5D02CC5B, OnePieceGrandBattle, NoRegion, 0},
	{0x6F8545DB, ICO, US, 0},
	{0xB01A4C95, ICO, JP, 0},
	{0x5C991F4E, ICO, NoRegion, 0},
	{0x7ACF7E03, ICO, NoRegion, 0},
	{0xAEAD1CA3, GT4, JP, 0},
	{0x44A61C8F, GT4, NoRegion, 0},
	{0x0086E35B, GT4, NoRegion, 0},
	{0x77E61C8A, GT4, NoRegion, 0},
	{0x85AE91B3, GT3, US, 0},
	{0xC220951A, GT3, NoRegion, 0},
	{0x60013EBD, GTConcept, EU, 0},
	{0xB590CE04, GTConcept, NoRegion, 0},
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
	{0x75182BE5, CrashBandicootWoC, US, 0},
	{0x5188ABCA, CrashBandicootWoC, US, 0},
	{0x3A03D62F, CrashBandicootWoC, EU, 0},
	{0x013E349D, ResidentEvil4, US, 0},
	{0x6BA2F6B9, ResidentEvil4, NoRegion, 0},
	{0x60FA8C69, ResidentEvil4, JP, 0},
	{0x72E1E60E, Spartan, NoRegion, 0},
	{0x5ED8FB53, AceCombat4, JP, 0},
	{0x1B9B7563, AceCombat4, NoRegion, 0},
	{0xEC432B24, Drakengard2, NoRegion, 0},
	{0xFC46EA61, Tekken5, JP, 0},
	{0x1F88EE37, Tekken5, EU, 0},
	{0x1F88BECD, Tekken5, EU, 0},	//language selector...
	{0x652050D2, Tekken5, US, 0},
	{0x9E98B8AE, IkkiTousen, JP, 0},
	{0xD6385328, GodOfWar, US, 0},
	{0xFB0E6D72, GodOfWar, EU, 0},
	{0xEB001875, GodOfWar, EU, 0},
	{0xA61A4C6D, GodOfWar, NoRegion, 0},
	{0xE23D532B, GodOfWar, NoRegion, 0},
	{0xDF1AF973, GodOfWar, NoRegion, 0},
	{0xD6385328, GodOfWar, NoRegion, 0},
	{0x2F123FD8, GodOfWar2, RU, 0},
	{0x2F123FD8, GodOfWar2, US, 0},
	{0x44A8A22A, GodOfWar2, EU, 0},
	{0x4340C7C6, GodOfWar2, NoRegion, 0},
	{0xF8CD3DF6, GodOfWar2, NoRegion, 0},
	{0x0B82BFF7, GodOfWar2, NoRegion, 0},
	{0x5D482F18, JackieChanAdv, NoRegion, 0},
	{0xf0a6d880, HarvestMoon, US, 0},
	{0x75c01a04, NamcoXCapcom, US, 0},
	{0xBF6F101F, GiTS, US, 0},
	{0x95CC86EF, GiTS, US, 0},
	{0xA5768F53, GiTS, JP, 0},
	{0x6BF11378, Onimusha3, US, 0},
	{0xF442260C, MajokkoALaMode2, JP, 0},
	{0x14FE77F7, TalesOfAbyss, US, 0},
	{0x045D77E9, TalesOfAbyss, JPUNDUB, 0},
	{0xAA5EC3A3, TalesOfAbyss, JP, 0},
	{0xFB236A46, SonicUnleashed, US, 0},
	{0x8C913264, SonicUnleashed, EU, 0},
	{0x4C7BB3C8, SimpsonsGame, NoRegion, 0},
	{0x4C94B32C, SimpsonsGame, NoRegion, 0},
	{0xD71B57F4, Genji, NoRegion, 0},
	{0xE04EA200, StarOcean3, EU, 0},
	{0x23A97857, StarOcean3, US, 0},
	{0xBEC32D49, StarOcean3, JP, 0},
	{0x8192A241, StarOcean3, JP, 0}, //NTSC JP special directors cut limited extra sugar on top edition (the special one :p)
	{0x23A97857, StarOcean3, JPUNDUB, 0},
	{0xCC96CE93, ValkyrieProfile2, US, 0},
	{0x774DE8E2, ValkyrieProfile2, JP, 0},
	{0x04CCB600, ValkyrieProfile2, EU, 0},
	{0xB65E141B, ValkyrieProfile2, EU, 0}, // PAL German
	{0x47B9B2FD, RadiataStories, US, 0},
	{0xE8FCF8EC, SMTNocturne, US, ZWriteMustNotClear},	// saves/reloads z buffer around shadow drawing, same issue with all the SMT games following
	{0xF0A31EE3, SMTNocturne, EU, ZWriteMustNotClear},	// SMTNocturne (Lucifers Call in EU)
	{0xAE0DE7B7, SMTNocturne, EU, ZWriteMustNotClear},	// SMTNocturne (Lucifers Call in EU)
	{0xD60DA6D4, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne
	{0x0e762e8d, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne Maniacs
	{0x47BA9034, SMTNocturne, JP, ZWriteMustNotClear},	// SMTNocturne Maniacs Chronicle
	{0xD7273511, SMTDDS1, US, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0x1683A6BE, SMTDDS1, EU, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0x44865CE1, SMTDDS1, JP, ZWriteMustNotClear},		// SMT Digital Devil Saga
	{0xD382C164, SMTDDS2, US, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0xD568B684, SMTDDS2, EU, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0xE47C1A9C, SMTDDS2, JP, ZWriteMustNotClear},		// SMT Digital Devil Saga 2
	{0x0B8AB37B, RozenMaidenGebetGarden, JP, 0},
	{0x1CC39DBD, SuikodenTactics, US, 0},
	{0x64C58FB4, TenchuFS, US, 0},
	{0xE7CCCB1E, TenchuFS, EU, 0},
	{0x1969B19A, TenchuFS, ES, 0},		//PAL Spanish
	{0x525C1994, TenchuFS, ASIA, 0},
	{0x767E383D, TenchuWoH, US, 0},
	{0x83261085, TenchuWoH, EU, 0},		//PAL German
	{0x8BC95883, Sly3, US, 0},
	{0x8164C614, Sly3, EU, 0},
	{0x07652DD9, Sly2, US, 0},
	{0xFDA1CBF6, Sly2, EU, 0},
	{0xA9C82AB9, DemonStone, US, 0},
	{0x7C7578F3, DemonStone, EU, 0},
	{0x506644B3, BigMuthaTruckers, EU, 0},
	{0x90F0D852, BigMuthaTruckers, US, 0},
	{0x5CC9BF81, TimeSplitters2, EU, 0},
};

hash_map<uint32, CRC::Game*> CRC::m_map;

CRC::Game CRC::Lookup(uint32 crc)
{
	if(m_map.empty())
	{
		for(int i = 0; i < countof(m_games); i++)
		{
			m_map[m_games[i].crc] = &m_games[i];
		}
	}

	hash_map<uint32, Game*>::iterator i = m_map.find(crc);

	if(i != m_map.end())
	{
		return *i->second;
	}

	return m_games[0];
}
