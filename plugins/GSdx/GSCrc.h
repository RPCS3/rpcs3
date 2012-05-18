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
		NoTitle,
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
		HauntingGround,
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
		GT3,
		GTConcept,
		WildArms5,
		WildArms4,
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
		EvangelionJo,
		SuikodenTactics,
		CaptainTsubasa,
		Oneechanbara2Special,
		NarutimateAccel,
		Naruto,
		EternalPoison,
		LegoBatman,
		XE3,
		TenchuWoH,
		TenchuFS,
		Sly3,
		Sly2,
		ShadowofRome,
		ArctheLad,
		DemonStone,
		BigMuthaTruckers,
		TimeSplitters2,
		ReZ,
		LordOfTheRingsTwoTowers,
		LordOfTheRingsThirdAge,
		RedDeadRevolver,
		SpidermanWoS,
		HeavyMetalThunder,
		SilentHill3,
		SilentHill2,
		BleachBladeBattlers,
		CastlevaniaCoD,
		CastlevaniaLoI,
		FinalFightStreetwise,
		CrashNburn,
		TombRaiderUnderworld,
		TombRaiderAnniversary,
		TombRaiderLegend,
		SSX3,
		Black,
		VF4,
		TyTasmanianTiger,
		TyTasmanianTiger2,
		FFVIIDoC,
		DigimonRumbleArena2,
		StarWarsForceUnleashed,
		StarWarsBattlefront,
		StarWarsBattlefront2,
		BlackHawkDown,
		DevilMayCry3,
		BurnoutTakedown,
		BurnoutRevenge,
		BurnoutDominator,
		MidnightClub3,
		XmenOriginsWolverine,
		CallofDutyFinalFronts,
		SpyroNewBeginning,
		SpyroEternalNight,
		TalesOfLegendia,
		NanoBreaker,
		Kunoichi,
		Yakuza,
		Yakuza2,
		SkyGunner,
		JamesBondEverythingOrNothing,
		Siren,
		ZettaiZetsumeiToshi2,
		VF4EVO,
		Dororo,
		ShinOnimusha,
		GetaWay,
		GetaWayBlackMonday,
		SakuraTaisen,
		SakuraWarsSoLongMyLove,
		FightingBeautyWulong,
		TouristTrophy,
		GTASanAndreas,
		FrontMission5,
		GodHand,
		KnightsOfTheTemple2,
		UltramanFightingEvolution,
		RogueGalaxy,
		DeathByDegreesTekkenNinaWilliams,
		AlpineRacer3,
		HummerBadlands,
		SengokuBasara,
		Grandia3,
		TalesofSymphonia,
		TalesofDestiny,
		SDGundamGGeneration,
		SoulCalibur3,
		Simple2000Vol114,
		SeintoSeiya,
		UrbanReign,
		TitleCount,
	};

	enum Region
	{
		NoRegion,
		US,
		EU,
		JP,
		JPUNDUB,
		RU,
		FR,
		DE,
		IT,
		ES,
		CH,
		ASIA,
		KO,
		RegionCount,
	};

	enum Flags
	{
		PointListPalette = 1,
		ZWriteMustNotClear = 2,
	};

	struct Game
	{
		uint32 crc;
		Title title;
		Region region;
		uint32 flags;
	};

private:
	static Game m_games[];
	static hash_map<uint32, Game*> m_map;

public:
	static Game Lookup(uint32 crc);
};
