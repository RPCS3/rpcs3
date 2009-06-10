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

#include "StdAfx.h"
#include "GSCrc.h"

CRC::Game CRC::m_games[] = 
{
	{0x00000000, None, Unknown},
	{0x2113EA2E, MetalSlug6, Unknown},
	{0x42E05BAF, TomoyoAfter, JP},
	{0x7800DC84, Clannad, JP},
	{0xa39517ab, FFX, EU},
	{0xa39517ae, FFX, FR},
	{0x941bb7d9, FFX, DE},
	{0xa39517a9, FFX, IT},
	{0x941bb7de, FFX, ES},
	{0xb4414ea1, FFX, RU},
	{0xee97db5b, FFX, RU},
	{0xaec495cc, FFX, RU},
	{0xbb3d833a, FFX, US},
	{0x6a4efe60, FFX, JP},
	{0x3866ca7e, FFX, ASIA}, // int.
	{0x658597e2, FFX, JP}, // int.
	{0x9aac5309, FFX2, EU},
	{0x9aac530c, FFX2, FR},
	{0x9aac530a, FFX2, FR}, // ?
	{0x9aac530d, FFX2, DE},
	{0x9aac530b, FFX2, IT},
	{0x48fe0c71, FFX2, US},
	{0xe1fd9a2d, FFX2, JP}, // int.
	{0x78da0252, FFXII, EU},
	{0xc1274668, FFXII, EU},
	{0xdc2a467e, FFXII, EU},
	{0xca284668, FFXII, EU},
	{0x280AD120, FFXII, JP},
	{0x8BE3D7B2, ShadowHearts, Unknown},
	{0xDEFA4763, ShadowHearts, US},
	{0x21068223, Okami, US},
	{0x891f223f, Okami, FR},
	{0xC5DEFEA0, Okami, JP},
	{0x053D2239, MetalGearSolid3, US},
	{0x086273D2, MetalGearSolid3, FR},
	{0x26A6E286, MetalGearSolid3, EU},
	{0xAA31B5BF, MetalGearSolid3, Unknown},
	{0x9F185CE1, MetalGearSolid3, Unknown},
	{0x98D4BC93, MetalGearSolid3, EU},
	{0x86BC3040, MetalGearSolid3, US}, //Subsistance disc 1
	{0x0481AD8A, MetalGearSolid3, JP},
	{0x79ED26AD, MetalGearSolid3, EU},
	{0x5E31EA42, MetalGearSolid3, EU},
	{0x278722BF, DBZBT2, US},
	{0xFE961D28, DBZBT2, US},
	{0x0393B6BE, DBZBT2, EU},
	{0xE2F289ED, DBZBT2, JP}, // Sparking Neo!
	{0x35AA84D1, DBZBT2, Unknown},	
	{0x428113C2, DBZBT3, US},
	{0xA422BB13, DBZBT3, EU},
	{0x983c53d2, DBZBT3, Unknown},	
	{0x72B3802A, SFEX3, US},
	{0x71521863, SFEX3, US},
	{0x28703748, Bully, US},
	{0xC78A495D, BullyCC, US},
	{0xC19A374E, SoTC, US},
	{0x7D8F539A, SoTC, EU},
	{0x3122B508, OnePieceGrandAdventure, US},
	{0x8DF14A24, OnePieceGrandAdventure, Unknown},
	{0x6F8545DB, ICO, US},
	{0xB01A4C95, ICO, JP},
	{0x5C991F4E, ICO, Unknown},
	{0x7ACF7E03, ICO, Unknown},
	{0xAEAD1CA3, GT4, JP},
	{0x44A61C8F, GT4, Unknown},
	{0x0086E35B, GT4, Unknown},
	{0x77E61C8A, GT4, Unknown},
	{0xC164550A, WildArms5, JPUNDUB},
	{0xC1640D2C, WildArms5, US},
	{0x0FCF8FE4, WildArms5, EU},
	{0x2294D322, WildArms5, JP}, 
	{0x565B6170, WildArms5, JP},
	{0x8B029334, Manhunt2, Unknown},
	{0x09F49E37, CrashBandicootWoC, Unknown},
	{0x013E349D, ResidentEvil4, US},
	{0x6BA2F6B9, ResidentEvil4, Unknown},
	{0x60FA8C69, ResidentEvil4, JP},
	{0x72E1E60E, Spartan, Unknown},
	{0x5ED8FB53, AceCombat4, JP},
	{0x1B9B7563, AceCombat4, Unknown},
	{0xEC432B24, Drakengard2, Unknown},
	{0xFC46EA61, Tekken5, JP},
	{0x1F88EE37, Tekken5, Unknown},
	{0x652050D2, Tekken5, Unknown}, 
	{0x9E98B8AE, IkkiTousen, JP},
	{0xD6385328, GodOfWar, US},
	{0xFB0E6D72, GodOfWar, EU},
	{0xEB001875, GodOfWar, EU},
	{0xA61A4C6D, GodOfWar, Unknown},
	{0xE23D532B, GodOfWar, Unknown},
	{0xDF1AF973, GodOfWar, Unknown},
	{0xD6385328, GodOfWar, Unknown},	
	{0x2F123FD8, GodOfWar2, RU},
	{0x2F123FD8, GodOfWar2, US},
	{0x44A8A22A, GodOfWar2, EU},
	{0x4340C7C6, GodOfWar2, Unknown},	
	{0x5D482F18, JackieChanAdv, Unknown},
	{0xf0a6d880, HarvestMoon, US},
	{0x75c01a04, NamcoXCapcom, US},
	{0xBF6F101F, GiTS, US},
	{0xA5768F53, GiTS, JP},
	{0x6BF11378, Onimusha3, US},	
	{0xF442260C, MajokkoALaMode2, JP},
	{0x14FE77F7, TalesOfAbyss, US},
	{0x045D77E9, TalesOfAbyss, US}, // undub
	{0xAA5EC3A3, TalesOfAbyss, JP}, 
	{0xFB236A46, SonicUnleashed, US},
	{0x4C7BB3C8, SimpsonsGame, Unknown},
	{0x4C94B32C, SimpsonsGame, Unknown},
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
		return *(*i).second;
	}

	return m_games[0];
}
