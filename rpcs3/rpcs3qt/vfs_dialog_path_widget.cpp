#include "vfs_dialog_path_widget.h"

#include <QFileDialog>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPushButton>

vfs_dialog_path_widget::vfs_dialog_path_widget(const QString& name, const QString& current_path, QString default_path, gui_save list_location, std::shared_ptr<gui_settings> _gui_settings, QWidget* parent)
	: QWidget(parent), m_default_path(std::move(default_path)), m_list_location(std::move(list_location)), m_gui_settings(std::move(_gui_settings))
{
	m_dir_list = new QListWidget(this);

	const QStringList all_dirs = m_gui_settings->GetValue(m_list_location).toStringList();

	QListWidgetItem* selected_item = nullptr;

	for (const QString& dir : all_dirs)
	{
		QListWidgetItem* item = new QListWidgetItem(dir, m_dir_list);
		if (dir == current_path)
			selected_item = item;
	}

	// We must show the currently selected config.
	if (!selected_item)
		selected_item = new QListWidgetItem(current_path, m_dir_list);

	selected_item->setSelected(true);

	m_dir_list->setMinimumWidth(m_dir_list->sizeHintForColumn(0));

	QPushButton* add_directory_button = new QPushButton(QStringLiteral("+"));
	add_directory_button->setToolTip(tr("Add new directory"));
	add_directory_button->setFixedWidth(add_directory_button->sizeHint().height()); // Make button square
	connect(add_directory_button, &QAbstractButton::clicked, this, &vfs_dialog_path_widget::add_new_directory);

	QPushButton* button_remove_dir = new QPushButton(QStringLiteral("-"));
	button_remove_dir->setToolTip(tr("Remove directory"));
	button_remove_dir->setFixedWidth(button_remove_dir->sizeHint().height()); // Make button square
	button_remove_dir->setEnabled(false);
	connect(button_remove_dir, &QAbstractButton::clicked, this, &vfs_dialog_path_widget::remove_directory);

	QHBoxLayout* selected_config_layout = new QHBoxLayout;
	m_selected_config_label = new QLabel(current_path.isEmpty() ? EmptyPath : current_path);
	selected_config_layout->addWidget(new QLabel(tr("Used %0 directory:").arg(name)));
	selected_config_layout->addWidget(m_selected_config_label);
	selected_config_layout->addStretch();
	selected_config_layout->addWidget(add_directory_button);
	selected_config_layout->addWidget(button_remove_dir);

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(m_dir_list);
	vbox->addLayout(selected_config_layout);

	setLayout(vbox);

	connect(m_dir_list, &QListWidget::currentRowChanged, this, [this, button_remove_dir](int row)
	{
		QListWidgetItem* item = m_dir_list->item(row);
		m_selected_config_label->setText((item && !item->text().isEmpty()) ? item->text() : EmptyPath);
		button_remove_dir->setEnabled(item && row > 0);
	});
}

void vfs_dialog_path_widget::reset() const
{
	m_dir_list->clear();
	m_dir_list->setCurrentItem(new QListWidgetItem(m_default_path, m_dir_list));
}

void vfs_dialog_path_widget::add_new_directory() const
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Choose a directory"), QCoreApplication::applicationDirPath(), QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
		return;

	if (!dir.endsWith("/"))
		dir += '/';

	m_dir_list->setCurrentItem(new QListWidgetItem(dir, m_dir_list));
}

void vfs_dialog_path_widget::remove_directory() const
{
	const int row = m_dir_list->currentRow();
	if (row > 0)
	{
		QListWidgetItem* item = m_dir_list->takeItem(row);
		delete item;
	}
}

QStringList vfs_dialog_path_widget::get_dir_list() const
{
	QStringList all_dirs;
	for (int i = 0; i < m_dir_list->count(); ++i)
	{
		all_dirs += m_dir_list->item(i)->text();
	}
	return all_dirs;
}

std::string vfs_dialog_path_widget::get_selected_path() const
{
	return m_selected_config_label->text() == EmptyPath ? "" : m_selected_config_label->text().toStdString();
}
