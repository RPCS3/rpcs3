#pragma once

#include <QDialog>
#include <QTreeWidgetItem>

#include "Utilities/bin_patch.h"

namespace Ui
{
	class patch_manager_dialog;
}

class patch_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit patch_manager_dialog(QWidget* parent = nullptr);
	~patch_manager_dialog();

private Q_SLOTS:
	void filter_patches(const QString& term);
	void on_item_selected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void on_item_changed(QTreeWidgetItem *item, int column);
	void on_custom_context_menu_requested(const QPoint& pos);

private:
	void load_patches();
	void populate_tree();
	void save();

	void update_patch_info(const patch_engine::patch_info& info);

	patch_engine::patch_map m_map;

	Ui::patch_manager_dialog *ui;
};
