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
	{0x00000000, None, Unknown, false},
	{0x2113EA2E, MetalSlug6, Unknown, false},
	{0x42E05BAF, TomoyoAfter, JP, false},
	{0x7800DC84, Clannad, JP, false},
	{0xa39517ab, FFX, EU, true},
	{0xa39517ae, FFX, FR, true},
	{0x941bb7d9, FFX, DE, true},
	{0xa39517a9, FFX, IT, true},
	{0x941bb7de, FFX, ES, true},
	{0xb4414ea1, FFX, RU, true},
	{0xee97db5b, FFX, RU, true},
	{0xaec495cc, FFX, RU, true},
	{0xbb3d833a, FFX, US, true},
	{0x6a4efe60, FFX, JP, true},
	{0x3866ca7e, FFX, ASIA, true}, // int.
	{0x658597e2, FFX, JP, true}, // int.
	{0x9aac5309, FFX2, EU, true},
	{0x9aac530c, FFX2, FR, true},
	{0x9aac530a, FFX2, FR, true}, // ?
	{0x9aac530d, FFX2, DE, true},
	{0x9aac530b, FFX2, IT, true},
	{0x48fe0c71, FFX2, US, true},
	{0xe1fd9a2d, FFX2, JP, true}, // int.
	{0x78da0252, FFXII, EU, false},
	{0xc1274668, FFXII, EU, false},
	{0xdc2a467e, FFXII, EU, false},
	{0xca284668, FFXII, EU, false},
	{0x280AD120, FFXII, JP, false},
	{0x8BE3D7B2, ShadowHearts, Unknown, false},
	{0xDEFA4763, ShadowHearts, US, false},
	{0x21068223, Okami, US, false},
	{0x891f223f, Okami, FR, false},
	{0xC5DEFEA0, Okami, JP, false},
	{0x053D2239, MetalGearSolid3, US, false},
	{0x086273D2, MetalGearSolid3, FR, false},
	{0x26A6E286, MetalGearSolid3, EU, false},
	{0xAA31B5BF, MetalGearSolid3, Unknown, false},
	{0x9F185CE1, MetalGearSolid3, Unknown, false},
	{0x98D4BC93, MetalGearSolid3, EU, false},
	{0x86BC3040, MetalGearSolid3, US, false}, //Subsistance disc 1
	{0x0481AD8A, MetalGearSolid3, JP, false},
	{0x278722BF, DBZBT2, US, false},
	{0xFE961D28, DBZBT2, US, false},
	{0x0393B6BE, DBZBT2, EU, false},
	{0xE2F289ED, DBZBT2, JP, false}, // Sparking Neo!
	{0x35AA84D1, DBZBT2, Unknown, false},	
	{0x428113C2, DBZBT3, US, false},
	{0xA422BB13, DBZBT3, EU, false},
	{0x983c53d2, DBZBT3, Unknown, false},	
	{0x72B3802A, SFEX3, US, false},
	{0x71521863, SFEX3, US, false},
	{0x28703748, Bully, US, false},
	{0xC78A495D, BullyCC, US, false},
	{0xC19A374E, SoTC, US, false},
	{0x7D8F539A, SoTC, EU, false},
	{0x3122B508, OnePieceGrandAdventure, US, false},
	{0x6F8545DB, ICO, US, false},
	{0xB01A4C95, ICO, JP, false},
	{0x5C991F4E, ICO, Unknown, false},
	{0xAEAD1CA3, GT4, JP, false},
	{0x44A61C8F, GT4, Unknown, false},
	{0x0086E35B, GT4, Unknown, false},
	{0x77E61C8A, GT4, Unknown, false},
	{0xC164550A, WildArms5, JPUNDUB, false},
	{0xC1640D2C, WildArms5, US, false},
	{0x0FCF8FE4, WildArms5, EU, false},
	{0x2294D322, WildArms5, JP, false}, 
	{0x565B6170, WildArms5, JP, false},
	{0x8B029334, Manhunt2, Unknown, false},
	{0x09F49E37, CrashBandicootWoC, Unknown, false},
	{0x013E349D, ResidentEvil4, US, false},
	{0x6BA2F6B9, ResidentEvil4, Unknown, false},
	{0x60FA8C69, ResidentEvil4, JP, false},
	{0x72E1E60E, Spartan, Unknown, false},
	{0x5ED8FB53, AceCombat4, JP, false},
	{0x1B9B7563, AceCombat4, Unknown, false},
	{0xEC432B24, Drakengard2, Unknown, false},
	{0xFC46EA61, Tekken5, JP, false},
	{0x1F88EE37, Tekken5, Unknown, false},
	{0x652050D2, Tekken5, Unknown, false}, 
	{0x9E98B8AE, IkkiTousen, JP, false},
	{0xD6385328, GodOfWar, US, false},
	{0xFB0E6D72, GodOfWar, EU, false},
	{0xA61A4C6D, GodOfWar, Unknown, false},
	{0xE23D532B, GodOfWar, Unknown, false},
	{0x2F123FD8, GodOfWar2, RU, false},
	{0x5D482F18, JackieChanAdv, Unknown, false},
	{0xf0a6d880, HarvestMoon, US, true},
	{0x75c01a04, NamcoXCapcom, US, false},
	{0xBF6F101F, GiTS, US, false},
	{0xA5768F53, GiTS, JP, false},
	{0x6BF11378, Onimusha3, US, false},	
	{0xF442260C, MajokkoALaMode2, JP, false},
	{0x14FE77F7, TalesOfAbyss, US, false},
	{0x045D77E9, TalesOfAbyss, US, false}, // undub
	{0xAA5EC3A3, TalesOfAbyss, JP, false}, 
};

CAtlMap<DWORD, CRC::Game*> CRC::m_map;

CRC::Game CRC::Lookup(DWORD crc)
{
	if(m_map.IsEmpty())
	{
		for(int i = 0; i < countof(m_games); i++)
		{
			m_map[m_games[i].crc] = &m_games[i];
		}
	}

	if(CAtlMap<DWORD, Game*>::CPair* pair = m_map.Lookup(crc))
	{
		return *pair->m_value;
	}

	return m_games[0];
}
