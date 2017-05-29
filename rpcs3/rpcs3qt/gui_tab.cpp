#include "gui_tab.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QAction>
#include <QDesktopServices>
#include <QCheckBox>

gui_tab::gui_tab(std::shared_ptr<gui_settings> xSettings, QWidget *parent) : QWidget(parent), xgui_settings(xSettings)
{
	// Left Widgets
	// configs
	QGroupBox *gb_configs = new QGroupBox(tr("GUI Configs"), this);
	QVBoxLayout *vbox_configs = new QVBoxLayout();
	QHBoxLayout *hbox_configs = new QHBoxLayout();
	combo_configs = new QComboBox(this);
	QPushButton *pb_apply_config = new QPushButton(tr("Apply"), this);
	// control buttons
	QGroupBox *gb_controls = new QGroupBox(tr("GUI Controls"), this);
	QVBoxLayout *vbox_controls = new QVBoxLayout();
	QPushButton *pb_reset_default = new QPushButton(tr("Reset GUI to Default"), this);
	QPushButton *pb_backup_config = new QPushButton(tr("Save Current Configuration"), this);
	QPushButton *pb_open_folder = new QPushButton(tr("Open Config/Sheet Folder"), this);
	QCheckBox *cb_show_welcome = new QCheckBox(tr("Show Welcome Screen"), this);
	cb_show_welcome->setChecked(xSettings->GetValue(GUI::ib_show_welcome).toBool());

	// Right Widgets
	QGroupBox *gb_stylesheets = new QGroupBox(tr("Stylesheets"), this);
	QVBoxLayout *vbox_stylesheets = new QVBoxLayout();
	QHBoxLayout *hbox_stylesheets = new QHBoxLayout();
	combo_stylesheets = new QComboBox(this);
	QPushButton *pb_apply_stylesheet = new QPushButton(tr("Apply"), this);

	// Left layout
	QVBoxLayout *vbox_left = new QVBoxLayout();

	hbox_configs->addWidget(pb_apply_config);
	vbox_configs->addWidget(combo_configs);
	vbox_configs->addLayout(hbox_configs);
	gb_configs->setLayout(vbox_configs);

	vbox_controls->addWidget(cb_show_welcome);
	vbox_controls->addWidget(pb_reset_default);
	vbox_controls->addWidget(pb_backup_config);
	vbox_controls->addWidget(pb_open_folder);
	gb_controls->setLayout(vbox_controls);

	vbox_left->addWidget(gb_configs);
	vbox_left->addWidget(gb_controls);
	vbox_left->addStretch(1);

	// Right layout
	QVBoxLayout *vbox_right = new QVBoxLayout();
	hbox_stylesheets->addWidget(pb_apply_stylesheet);
	vbox_stylesheets->addWidget(combo_stylesheets);
	vbox_stylesheets->addLayout(hbox_stylesheets);
	gb_stylesheets->setLayout(vbox_stylesheets);
	vbox_right->addWidget(gb_stylesheets);
	vbox_right->addStretch(1);

	// Main Layout
	QHBoxLayout *hbox = new QHBoxLayout();
	hbox->addLayout(vbox_left);
	hbox->addLayout(vbox_right);
	setLayout(hbox);

	// Connections
	connect(pb_reset_default, &QAbstractButton::clicked, this, &gui_tab::OnResetDefault);
	connect(pb_backup_config, &QAbstractButton::clicked, this, &gui_tab::OnBackupCurrentConfig);
	connect(pb_apply_config, &QAbstractButton::clicked, this, &gui_tab::OnApplyConfig);
	connect(pb_apply_stylesheet, &QAbstractButton::clicked, this, &gui_tab::OnApplyStylesheet);
	connect(pb_open_folder, &QAbstractButton::clicked, [=]() {QDesktopServices::openUrl(xgui_settings->GetSettingsDir()); });
	connect(cb_show_welcome, &QCheckBox::clicked, [=](bool val) {xSettings->SetValue(GUI::ib_show_welcome, val); });

	AddConfigs();
	AddStylesheets();
}

void gui_tab::Accept()
{
	// Only attempt to load a config if changes occurred.
	if (m_startingConfig != xgui_settings->GetValue(GUI::m_currentConfig).toString())
	{
		OnApplyConfig();
	}
	if (m_startingStylesheet != xgui_settings->GetValue(GUI::m_currentStylesheet).toString())
	{
		OnApplyStylesheet();
	}
}

void gui_tab::AddConfigs()
{
	combo_configs->clear();
	
	combo_configs->addItem(tr("default"));

	for (QString entry : xgui_settings->GetConfigEntries())
	{
		if (entry != tr("default"))
		{
			combo_configs->addItem(entry);
		}
	}

	QString currentSelection = tr("CurrentSettings");
	m_startingConfig = currentSelection;

	int index = combo_configs->findText(currentSelection);
	if (index != -1)
	{
		combo_configs->setCurrentIndex(index);
	}
	else
	{
		LOG_WARNING(GENERAL, "Trying to set an invalid config index ", index);
	}
}

void gui_tab::AddStylesheets()
{
	combo_stylesheets->clear();

	combo_stylesheets->addItem(tr("default"));

	for (QString entry : xgui_settings->GetStylesheetEntries())
	{
		if (entry != tr("default"))
		{
			combo_stylesheets->addItem(entry);
		}
	}

	QString currentSelection = xgui_settings->GetValue(GUI::m_currentStylesheet).toString();
	m_startingStylesheet = currentSelection;

	int index = combo_stylesheets->findText(currentSelection);
	if (index != -1)
	{
		combo_stylesheets->setCurrentIndex(index);
	}
	else
	{
		LOG_WARNING(GENERAL, "Trying to set an invalid stylesheets index ", index);
	}
}

void gui_tab::OnResetDefault()
{
	if (QMessageBox::question(this, tr("Reset GUI to default?"), tr("This will include your stylesheet as well. Do you wish to proceed?"), 
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		xgui_settings->Reset(true);
		xgui_settings->ChangeToConfig(tr("default"));
		emit GuiStylesheetRequest(tr("default"));
		emit GuiSettingsSyncRequest();
		AddConfigs();
		AddStylesheets();
	}
}

void gui_tab::OnBackupCurrentConfig()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Choose a unique name"));
	dialog->setLabelText(tr("Configuration Name: "));
	dialog->resize(500, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(500, 100);
		QString friendlyName = dialog->textValue();
		if (friendlyName == "")
		{
			QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
			continue;
		}
		if (friendlyName.contains("."))
		{
			QMessageBox::warning(this, tr("Error"), tr("Must choose a name with no '.'"));
			continue;
		}
		if (combo_configs->findText(friendlyName) != -1)
		{
			QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
			continue;
		}
		emit GuiSettingsSaveRequest();
		xgui_settings->SaveCurrentConfig(friendlyName);
		combo_configs->addItem(friendlyName);
		combo_configs->setCurrentIndex(combo_configs->findText(friendlyName));
		break;
	}
}

void gui_tab::OnApplyConfig()
{
	QString name = combo_configs->currentText();
	xgui_settings->SetValue(GUI::m_currentConfig, name);
	xgui_settings->ChangeToConfig(name);
	emit GuiSettingsSyncRequest();
}

void gui_tab::OnApplyStylesheet()
{
	xgui_settings->SetValue(GUI::m_currentStylesheet, combo_stylesheets->currentText());
	emit GuiStylesheetRequest(xgui_settings->GetCurrentStylesheetPath());
}
