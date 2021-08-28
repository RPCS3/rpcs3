#include "gui_settings.h"

#include "qt_utils.h"
#include "localized.h"

#include "Emu/System.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

gui_settings::gui_settings(QObject* parent) : settings(parent)
{
	m_settings.reset(new QSettings(ComputeSettingsDir() + gui::Settings + ".ini", QSettings::Format::IniFormat, parent));
}

QStringList gui_settings::GetGameListCategoryFilters() const
{
	QStringList filterList;

	if (GetCategoryVisibility(Category::HDD_Game)) filterList.append(cat::cat_hdd_game);
	if (GetCategoryVisibility(Category::Disc_Game)) filterList.append(cat::cat_disc_game);
	if (GetCategoryVisibility(Category::PS1_Game)) filterList.append(cat::cat_ps1_game);
	if (GetCategoryVisibility(Category::PS2_Game)) filterList.append(cat::ps2_games);
	if (GetCategoryVisibility(Category::PSP_Game)) filterList.append(cat::psp_games);
	if (GetCategoryVisibility(Category::Home)) filterList.append(cat::cat_home);
	if (GetCategoryVisibility(Category::Media)) filterList.append(cat::media);
	if (GetCategoryVisibility(Category::Data)) filterList.append(cat::data);
	if (GetCategoryVisibility(Category::Unknown_Cat)) filterList.append(cat::cat_unknown);
	if (GetCategoryVisibility(Category::Others)) filterList.append(cat::others);

	return filterList;
}

bool gui_settings::GetCategoryVisibility(int cat) const
{
	gui_save value;

	switch (cat)
	{
	case Category::HDD_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
	case Category::PS1_Game:
		value = gui::cat_ps1_game; break;
	case Category::PS2_Game:
		value = gui::cat_ps2_game; break;
	case Category::PSP_Game:
		value = gui::cat_psp_game; break;
	case Category::Home:
		value = gui::cat_home; break;
	case Category::Media:
		value = gui::cat_audio_video; break;
	case Category::Data:
		value = gui::cat_game_data; break;
	case Category::Unknown_Cat:
		value = gui::cat_unknown; break;
	case Category::Others:
		value = gui::cat_other; break;
	default:
		cfg_log.warning("GetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	return GetValue(value).toBool();
}

void gui_settings::SetCategoryVisibility(int cat, const bool& val) const
{
	gui_save value;

	switch (cat)
	{
	case Category::HDD_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
	case Category::Home:
		value = gui::cat_home; break;
	case Category::PS1_Game:
		value = gui::cat_ps1_game; break;
	case Category::PS2_Game:
		value = gui::cat_ps2_game; break;
	case Category::PSP_Game:
		value = gui::cat_psp_game; break;
	case Category::Media:
		value = gui::cat_audio_video; break;
	case Category::Data:
		value = gui::cat_game_data; break;
	case Category::Unknown_Cat:
		value = gui::cat_unknown; break;
	case Category::Others:
		value = gui::cat_other; break;
	default:
		cfg_log.warning("SetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	SetValue(value, val);
}

void gui_settings::ShowBox(QMessageBox::Icon icon, const QString& title, const QString& text, const gui_save& entry, int* result = nullptr, QWidget* parent = nullptr, bool always_on_top = false)
{
	const std::string dialog_type = icon != QMessageBox::Information ? "Confirmation" : "Info";
	const bool has_gui_setting = !entry.name.isEmpty();

	if (has_gui_setting && !GetValue(entry).toBool())
	{
		cfg_log.notice("%s Dialog for Entry %s was ignored", dialog_type, sstr(entry.name));
		return;
	}

	const QFlags<QMessageBox::StandardButton> buttons = icon != QMessageBox::Information ? QMessageBox::Yes | QMessageBox::No : QMessageBox::Ok;

	QMessageBox* mb = new QMessageBox(icon, title, text, buttons, parent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | (always_on_top ? Qt::WindowStaysOnTopHint : Qt::Widget));
	mb->deleteLater();
	mb->setTextFormat(Qt::RichText);

	if (has_gui_setting && icon != QMessageBox::Critical)
	{
		mb->setCheckBox(new QCheckBox(tr("Don't show again")));
	}

	connect(mb, &QMessageBox::finished, [&](int res)
	{
		if (result)
		{
			*result = res;
		}

		const auto checkBox = mb->checkBox();

		if (checkBox && checkBox->isChecked())
		{
			SetValue(entry, false);
			cfg_log.notice("%s Dialog for Entry %s is now disabled", dialog_type, sstr(entry.name));
		}
	});

	mb->exec();
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
	}

	return true;
}

void gui_settings::SetGamelistColVisibility(int col, bool val) const
{
	SetValue(GetGuiSaveForColumn(col), val);
}

void gui_settings::SetCustomColor(int col, const QColor& val) const
{
	SetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::gl_icon_color), val);
}

logs::level gui_settings::GetLogLevel() const
{
	return logs::level(GetValue(gui::l_level).toUInt());
}

bool gui_settings::GetGamelistColVisibility(int col) const
{
	return GetValue(GetGuiSaveForColumn(col)).toBool();
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

gui_save gui_settings::GetGuiSaveForColumn(int col)
{
	// hide sound format, parental level, firmware version and path by default
	const bool show = col != gui::column_sound && col != gui::column_parental && col != gui::column_firmware && col != gui::column_path;
	return gui_save{ gui::game_list, "visibility_" + gui::get_game_list_column_name(static_cast<gui::game_list_columns>(col)), show };
}
