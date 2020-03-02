#include "pkg_install_dialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

constexpr int FullPathRole = Qt::UserRole + 0;
constexpr int BaseDisplayRole = Qt::UserRole + 1;

pkg_install_dialog::pkg_install_dialog(const QStringList& paths, QWidget* parent)
	: QDialog(parent)
{
	m_dir_list = new QListWidget(this);

	class numbered_widget_item final : public QListWidgetItem
	{
	public:
		explicit numbered_widget_item(const QString& text, QListWidget* listview = nullptr, int type = QListWidgetItem::Type)
			: QListWidgetItem(text, listview, type)
		{
		}

		QVariant data(int role) const override
		{
			QVariant result;
			switch (role)
			{
			case Qt::DisplayRole:
				result = QStringLiteral("%1. %2").arg(listWidget()->row(this) + 1).arg(data(BaseDisplayRole).toString());
				break;
			case BaseDisplayRole:
				result = QListWidgetItem::data(Qt::DisplayRole);
				break;
			default:
				result = QListWidgetItem::data(role);
				break;
			}
			return result;
		}

		bool operator<(const QListWidgetItem& other) const override
		{
			return data(BaseDisplayRole).toString() < other.data(BaseDisplayRole).toString();
		}
	};

	for (const QString& path : paths)
	{
		QListWidgetItem* item = new numbered_widget_item(QFileInfo(path).fileName(), m_dir_list);
		// Save full path in a custom data role
		item->setData(FullPathRole, path);
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

	connect(buttons, &QDialogButtonBox::clicked, [this, buttons](QAbstractButton* button)
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

	connect(m_dir_list, &QListWidget::itemChanged, [this, buttons](QListWidgetItem*)
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
	connect(move_up, &QToolButton::clicked, [this]() { MoveItem(-1); });

	QToolButton* move_down = new QToolButton;
	move_down->setArrowType(Qt::DownArrow);
	move_down->setToolTip(tr("Move selected item down"));
	connect(move_down, &QToolButton::clicked, [this]() { MoveItem(1); });

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

void pkg_install_dialog::MoveItem(int offset)
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

QStringList pkg_install_dialog::GetPathsToInstall() const
{
	QStringList result;

	for (int i = 0; i < m_dir_list->count(); i++)
	{
		const QListWidgetItem* item = m_dir_list->item(i);
		if (item->checkState() == Qt::Checked)
		{
			result.append(item->data(FullPathRole).toString());
		}
	}

	return result;
}
