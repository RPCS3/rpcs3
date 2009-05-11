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

#pragma once

class CRC
{
public:
	enum Title
	{
		None,
		MetalSlug6,
		TomoyoAfter,
		Clannad,
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
		TitleCount,
	};

	enum Region 
	{
		Unknown, 
		US, 
		EU, 
		JP, 
		JPUNDUB, 
		RU, 
		FR,
		DE,
		IT,
		ES,
		ASIA
	};

	struct Game 
	{
		DWORD crc; 
		Title title; 
		Region region;
	};

private:
	static Game m_games[];
	static hash_map<DWORD, Game*> m_map;

public:
	static Game Lookup(DWORD crc);
};
