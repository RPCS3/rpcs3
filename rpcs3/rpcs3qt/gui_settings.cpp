#include "gui_settings.h"

#include "game_list_frame.h"

#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

gui_settings::gui_settings(QObject* parent) : QObject(parent), settings(ComputeSettingsDir() + tr("CurrentSettings") + ".ini", QSettings::Format::IniFormat, parent),
	settingsDir(ComputeSettingsDir())
{
}

gui_settings::~gui_settings()
{
	settings.sync();
}

QString gui_settings::ComputeSettingsDir()
{
	return QString::fromStdString(fs::get_config_dir()) + "/GuiConfigs/";
}

void gui_settings::ChangeToConfig(const QString& name)
{
	if (name != tr("CurrentSettings"))
	{ // don't try to change to yourself.
		Reset(false);

		QSettings other(settingsDir.absoluteFilePath(name + ".ini"), QSettings::IniFormat);

		QStringList keys = other.allKeys();
		for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
		{
			settings.setValue(*i, other.value(*i));
		}
		settings.sync();
	}
}

void gui_settings::Reset(bool removeMeta)
{
	if (removeMeta)
	{
		settings.clear();
	}
	else
	{
		settings.remove(GUI::logger);
		settings.remove(GUI::main_window);
		settings.remove(GUI::game_list);
	}
}

QVariant gui_settings::GetValue(const GUI_SAVE& entry)
{
	return settings.value(entry.key + "/" + entry.name, entry.def);
}

QVariant gui_settings::List2Var(const q_pair_list& list)
{
	QByteArray ba;
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << list;
	return QVariant(ba);
}

q_pair_list gui_settings::Var2List(const QVariant& var)
{
	q_pair_list list;
	QByteArray ba = var.toByteArray();
	QDataStream stream(&ba, QIODevice::ReadOnly);
	stream >> list;
	return list;
}

QIcon gui_settings::colorizedIcon(const QIcon& icon, const QColor& oldColor, const QColor& newColor, bool useSpecialMasks)
{
	QPixmap pixmap = icon.pixmap(icon.availableSizes().at(0));
	QBitmap mask = pixmap.createMaskFromColor(oldColor, Qt::MaskOutColor);
	pixmap.fill(newColor);
	pixmap.setMask(mask);

	// special masks for disc icon and others

	if (useSpecialMasks)
	{
		auto saturatedColor = [](const QColor& col, float sat /* must be < 1 */)
		{
			int r = col.red() + sat * (255 - col.red());
			int g = col.green() + sat * (255 - col.green());
			int b = col.blue() + sat * (255 - col.blue());
			return QColor(r, g, b, col.alpha());
		};

		QColor colorS1(Qt::white);
		QPixmap pixmapS1 = icon.pixmap(icon.availableSizes().at(0));
		QBitmap maskS1 = pixmapS1.createMaskFromColor(colorS1, Qt::MaskOutColor);
		pixmapS1.fill(colorS1);
		pixmapS1.setMask(maskS1);

		QColor colorS2(0, 173, 246, 255);
		QPixmap pixmapS2 = icon.pixmap(icon.availableSizes().at(0));
		QBitmap maskS2 = pixmapS2.createMaskFromColor(colorS2, Qt::MaskOutColor);
		pixmapS2.fill(saturatedColor(newColor, 0.6f));
		pixmapS2.setMask(maskS2);

		QColor colorS3(0, 132, 244, 255);
		QPixmap pixmapS3 = icon.pixmap(icon.availableSizes().at(0));
		QBitmap maskS3 = pixmapS3.createMaskFromColor(colorS3, Qt::MaskOutColor);
		pixmapS3.fill(saturatedColor(newColor, 0.3f));
		pixmapS3.setMask(maskS3);

		QPainter painter(&pixmap);
		painter.drawPixmap(QPoint(0, 0), pixmapS1);
		painter.drawPixmap(QPoint(0, 0), pixmapS2);
		painter.drawPixmap(QPoint(0, 0), pixmapS3);
		painter.end();
	}

	return QIcon(pixmap);
}

void gui_settings::SetValue(const GUI_SAVE& entry, const QVariant& value)
{
	settings.beginGroup(entry.key);
	settings.setValue(entry.name, value);
	settings.endGroup();
}

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;
	if (GetCategoryVisibility(Category::Non_Disc_Game)) filterList.append(category::non_disc_games);
	if (GetCategoryVisibility(Category::Disc_Game)) filterList.append(category::disc_Game);
	if (GetCategoryVisibility(Category::Home)) filterList.append(category::home);
	if (GetCategoryVisibility(Category::Media)) filterList.append(category::media);
	if (GetCategoryVisibility(Category::Data)) filterList.append(category::data);
	if (GetCategoryVisibility(Category::Unknown_Cat)) filterList.append(category::unknown);
	if (GetCategoryVisibility(Category::Others)) filterList.append(category::others);
	return filterList;
}

