#pragma once

#include <QDialog>
#include <QTreeWidgetItem>
#include <QDragMoveEvent>
#include <QMimeData>

#include "gui_game_info.h"
#include "Utilities/bin_patch.h"
#include <unordered_map>

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
		QString config_value_key;
		std::map<QString, QVariant> config_values;
	};

	const QString tr_all_titles   = tr("All titles - Warning: These patches apply to all games!");
	const QString tr_all_serials  = tr("All serials");
	const QString tr_all_versions = tr("All versions");

public:
	explicit patch_manager_dialog(std::shared_ptr<gui_settings> gui_settings, const std::vector<game_info>& games, const std::string& title_id, const std::string& version, QWidget* parent = nullptr);
	~patch_manager_dialog();

	int exec() override;

private Q_SLOTS:
	void filter_patches(const QString& term);
	void handle_item_selected(QTreeWidgetItem* current, QTreeWidgetItem* previous);
	void handle_item_changed(QTreeWidgetItem* item, int column);
	void handle_config_value_changed(double value);
	void handle_custom_context_menu_requested(const QPoint& pos);
	void handle_show_owned_games_only(Qt::CheckState state);

private:
	void refresh(bool restore_layout = false);
	void load_patches(bool show_error);
	void populate_tree();
	void save_config() const;
	void update_patch_info(const gui_patch_info& info, bool force_update) const;
	static bool is_valid_file(const QMimeData& md, QStringList* drop_paths = nullptr);
	void download_update(bool automatic, bool auto_accept);
	bool handle_json(const QByteArray& data);

	std::shared_ptr<gui_settings> m_gui_settings;

	bool m_expand_current_match = false;
	QString m_search_version;

	std::unordered_map<std::string, std::set<std::string>> m_owned_games;
	bool m_show_owned_games_only = false;

	patch_engine::patch_map m_map;

	downloader* m_downloader = nullptr;
	bool m_download_automatic = false;
	bool m_download_auto_accept = false;

	std::unique_ptr<Ui::patch_manager_dialog> ui;

protected:
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void closeEvent(QCloseEvent* event) override;
};
