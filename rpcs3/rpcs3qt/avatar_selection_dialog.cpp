#include "stdafx.h"
#include "avatar_selection_dialog.h"

#include "Emu/NP/np_avatar.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/system_utils.hpp"
#include "Utilities/File.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtConcurrent>


LOG_CHANNEL(gui_log, "GUI");

constexpr int avatar_icon_size = 64;
constexpr int current_icon_size = 120;

avatar_selection_dialog::avatar_selection_dialog(const std::string& user_id, QWidget* parent)
	: QDialog(parent)
	, m_user_id(user_id)
{
	setWindowTitle(tr("Choose Avatar - User %0").arg(QString::fromStdString(m_user_id)));
	setMinimumSize(QSize(640, 520));

	// Current avatar
	m_current_icon = new QLabel(this);
	m_current_icon->setFixedSize(current_icon_size, current_icon_size);

	m_current_label = new QLabel(this);
	m_current_label->setWordWrap(true);

	QHBoxLayout* current_layout = new QHBoxLayout();
	current_layout->addWidget(m_current_icon);
	current_layout->addWidget(m_current_label, 1);

	// Avatar list
	m_list = new QListWidget(this);
	m_list->setViewMode(QListView::IconMode);
	m_list->setIconSize(QSize(avatar_icon_size, avatar_icon_size));
	m_list->setGridSize(QSize(avatar_icon_size + 16, avatar_icon_size + 16));
	m_list->setResizeMode(QListView::Adjust);
	m_list->setMovement(QListView::Static);
	m_list->setUniformItemSizes(true);
	m_list->setSelectionMode(QAbstractItemView::SingleSelection);

	m_status_label = new QLabel(this);
	m_status_label->setWordWrap(true);

	// Buttons
	QPushButton* browse_button = new QPushButton(tr("&Browse PNG..."), this);
	browse_button->setAutoDefault(false);
	browse_button->setToolTip(tr("Use a PNG image as avatar."));

	QPushButton* default_button = new QPushButton(tr("Use &Default Avatar"), this);
	default_button->setAutoDefault(false);

	m_select_button = new QPushButton(tr("&Select"), this);
	m_select_button->setEnabled(false);

	QPushButton* close_button = new QPushButton(tr("&Close"), this);
	close_button->setAutoDefault(false);

	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(browse_button);
	button_layout->addWidget(default_button);
	button_layout->addStretch();
	button_layout->addWidget(m_select_button);
	button_layout->addWidget(close_button);

	QVBoxLayout* main_layout = new QVBoxLayout();
	main_layout->addLayout(current_layout);
	main_layout->addWidget(m_list, 1);
	main_layout->addWidget(m_status_label);
	main_layout->addLayout(button_layout);
	setLayout(main_layout);

	connect(m_list, &QListWidget::itemSelectionChanged, this, [this]()
	{
		const QListWidgetItem* item = m_list->currentItem();
		m_select_button->setEnabled(item && item->isSelected() && item->data(usable_role).toBool());
	});
	connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item)
	{
		if (item && item->data(usable_role).toBool())
		{
			select_avatar(item->data(path_role).toString().toStdString());
		}
	});
	connect(m_select_button, &QAbstractButton::clicked, this, [this]()
	{
		if (const QListWidgetItem* item = m_list->currentItem(); item && item->data(usable_role).toBool())
		{
			select_avatar(item->data(path_role).toString().toStdString());
		}
	});
	connect(default_button, &QAbstractButton::clicked, this, [this](){ select_avatar({}); });
	connect(browse_button, &QAbstractButton::clicked, this, &avatar_selection_dialog::browse_avatar);
	connect(close_button, &QAbstractButton::clicked, this, &avatar_selection_dialog::close);

	update_current_avatar();
	load_avatars();
}

avatar_selection_dialog::~avatar_selection_dialog()
{
	m_watcher.cancel();
	m_watcher.waitForFinished();
}

