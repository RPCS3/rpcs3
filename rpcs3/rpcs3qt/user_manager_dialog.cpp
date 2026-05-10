#include <QRegularExpressionValidator>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QScreen>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>
#include <QGuiApplication>

#include "user_manager_dialog.h"
#include "table_item_delegate.h"
#include "main_application.h"
#include "gui_settings.h"
#include "persistent_settings.h"
#include "qt_utils.h"

#include "Emu/System.h"
#include "Emu/system_utils.hpp"

#include "Utilities/File.h"
#include "util/logs.hpp"

LOG_CHANNEL(gui_log, "GUI");

user_manager_dialog::user_manager_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget* parent)
	: QDialog(parent)
	, m_gui_settings(std::move(gui_settings))
	, m_persistent_settings(std::move(persistent_settings))
{
	setWindowTitle(tr("User Manager"));
	setMinimumSize(QSize(500, 400));
	setModal(true);

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
	m_table->setCornerButtonEnabled(false);
	m_table->setAlternatingRowColors(true);
	m_table->setHorizontalHeaderLabels(QStringList() << tr("User ID") << tr("User Name"));
	m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	m_table->horizontalHeader()->setStretchLastSection(true);
	m_table->horizontalHeader()->setDefaultSectionSize(150);
	m_table->installEventFilter(this);

	QPushButton* push_remove_user = new QPushButton(tr("&Delete User"), this);
	push_remove_user->setAutoDefault(false);

	QPushButton* push_create_user = new QPushButton(tr("&Create User"), this);
	push_create_user->setAutoDefault(false);

	QPushButton* push_login_user = new QPushButton(tr("&Log In User"), this);
	push_login_user->setAutoDefault(false);

	QPushButton* push_rename_user = new QPushButton(tr("&Rename User"), this);
	push_rename_user->setAutoDefault(false);

	QPushButton* push_close = new QPushButton(tr("&Close"), this);
	push_close->setAutoDefault(false);

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

	// Get the active user
	m_active_user = m_persistent_settings->GetCurrentUser("00000001").toStdString();

	// Get the real active user (might differ, set by cli)
	if (m_active_user != Emu.GetUsr())
	{
		m_active_user = Emu.GetUsr();
	}

	UpdateTable();

	restoreGeometry(m_gui_settings->GetValue(gui::um_geometry).toByteArray());

	// Use this in multiple connects to protect the current user from deletion/rename.
	const auto enable_buttons = [=, this]()
	{
		const u32 key = GetUserKey();
		if (key == 0)
		{
			push_login_user->setEnabled(false);
			push_rename_user->setEnabled(false);
			push_remove_user->setEnabled(false);
			return;
		}

		const bool enable = m_user_list[key].GetUserId() != m_active_user;

		push_login_user->setEnabled(enable);
		push_rename_user->setEnabled(true);
		push_remove_user->setEnabled(enable);
	};

	enable_buttons();

	// Connects and events
	connect(push_close, &QAbstractButton::clicked, this, &user_manager_dialog::close);
	connect(push_remove_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRemove);
	connect(push_rename_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserRename);
	connect(push_create_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserCreate);
	connect(push_login_user, &QAbstractButton::clicked, this, &user_manager_dialog::OnUserLogin);
	connect(this, &user_manager_dialog::OnUserLoginSuccess, this, enable_buttons);
	connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked, this, &user_manager_dialog::OnSort);
	connect(m_table, &QTableWidget::customContextMenuRequested, this, &user_manager_dialog::ShowContextMenu);
	connect(m_table, &QTableWidget::itemDoubleClicked, this, &user_manager_dialog::OnUserLogin);
	connect(m_table, &QTableWidget::itemSelectionChanged, this, enable_buttons);
}

