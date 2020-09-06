#pragma once

#include <QDialog>
#include <QTreeWidgetItem>
#include <QDragMoveEvent>
#include <QMimeData>

#include "Utilities/bin_patch.h"

namespace Ui
{
	class patch_manager_dialog;
}

class downloader;
class gui_settings;

class patch_manager_dialog : public QDialog
{
	Q_OBJECT

	struct gui_patch_info
	{
		QString hash;
		QString title;
		QString serial;
		QString app_version;
		QString author;
		QString notes;
		QString description;
		QString patch_version;
	};

	const QString tr_all_titles   = tr("All titles - Warning: These patches will apply globally to all games. Use with caution!");
	const QString tr_all_serials  = tr("All serials");
	const QString tr_all_versions = tr("All versions");

public:
	explicit patch_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::unordered_map<std::string, std::set<std::string>> games, QWidget* parent = nullptr);
	~patch_manager_dialog();

	int exec() override;

private Q_SLOTS:
	void filter_patches(const QString& term);
	void handle_item_selected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void handle_item_changed(QTreeWidgetItem *item, int column);
	void handle_custom_context_menu_requested(const QPoint& pos);
	void handle_legacy_patches_enabled(int state);
	void handle_show_owned_games_only(int state);

private:
	void refresh(bool restore_layout = false);
	void load_patches(bool show_error);
	void populate_tree();
	void save_config();
	void update_patch_info(const gui_patch_info& info);
	bool is_valid_file(const QMimeData& md, QStringList* drop_paths = nullptr);
	void download_update();
	bool handle_json(const QByteArray& data);

	std::shared_ptr<gui_settings> m_gui_settings;

	std::unordered_map<std::string, std::set<std::string>> m_owned_games;
	bool m_show_owned_games_only = false;

	patch_engine::patch_map m_map;
	bool m_legacy_patches_enabled = false;

	downloader* m_downloader = nullptr;

	Ui::patch_manager_dialog *ui;

protected:
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
};
