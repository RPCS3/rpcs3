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
		QListWidgetItem* item = add_directory(dir);

		if (dir == current_path)
			selected_item = item;
	}

	// We must show the currently selected config.
	if (!selected_item)
		selected_item = add_directory(current_path);

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

	update_selection();

	connect(m_dir_list->model(), &QAbstractItemModel::dataChanged, this, [this](const QModelIndex&, const QModelIndex&, const QList<int>& roles)
	{
		if (m_is_changing_data || (!roles.empty() && !roles.contains(Qt::ItemDataRole::CheckStateRole)))
		{
			return;
		}

		// Check if an item was selected
		const QString selected_path = m_selected_config_label->text() == EmptyPath ? "" : m_selected_config_label->text();
		QAbstractItemModel* model = m_dir_list->model();

		for (int i = 0; i < model->rowCount(); ++i)
		{
			if (model->index(i, 0).data(Qt::ItemDataRole::CheckStateRole).toInt() != Qt::Checked) continue;

			const QString path = model->index(i, 0).data(Qt::ItemDataRole::DisplayRole).toString();
			if (path == selected_path) continue;

			// Select new path
			m_selected_config_label->setText(path.isEmpty() ? EmptyPath : path);
			update_selection();
			break;
		}
	});

	connect(m_dir_list, &QListWidget::currentRowChanged, this, [this, button_remove_dir](int row)
	{
		button_remove_dir->setEnabled(m_dir_list->item(row) && row > 0);
	});
}

void vfs_dialog_path_widget::reset()
{
	m_dir_list->clear();
	m_dir_list->setCurrentItem(add_directory(m_default_path));
	update_selection();
}

QListWidgetItem* vfs_dialog_path_widget::add_directory(const QString& path)
{
	// Make sure to only add a path once
	for (int i = 0; i < m_dir_list->count(); ++i)
	{
		if (m_dir_list->item(i)->text() == path)
		{
			return nullptr;
		}
	}

	QListWidgetItem* item = new QListWidgetItem(path, m_dir_list);
	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

	return item;
}

void vfs_dialog_path_widget::add_new_directory()
{
	QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Choose a directory"), QCoreApplication::applicationDirPath(), QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
		return;

	if (!dir.endsWith("/"))
		dir += '/';

	m_dir_list->setCurrentItem(add_directory(dir));
	update_selection();
}

void vfs_dialog_path_widget::remove_directory()
{
	if (const int row = m_dir_list->currentRow(); row > 0)
	{
		QListWidgetItem* item = m_dir_list->takeItem(row);
		delete item;

		update_selection();
	}
}

void vfs_dialog_path_widget::update_selection()
{
	const int count = m_dir_list->count();
	if (count <= 0) return;

	const QString selected_path = m_selected_config_label->text();
	bool found_path = false;

	m_is_changing_data = true;

	for (int i = 0; i < count; i++)
	{
		if (QListWidgetItem* item = m_dir_list->item(i))
		{
			const bool is_selected = item->text() == selected_path;
			item->setCheckState(is_selected ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

			if (is_selected)
			{
				m_dir_list->setCurrentItem(item);
				found_path = true;
			}
		}
	}

	if (!found_path)
	{
		QListWidgetItem* item = m_dir_list->item(0);
		m_selected_config_label->setText(item->text().isEmpty() ? EmptyPath : item->text());
		item->setCheckState(Qt::CheckState::Checked);
		m_dir_list->setCurrentItem(item);
	}

	m_is_changing_data = false;
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