void user_manager_dialog::UpdateTable(bool mark_only)
{
	// For indicating logged-in user.
	QFont bold_font;
	bold_font.setBold(true);

	if (mark_only)
	{
		const QString active_user = QString::fromStdString(m_active_user);

		for (int i = 0; i < m_table->rowCount(); i++)
		{
			QTableWidgetItem* user_id_item = m_table->item(i, 0);
			QTableWidgetItem* username_item = m_table->item(i, 1);

			// Compare current config value with the one in this user
			if (active_user == user_id_item->text())
			{
				user_id_item->setFont(bold_font);
				username_item->setFont(bold_font);
			}
			else
			{
				user_id_item->setFont(QFont());
				username_item->setFont(QFont());
			}
		}
		return;
	}

	// Get the user folders in the home directory and the currently logged in user.
	m_user_list.clear();
	m_user_list = user_account::GetUserAccounts(rpcs3::utils::get_hdd0_dir() + "home");

	// Clear and then repopulate the table with the list gathered above.
	m_table->setRowCount(static_cast<int>(m_user_list.size()));

	int row = 0;
	for (auto& [id, account] : m_user_list)
	{
		QTableWidgetItem* user_id_item = new QTableWidgetItem(QString::fromStdString(account.GetUserId()));
		user_id_item->setData(Qt::UserRole, id); // For sorting to work properly
		user_id_item->setFlags(user_id_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 0, user_id_item);

		QTableWidgetItem* username_item = new QTableWidgetItem(QString::fromStdString(account.GetUsername()));
		username_item->setData(Qt::UserRole, id); // For sorting to work properly
		username_item->setFlags(username_item->flags() & ~Qt::ItemIsEditable);
		m_table->setItem(row, 1, username_item);

		// Compare current config value with the one in this user (only 8 digits in userId)
		if (m_active_user.starts_with(account.GetUserId()))
		{
			user_id_item->setFont(bold_font);
			username_item->setFont(bold_font);
		}
		++row;
	}

	// GUI resizing
	m_table->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	m_table->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

	const QSize table_size(
		m_table->verticalHeader()->width() + m_table->horizontalHeader()->length() + m_table->frameWidth() * 2,
		m_table->horizontalHeader()->height() + m_table->verticalHeader()->length() + m_table->frameWidth() * 2);

	const QSize preferred_size = minimumSize().expandedTo(sizeHint() - m_table->sizeHint() + table_size).expandedTo(size());
	const QSize max_size(preferred_size.width(), static_cast<int>(QGuiApplication::primaryScreen()->size().height() * 0.6));

	resize(preferred_size.boundedTo(max_size));
}

// Remove a user folder, needs to be confirmed.
void user_manager_dialog::OnUserRemove()
{
	const u32 key = GetUserKey();
	if (key == 0)
	{
		return;
	}

	const QString username = QString::fromStdString(m_user_list[key].GetUsername());
	const QString user_id = QString::fromStdString(m_user_list[key].GetUserId());
	const std::string user_dir = m_user_list[key].GetUserDir();

	if (QMessageBox::question(this, tr("Delete Confirmation"), tr("Are you sure you want to delete the following user?\n\nUser ID: %0\nUsername: %1\n\n"
		"This will remove all files in:\n%2").arg(user_id).arg(username).arg(QString::fromStdString(user_dir)), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		gui_log.warning("Deleting user: %s", user_dir);
		fs::remove_all(user_dir);
		UpdateTable();
	}
}

void user_manager_dialog::GenerateUser(const std::string& user_id, const std::string& username)
{
	ensure(rpcs3::utils::check_user(user_id) > 0);

	// Create user folders and such.
	const std::string home_dir = rpcs3::utils::get_hdd0_dir() + "home/";
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
	const QRegularExpressionValidator validator(QRegularExpression("^[A-Za-z0-9_-]{3,16}$"));

	int pos = 0;
	QString text = text_to_validate;
	return (validator.validate(text, pos) == QValidator::Acceptable);
}

void user_manager_dialog::OnUserRename()
{
	const u32 key = GetUserKey();
	if (key == 0)
	{
		return;
	}

	const std::string user_id = m_user_list[key].GetUserId();
	const std::string username = m_user_list[key].GetUsername();
	const QString q_username = QString::fromStdString(username);

	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("Rename User"));
	dialog->setLabelText(tr("User Id: %0\nOld Username: %1\n\nNew Username: ").arg(QString::fromStdString(user_id)).arg(q_username));
	dialog->setTextValue(q_username);
	dialog->resize(200, 100);

	while (dialog->exec() != QDialog::Rejected)
	{
		dialog->resize(200, 100);

		const QString text_to_validate = dialog->textValue();
		if (!ValidateUsername(text_to_validate))
		{
			QMessageBox::warning(this, tr("Error"), tr("Name must be between 3 and 16 characters and only consist of letters, numbers, underscores, and hyphens."));
			continue;
		}

		const std::string username_file = rpcs3::utils::get_hdd0_dir() + "home/" + user_id + "/localusername";
		const std::string new_username = text_to_validate.toStdString();

		if (fs::write_file(username_file, fs::rewrite, new_username))
		{
			gui_log.success("Renamed user %s with id %s to %s", username, user_id, new_username);
		}
		else
		{
			gui_log.fatal("Could not rename user %s with id %s to %s", username, user_id, new_username);
		}

		UpdateTable();
		break;
	}
}

