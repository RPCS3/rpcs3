#pragma once

#include "gui_settings.h"

#include <QListWidget>
#include <QLabel>

#include <memory>

namespace cfg
{
	class string;
}

class vfs_dialog_path_widget : public QWidget
{
	Q_OBJECT

public:
	explicit vfs_dialog_path_widget(const QString& name, const QString& current_path, QString default_path, gui_save list_location, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);
	QStringList get_dir_list() const;
	std::string get_selected_path() const;

	// Reset this widget without saving the settings yet
	void reset();

protected:
	QListWidgetItem* add_directory(const QString& path);
	void add_new_directory();
	void remove_directory();

	void update_selection();

	const QString EmptyPath = tr("Empty Path");

	QString m_default_path;
	gui_save m_list_location;
	std::shared_ptr<gui_settings> m_gui_settings;

	// UI variables needed in higher scope
	QListWidget* m_dir_list = nullptr;
	QLabel* m_selected_config_label = nullptr;
	bool m_is_changing_data = false;
};
