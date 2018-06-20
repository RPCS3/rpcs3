#include "user_manager_dialog.h"
#include "table_item_delegate.h"

#include "Utilities/StrUtil.h"

#include <QRegExpValidator>
#include <QInputDialog>

namespace
{
	std::vector<UserAccount*> GetUserAccounts(const std::string& base_dir)
	{
		std::vector<UserAccount*> user_list;

		// I believe this gets the folder list sorted alphabetically by default,
		// but I can't find proof of this always being true.
		for (const auto& user_folder : fs::dir(base_dir))
		{
			if (!user_folder.is_directory)
			{
				continue;
			}

			// Is the folder name exactly 8 all-numerical characters long?
			// We use strtol to find any non-numeric characters in folder name.
			char* non_numeric_char;
			std::strtol(user_folder.name.c_str(), &non_numeric_char, 10);
			if (user_folder.name.length() != 8 || *non_numeric_char != '\0')
			{
				continue;
			}

			// Does the localusername file exist?
			if (!fs::is_file(base_dir + "/" + user_folder.name + "/localusername"))
			{
				continue;
			}

			user_list.emplace_back(new UserAccount(user_folder.name));
		}
		return user_list;
	}
}

user_manager_dialog::user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, QWidget* parent)
	: QDialog(parent), m_user_list(), m_sort_column(1), m_sort_ascending(true), m_gui_settings(gui_settings), m_emu_settings(emu_settings)
{
	setWindowTitle(tr("User Manager"));
	setMinimumSize(QSize(400, 400));
	setModal(true);

	m_emu_settings->LoadSettings();
	Init();
}

void user_manager_dialog::Init()
{
	// Table
	m_table = new QTableWidget(this);
	m_table->setItemDelegate(new table_item_delegate(this)); // to get rid of cell selection rectangles
	m_table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_table->setColumnCount(2);
	m_table->setHorizontalHeaderLabels(QStringList() << tr("User ID") << tr("User Name"));
	m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

	QPushButton* push_remove_user = new QPushButton(tr("Delete User"), this);
	QPushButton* push_create_user = new QPushButton(tr("Create User"), this);
	QPushButton* push_login_user = new QPushButton(tr("Log In User"), this);
	QPushButton* push_rename_user = new QPushButton(tr("Rename User"), this);

	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(true);

	// Button Layout
	QHBoxLayout* hbox_buttons = new QHBoxLayout();
	hbox_buttons->addWidget(push_create_user);
	hbox_buttons->addWidget(push_login_user);
	hbox_buttons->addWidget(push_rename_user);
	hbox_buttons->addWidget(push_remove_user);
	hbox_buttons->addStretch();
	hbox_buttons->addWidget(push_close);

	// Main Layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_table);
	vbox_main->addLayout(hbox_buttons);
	setLayout(vbox_main);

	m_active_user = m_emu_settings->GetSetting(emu_settings::SelectedUser);
	UpdateTable();

	restoreGeometry(m_gui_settings->GetValue(gui::um_geometry).toByteArray());

	// Use this in multiple connects to protect the current user from deletion/rename.
	auto enableButtons = [=]()
	{
		int idx = m_table->currentRow();
		if (idx < 0)
		{
			push_rename_user->setEnabled(false);
			push_remove_user->setEnabled(false);
			return;
		}

		int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
		std::string idx_user = m_user_list[idx_real]->GetUserId();
		bool enable = idx_user != m_active_user;

		push_rename_user->setEnabled(enable);
		push_remove_user->setEnabled(enable);
	};

	enableButtons();

	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &user_manager_dialog::close);
	connect(push_remove_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRemove);
	connect(push_rename_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRename);
	connect(push_create_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserCreate);
	connect(push_login_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserLogin);
	connect(this, &user_manager_dialog::OnUserLoginSuccess, this, enableButtons);
	connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, &user_manager_dialog::OnSort);
	connect(m_table, &QTableWidget::customContextMenuRequested, this, &user_manager_dialog::ShowContextMenu);
	connect(m_table, &QTableWidget::itemDoubleClicked, this, &user_manager_dialog::OnUserLogin);
	connect(m_table, &QTableWidget::itemSelectionChanged, this, enableButtons);
}