bool gui_settings::GetCategoryVisibility(int cat)
{
	GUI_SAVE value;

	switch (cat)
	{
	case Category::Non_Disc_Game:
		value = GUI::cat_hdd_game; break;
	case Category::Disc_Game:
		value = GUI::cat_disc_game; break;
	case Category::Home:
		value = GUI::cat_home; break;
	case Category::Media:
		value = GUI::cat_audio_video; break;
	case Category::Data:
		value = GUI::cat_game_data; break;
	case Category::Unknown_Cat:
		value = GUI::cat_unknown; break;
	case Category::Others:
		value = GUI::cat_other; break;
	default:
		LOG_WARNING(GENERAL, "GetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	return GetValue(value).toBool();
}

void gui_settings::SetCategoryVisibility(int cat, const bool& val)
{
	GUI_SAVE value;

	switch (cat)
	{
	case Category::Non_Disc_Game:
		value = GUI::cat_hdd_game; break;
	case Category::Disc_Game:
		value = GUI::cat_disc_game; break;
	case Category::Home:
		value = GUI::cat_home; break;
	case Category::Media:
		value = GUI::cat_audio_video; break;
	case Category::Data:
		value = GUI::cat_game_data; break;
	case Category::Unknown_Cat:
		value = GUI::cat_unknown; break;
	case Category::Others:
		value = GUI::cat_other; break;
	default:
		LOG_WARNING(GENERAL, "SetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	SetValue(value, val);
}

void gui_settings::ShowInfoBox(const GUI_SAVE& entry, const QString& title, const QString& text, QWidget* parent)
{
	if (GetValue(entry).toBool())
	{
		QCheckBox* cb = new QCheckBox(tr("Don't show again"));
		QMessageBox* mb = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
		mb->setCheckBox(cb);
		mb->deleteLater();
		mb->exec();
		if (mb->checkBox()->isChecked())
		{
			SetValue(entry, false);
			LOG_WARNING(GENERAL, "Entry %s was set to false", sstr(entry.name));
		}
	}
	else LOG_WARNING(GENERAL, "Entry %s is false, Info Box not shown", sstr(entry.name));
}

void gui_settings::SetGamelistColVisibility(int col, bool val)
{
	// hide sound format and parental level
	bool show = col != 8 && col != 9;
	SetValue(GUI_SAVE(GUI::game_list, "Col" + QString::number(col) + "visible", show), val);
}

void gui_settings::SetCustomColor(int col, const QColor& val)
{
	SetValue(GUI_SAVE(GUI::meta, "CustomColor" + QString::number(col), GUI::mw_tool_bar_color), val);
}

void gui_settings::SaveCurrentConfig(const QString& friendlyName)
{
	SetValue(GUI::m_currentConfig, friendlyName);
	BackupSettingsToTarget(friendlyName);
}

logs::level gui_settings::GetLogLevel()
{
	return (logs::level) GetValue(GUI::l_level).toUInt();
}

bool gui_settings::GetGamelistColVisibility(int col)
{
	// hide sound format and parental level
	bool show = col != 8 && col != 9;
	return GetValue(GUI_SAVE(GUI::game_list, "Col" + QString::number(col) + "visible", show)).toBool();
}

QColor gui_settings::GetCustomColor(int col)
{
	return GetValue(GUI_SAVE(GUI::meta, "CustomColor" + QString::number(col), GUI::mw_tool_bar_color)).value<QColor>();
}

QStringList gui_settings::GetConfigEntries()
{
	QStringList nameFilter;
	nameFilter << "*.ini";
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

void gui_settings::BackupSettingsToTarget(const QString& friendlyName)
{	
	QSettings target(ComputeSettingsDir() + friendlyName + ".ini", QSettings::Format::IniFormat);
	QStringList keys = settings.allKeys();
	for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
	{
		if (!i->startsWith(GUI::meta))
		{
			target.setValue(*i, settings.value(*i));
		}
	}
	target.sync();
}

QStringList gui_settings::GetStylesheetEntries()
{
	QStringList nameFilter;
	nameFilter << "*.qss";
	QString path = settingsDir.absolutePath();
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

QString gui_settings::GetCurrentStylesheetPath()
{
	return settingsDir.absoluteFilePath(GetValue(GUI::m_currentStylesheet).toString() + ".qss");
}
