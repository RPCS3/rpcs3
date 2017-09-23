#include "vfs_dialog_tab.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

vfs_dialog_tab::vfs_dialog_tab(const vfs_settings_info& settingsInfo, gui_settings* guiSettings, emu_settings* emuSettings, QWidget* parent) : QWidget(parent),
m_info(settingsInfo), m_gui_settings(guiSettings), m_emu_settings(emuSettings)
{
	m_dirList = new QListWidget(this);

	QStringList alldirs = m_gui_settings->GetValue(m_info.listLocation).toStringList();

	// We must show the currently selected config.
	if (alldirs.contains(EmuConfigDir()) == false)
	{
		new QListWidgetItem(EmuConfigDir(), m_dirList);
	}
	for (QString dir : alldirs)
	{
		new QListWidgetItem(dir, m_dirList);
	}

	m_dirList->setMinimumWidth(m_dirList->sizeHintForColumn(0));

	QHBoxLayout* selectedConfigLayout = new QHBoxLayout;
	QLabel* selectedMessage = new QLabel(m_info.name + " directory:");
	m_selectedConfigLabel = new QLabel();
	m_selectedConfigLabel->setText(EmuConfigDir());
	if (m_selectedConfigLabel->text() == "")
	{
		m_selectedConfigLabel->setText(EmptyPath);
	}
	selectedConfigLayout->addWidget(selectedMessage);
	selectedConfigLayout->addWidget(m_selectedConfigLabel);
	selectedConfigLayout->addStretch();

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_dirList);
	vbox->addLayout(selectedConfigLayout);

	setLayout(vbox);

	connect(m_dirList, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item)
	{
		if (item->text() == "")
		{
			m_selectedConfigLabel->setText(EmptyPath);
		}
		else
		{
			m_selectedConfigLabel->setText(item->text());
		}
	});
}

void vfs_dialog_tab::SaveSettings()
{
	QStringList allDirs;
	for (int i = 0; i < m_dirList->count(); ++i)
	{
		allDirs += m_dirList->item(i)->text();
	}

	m_gui_settings->SetValue(m_info.listLocation, allDirs);
	if (m_selectedConfigLabel->text() == EmptyPath)
	{
		m_info.cfg_node->from_string("");
		m_emu_settings->SetSetting(m_info.settingLoc, "");
	}
	else
	{
		m_info.cfg_node->from_string(sstr(m_selectedConfigLabel->text()));
		m_emu_settings->SetSetting(m_info.settingLoc, sstr(m_selectedConfigLabel->text()));
	}
	m_emu_settings->SaveSettings();
}

void vfs_dialog_tab::Reset()
{
	m_dirList->clear();
	m_info.cfg_node->from_default();
	m_selectedConfigLabel->setText(EmuConfigDir());
	if (m_selectedConfigLabel->text() == "")
	{
		m_selectedConfigLabel->setText(EmptyPath);
	}
	m_dirList->addItem(new QListWidgetItem(EmuConfigDir()));
	m_gui_settings->SetValue(m_info.listLocation, QStringList(EmuConfigDir()));
}

void vfs_dialog_tab::AddNewDirectory()
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, "Choose a directory", QCoreApplication::applicationDirPath());
	if (dir != "")
	{
		if (dir.endsWith("/") == false) dir += '/';
		new QListWidgetItem(dir, m_dirList);
		m_selectedConfigLabel->setText(dir);
	}
}

QString vfs_dialog_tab::EmuConfigDir()
{
	return qstr(m_info.cfg_node->to_string());
}
