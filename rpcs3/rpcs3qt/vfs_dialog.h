#pragma once

#include <QDialog>

class gui_settings;

class vfs_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit vfs_dialog(std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);
private:
	std::shared_ptr<gui_settings> m_gui_settings;
};
