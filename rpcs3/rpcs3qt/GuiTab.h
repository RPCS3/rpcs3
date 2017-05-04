#ifndef GUITAB_H
#define GUITAB_H

#include "GuiSettings.h"

#include <QWidget>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

#include <memory>

class GuiTab : public QWidget
{
	Q_OBJECT

public:
	explicit GuiTab(std::shared_ptr<GuiSettings> guiSettings, QWidget *parent = 0);

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

	std::shared_ptr<GuiSettings> xGuiSettings;
};

#endif // GUITAB_H
