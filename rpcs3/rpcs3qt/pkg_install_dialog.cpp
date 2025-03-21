#include "pkg_install_dialog.h"
#include "game_compatibility.h"
#include "numbered_widget_item.h"
#include "richtext_item_delegate.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

enum Roles
{
	FullPathRole    = Qt::UserRole + 0,
	ChangelogRole   = Qt::UserRole + 1,
	TitleRole       = Qt::UserRole + 2,
	TitleIdRole     = Qt::UserRole + 3,
	VersionRole     = Qt::UserRole + 4,
};

pkg_install_dialog::pkg_install_dialog(const QStringList& paths, game_compatibility* compat, QWidget* parent)
	: QDialog(parent)
{
	m_dir_list = new QListWidget(this);
	m_dir_list->setItemDelegate(new richtext_item_delegate(m_dir_list->itemDelegate()));

	for (const QString& path : paths)
	{
		const compat::package_info info = game_compatibility::GetPkgInfo(path, compat);
		if (!info.is_valid)
		{
			continue;
		}

		const QFileInfo file_info(path);

		// We have to build our complicated localized string in some annoying manner
		QString accumulated_info;
		QString tooltip;

		const auto append_comma = [&accumulated_info]()
		{
			if (!accumulated_info.isEmpty())
			{
				accumulated_info += ", ";
			}
		};

		if (!info.title_id.isEmpty())
		{
			accumulated_info = info.title_id;
		}

		if (info.type != compat::package_type::other)
		{
			append_comma();

			if (info.type == compat::package_type::dlc)
			{
				accumulated_info += tr("DLC", "Package type info (DLC)");
			}
			else
			{
				accumulated_info += tr("Update", "Package type info (Update)");
			}
		}
		else if (!info.local_cat.isEmpty())
		{
			append_comma();
			accumulated_info += info.local_cat;
		}

		if (!info.version.isEmpty())
		{
			append_comma();
			accumulated_info += tr("v.%0", "Version info").arg(info.version);
		}

		if (info.changelog.isEmpty())
		{
			tooltip = tr("No info", "Changelog info placeholder");
		}
		else
		{
			tooltip = tr("Changelog:\n\n%0", "Changelog info").arg(info.changelog);
		}

		append_comma();
		accumulated_info += file_info.fileName();

		const QString text = tr("<b>%0</b> (%2)", "Package text").arg(info.title.simplified()).arg(accumulated_info);

		QListWidgetItem* item = new numbered_widget_item(text, m_dir_list);
		item->setData(Roles::FullPathRole, info.path);
		item->setData(Roles::ChangelogRole, info.changelog);
		item->setData(Roles::TitleRole, info.title);
		item->setData(Roles::TitleIdRole, info.title_id);
		item->setData(Roles::VersionRole, info.version);
		item->setToolTip(tooltip);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Checked);
	}

	m_dir_list->sortItems();
	m_dir_list->setCurrentRow(0);
	m_dir_list->setMinimumWidth((m_dir_list->sizeHintForColumn(0) * 125) / 100);

	// Create buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
	buttons->button(QDialogButtonBox::Ok)->setText(tr("Install"));
	buttons->button(QDialogButtonBox::Ok)->setDefault(true);

	connect(buttons, &QDialogButtonBox::clicked, this, [this, buttons](QAbstractButton* button)
	{
		if (button == buttons->button(QDialogButtonBox::Ok))
		{
			accept();
		}
		else if (button == buttons->button(QDialogButtonBox::Cancel))
		{
			reject();
		}
	});

	connect(m_dir_list, &QListWidget::itemChanged, this, [this, buttons](QListWidgetItem*)
	{
		bool any_checked = false;
		for (int i = 0; i < m_dir_list->count(); i++)
		{
			if (m_dir_list->item(i)->checkState() == Qt::Checked)
			{
				any_checked = true;
				break;
			}
		}

		buttons->button(QDialogButtonBox::Ok)->setEnabled(any_checked);
	});

	QToolButton* move_up = new QToolButton;
	move_up->setArrowType(Qt::UpArrow);
	move_up->setToolTip(tr("Move selected item up"));
	connect(move_up, &QToolButton::clicked, this, [this]() { MoveItem(-1); });

	QToolButton* move_down = new QToolButton;
	move_down->setArrowType(Qt::DownArrow);
	move_down->setToolTip(tr("Move selected item down"));
	connect(move_down, &QToolButton::clicked, this, [this]() { MoveItem(1); });

	QHBoxLayout* hbox = new QHBoxLayout;
	hbox->addStretch();
	hbox->addWidget(move_up);
	hbox->addWidget(move_down);

	QLabel* description = new QLabel(tr("You are about to install multiple packages.\nReorder and/or exclude them if needed, then click \"Install\" to proceed."));

	QVBoxLayout* vbox = new QVBoxLayout;
	vbox->addWidget(description);
	vbox->addLayout(hbox);
	vbox->addWidget(m_dir_list);
	vbox->addWidget(buttons);

	setLayout(vbox);
	setWindowTitle(tr("Batch PKG Installation"));
	setObjectName("pkg_install_dialog");
}

void pkg_install_dialog::MoveItem(int offset) const
{
	const int src_index = m_dir_list->currentRow();
	const int dest_index = src_index + offset;

	if (src_index >= 0 && src_index < m_dir_list->count() &&
		dest_index >= 0 && dest_index < m_dir_list->count())
	{
		QListWidgetItem* item = m_dir_list->takeItem(src_index);
		m_dir_list->insertItem(dest_index, item);
		m_dir_list->setCurrentItem(item);
	}
}

std::vector<compat::package_info> pkg_install_dialog::GetPathsToInstall() const
{
	std::vector<compat::package_info> result;

	for (int i = 0; i < m_dir_list->count(); i++)
	{
		const QListWidgetItem* item = m_dir_list->item(i);
		if (item && item->checkState() == Qt::Checked)
		{
			compat::package_info info;
			info.path      = item->data(Roles::FullPathRole).toString();
			info.title     = item->data(Roles::TitleRole).toString();
			info.title_id  = item->data(Roles::TitleIdRole).toString();
			info.changelog = item->data(Roles::ChangelogRole).toString();
			info.version   = item->data(Roles::VersionRole).toString();
			result.push_back(info);
		}
	}

	return result;
}
