#include "vfs_dialog_tab.h"
#include "Utilities/Config.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

vfs_dialog_tab::vfs_dialog_tab(vfs_settings_info settingsInfo, std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget* parent)
	: QWidget(parent), m_info(std::move(settingsInfo)), m_gui_settings(std::move(guiSettings)), m_emu_settings(std::move(emuSettings))
{
	m_dir_dist = new QListWidget(this);

	QStringList alldirs = m_gui_settings->GetValue(m_info.listLocation).toStringList();
	const QString current_dir = qstr(m_info.cfg_node->to_string());

	QListWidgetItem* selected_item = nullptr;

	for (const QString& dir : alldirs)
	{
		QListWidgetItem* item = new QListWidgetItem(dir, m_dir_dist);
		if (dir == current_dir)
			selected_item = item;
	}

	// We must show the currently selected config.
	if (!selected_item)
		selected_item = new QListWidgetItem(current_dir, m_dir_dist);

	selected_item->setSelected(true);

	m_dir_dist->setMinimumWidth(m_dir_dist->sizeHintForColumn(0));

	QPushButton* addDir = new QPushButton(QStringLiteral("+"));
	addDir->setToolTip(tr("Add new directory"));
	addDir->setFixedWidth(addDir->sizeHint().height()); // Make button square
	connect(addDir, &QAbstractButton::clicked, this, &vfs_dialog_tab::AddNewDirectory);

	QPushButton* button_remove_dir = new QPushButton(QStringLiteral("-"));
	button_remove_dir->setToolTip(tr("Remove directory"));
	button_remove_dir->setFixedWidth(button_remove_dir->sizeHint().height()); // Make button square
	button_remove_dir->setEnabled(false);
	connect(button_remove_dir, &QAbstractButton::clicked, this, &vfs_dialog_tab::RemoveDirectory);

	QHBoxLayout* selected_config_layout = new QHBoxLayout;
	m_selected_config_label = new QLabel(current_dir.isEmpty() ? EmptyPath : current_dir);
	selected_config_layout->addWidget(new QLabel(tr("%0 directory:").arg(m_info.name)));
	selected_config_layout->addWidget(m_selected_config_label);
	selected_config_layout->addStretch();
	selected_config_layout->addWidget(addDir);
	selected_config_layout->addWidget(button_remove_dir);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_dir_dist);
	vbox->addLayout(selected_config_layout);

	setLayout(vbox);

	connect(m_dir_dist, &QListWidget::currentRowChanged, button_remove_dir, [this, button_remove_dir](int row)
	{
		QListWidgetItem* item = m_dir_dist->item(row);
		m_selected_config_label->setText((item && !item->text().isEmpty()) ? item->text() : EmptyPath);
		button_remove_dir->setEnabled(item && row > 0);
	});
}

void vfs_dialog_tab::SetSettings() const
{
	QStringList allDirs;
	for (int i = 0; i < m_dir_dist->count(); ++i)
	{
		allDirs += m_dir_dist->item(i)->text();
	}
	m_gui_settings->SetValue(m_info.listLocation, allDirs);

	const std::string new_dir = m_selected_config_label->text() == EmptyPath ? "" : sstr(m_selected_config_label->text());
	m_info.cfg_node->from_string(new_dir);
	m_emu_settings->SetSetting(m_info.settingLoc, new_dir);
}

void vfs_dialog_tab::Reset() const
{
	m_dir_dist->clear();
	m_dir_dist->setCurrentItem(new QListWidgetItem(qstr(m_info.cfg_node->def), m_dir_dist));
}

void vfs_dialog_tab::AddNewDirectory() const
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Choose a directory"), QCoreApplication::applicationDirPath(), QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
		return;

	if (!dir.endsWith("/"))
		dir += '/';

	m_dir_dist->setCurrentItem(new QListWidgetItem(dir, m_dir_dist));
}

void vfs_dialog_tab::RemoveDirectory() const
{
	const int row = m_dir_dist->currentRow();
	if (row > 0)
	{
		QListWidgetItem* item = m_dir_dist->item(row);
		delete item;
	}
}