void user_manager_dialog::UpdateTable()
{
	// Get the user folders in the home directory and the currently logged in user.
	m_user_list.clear();
	m_user_list = GetUserAccounts(Emu.GetHddDir() + "home");

	// Clear and then repopulate the table with the list gathered above.
	m_table->setRowCount(static_cast<int>(m_user_list.size()));

	// For indicating logged-in user.
	QFont bold_font;
	bold_font.setBold(true);

	int row = 0;
	for (UserAccount* user : m_user_list)
	{
		QTableWidgetItem* user_id_item = new QTableWidgetItem(qstr(user->GetUserId()));
		user_id_item->setData(Qt::UserRole, row); // For sorting to work properly
		user_id_item->setFlags(user_id_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, user_id_item);

		QTableWidgetItem* username_item = new QTableWidgetItem(qstr(user->GetUserName()));
		username_item->setData(Qt::UserRole, row); // For sorting to work properly
		username_item->setFlags(username_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 1, username_item);

		// Compare current config value with the one in this user (only 8 digits in userId)
		if (m_active_user.compare(0, 8, user->GetUserId()) == 0)
		{
			user_id_item->setFont(bold_font);
			username_item->setFont(bold_font);
		}
		++row;
	}
	// GUI resizing
	m_table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_table->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	QSize table_size = QSize(
		m_table->verticalHeader()->width() + m_table->horizontalHeader()->length() + m_table->frameWidth() * 2,
		m_table->horizontalHeader()->height() + m_table->verticalHeader()->length() + m_table->frameWidth() * 2);

	QSize preferred_size = minimumSize().expandedTo(sizeHint() - m_table->sizeHint() + table_size).expandedTo(size());
	QSize max_size = QSize(preferred_size.width(), static_cast<int>(QApplication::desktop()->screenGeometry().height()*.6));

	resize(preferred_size.boundedTo(max_size));
}

// Remove a user folder, needs to be confirmed.
void user_manager_dialog::OnUserRemove()
{
	const int idx = m_table->currentRow();
	if (idx < 0)
	{
		return;
	}

	const int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	const QString username = qstr(m_user_list[idx_real]->GetUserName());
	const QString user_id = qstr(m_user_list[idx_real]->GetUserId());
	const std::string user_dir = m_user_list[idx_real]->GetUserDir();

	if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete the following user?\n\nUser ID: %0\nUsername: %1\n\n"
		"This will remove all files in:\n%2").arg(user_id).arg(username).arg(qstr(user_dir)), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		LOG_WARNING(GENERAL, "Deleting user: %s", user_dir);
		fs::remove_all(user_dir);
		UpdateTable();
	}
}

void user_manager_dialog::GenerateUser(const std::string& user_id, const std::string& username)
{
	// Create user folders and such.
	const std::string home_dir = Emu.GetHddDir() + "home/";
	const std::string user_dir = home_dir + user_id;
	fs::create_dir(home_dir);
	fs::create_dir(user_dir + "/");
	fs::create_dir(user_dir + "/exdata/");
	fs::create_dir(user_dir + "/savedata/");
	fs::create_dir(user_dir + "/trophy/");
	fs::write_file(user_dir + "/localusername", fs::create + fs::excl + fs::write, username);
}

bool user_manager_dialog::ValidateUsername(const QString& text_to_validate)
{
	// "Entire string (^...$) must be between 3 and 16 characters
	// and only consist of letters, numbers, underscores, and hyphens."
	QRegExpValidator validator(QRegExp("^[A-Za-z0-9_-]{3,16}$"));

	int pos = 0;
	QString text = text_to_validate;
	return (validator.validate(text, pos) == QValidator::Acceptable);
}

void user_manager_dialog::OnUserRename()
{
	const int idx = m_table->currentRow();
	if (idx < 0)
	{
		return;
	}

	const int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	const std::string user_id = m_user_list[idx_real]->GetUserId();
	const std::string username = m_user_list[idx_real]->GetUserName();

	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Rename User"));
	dialog->setLabelText(tr("User Id: %0\nOld Username: %1\n\nNew Username: ").arg(qstr(user_id)).arg(qstr(username)));
	dialog->resize(200, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(200, 100);

		QString text_to_validate = dialog->textValue();
		if (!ValidateUsername(text_to_validate))
		{
			QMessageBox::warning(this, tr("Error"), tr("Name must be between 3 and 16 characters and only consist of letters, numbers, underscores, and hyphens."));
			continue;
		}

		const std::string username_file = Emu.GetHddDir() + "home/" + user_id + "/localusername";
		const std::string new_username = text_to_validate.toStdString();

		if (fs::write_file(username_file, fs::rewrite, new_username))
		{
			LOG_SUCCESS(GENERAL, "Renamed user %s with id %s to %s", username, user_id, new_username);
		}
		else
		{
			LOG_FATAL(GENERAL, "Could not rename user %s with id %s to %s", username, user_id, new_username);
		}

		UpdateTable();
		break;
	}
}

