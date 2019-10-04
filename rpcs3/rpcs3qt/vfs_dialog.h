#pragma once

#include "vfs_dialog_tab.h"

#include <QTabWidget>
#include <QDialog>

class vfs_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit vfs_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget* parent = nullptr);
private:
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
};
