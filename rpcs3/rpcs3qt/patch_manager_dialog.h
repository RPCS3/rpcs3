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

public:
	explicit patch_manager_dialog(std::shared_ptr<gui_settings> gui_settings, QWidget* parent = nullptr);
	~patch_manager_dialog();

	int exec() override;

private Q_SLOTS:
	void filter_patches(const QString& term);
	void on_item_selected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void on_item_changed(QTreeWidgetItem *item, int column);
	void on_custom_context_menu_requested(const QPoint& pos);
	void on_legacy_patches_enabled(int state);

private:
	void refresh(bool restore_layout = false);
	void load_patches();
	void populate_tree();
	void save_config();
	void update_patch_info(const gui_patch_info& info);
	bool is_valid_file(const QMimeData& md, QStringList* drop_paths = nullptr);

	std::shared_ptr<gui_settings> m_gui_settings;

	patch_engine::patch_map m_map;
	bool m_legacy_patches_enabled = false;

	Ui::patch_manager_dialog *ui;

protected:
	void dropEvent(QDropEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
};
