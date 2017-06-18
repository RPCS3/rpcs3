#include "vfs_dialog_tab.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

vfs_dialog_tab::vfs_dialog_tab(const vfs_settings_info& settingsInfo, gui_settings* guiSettings, emu_settings* emuSettings, QWidget* parent) : QWidget(parent),
m_info(settingsInfo), m_gui_settings(guiSettings), m_emu_settings(emuSettings)
{
	dirList = new QListWidget(this);

	QStringList alldirs = m_gui_settings->GetValue(m_info.listLocation).toStringList();

	// We must show the currently selected config.
	if (alldirs.contains(EmuConfigDir()) == false)
	{
		new QListWidgetItem(EmuConfigDir(), dirList);
	}
	for (QString dir : alldirs)
	{
		new QListWidgetItem(dir, dirList);
	}
	dirList->setMinimumWidth(dirList->sizeHintForColumn(0));

	QHBoxLayout* selectedConfigLayout = new QHBoxLayout;
	QLabel* selectedMessage = new QLabel(m_info.name + " directory:");
	selectedConfigLabel = new QLabel();
	selectedConfigLabel->setText(EmuConfigDir());
	selectedConfigLayout->addWidget(selectedMessage);
	selectedConfigLayout->addWidget(selectedConfigLabel);
	selectedConfigLayout->addStretch();

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(dirList);
	vbox->addLayout(selectedConfigLayout);

	setLayout(vbox);

	connect(dirList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item)
	{
		selectedConfigLabel->setText(item->text());
	});
}

void vfs_dialog_tab::SaveSettings()
{
	QStringList allDirs;
	for (int i = 0; i < dirList->count(); ++i)
	{
		allDirs += dirList->item(i)->text();
	}

	m_gui_settings->SetValue(m_info.listLocation, allDirs);
	m_info.cfg_node->from_string(sstr(selectedConfigLabel->text()));
	m_emu_settings->SetSetting(m_info.settingLoc, sstr(selectedConfigLabel->text()));
	m_emu_settings->SaveSettings();
}

void vfs_dialog_tab::Reset()
{
	dirList->clear();
	m_info.cfg_node->from_default();
	selectedConfigLabel->setText(EmuConfigDir());
	dirList->addItem(new QListWidgetItem(EmuConfigDir()));
	m_gui_settings->SetValue(m_info.listLocation, QStringList(EmuConfigDir()));
}

void vfs_dialog_tab::AddNewDirectory()
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, "Choose a directory", QCoreApplication::applicationDirPath());
	if (dir != "")
	{
		if (dir.endsWith("/") == false) dir += '/';
		new QListWidgetItem(dir, dirList);
		selectedConfigLabel->setText(dir);
	}
}

QString vfs_dialog_tab::EmuConfigDir()
{
	return qstr(m_info.cfg_node->to_string());
}
