#include "gui_settings.h"

#include "qt_utils.h"
#include "category.h"

#include "Emu/System.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>

LOG_CHANNEL(cfg_log, "CFG");

extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op);

namespace gui
{
	QString stylesheet;
	bool custom_stylesheet_active = false;

	QString get_savestate_list_column_name(savestate_list_columns col)
	{
		switch (col)
		{
		case gui::savestate_list_columns::name: return "savestate_column_name";
		case gui::savestate_list_columns::compatible: return "savestate_column_compatible";
		case gui::savestate_list_columns::date: return "savestate_column_date";
		case gui::savestate_list_columns::path: return "savestate_column_path";
		case gui::savestate_list_columns::count: return "";
		}

		fmt::throw_exception("get_savestate_list_column_name: Invalid column");
	}

	QString get_savestate_game_list_column_name(savestate_game_list_columns col)
	{
		switch (col)
		{
		case gui::savestate_game_list_columns::icon: return "savestate_game_column_icon";
		case gui::savestate_game_list_columns::name: return "savestate_game_column_name";
		case gui::savestate_game_list_columns::savestates: return "savestate_game_column_savestates";
		case gui::savestate_game_list_columns::count: return "";
		}

		fmt::throw_exception("get_savestate_game_list_column_name: Invalid column");
	}

	QString get_game_list_column_name(game_list_columns col)
	{
		switch (col)
		{
		case game_list_columns::icon: return "column_icon";
		case game_list_columns::name: return "column_name";
		case game_list_columns::serial: return "column_serial";
		case game_list_columns::firmware: return "column_firmware";
		case game_list_columns::version: return "column_version";
		case game_list_columns::category: return "column_category";
		case game_list_columns::path: return "column_path";
		case game_list_columns::move: return "column_move";
		case game_list_columns::resolution: return "column_resolution";
		case game_list_columns::sound: return "column_sound";
		case game_list_columns::parental: return "column_parental";
		case game_list_columns::last_play: return "column_last_play";
		case game_list_columns::playtime: return "column_playtime";
		case game_list_columns::compat: return "column_compat";
		case game_list_columns::dir_size: return "column_dir_size";
		case game_list_columns::count: return "";
		}
	
		fmt::throw_exception("get_game_list_column_name: Invalid column");
	}

	QString get_trophy_list_column_name(trophy_list_columns col)
	{
		switch (col)
		{
		case trophy_list_columns::icon: return "trophy_column_icon";
		case trophy_list_columns::name: return "trophy_column_name";
		case trophy_list_columns::description: return "trophy_column_description";
		case trophy_list_columns::type: return "trophy_column_type";
		case trophy_list_columns::is_unlocked: return "trophy_column_is_unlocked";
		case trophy_list_columns::id: return "trophy_column_id";
		case trophy_list_columns::platinum_link: return "trophy_column_platinum_link";
		case trophy_list_columns::time_unlocked: return "trophy_column_time_unlocked";
		case trophy_list_columns::count: return "";
		}
	
		fmt::throw_exception("get_trophy_list_column_name: Invalid column");
	}

	QString get_trophy_game_list_column_name(trophy_game_list_columns col)
	{
		switch (col)
		{
		case trophy_game_list_columns::icon: return "trophy_game_column_icon";
		case trophy_game_list_columns::name: return "trophy_game_column_name";
		case trophy_game_list_columns::progress: return "trophy_game_column_progress";
		case trophy_game_list_columns::trophies: return "trophy_game_column_trophies";
		case trophy_game_list_columns::count: return "";
		}
	
		fmt::throw_exception("get_trophy_game_list_column_name: Invalid column");
	}
}

gui_settings::gui_settings(QObject* parent) : settings(parent)
{
	m_settings = std::make_unique<QSettings>(ComputeSettingsDir() + gui::Settings + ".ini", QSettings::Format::IniFormat, parent);
}

QStringList gui_settings::GetGameListCategoryFilters(bool is_list_mode) const
{
	QStringList filters;

	if (GetCategoryVisibility(Category::HDD_Game, is_list_mode)) filters.append(cat::cat_hdd_game);
	if (GetCategoryVisibility(Category::Disc_Game, is_list_mode)) filters.append(cat::cat_disc_game);
	if (GetCategoryVisibility(Category::PS1_Game, is_list_mode)) filters.append(cat::cat_ps1_game);
	if (GetCategoryVisibility(Category::PS2_Game, is_list_mode)) filters.append(cat::ps2_games);
	if (GetCategoryVisibility(Category::PSP_Game, is_list_mode)) filters.append(cat::psp_games);
	if (GetCategoryVisibility(Category::Home, is_list_mode)) filters.append(cat::cat_home);
	if (GetCategoryVisibility(Category::Media, is_list_mode)) filters.append(cat::media);
	if (GetCategoryVisibility(Category::Data, is_list_mode)) filters.append(cat::data);
	if (GetCategoryVisibility(Category::OS, is_list_mode)) filters.append(cat::os);
	if (GetCategoryVisibility(Category::Unknown_Cat, is_list_mode)) filters.append(cat::cat_unknown);
	if (GetCategoryVisibility(Category::Others, is_list_mode)) filters.append(cat::others);

	return filters;
}