void user_manager_dialog::OnUserCreate()
{
	// If the user list is sorted by default from fs::dir, then we just need the last one in the list.
	std::string largest_user_id = m_user_list[m_user_list.size() - 1]->GetUserId();

	// Add one to the largest user id, then reformat the result into an 8-digit string.
	u8 next_largest = static_cast<u8>(std::stoul(largest_user_id) + 1u);
	const std::string next_user_id = fmt::format("%08d", next_largest);

	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("New User"));
	dialog->setLabelText(tr("New User ID: %0\n\nNew Username: ").arg(qstr(next_user_id)));
	dialog->resize(200, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(200, 100);

		QString text_to_validate = dialog->textValue();
		if (!ValidateUsername(text_to_validate))
		{
			QMessageBox::warning(this, tr("Error"), tr("Name must be between 3 and 16 characters and only consist of letters, numbers, underscores, and hyphens."));
			continue;
		}

		GenerateUser(next_user_id, text_to_validate.toStdString());
		UpdateTable();
		break;
	}
}

void user_manager_dialog::OnUserLogin()
{
	int idx = m_table->currentRow();
	if (idx < 0)
	{
		return;
	}

	int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	std::string selected_user_id = m_user_list[idx_real]->GetUserId();

	m_active_user = selected_user_id;
	m_emu_settings->SetSetting(emu_settings::SelectedUser, m_active_user);
	m_emu_settings->SaveSettings();
	UpdateTable();
	Q_EMIT OnUserLoginSuccess();
}

void user_manager_dialog::OnSort(int logicalIndex)
{
	if (logicalIndex < 0)
	{
		return;
	}
	else if (logicalIndex == m_sort_column)
	{
		m_sort_ascending ^= true;
	} 
	else
	{
		m_sort_ascending = true;
	}
	m_sort_column = logicalIndex;
	Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_table->sortByColumn(m_sort_column, sort_order);
}

void user_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	int idx = m_table->currentRow();
	if (idx < 0)
	{
		return;
	}

	QPoint global_pos = m_table->mapToGlobal(pos);
	QMenu* menu = new QMenu();

	// Create submenu for sort options.
	QMenu* sort_menu = menu->addMenu(tr("&Sort By"));
	QAction* user_id_act = sort_menu->addAction(tr("User ID"));
	QAction* username_act = sort_menu->addAction(tr("User Name"));

	QAction* remove_act = menu->addAction(tr("&Remove"));
	QAction* rename_act = menu->addAction(tr("&Rename"));
	QAction* login_act = menu->addAction(tr("&Login"));
	QAction* show_dir_act = menu->addAction(tr("&Open User Directory"));

	// Only enable actions if selected user is not logged in user.
	int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	std::string idx_user = m_user_list[idx_real]->GetUserId();
	bool enable = idx_user != m_active_user;

	remove_act->setEnabled(enable);
	rename_act->setEnabled(enable);

	// Connects and Events
	connect(remove_act, &QAction::triggered, this, &user_manager_dialog::OnUserRemove);
	connect(rename_act, &QAction::triggered, this, &user_manager_dialog::OnUserRename);
	connect(login_act, &QAction::triggered, this, &user_manager_dialog::OnUserLogin);
	connect(show_dir_act, &QAction::triggered, [=]()
	{
		int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
		QString path = qstr(m_user_list[idx_real]->GetUserDir());
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});

	connect(user_id_act, &QAction::triggered, this, [=] {OnSort(0); });
	connect(username_act, &QAction::triggered, this, [=] {OnSort(1); });

	menu->exec(global_pos);
}

void user_manager_dialog::closeEvent(QCloseEvent *event)
{
	m_gui_settings->SetValue(gui::um_geometry, saveGeometry());
	QDialog::closeEvent(event);
}
