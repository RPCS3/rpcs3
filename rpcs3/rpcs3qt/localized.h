#pragma once

#include "category.h"

#include <QString>
#include <QObject>

#include "util/types.hpp"

typedef std::map<const QString, const QString> localized_cat;

class Localized : public QObject
{
	Q_OBJECT

public:
	Localized() {}

	QString GetVerboseTimeByMs(quint64 elapsed_ms, bool show_days = false) const;
	static std::string GetStringFromU32(const u32& key, const std::map<u32, QString>& map, bool combined = false);

	const struct category // (see PARAM.SFO in psdevwiki.com) TODO: Disc Categories
	{
		// PS3 bootable
		const QString app_music = tr("Music App");
		const QString app_photo = tr("Photo App");
		const QString app_store = tr("Store App");
		const QString app_tv    = tr("TV App");
		const QString app_video = tr("Video App");
		const QString bc_video  = tr("Broadcast Video");
		const QString disc_game = tr("Disc Game");
		const QString hdd_game  = tr("HDD Game");
		const QString home      = tr("Home");
		const QString network   = tr("Network");
		const QString store_fe  = tr("Store");
		const QString web_tv    = tr("Web TV");
		const QString os_app    = tr("Operating System");

		// PS2 bootable
		const QString ps2_game = tr("PS2 Classics");
		const QString ps2_inst = tr("PS2 Game");

		// PS1 bootable
		const QString ps1_game = tr("PS1 Classics");

		// PSP bootable
		const QString psp_game = tr("PSP Game");
		const QString psp_mini = tr("PSP Minis");
		const QString psp_rema = tr("PSP Remasters");

		// Data
		const QString ps3_data = tr("PS3 Game Data");
		const QString ps2_data = tr("PS2 Emulator Data");

		// Save
		const QString ps3_save = tr("PS3 Save Data");
		const QString psp_save = tr("PSP Minis Save Data");

		// others
		const QString trophy  = tr("Trophy");
		const QString unknown = tr("Unknown");
		const QString other   = tr("Other");

		const localized_cat cat_boot =
		{
			{ cat::cat_app_music, app_music }, // media
			{ cat::cat_app_photo, app_photo }, // media
			{ cat::cat_app_store, app_store }, // media
			{ cat::cat_app_tv   , app_tv    }, // media
			{ cat::cat_app_video, app_video }, // media
			{ cat::cat_bc_video , bc_video  }, // media
			{ cat::cat_web_tv   , web_tv    }, // media
			{ cat::cat_home     , home      }, // home
			{ cat::cat_network  , network   }, // other
			{ cat::cat_store_fe , store_fe  }, // other
			{ cat::cat_disc_game, disc_game }, // disc_game
			{ cat::cat_hdd_game , hdd_game  }, // hdd_game
			{ cat::cat_ps2_game , ps2_game  }, // ps2_games
			{ cat::cat_ps2_inst , ps2_inst  }, // ps2_games
			{ cat::cat_ps1_game , ps1_game  }, // ps1_game
			{ cat::cat_psp_game , psp_game  }, // psp_games
			{ cat::cat_psp_mini , psp_mini  }, // psp_games
			{ cat::cat_psp_rema , psp_rema  }, // psp_games
			{ cat::cat_ps3_os   , os_app    }, // other
		};

		const localized_cat cat_data =
		{
			{ cat::cat_ps3_data, ps3_data }, // data
			{ cat::cat_ps2_data, ps2_data }, // data
			{ cat::cat_ps3_save, ps3_save }, // data
			{ cat::cat_psp_save, psp_save }  // data
		};
	} category;

	const struct parental
	{
		// These values are partly generalized. They can vary between country and category
		// Normally only values 1,2,3,5,7 and 9 are used
		const std::map<u32, QString> level
		{
			{ 1,  tr("0+") },
			{ 2,  tr("3+") },
			{ 3,  tr("7+") },
			{ 4,  tr("10+") },
			{ 5,  tr("12+") },
			{ 6,  tr("15+") },
			{ 7,  tr("16+") },
			{ 8,  tr("17+") },
			{ 9,  tr("18+") },
			{ 10, tr("Level 10") },
			{ 11, tr("Level 11") }
		};
	} parental;

	const struct resolution
	{
		// there might be different values for other categories
		resolution();
		const std::map<u32, QString> mode;
	} resolution;

	const struct sound
	{
		sound();
		const std::map<u32, QString> format;
	} sound;

	const struct title_t
	{
		title_t();
		const std::map<std::string, QString> titles;
	} title;
};