bool gui_settings::GetCategoryVisibility(int cat, bool is_list_mode) const
{
	const gui_save value = GetGuiSaveForCategory(cat, is_list_mode);
	return GetValue(value).toBool();
}

void gui_settings::SetCategoryVisibility(int cat, bool val, bool is_list_mode) const
{
	const gui_save value = GetGuiSaveForCategory(cat, is_list_mode);
	SetValue(value, val);
}

void gui_settings::ShowBox(QMessageBox::Icon icon, const QString& title, const QString& text, const gui_save& entry, int* result = nullptr, QWidget* parent = nullptr, bool always_on_top = false)
{
	const std::string dialog_type = icon != QMessageBox::Information ? "Confirmation" : "Info";
	const bool has_gui_setting = !entry.name.isEmpty();

	if (has_gui_setting && !GetValue(entry).toBool())
	{
		cfg_log.notice("%s Dialog for Entry %s was ignored", dialog_type, entry.name);
		return;
	}

	const QFlags<QMessageBox::StandardButton> buttons = icon != QMessageBox::Information ? QMessageBox::Yes | QMessageBox::No : QMessageBox::Ok;

	QMessageBox mb(icon, title, text, buttons, parent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | (always_on_top ? Qt::WindowStaysOnTopHint : Qt::Widget));
	mb.setTextFormat(Qt::RichText);

	if (has_gui_setting && icon != QMessageBox::Critical)
	{
		mb.setCheckBox(new QCheckBox(tr("Don't show again")));
	}

	connect(&mb, &QMessageBox::finished, [&](int res)
	{
		if (result)
		{
			*result = res;
		}

		const auto checkBox = mb.checkBox();

		if (checkBox && checkBox->isChecked())
		{
			SetValue(entry, false);
			cfg_log.notice("%s Dialog for Entry %s is now disabled", dialog_type, entry.name);
		}
	});

	mb.exec();
}

void gui_settings::ShowConfirmationBox(const QString& title, const QString& text, const gui_save& entry, int* result = nullptr, QWidget* parent = nullptr)
{
	ShowBox(QMessageBox::Question, title, text, entry, result, parent, true);
}

void gui_settings::ShowInfoBox(const QString& title, const QString& text, const gui_save& entry, QWidget* parent = nullptr)
{
	ShowBox(QMessageBox::Information, title, text, entry, nullptr, parent, false);
}

bool gui_settings::GetBootConfirmation(QWidget* parent, const gui_save& gui_save_entry)
{
	// Ensure no game has booted inbetween
	const auto guard = Emu.MakeEmulationStateGuard();

	const auto info = Emu.GetEmulationIdentifier();
	const auto old_status = Emu.GetStatus(false);

	qt_events_aware_op(16, [&]()
	{
		if (Emu.GetStatus(false) != system_state::stopping)
		{
			ensure(info == Emu.GetEmulationIdentifier(old_status == system_state::stopping));
			return true;
		}

		return false;
	});

	if (!Emu.IsStopped())
	{
		QString title = tr("Close Running Game?");
		QString message = tr("Performing this action will close the current game.<br>Do you really want to continue?<br><br>Any unsaved progress will be lost!<br>");

		if (gui_save_entry == gui::ib_confirm_boot)
		{
			message = tr("Booting another game will close the current game.<br>Do you really want to boot another game?<br><br>Any unsaved progress will be lost!<br>");
		}
		else if (gui_save_entry == gui::ib_confirm_exit)
		{
			title = tr("Exit RPCS3?");
			message = tr("A game is currently running. Do you really want to close RPCS3?<br><br>Any unsaved progress will be lost!<br>");
		}

		int result = QMessageBox::Yes;

		ShowBox(QMessageBox::Question, title, message, gui_save_entry, &result, parent);

		if (result != QMessageBox::Yes)
		{
			return false;
		}

		cfg_log.notice("User accepted to stop the current emulation.");
	}

	return true;
}

void gui_settings::SetSavestateGamelistColVisibility(gui::savestate_game_list_columns col, bool val) const
{
	SetValue(GetGuiSaveForSavestateGameColumn(col), val);
}

void gui_settings::SetSavestateListColVisibility(gui::savestate_list_columns col, bool val) const
{
	SetValue(GetGuiSaveForSavestateColumn(col), val);
}

void gui_settings::SetTrophyGamelistColVisibility(gui::trophy_game_list_columns col, bool val) const
{
	SetValue(GetGuiSaveForTrophyGameColumn(col), val);
}

void gui_settings::SetTrophylistColVisibility(gui::trophy_list_columns col, bool val) const
{
	SetValue(GetGuiSaveForTrophyColumn(col), val);
}

void gui_settings::SetGamelistColVisibility(gui::game_list_columns col, bool val) const
{
	SetValue(GetGuiSaveForGameColumn(col), val);
}

void gui_settings::SetCustomColor(int col, const QColor& val) const
{
	SetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::gl_icon_color), val);
}

