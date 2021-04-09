#pragma once

#include <QDialog>
#include <QTreeWidgetItem>
#include <QDragMoveEvent>
#include <QMimeData>
#include <set>
#include "Emu\GameInfo.h"
#include <iostream>
namespace Ui
{
	class game_update_manager_dialog;
}

class downloader;
class gui_settings;
class persistent_settings;
class game_compatibility;

class game_update_manager_dialog : public QDialog
{
	Q_OBJECT
	struct gui_patch_info
	{
		QString serial;
		QString Title;
		QString currentver;
		QString size;
		QString hash;
		QString rsysv;
		QString pv;
		QString url;
	};

public:
	explicit game_update_manager_dialog(GameInfo games, std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, game_compatibility* compat, QWidget* parent = nullptr);
	~game_update_manager_dialog();
	int exec() override;

private Q_SLOTS:
	void handle_item_selected(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/);

private:
	downloader* m_downloader = nullptr;
	downloader* P_m_downloader = nullptr;
	game_update_manager_dialog* m_game_update_manager_dialog;
	Ui::game_update_manager_dialog* ui;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<persistent_settings> m_persistent_settings;
	game_compatibility* m_game_compatibillty;
	GameInfo m_Gameinfo;
	void download_update(std::string Url);
	bool handle_XML(const QByteArray& data, std::string name, std::string app_ver);
	void update_patch_info(const game_update_manager_dialog::gui_patch_info& info);
	void download();
	void install(const QByteArray& data);

protected:
};
