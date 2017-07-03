#pragma once

#include "gui_settings.h"

#include <QWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

#include <memory>

class gui_tab : public QWidget
{
	Q_OBJECT

public:
	explicit gui_tab(std::shared_ptr<gui_settings> guiSettings, QWidget *parent = 0);

public Q_SLOTS:
	void Accept();
Q_SIGNALS:
	void GuiSettingsSyncRequest();
	void GuiSettingsSaveRequest();
	void GuiStylesheetRequest(const QString& path);
private Q_SLOTS:
	void OnResetDefault();
	void OnBackupCurrentConfig();
	void OnApplyConfig();
	void OnApplyStylesheet();
private:
	void AddConfigs();
	void AddStylesheets();

	QComboBox *combo_configs;
	QComboBox *combo_stylesheets;

	QString m_startingStylesheet;
	QString m_startingConfig;

	std::shared_ptr<gui_settings> xgui_settings;
};
