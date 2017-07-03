#pragma once

#include "emu_settings.h"

#include <QLineEdit>
#include <QListWidget>
#include <QWidget>

#include <memory>

class core_tab : public QWidget
{
	Q_OBJECT

public:
	explicit core_tab(std::shared_ptr<emu_settings> xSettings, QWidget *parent = 0);
public Q_SLOTS:
	void SaveSelectedLibraries();

private:
	bool shouldSaveLibs = false;
	QListWidget *lleList;
	QLineEdit *searchBox;

	std::shared_ptr<emu_settings> xemu_settings;
};
