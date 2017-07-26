#pragma once

#include "gui_settings.h"
#include "emu_settings.h"

#include <QTabWidget>
#include <QDialog>

class vfs_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit vfs_dialog(QWidget* parent = nullptr);
private:
	gui_settings m_gui_settings;
	emu_settings m_emu_settings;

	QTabWidget* tabs;
};
