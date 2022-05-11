#pragma once

#include "vfs_dialog_path_widget.h"

#include <QDialog>
#include <QLineEdit>

namespace cfg
{
	struct device_info;
}

class gui_settings;

class vfs_dialog_usb_input : public QDialog
{
	Q_OBJECT

public:
	explicit vfs_dialog_usb_input(const QString& name, const cfg::device_info& default_info, cfg::device_info* info, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent = nullptr);

private:
	std::shared_ptr<gui_settings> m_gui_settings;
	gui_save m_gui_save;
	vfs_dialog_path_widget* m_path_widget;
	QLineEdit* m_vid_edit = nullptr;
	QLineEdit* m_pid_edit = nullptr;
	QLineEdit* m_serial_edit = nullptr;
};
