#include "user_manager_dialog.h"

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
		for (const auto& userFolder : fs::dir(base_dir))
		{
			if (!userFolder.is_directory)
			{
				continue;
			}

			// Is the folder name exactly 8 all-numerical characters long?
			// We use strtol to find any non-numeric characters in folder name.
			char* nonNumericChar;
			std::strtol(userFolder.name.c_str(), &nonNumericChar, 10);
			if (userFolder.name.length() != 8 || *nonNumericChar != '\0')
			{
				continue;
			}

			// Does the localusername file exist?
			if (!fs::is_file(fmt::format("%s/%s/localusername", base_dir, userFolder.name)))
			{
				continue;
			}

			UserAccount* user_entry = new UserAccount(userFolder.name);
			user_list.emplace_back(user_entry);
		}
		return user_list;
	}
}

user_manager_dialog::user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, const std::string& dir, QWidget* parent)
	: QDialog(parent), m_user_list(), m_dir(dir), m_sort_column(1), m_sort_ascending(true), m_gui_settings(gui_settings), m_emu_settings(emu_settings)
{
	setWindowTitle(tr("User Manager"));
	setMinimumSize(QSize(400, 400));
	setModal(true);

	m_emu_settings->LoadSettings();
	Init(dir);
}

void user_manager_dialog::Init(const std::string& dir)
{
	// Table
	m_table = new QTableWidget(this);

	//m_table->setItemDelegate(new table_item_delegate(this)); // to get rid of cell selection rectangles include "table_item_delegate.h"
	m_table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setContextMenuPolicy(Qt::CustomContextMenu);
	m_table->setColumnCount(2);
	m_table->setHorizontalHeaderLabels(QStringList() << tr("User ID") << tr("User Name"));

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

	// main layout
	QVBoxLayout* vbox_main = new QVBoxLayout();
	vbox_main->setAlignment(Qt::AlignCenter);
	vbox_main->addWidget(m_table);
	vbox_main->addLayout(hbox_buttons);
	setLayout(vbox_main);

	m_selected_user = m_emu_settings->GetSetting(emu_settings::SelectedUser);
	UpdateTable();

	restoreGeometry(m_gui_settings->GetValue(gui::um_geometry).toByteArray());

	// Use this in multiple connects to protect the current user from deletion/rename.
	auto enableButtons = [=]()
	{
		int idx = m_table->currentRow();
		if (idx < 0)
		{
			return;
		}
		int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
		std::string idx_user = m_user_list[idx_real]->GetUserId();
		bool enable = idx_user != m_selected_user;

		push_rename_user->setEnabled(enable);
		push_remove_user->setEnabled(enable);
	};

	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &user_manager_dialog::close);
	connect(push_remove_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRemove);
	connect(push_rename_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRename);
	connect(push_create_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserCreate);
	connect(push_login_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserLogin);
	connect(this, &user_manager_dialog::OnUserLoginSuccess, this, enableButtons);
	connect(m_table, &QTableWidget::itemDoubleClicked, this, &user_manager_dialog::OnUserLogin);
	connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, &user_manager_dialog::OnSort);
	connect(m_table, &QTableWidget::customContextMenuRequested, this, &user_manager_dialog::ShowContextMenu);
	connect(m_table, &QTableWidget::itemClicked, this, enableButtons);
	connect(m_table, &QTableWidget::itemSelectionChanged, this, enableButtons);
}

void user_manager_dialog::UpdateTable()
{
	if (m_dir == "")
	{
		// fmt::format(%shome ... is harder to read than straight concatenation.
		m_dir = Emu.GetHddDir() + "home";
	}

	// Get the user folders in the home directory and the currently logged in user.
	m_user_list.clear();
	m_user_list = GetUserAccounts(m_dir);

	// Clear and then repopulate the table with the list gathered above.
	m_table->setRowCount(static_cast<int>(m_user_list.size()));

	// For indicating logged-in user.
	QFont boldFont;
	boldFont.setBold(true);

	int row = 0;
	for (UserAccount* user : m_user_list)
	{
		QString userId = qstr(user->GetUserId());
		QString userName = qstr(user->GetUserName());

		QTableWidgetItem* userIdItem = new QTableWidgetItem(userId);
		userIdItem->setData(Qt::UserRole, row); // For sorting to work properly
		userIdItem->setFlags(userIdItem->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, userIdItem);

		QTableWidgetItem* userNameItem = new QTableWidgetItem(userName);
		userNameItem->setData(Qt::UserRole, row); // For sorting to work properly
		userNameItem->setFlags(userNameItem->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 1, userNameItem);

		// Compare current config value with the one in this user (only 8 digits in userId)
		if (m_selected_user.compare(0, 8, user->GetUserId()) == 0)
		{
			userIdItem->setFont(boldFont);
			userNameItem->setFont(boldFont);
		}
		++row;
	}
	// GUI resizing
	m_table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_table->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	QSize tableSize = QSize(
		m_table->verticalHeader()->width() + m_table->horizontalHeader()->length() + m_table->frameWidth() * 2,
		m_table->horizontalHeader()->height() + m_table->verticalHeader()->length() + m_table->frameWidth() * 2);

	QSize preferredSize = minimumSize().expandedTo(sizeHint() - m_table->sizeHint() + tableSize).expandedTo(size());
	QSize maxSize = QSize(preferredSize.width(), static_cast<int>(QApplication::desktop()->screenGeometry().height()*.6));

	resize(preferredSize.boundedTo(maxSize));
}

//Remove a user folder, needs to be confirmed.
void user_manager_dialog::OnUserRemove()
{
	int idx = m_table->currentRow();
	int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete:\n%1?").arg(qstr(m_user_list[idx_real]->GetUserName())), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		fs::remove_all(m_user_list[idx_real]->GetUserDir());
		UpdateTable();
	}
}

