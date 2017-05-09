#include "GuiTab.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QAction>
#include <QDesktopServices>

GuiTab::GuiTab(std::shared_ptr<GuiSettings> xSettings, QWidget *parent) : QWidget(parent), xGuiSettings(xSettings)
{
	// Left Widgets
	// configs
	QGroupBox *gb_configs = new QGroupBox(tr("Gui configs"), this);
	QVBoxLayout *vbox_configs = new QVBoxLayout();
	QHBoxLayout *hbox_configs = new QHBoxLayout();
	combo_configs = new QComboBox(this);
	QPushButton *pb_apply_config = new QPushButton(tr("Apply"), this);
	// control buttons
	QGroupBox *gb_controls = new QGroupBox(tr("Gui controls"), this);
	QVBoxLayout *vbox_controls = new QVBoxLayout();
	QPushButton *pb_reset_default = new QPushButton(tr("Reset Gui to Default"), this);
	QPushButton *pb_backup_config = new QPushButton(tr("Save Current Configuration"), this);
	QPushButton *pb_open_folder = new QPushButton(tr("Open Config/Sheet Folder"));

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
	connect(pb_reset_default, &QAbstractButton::clicked, this, &GuiTab::OnResetDefault);
	connect(pb_backup_config, &QAbstractButton::clicked, this, &GuiTab::OnBackupCurrentConfig);
	connect(pb_apply_config, &QAbstractButton::clicked, this, &GuiTab::OnApplyConfig);
	connect(pb_apply_stylesheet, &QAbstractButton::clicked, this, &GuiTab::OnApplyStylesheet);
	connect(pb_open_folder, &QAbstractButton::clicked, [=]() {QDesktopServices::openUrl(xGuiSettings->GetSettingsDir()); });
	AddConfigs();
	AddStylesheets();
}

void GuiTab::Accept()
{
	OnApplyConfig();
	OnApplyStylesheet();
}

void GuiTab::AddConfigs()
{
	combo_configs->clear();
	
	combo_configs->addItem(tr("default"));

	for (QString entry : xGuiSettings->GetConfigEntries())
	{
		if (entry != tr("default"))
		{
			combo_configs->addItem(entry);
		}
	}

	QString currentSelection = "CurrentSettings";

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

void GuiTab::AddStylesheets()
{
	combo_stylesheets->clear();

	combo_stylesheets->addItem(tr("default"));

	for (QString entry : xGuiSettings->GetStylesheetEntries())
	{
		if (entry != tr("default"))
		{
			combo_stylesheets->addItem(entry);
		}
	}

	QString currentSelection = xGuiSettings->GetCurrentStylesheet();
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

void GuiTab::OnResetDefault()
{
	if (QMessageBox::question(this, tr("Reset Gui to default?"), tr("This will include your stylesheet as well. Do you wish to proceed?"), 
		QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
	{
		xGuiSettings->Reset(true);
		xGuiSettings->ChangeToConfig(tr("default"));
		emit GuiStylesheetRequest(tr("default"));
		emit GuiSettingsSyncRequest();
		AddConfigs();
		AddStylesheets();
	}
}

void GuiTab::OnBackupCurrentConfig()
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
		xGuiSettings->SaveCurrentConfig(friendlyName);
		combo_configs->addItem(friendlyName);
		combo_configs->setCurrentIndex(combo_configs->findText(friendlyName));
		break;
	}
}

void GuiTab::OnApplyConfig()
{
	QString name = combo_configs->currentText();
	xGuiSettings->SetCurrentConfig(name);
	xGuiSettings->ChangeToConfig(name);
	emit GuiSettingsSyncRequest();
}

void GuiTab::OnApplyStylesheet()
{
	xGuiSettings->SetStyleSheet(combo_stylesheets->currentText());
	emit GuiStylesheetRequest(xGuiSettings->GetCurrentStylesheetPath());
}