void user_manager_dialog::OnUserCreate()
{
	// Take the smallest user id > 0, then reformat the result into an 8-digit string.
	u32 smallest = 1;

	for (auto it = m_user_list.begin(); it != m_user_list.end(); ++it)
	{
		if (it->first > smallest)
		{
			break;
		}

		smallest++;
	}

	if (smallest >= 100000000) // Only 8 digits allowed
	{
		QMessageBox::warning(this, tr("Error"), tr("Cannot add more users."));
		return;
	}

	const std::string next_user_id = fmt::format("%08d", smallest);
	ensure(rpcs3::utils::check_user(next_user_id) > 0);

	QInputDialog* dialog = new QInputDialog(this);
	dialog->setWindowTitle(tr("New User"));
	dialog->setLabelText(tr("New User ID: %0\n\nNew Username: ").arg(QString::fromStdString(next_user_id)));
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
	if (!Emu.IsStopped())
	{
		if (QMessageBox::question(this, tr("Stop emulator?"),
			tr("In order to change the user you have to stop the emulator first.\n\nStop the emulator now?"),
			QMessageBox::Yes | QMessageBox::Abort) != QMessageBox::Yes)
		{
			return;
		}

		gui_log.notice("Stopping current emulation in order to change the current user.");
		Emu.GracefulShutdown(false);
	}

	const u32 key = GetUserKey();
	const std::string new_user = m_user_list[key].GetUserId();

	main_application::InitializeEmulator(new_user, Emu.HasGui());

	m_active_user = new_user;
	m_persistent_settings->SetValue(gui::persistent::active_user, QString::fromStdString(m_active_user));
	UpdateTable(true);
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
	m_table->sortByColumn(m_sort_column, m_sort_ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
}

void user_manager_dialog::ShowContextMenu(const QPoint &pos)
{
	const u32 key = GetUserKey();
	if (key == 0)
	{
		return;
	}

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
	const bool enable = m_user_list[key].GetUserId() != m_active_user;

	remove_act->setEnabled(enable);
	rename_act->setEnabled(enable);

	// Connects and Events
	connect(remove_act, &QAction::triggered, this, &user_manager_dialog::OnUserRemove);
	connect(rename_act, &QAction::triggered, this, &user_manager_dialog::OnUserRename);
	connect(login_act, &QAction::triggered, this, &user_manager_dialog::OnUserLogin);
	connect(show_dir_act, &QAction::triggered, this, [this, key]()
	{
		const QString path = QString::fromStdString(m_user_list[key].GetUserDir());
		gui::utils::open_dir(path);
	});

	connect(user_id_act, &QAction::triggered, this, [this] {OnSort(0); });
	connect(username_act, &QAction::triggered, this, [this] {OnSort(1); });

	menu->exec(m_table->viewport()->mapToGlobal(pos));
}

// Returns the current user's key > 0. if no user is selected, return 0
u32 user_manager_dialog::GetUserKey() const
{
	const int idx = m_table->currentRow();
	if (idx < 0)
	{
		return 0;
	}

	const QTableWidgetItem* item = m_table->item(idx, 0);
	if (!item)
	{
		return 0;
	}

	const u32 idx_real = item->data(Qt::UserRole).toUInt();
	if (!m_user_list.contains(idx_real))
	{
		return 0;
	}

	return idx_real;
}

void user_manager_dialog::closeEvent(QCloseEvent *event)
{
	m_gui_settings->SetValue(gui::um_geometry, saveGeometry());
	QDialog::closeEvent(event);
}

bool user_manager_dialog::eventFilter(QObject *object, QEvent *event)
{
	const u32 key = GetUserKey();
	if (key == 0 || object != m_table || m_user_list[key].GetUserId() == m_active_user)
	{
		return QDialog::eventFilter(object, event);
	}

	if (event->type() == QEvent::KeyRelease)
	{
		const QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

		switch (key_event->key())
		{
		case Qt::Key_F2:
			OnUserRename();
			break;
		case Qt::Key_Delete:
			OnUserRemove();
			break;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			OnUserLogin();
			break;
		default:
			break;
		}
	}

	return QDialog::eventFilter(object, event);
}

void user_manager_dialog::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (!ev) return;

	// Qt's itemDoubleClicked signal doesn't distinguish between mouse buttons and there is no simple way to get the pressed button.
	// So we have to ignore this event when another button is pressed.
	if (ev->button() != Qt::LeftButton)
	{
		ev->ignore();
		return;
	}

	QDialog::mouseDoubleClickEvent(ev);
}