QPixmap avatar_selection_dialog::get_user_avatar(const std::string& user_id, int size)
{
	const std::vector<u8> png = np::avatar::get_user_image(user_id, SCE_NP_AVATAR_SIZE_LARGE);

	QImage image;
	if (png.empty() || !image.loadFromData(png.data(), static_cast<int>(png.size()), "PNG"))
	{
		QPixmap placeholder(size, size);
		placeholder.fill(Qt::gray);
		return placeholder;
	}

	return QPixmap::fromImage(image.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void avatar_selection_dialog::load_avatars()
{
	m_paths = np::avatar::get_installed_avatars(m_user_id);

	QPixmap placeholder(avatar_icon_size, avatar_icon_size);
	placeholder.fill(Qt::transparent);

	const std::string avatar_dir = rpcs3::utils::get_hdd0_dir() + "home/" + m_user_id + "/psn_avatar/";

	for (const std::string& path : m_paths)
	{
		QListWidgetItem* item = new QListWidgetItem(QIcon(placeholder), QString(), m_list);
		item->setData(path_role, QString::fromStdString(path));
		item->setData(usable_role, false);
		item->setToolTip(QString::fromStdString(path.substr(avatar_dir.size())));
		item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		item->setSizeHint(QSize(avatar_icon_size + 12, avatar_icon_size + 12));
	}

	update_status();

	if (m_paths.empty())
	{
		return;
	}

	connect(&m_watcher, &QFutureWatcher<QImage>::resultReadyAt, this, [this](int index)
	{
		QListWidgetItem* item = m_list->item(index);
		const QImage image = m_watcher.resultAt(index);

		m_loaded_count++;

		if (!item)
		{
			return;
		}

		if (image.isNull())
		{
			// Missing RAP license or broken file
			m_unusable_count++;
			item->setHidden(true);
		}
		else
		{
			item->setIcon(QIcon(QPixmap::fromImage(image)));
			item->setData(usable_role, true);
			item->setFlags(item->flags() | Qt::ItemIsEnabled);

			if (item->data(path_role).toString().toStdString() == np::avatar::get_selected_avatar(m_user_id))
			{
				m_list->setCurrentItem(item);
			}
		}

		update_status();
	});
	connect(&m_watcher, &QFutureWatcher<QImage>::finished, this, [this]()
	{
		update_status();

		if (QListWidgetItem* item = m_list->currentItem())
		{
			m_list->scrollToItem(item);
		}
	});

	m_watcher.setFuture(QtConcurrent::mapped(m_paths, [](const std::string& path) -> QImage
	{
		if (!np::avatar::is_file_usable(path))
		{
			return {};
		}

		const std::vector<u8> png = np::avatar::load_file(path);

		QImage image;
		if (png.empty() || !image.loadFromData(png.data(), static_cast<int>(png.size()), "PNG"))
		{
			return {};
		}

		return image.scaled(avatar_icon_size, avatar_icon_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}));
}

void avatar_selection_dialog::update_current_avatar()
{
	m_current_icon->setPixmap(get_user_avatar(m_user_id, current_icon_size));

	const std::string selected = np::avatar::get_selected_avatar(m_user_id);

	if (selected.empty())
	{
		m_current_label->setText(tr("Current avatar: Default"));
	}
	else
	{
		m_current_label->setText(tr("Current avatar:\n%0").arg(QString::fromStdString(selected)));
	}
}

void avatar_selection_dialog::update_status()
{
	if (m_paths.empty())
	{
		m_status_label->setText(tr("No PSN avatars are installed for this user. You can install avatar packages (PKG) with File > Install Packages/Raps/Edats."));
		return;
	}

	QString text;

	if (m_watcher.isRunning())
	{
		text = tr("Loading avatars... %0 / %1").arg(m_loaded_count).arg(m_paths.size());
	}
	else
	{
		text = tr("%0 avatars available.").arg(m_paths.size() - m_unusable_count);
	}

	if (m_unusable_count > 0)
	{
		text += ' ' + tr("%0 avatars are hidden because their RAP license is missing or they could not be decrypted.").arg(m_unusable_count);
	}

	m_status_label->setText(text);
}

void avatar_selection_dialog::select_avatar(const std::string& path)
{
	if (!path.empty() && np::avatar::load_file(path).empty())
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to load the avatar:\n%0").arg(QString::fromStdString(path)));
		return;
	}

	if (!np::avatar::set_selected_avatar(m_user_id, path))
	{
		QMessageBox::warning(this, tr("Error"), tr("Failed to save the avatar of user %0.").arg(QString::fromStdString(m_user_id)));
		return;
	}

	update_current_avatar();
}

void avatar_selection_dialog::browse_avatar()
{
	const QString path = QFileDialog::getOpenFileName(this, tr("Select Avatar Image"), QString(), tr("Avatar Images (*.png *.edat);;All Files (*)"));

	if (path.isEmpty())
	{
		return;
	}

	select_avatar(path.toStdString());
}
