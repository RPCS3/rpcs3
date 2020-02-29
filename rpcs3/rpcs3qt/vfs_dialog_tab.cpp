#include "vfs_dialog_tab.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

vfs_dialog_tab::vfs_dialog_tab(vfs_settings_info settingsInfo, std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget* parent)
	: QWidget(parent), m_info(std::move(settingsInfo)), m_gui_settings(std::move(guiSettings)), m_emu_settings(std::move(emuSettings))
{
	m_dirList = new QListWidget(this);

	QStringList alldirs = m_gui_settings->GetValue(m_info.listLocation).toStringList();
	const QString current_dir = qstr(m_info.cfg_node->to_string());

	QListWidgetItem* selected_item = nullptr;

	for (const QString& dir : alldirs)
	{
		QListWidgetItem* item = new QListWidgetItem(dir, m_dirList);
		if (dir == current_dir)
			selected_item = item;
	}

	// We must show the currently selected config.
	if (!selected_item)
		selected_item = new QListWidgetItem(current_dir, m_dirList);

	selected_item->setSelected(true);

	m_dirList->setMinimumWidth(m_dirList->sizeHintForColumn(0));

	QPushButton* addDir = new QPushButton(QStringLiteral("+"));
	addDir->setToolTip(tr("Add new directory"));
	addDir->setFixedWidth(addDir->sizeHint().height()); // Make button square
	connect(addDir, &QAbstractButton::clicked, this, &vfs_dialog_tab::AddNewDirectory);

	QPushButton* removeDir = new QPushButton(QStringLiteral("-"));
	removeDir->setToolTip(tr("Remove directory"));
	removeDir->setFixedWidth(removeDir->sizeHint().height()); // Make button square
	removeDir->setEnabled(false);
	connect(removeDir, &QAbstractButton::clicked, this, &vfs_dialog_tab::RemoveDirectory);

	QHBoxLayout* selectedConfigLayout = new QHBoxLayout;
	m_selectedConfigLabel = new QLabel(current_dir.isEmpty() ? EmptyPath : current_dir);
	selectedConfigLayout->addWidget(new QLabel(m_info.name + tr(" directory:")));
	selectedConfigLayout->addWidget(m_selectedConfigLabel);
	selectedConfigLayout->addStretch();
	selectedConfigLayout->addWidget(addDir);
	selectedConfigLayout->addWidget(removeDir);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_dirList);
	vbox->addLayout(selectedConfigLayout);

	setLayout(vbox);

	connect(m_dirList, &QListWidget::currentItemChanged, [this](QListWidgetItem* current, QListWidgetItem*)
	{
		if (!current)
			return;

		m_selectedConfigLabel->setText(current->text().isEmpty() ? EmptyPath : current->text());
	});

	connect(m_dirList, &QListWidget::currentRowChanged, [this, removeDir](int row)
	{
		SetCurrentRow(row);
		removeDir->setEnabled(row > 0);
	});
}

void vfs_dialog_tab::SetSettings()
{
	QStringList allDirs;
	for (int i = 0; i < m_dirList->count(); ++i)
	{
		allDirs += m_dirList->item(i)->text();
	}
	m_gui_settings->SetValue(m_info.listLocation, allDirs);

	const std::string new_dir = m_selectedConfigLabel->text() == EmptyPath ? "" : sstr(m_selectedConfigLabel->text());
	m_info.cfg_node->from_string(new_dir);
	m_emu_settings->SetSetting(m_info.settingLoc, new_dir);
}

void vfs_dialog_tab::Reset()
{
	m_dirList->clear();
	m_dirList->setCurrentItem(new QListWidgetItem(qstr(m_info.cfg_node->def), m_dirList));
}

void vfs_dialog_tab::AddNewDirectory()
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Choose a directory"), QCoreApplication::applicationDirPath(), QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
		return;

	if (!dir.endsWith("/"))
		dir += '/';

	m_dirList->setCurrentItem(new QListWidgetItem(dir, m_dirList));
}

void vfs_dialog_tab::RemoveDirectory()
{
	QListWidgetItem* item = m_dirList->takeItem(m_currentRow);
	delete item;
}