logs::level gui_settings::GetLogLevel() const
{
	return logs::level(GetValue(gui::l_level).toUInt());
}

bool gui_settings::GetSavestateGamelistColVisibility(gui::savestate_game_list_columns col) const
{
	return GetValue(GetGuiSaveForSavestateGameColumn(col)).toBool();
}

bool gui_settings::GetSavestateListColVisibility(gui::savestate_list_columns col) const
{
	return GetValue(GetGuiSaveForSavestateColumn(col)).toBool();
}

bool gui_settings::GetTrophyGamelistColVisibility(gui::trophy_game_list_columns col) const
{
	return GetValue(GetGuiSaveForTrophyGameColumn(col)).toBool();
}

bool gui_settings::GetTrophylistColVisibility(gui::trophy_list_columns col) const
{
	return GetValue(GetGuiSaveForTrophyColumn(col)).toBool();
}

bool gui_settings::GetGamelistColVisibility(gui::game_list_columns col) const
{
	return GetValue(GetGuiSaveForGameColumn(col)).toBool();
}

QColor gui_settings::GetCustomColor(int col) const
{
	return GetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::gl_icon_color)).value<QColor>();
}

QStringList gui_settings::GetStylesheetEntries() const
{
	const QStringList name_filter = QStringList("*.qss");
	QStringList res = gui::utils::get_dir_entries(m_settings_dir, name_filter);
#if !defined(_WIN32)
	// Makes stylesheets load if using AppImage (App Bundle) or installed to /usr/bin
#ifdef __APPLE__
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../Resources/GuiConfigs/";
#else
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/";
#ifdef DATADIR
	const QString data_dir = (DATADIR);
	res.append(gui::utils::get_dir_entries(data_dir + "/GuiConfigs/", name_filter));
#endif
#endif
	res.append(gui::utils::get_dir_entries(QCoreApplication::applicationDirPath() + "/GuiConfigs/", name_filter));
	res.append(gui::utils::get_dir_entries(platformStylesheetDir, name_filter));
	res.removeDuplicates();
#endif
	res.sort();
	return res;
}

QSize gui_settings::SizeFromSlider(int pos)
{
	return gui::gl_icon_size_min + (gui::gl_icon_size_max - gui::gl_icon_size_min) * (1.f * pos / gui::gl_max_slider_pos);
}

gui_save gui_settings::GetGuiSaveForSavestateGameColumn(gui::savestate_game_list_columns col)
{
	return gui_save{ gui::savestate, "visibility_" + gui::get_savestate_game_list_column_name(col), true };
}

gui_save gui_settings::GetGuiSaveForSavestateColumn(gui::savestate_list_columns col)
{
	return gui_save{ gui::savestate, "visibility_" + gui::get_savestate_list_column_name(col), true };
}

gui_save gui_settings::GetGuiSaveForTrophyGameColumn(gui::trophy_game_list_columns col)
{
	return gui_save{ gui::trophy, "visibility_" + gui::get_trophy_game_list_column_name(col), true };
}

gui_save gui_settings::GetGuiSaveForTrophyColumn(gui::trophy_list_columns col)
{
	return gui_save{ gui::trophy, "visibility_" + gui::get_trophy_list_column_name(col), true };
}

gui_save gui_settings::GetGuiSaveForGameColumn(gui::game_list_columns col)
{
	// hide sound format, parental level, firmware version and path by default
	const bool show = col != gui::game_list_columns::sound && col != gui::game_list_columns::parental && col != gui::game_list_columns::firmware && col != gui::game_list_columns::path;
	return gui_save{ gui::game_list, "visibility_" + gui::get_game_list_column_name(col), show };
}

gui_save gui_settings::GetGuiSaveForCategory(int cat, bool is_list_mode)
{
	switch (cat)
	{
	case Category::HDD_Game: return is_list_mode ? gui::cat_hdd_game : gui::grid_cat_hdd_game;
	case Category::Disc_Game: return is_list_mode ? gui::cat_disc_game : gui::grid_cat_disc_game;
	case Category::Home: return is_list_mode ? gui::cat_home : gui::grid_cat_home;
	case Category::PS1_Game: return is_list_mode ? gui::cat_ps1_game : gui::grid_cat_ps1_game;
	case Category::PS2_Game: return is_list_mode ? gui::cat_ps2_game : gui::grid_cat_ps2_game;
	case Category::PSP_Game: return is_list_mode ? gui::cat_psp_game : gui::grid_cat_psp_game;
	case Category::Media: return is_list_mode ? gui::cat_audio_video : gui::grid_cat_audio_video;
	case Category::Data: return is_list_mode ? gui::cat_game_data : gui::grid_cat_game_data;
	case Category::OS: return is_list_mode ? gui::cat_os : gui::grid_cat_os;
	case Category::Unknown_Cat: return is_list_mode ? gui::cat_unknown : gui::grid_cat_unknown;
	case Category::Others: return is_list_mode ? gui::cat_other : gui::grid_cat_other;
	default:
		cfg_log.warning("GetGuiSaveForCategory: wrong cat <%d>", cat);
		return {};
	}
}
