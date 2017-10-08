#pragma once

#include "gui_settings.h"
#include "emu_settings.h"

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
