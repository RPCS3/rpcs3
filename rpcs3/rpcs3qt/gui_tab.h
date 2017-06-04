#ifndef GUITAB_H
#define GUITAB_H

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

public slots:
	void Accept();
signals:
	void GuiSettingsSyncRequest();
	void GuiSettingsSaveRequest();
	void GuiStylesheetRequest(const QString& path);
private slots:
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

#endif // GUITAB_H
