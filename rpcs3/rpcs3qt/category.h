#pragma once

#include <QString>
#include <QStringList>

enum Category
{
	Disc_Game,
	HDD_Game,
	PS1_Game,
	PS2_Game,
	PSP_Game,
	Home,
	Media,
	Data,
	OS,
	Unknown_Cat,
	Others,
};

namespace cat
{
	const QString cat_app_music = "AM";
	const QString cat_app_photo = "AP";
	const QString cat_app_store = "AS";
	const QString cat_app_tv    = "AT";
	const QString cat_app_video = "AV";
	const QString cat_bc_video  = "BV";
	const QString cat_web_tv    = "WT";
	const QString cat_home      = "HM";
	const QString cat_network   = "CB";
	const QString cat_store_fe  = "SF";
	const QString cat_disc_game = "DG";
	const QString cat_hdd_game  = "HG";
	const QString cat_ps2_game  = "2P";
	const QString cat_ps2_inst  = "2G";
	const QString cat_ps1_game  = "1P";
	const QString cat_psp_game  = "PP";
	const QString cat_psp_mini  = "MN";
	const QString cat_psp_rema  = "PE";
	const QString cat_ps3_data  = "GD";
	const QString cat_ps2_data  = "2D";
	const QString cat_ps3_save  = "SD";
	const QString cat_psp_save  = "MS";

	const QString cat_ps3_os    = "/OS";

	const QString cat_unknown = "Unknown";

	const QStringList ps2_games = { cat_ps2_game, cat_ps2_inst };
	const QStringList psp_games = { cat_psp_game, cat_psp_mini, cat_psp_rema };
	const QStringList media = { cat_app_photo, cat_app_video, cat_bc_video, cat_app_music, cat_app_store, cat_app_tv, cat_web_tv };
	const QStringList data = { cat_ps3_data, cat_ps2_data, cat_ps3_save, cat_psp_save };
	const QStringList os = { cat_ps3_os };
	const QStringList others = { cat_network, cat_store_fe };
}