void user_manager_dialog::GenerateUser(const std::string& username)
{
	// If the user list is sorted by defult from fs::dir, then we just need the last one in the list.
	std::string largestUserId = m_user_list[m_user_list.size() - 1]->GetUserId();

	// Add one to the largest user id, then reformat the result into an 8-digit string.
	u8 nextLargest = static_cast<u8>(std::stoul(largestUserId) + 1u);
	char* buf;
	sprintf(buf, "%08d", nextLargest);
	std::string nextUserId(buf);

	// Create user folders and such.
	const std::string homeDir = Emu.GetHddDir() + "home/";
	const std::string userDir = homeDir + nextUserId;
	fs::create_dir(homeDir);
	fs::create_dir(fmt::format("%s/", userDir));
	fs::create_dir(fmt::format("%s/exdata/", userDir));
	fs::create_dir(fmt::format("%s/savedata/", userDir));
	fs::create_dir(fmt::format("%s/trophy/", userDir));
	fs::write_file(fmt::format("%s/localusername", userDir), fs::create + fs::excl + fs::write, username);
}

bool user_manager_dialog::ValidateUsername(const QString& textToValidate)
{
	// "Entire string (^...$) must be between 3 and 16 characters
	// and only consist of letters, numbers, underscores, and hyphens."
	QRegExpValidator validator(QRegExp("^[A-Za-z0-9_-]{3,16}$"));

	int pos = 0;
	QString text = textToValidate;
	return (validator.validate(text, pos) == QValidator::Acceptable);
}

void user_manager_dialog::OnUserRename()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Rename User"));
	dialog->setLabelText(tr("New Username: "));
	dialog->resize(200, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(200, 100);
		QString textToValidate = dialog->textValue();
		if (!ValidateUsername(textToValidate))
		{
			QMessageBox::warning(this, tr("Error"), tr("Name must be between 3 and 16 characters and only consist of letters, numbers, underscores, and hyphens."));
			continue;
		}
		int idx = m_table->currentRow();
		int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();

		const std::string userId = m_user_list[idx_real]->GetUserId();
		const std::string usernameFile = Emu.GetHddDir() + "home/" + userId + "/localusername";
		fs::write_file(usernameFile, fs::rewrite, textToValidate.toStdString());
		UpdateTable();
		break;
	}
}

void user_manager_dialog::OnUserCreate()
{
	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("New User"));
	dialog->setLabelText(tr("New Username: "));
	dialog->resize(200, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(200, 100);
		QString textToValidate = dialog->textValue();
		if (!ValidateUsername(textToValidate))
		{
			QMessageBox::warning(this, tr("Error"), tr("Name must be between 3 and 16 characters and only consist of letters, numbers, underscores, and hyphens."));
			continue;
		}
		GenerateUser(textToValidate.toStdString());
		UpdateTable();
		break;
	}
}

void user_manager_dialog::OnUserLogin()
{
	int idx = m_table->currentRow();
	int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	std::string selectedUserId = m_user_list[idx_real]->GetUserId();

	m_selected_user = selectedUserId;
	m_emu_settings->SetSetting(emu_settings::SelectedUser, m_selected_user);
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
	Qt::SortOrder sort_order = m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder;
	m_table->sortByColumn(m_sort_column, sort_order);
	m_sort_column = logicalIndex;
}

void user_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	int idx = m_table->currentRow();
	if (idx < 0)
	{
		return;
	}
	QPoint globalPos = m_table->mapToGlobal(pos);
	QMenu* menu = new QMenu();

	// Create all the actions before adding them to the menu/submenus.
	QAction* userIdAct = new QAction(tr("User ID"), this);
	QAction* userNameAct = new QAction(tr("User Name"), this);

	QAction* removeAct = new QAction(tr("&Remove"), this);
	QAction* renameAct = new QAction(tr("&Rename"), this);
	QAction* loginAct = new QAction(tr("&Login"), this);
	QAction* showDirAct = new QAction(tr("&Open User Directory"), this);

	//Create submenu for sort options.
	m_sort_options = new QMenu(tr("&Sort By"));
	m_sort_options->addAction(userIdAct);
	m_sort_options->addAction(userNameAct);

	// Add all options and submenus to the context menu.
	menu->addMenu(m_sort_options);
	menu->addSeparator();
	menu->addAction(removeAct);
	menu->addAction(renameAct);
	menu->addAction(loginAct);
	menu->addAction(showDirAct);

	// Only enable actions if selected user is not logged in user.
	int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
	std::string idx_user = m_user_list[idx_real]->GetUserId();
	bool enable = idx_user != m_selected_user;

	removeAct->setEnabled(enable);
	renameAct->setEnabled(enable);

	// Connects and Events
	connect(removeAct, &QAction::triggered, this, &user_manager_dialog::OnUserRemove);
	connect(renameAct, &QAction::triggered, this, &user_manager_dialog::OnUserRename);
	connect(loginAct, &QAction::triggered, this, &user_manager_dialog::OnUserLogin);
	connect(showDirAct, &QAction::triggered, [=]()
	{
		int idx_real = m_table->item(idx, 0)->data(Qt::UserRole).toInt();
		QString path = qstr(m_user_list[idx_real]->GetUserDir());
		QDesktopServices::openUrl(QUrl("file:///" + path));
	});

	connect(userIdAct, &QAction::triggered, this, [=] {OnSort(0); });
	connect(userNameAct, &QAction::triggered, this, [=] {OnSort(1); });

	menu->exec(globalPos);
}

void user_manager_dialog::closeEvent(QCloseEvent *event)
{
	m_gui_settings->SetValue(gui::um_geometry, saveGeometry());
	QDialog::closeEvent(event);
}
