#include "main_window.h"
#include "qt_utils.h"
#include "vfs_dialog.h"
#include "save_manager_dialog.h"
#include "trophy_manager_dialog.h"
#include "savestate_manager_dialog.h"
#include "user_manager_dialog.h"
#include "screenshot_manager_dialog.h"
#include "kernel_explorer.h"
#include "game_list_frame.h"
#include "debugger_frame.h"
#include "log_frame.h"
#include "settings_dialog.h"
#include "rpcn_settings_dialog.h"
#include "auto_pause_settings_dialog.h"
#include "cg_disasm_window.h"
#include "log_viewer.h"
#include "memory_viewer_panel.h"
#include "rsx_debugger.h"
#include "about_dialog.h"
#include "pad_settings_dialog.h"
#include "progress_dialog.h"
#include "skylander_dialog.h"
#include "infinity_dialog.h"
#include "dimensions_dialog.h"
#include "kamen_rider_dialog.h"
#include "cheat_manager.h"
#include "patch_manager_dialog.h"
#include "patch_creator_dialog.h"
#include "pkg_install_dialog.h"
#include "category.h"
#include "gui_settings.h"
#include "input_dialog.h"
#include "camera_settings_dialog.h"
#include "ps_move_tracker_dialog.h"
#include "ipc_settings_dialog.h"
#include "shortcut_utils.h"
#include "config_checker.h"
#include "shortcut_dialog.h"
#include "system_cmd_dialog.h"
#include "emulated_pad_settings_dialog.h"
#include "emulated_logitech_g27_settings_dialog.h"
#include "basic_mouse_settings_dialog.h"
#include "vfs_tool_dialog.h"
#include "welcome_dialog.h"
#include "music_player_dialog.h"

#include <thread>
#include <unordered_set>

#include <QScreen>
#include <QDirIterator>
#include <QMimeData>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QBuffer>
#include <QTemporaryFile>
#include <QDesktopServices>

#include "rpcs3_version.h"
#include "Emu/IdManager.h"
#include "Emu/VFS.h"
#include "Emu/vfs_config.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Emu/system_config.h"
#include "Emu/savestate_utils.hpp"
#include "Emu/Cell/timers.hpp"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"
#include "Crypto/unzip.h"
#include "Crypto/decrypt_binaries.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Loader/PSF.h"
#include "Loader/mself.hpp"

#include "Utilities/Thread.h"
#include "util/sysinfo.hpp"
#include "util/serialization_ext.hpp"

#include "Input/gui_pad_thread.h"

#include "ui_main_window.h"

#include <QEventLoop>
#include <QTimer>

#ifdef _WIN32
#include "raw_mouse_settings_dialog.h"
#endif

#if defined(__linux__) || defined(__APPLE__) || (defined(_WIN32) && defined(ARCH_X64))
#define RPCS3_UPDATE_SUPPORTED
#endif

LOG_CHANNEL(gui_log, "GUI");

extern atomic_t<bool> g_user_asked_for_frame_capture;

class CPUDisAsm;
std::shared_ptr<CPUDisAsm> make_basic_ppu_disasm();

extern void qt_events_aware_op(int repeat_duration_ms, std::function<bool()> wrapped_op)
{
	ensure(wrapped_op);

	if (thread_ctrl::is_main())
	{
		// NOTE:
		// I noticed that calling this from an Emu callback can cause the
		// caller to get stuck for a while during newly opened Qt dialogs.
		// Adding a timeout here doesn't seem to do anything in that case.
		QEventLoop* event_loop = nullptr;

		std::shared_ptr<std::function<void()>> check_iteration;
		check_iteration = std::make_shared<std::function<void()>>([&]()
		{
			if (wrapped_op())
			{
				event_loop->exit(0);
			}
			else
			{
				QTimer::singleShot(repeat_duration_ms, *check_iteration);
			}
		});

		while (!wrapped_op())
		{
			// Init event loop
			event_loop = new QEventLoop();

			// Queue event initially
			QTimer::singleShot(0, *check_iteration);

			// Event loop
			event_loop->exec();

			// Cleanup
			event_loop->deleteLater();
		}
	}
	else
	{
		while (!wrapped_op())
		{
			if (repeat_duration_ms == 0)
			{
				std::this_thread::yield();
			}
			else if (thread_ctrl::get_current())
			{
				thread_ctrl::wait_for(repeat_duration_ms * 1000);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(repeat_duration_ms));
			}
		}
	}
}

main_window::main_window(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, std::shared_ptr<persistent_settings> persistent_settings, QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::main_window)
	, m_gui_settings(gui_settings)
	, m_emu_settings(std::move(emu_settings))
	, m_persistent_settings(std::move(persistent_settings))
	, m_updater(nullptr, gui_settings)
{
	Q_INIT_RESOURCE(resources);

	// We have to setup the ui before using a translation
	ui->setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose);
}

main_window::~main_window()
{
}

/* An init method is used so that RPCS3App can create the necessary connects before calling init (specifically the stylesheet connect).
 * Simplifies logic a bit.
 */
bool main_window::Init([[maybe_unused]] bool with_cli_boot)
{
	setAcceptDrops(true);

	// add toolbar widgets (crappy Qt designer is not able to)
	ui->toolBar->setObjectName("mw_toolbar");
	ui->sizeSlider->setRange(0, gui::gl_max_slider_pos);
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addWidget(ui->mw_searchbar);

	CreateActions();
	CreateDockWindows();
	CreateConnects();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10
	setWindowTitle(QString::fromStdString("RPCS3 " + rpcs3::get_verbose_version()));

	Q_EMIT RequestGlobalStylesheetChange();
	ConfigureGuiFromSettings();

	m_shortcut_handler = new shortcut_handler(gui::shortcuts::shortcut_handler_id::main_window, this, m_gui_settings);
	connect(m_shortcut_handler, &shortcut_handler::shortcut_activated, this, &main_window::handle_shortcut);

	show(); // needs to be done before creating the thumbnail toolbar

	// enable play options if a recent game exists
	const bool enable_play_last = !m_recent_game.actions.isEmpty() && m_recent_game.actions.first();

	const QString start_tooltip = enable_play_last ? tr("Play %0").arg(m_recent_game.actions.first()->text()) : tr("Play");

	if (enable_play_last)
	{
		ui->sysPauseAct->setText(tr("&Play last played game"));
		ui->sysPauseAct->setIcon(m_icon_play);
		ui->toolbar_start->setToolTip(start_tooltip);
	}

	ui->sysPauseAct->setEnabled(enable_play_last);
	ui->toolbar_start->setEnabled(enable_play_last);

	// RPCS3 Updater

	QMenu* download_menu = new QMenu(tr("Update Available!"));

	QAction* download_action = new QAction(tr("Download Update"), download_menu);
	connect(download_action, &QAction::triggered, this, [this]
	{
		m_updater.update(false);
	});

	download_menu->addAction(download_action);

#ifdef _WIN32
	// Use a menu at the top right corner to indicate the new version.
	QMenuBar *corner_bar = new QMenuBar(ui->menuBar);
	m_download_menu_action = corner_bar->addMenu(download_menu);
	ui->menuBar->setCornerWidget(corner_bar);
	ui->menuBar->cornerWidget()->setVisible(false);
#else
	// Append a menu to the right of the regular menus to indicate the new version.
	// Some distros just can't handle corner widgets at the moment.
	m_download_menu_action = ui->menuBar->addMenu(download_menu);
#endif

	ensure(m_download_menu_action);
	m_download_menu_action->setVisible(false);

	connect(&m_updater, &update_manager::signal_update_available, this, [this](bool update_available)
	{
		if (m_download_menu_action)
		{
			m_download_menu_action->setVisible(update_available);
		}
		if (ui->menuBar && ui->menuBar->cornerWidget())
		{
			ui->menuBar->cornerWidget()->setVisible(update_available);
		}
	});

#ifdef RPCS3_UPDATE_SUPPORTED
	if (const auto update_value = m_gui_settings->GetValue(gui::m_check_upd_start).toString(); update_value != gui::update_off)
	{
		const bool in_background = with_cli_boot || update_value == gui::update_bkg;
		const bool auto_accept   = !in_background && update_value == gui::update_auto;
		m_updater.check_for_updates(true, in_background, auto_accept, this);
	}
#endif

	// Disable vsh if not present.
	ui->bootVSHAct->setEnabled(fs::is_file(g_cfg_vfs.get_dev_flash() + "vsh/module/vsh.self"));

	// Focus to search bar by default
	ui->mw_searchbar->setFocus();

	// Refresh gamelist last
	m_game_list_frame->Refresh(true);

	update_gui_pad_thread();

	return true;
}

void main_window::update_gui_pad_thread()
{
	const bool enabled = m_gui_settings->GetValue(gui::nav_enabled).toBool();

	if (enabled && Emu.IsStopped())
	{
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
		if (!m_gui_pad_thread)
		{
			m_gui_pad_thread = std::make_unique<gui_pad_thread>();
		}

		m_gui_pad_thread->update_settings(m_gui_settings);
#endif
	}
	else
	{
		m_gui_pad_thread.reset();
	}
}

QString main_window::GetCurrentTitle()
{
	std::string title = Emu.GetTitleAndTitleID();
	if (title.empty())
	{
		title = Emu.GetLastBoot();
	}
	return QString::fromStdString(title);
}

// returns appIcon
QIcon main_window::GetAppIcon() const
{
	return m_app_icon;
}

void main_window::OnMissingFw()
{
	const QString title = tr("Missing Firmware Detected!");
	const QString message = tr("Commercial games require the firmware (PS3UPDAT.PUP file) to be installed."
	                           "\n<br>For information about how to obtain the required firmware read the <a %0 href=\"https://rpcs3.net/quickstart\">quickstart guide</a>.").arg(gui::utils::get_link_style());

	QMessageBox* mb = new QMessageBox(QMessageBox::Question, title, message, QMessageBox::Ok | QMessageBox::Cancel, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
	mb->setTextFormat(Qt::RichText);

	mb->button(QMessageBox::Ok)->setText(tr("Locate PS3UPDAT.PUP"));
	mb->setAttribute(Qt::WA_DeleteOnClose);
	mb->open();

	connect(mb, &QDialog::accepted, this, [this]()
	{
		QTimer::singleShot(1, [this](){ InstallPup(); }); // singleShot to avoid a Qt bug that causes a deletion of the sender during long slots.
	});
}

void main_window::ResizeIcons(int index)
{
	if (ui->sizeSlider->value() != index)
	{
		ui->sizeSlider->setSliderPosition(index);
		return; // ResizeIcons will be triggered again by setSliderPosition, so return here
	}

	if (m_save_slider_pos)
	{
		m_save_slider_pos = false;
		m_gui_settings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);

		// this will also fire when we used the actions, but i didn't want to add another boolean member
		SetIconSizeActions(index);
	}

	m_game_list_frame->ResizeIcons(index);
}

void main_window::handle_shortcut(gui::shortcuts::shortcut shortcut_key, const QKeySequence& key_sequence)
{
	gui_log.notice("Main window registered shortcut: %s (%s)", shortcut_key, key_sequence.toString());

	const system_state status = Emu.GetStatus();

	switch (shortcut_key)
	{
	case gui::shortcuts::shortcut::mw_toggle_fullscreen:
	{
		ui->toolbar_fullscreen->trigger();
		break;
	}
	case gui::shortcuts::shortcut::mw_exit_fullscreen:
	{
		if (isFullScreen())
			ui->toolbar_fullscreen->trigger();
		break;
	}
	case gui::shortcuts::shortcut::mw_refresh:
	{
		m_game_list_frame->Refresh(true);
		break;
	}
	case gui::shortcuts::shortcut::mw_pause:
	{
		if (status == system_state::running)
			Emu.Pause();
		break;
	}
	case gui::shortcuts::shortcut::mw_start:
	{
		if (status == system_state::paused)
			Emu.Resume();
		else if (status == system_state::ready)
			Emu.Run(true);
		break;
	}
	case gui::shortcuts::shortcut::mw_restart:
	{
		if (!Emu.GetBoot().empty())
			Emu.Restart();
		break;
	}
	case gui::shortcuts::shortcut::mw_stop:
	{
		if (status != system_state::stopped)
			Emu.GracefulShutdown(false, true);
		break;
	}
	default:
	{
		break;
	}
	}
}

void main_window::OnPlayOrPause()
{
	gui_log.notice("User triggered OnPlayOrPause");

	switch (Emu.GetStatus())
	{
	case system_state::ready: Emu.Run(true); return;
	case system_state::paused: Emu.Resume(); return;
	case system_state::running: Emu.Pause(); return;
	case system_state::stopped:
	{
		if (m_selected_game)
		{
			gui_log.notice("Booting from OnPlayOrPause...");
			Boot(m_selected_game->info.path, m_selected_game->info.serial);
		}
		else if (const std::string path = Emu.GetLastBoot(); !path.empty())
		{
			if (const auto error = Emu.Load(); error != game_boot_result::no_errors)
			{
				gui_log.error("Boot failed: reason: %s, path: %s", error, path);
				show_boot_error(error);
			}
		}
		else if (!m_recent_game.actions.isEmpty())
		{
			BootRecentAction(m_recent_game.actions.first(), false);
		}

		return;
	}
	case system_state::starting: break;
	default: fmt::throw_exception("Unreachable");
	}
}

void main_window::show_boot_error(game_boot_result status)
{
	QString message;
	switch (status)
	{
	case game_boot_result::nothing_to_boot:
		message = tr("No bootable content was found.");
		break;
	case game_boot_result::wrong_disc_location:
		message = tr("Disc could not be mounted properly. Make sure the disc is not in the dev_hdd0/game folder.");
		break;
	case game_boot_result::invalid_file_or_folder:
		message = tr("The selected file or folder is invalid or corrupted.");
		break;
	case game_boot_result::invalid_bdvd_folder:
		message = tr("The virtual dev_bdvd folder does not exist or is not empty.");
		break;
	case game_boot_result::install_failed:
		message = tr("Additional content could not be installed.");
		break;
	case game_boot_result::decryption_error:
		message = tr("Digital content could not be decrypted. This is usually caused by a missing or invalid license (RAP) file.");
		break;
	case game_boot_result::file_creation_error:
		message = tr("The emulator could not create files required for booting.");
		break;
	case game_boot_result::unsupported_disc_type:
		message = tr("This disc type is not supported yet.");
		break;
	case game_boot_result::savestate_corrupted:
		message = tr("Savestate data is corrupted or it's not an RPCS3 savestate.");
		break;
	case game_boot_result::savestate_version_unsupported:
		message = tr("Savestate versioning data differs from your RPCS3 build.");
		break;
	case game_boot_result::still_running:
		message = tr("A game or PS3 application is still running or has yet to be fully stopped.");
		break;
	case game_boot_result::firmware_version:
		message = tr("The game or PS3 application needs a more recent firmware version.");
		break;
	case game_boot_result::firmware_missing: // Handled elsewhere
	case game_boot_result::already_added: // Handled elsewhere
	case game_boot_result::currently_restricted:
	case game_boot_result::no_errors:
		return;
	case game_boot_result::generic_error:
		message = tr("Unknown error.");
		break;
	}
	const QString link = tr("<br /><br />For information on setting up the emulator and dumping your PS3 games, read the <a %0 href=\"https://rpcs3.net/quickstart\">quickstart guide</a>.").arg(gui::utils::get_link_style());

	QMessageBox* msg = new QMessageBox(this);
	msg->setWindowTitle(tr("Boot Failed"));
	msg->setIcon(QMessageBox::Critical);
	msg->setTextFormat(Qt::RichText);
	msg->setStandardButtons(QMessageBox::Ok);
	msg->setText(tr("Booting failed: %1 %2").arg(message).arg(link));
	msg->setAttribute(Qt::WA_DeleteOnClose);
	msg->open();
}

void main_window::Boot(const std::string& path, const std::string& title_id, bool direct, bool refresh_list, cfg_mode config_mode, const std::string& config_path)
{
	if (Emu.IsBootingRestricted())
	{
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this, gui::ib_confirm_boot))
	{
		return;
	}

	Emu.GracefulShutdown(false);

	m_app_icon = gui::utils::get_app_icon_from_path(path, title_id);

	if (const auto error = Emu.BootGame(path, title_id, direct, config_mode, config_path); error != game_boot_result::no_errors)
	{
		gui_log.error("Boot failed: reason: %s, path: %s", error, path);
		show_boot_error(error);
		return;
	}

	if (is_savestate_compatible(path))
	{
		gui_log.success("Boot of savestate successful.");
		AddRecentAction(gui::Recent_Game(QString::fromStdString(path), QString::fromStdString(Emu.GetTitleAndTitleID())), true);
	}
	else
	{
		gui_log.success("Boot successful.");
		AddRecentAction(gui::Recent_Game(QString::fromStdString(Emu.GetBoot()), QString::fromStdString(Emu.GetTitleAndTitleID())), false);
	}

	if (refresh_list)
	{
		m_game_list_frame->Refresh(true);
	}
}

void main_window::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	const QString path_last_elf = m_gui_settings->GetValue(gui::fd_boot_elf).toString();
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select (S)ELF To Boot"), path_last_elf, tr(
		"(S)ELF files (*BOOT.BIN *.elf *.self);;"
		"ELF files (BOOT.BIN *.elf);;"
		"SELF files (EBOOT.BIN *.self);;"
		"BOOT files (*BOOT.BIN);;"
		"BIN files (*.bin);;"
		"All executable files (*.SAVESTAT.zst *.SAVESTAT.gz *.SAVESTAT *.sprx *.SPRX *.self *.SELF *.bin *.BIN *.prx *.PRX *.elf *.ELF *.o *.O);;"
		"All files (*.*)"),
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (file_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	// If we resolved the filepath earlier we would end up setting the last opened dir to the unwanted
	// game folder in case of having e.g. a Game Folder with collected links to elf files.
	// Don't set last path earlier in case of cancelled dialog
	m_gui_settings->SetValue(gui::fd_boot_elf, file_path);
	const std::string path = QFileInfo(file_path).absoluteFilePath().toStdString();

	gui_log.notice("Booting from BootElf...");
	Boot(path, "", true, true);
}

void main_window::BootTest()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

#ifdef _WIN32
	const QString path_tests = QString::fromStdString(fs::get_config_dir()) + "/test/";
#elif defined(__linux__)
	const QString path_tests = QCoreApplication::applicationDirPath() + "/../share/rpcs3/test/";
#else
	const QString path_tests = QCoreApplication::applicationDirPath() + "/../Resources/test/";
#endif

	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select (S)ELF To Boot"), path_tests, tr(
		"(S)ELF files (*.elf *.self);;"
		"ELF files (*.elf);;"
		"SELF files (*.self);;"
		"All files (*.*)"),
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (file_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	const std::string path = QFileInfo(file_path).absoluteFilePath().toStdString();

	gui_log.notice("Booting from BootTest...");
	Boot(path, "", true);
}

void main_window::BootSavestate()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select Savestate To Boot"), QString::fromStdString(fs::get_config_dir() + "savestates/"), tr(
		"Savestate files (*.SAVESTAT *.SAVESTAT.zst *.SAVESTAT.gz);;"
		"All files (*.*)"),
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (file_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	const std::string path = QFileInfo(file_path).absoluteFilePath().toStdString();

	gui_log.notice("Booting from BootSavestate...");
	Boot(path, "", true);
}

void main_window::BootGame()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	const QString path_last_game = m_gui_settings->GetValue(gui::fd_boot_game).toString();
	const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select Game Folder"), path_last_game, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir_path.isEmpty())
	{
		if (stopped)
		{
			Emu.Resume();
		}
		return;
	}

	m_gui_settings->SetValue(gui::fd_boot_game, QFileInfo(dir_path).path());

	gui_log.notice("Booting from BootGame...");
	Boot(dir_path.toStdString(), "", false, true);
}

void main_window::BootVSH()
{
	gui_log.notice("Booting from BootVSH...");
	Boot(g_cfg_vfs.get_dev_flash() + "/vsh/module/vsh.self");
}

void main_window::BootRsxCapture(std::string path)
{
	if (path.empty())
	{
		bool is_stopped = false;

		if (Emu.IsRunning())
		{
			Emu.Pause();
			is_stopped = true;
		}

		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select RSX Capture"), QString::fromStdString(fs::get_config_dir() + "captures/"), tr("RRC files (*.rrc *.RRC *.rrc.gz *.RRC.GZ);;All files (*.*)"));

		if (file_path.isEmpty())
		{
			if (is_stopped)
			{
				Emu.Resume();
			}
			return;
		}
		path = file_path.toStdString();
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.GracefulShutdown(false);

	if (!Emu.BootRsxCapture(path))
	{
		gui_log.error("Capture Boot Failed. path: %s", path);
	}
	else
	{
		gui_log.success("Capture Boot Success. path: %s", path);
	}
}

bool main_window::InstallFileInExData(const std::string& extension, const QString& path, const std::string& filename)
{
	if (path.isEmpty() || filename.empty() || extension.empty())
	{
		return false;
	}

	// Copy file atomically with thread/process-safe error checking for file size
	const std::string to_path = rpcs3::utils::get_hdd0_dir() + "/home/" + Emu.GetUsr() + "/exdata/" + filename.substr(0, filename.find_last_of('.'));
	fs::pending_file to(to_path + "." + extension);
	fs::file from(path.toStdString());

	if (!to.file || !from)
	{
		return false;
	}

	to.file.write(from.to_vector<u8>());
	from.close();

	if (to.file.size() < 0x10)
	{
		// Not a RAP file
		return false;
	}

#ifdef _WIN32
	// In the case of an unexpected crash during the operation, the temporary file can be used as the deleted file
	// See below
	to.file.sync();

	// In case we want to rename upper-case file to lower-case
	// Windows will ignore such rename operation if the file exists
	// So delete it
	fs::remove_file(to_path + "." + fmt::to_upper(extension));
#endif

	return to.commit();
}

bool main_window::InstallPackages(QStringList file_paths, bool from_boot)
{
	if (file_paths.isEmpty())
	{
		ensure(!from_boot);

		// If this function was called without a path, ask the user for files to install.
		const QString path_last_pkg = m_gui_settings->GetValue(gui::fd_install_pkg).toString();
		const QStringList paths = QFileDialog::getOpenFileNames(this, tr("Select packages and/or rap files to install"),
			path_last_pkg, tr("All relevant (*.pkg *.PKG *.rap *.RAP *.edat *.EDAT);;Package files (*.pkg *.PKG);;Rap files (*.rap *.RAP);;Edat files (*.edat *.EDAT);;All files (*.*)"));

		if (paths.isEmpty())
		{
			return true;
		}

		file_paths.append(paths);
		const QFileInfo file_info(file_paths[0]);
		m_gui_settings->SetValue(gui::fd_install_pkg, file_info.path());
	}

	if (file_paths.count() == 1)
	{
		const QString file_path = file_paths.front();
		const QFileInfo file_info(file_path);

		if (file_info.isDir())
		{
			gui_log.notice("PKG: Trying to install packages from dir: '%s'", file_path);

			const QDir dir(file_path);
			const QStringList dir_file_paths = gui::utils::get_dir_entries(dir, {}, true);

			if (dir_file_paths.empty())
			{
				gui_log.notice("PKG: Could not find any files in dir: '%s'", file_path);
				return true;
			}

			return InstallPackages(dir_file_paths, from_boot);
		}

		if (file_info.suffix().compare("pkg", Qt::CaseInsensitive) == 0)
		{
			compat::package_info info = game_compatibility::GetPkgInfo(file_path, m_game_list_frame ? m_game_list_frame->GetGameCompatibility() : nullptr);

			if (!info.is_valid)
			{
				QMessageBox::warning(this, tr("Invalid package!"), tr("The selected package is invalid!\n\nPath:\n%0").arg(file_path));
				return false;
			}

			if (info.type != compat::package_type::other)
			{
				if (info.type == compat::package_type::dlc)
				{
					info.local_cat = tr("\nDLC", "Block for package type (DLC)");
				}
				else
				{
					info.local_cat = tr("\nUpdate", "Block for package type (Update)");
				}
			}
			else if (!info.local_cat.isEmpty())
			{
				info.local_cat = tr("\n%0", "Block for package type").arg(info.local_cat);
			}

			if (!info.title_id.isEmpty())
			{
				info.title_id = tr("\n%0", "Block for Title ID").arg(info.title_id);
			}

			if (!info.version.isEmpty())
			{
				info.version = tr("\nVersion %0", "Block for Version").arg(info.version);
			}

			if (!info.changelog.isEmpty())
			{
				info.changelog = tr("Changelog:\n%0", "Block for Changelog").arg(info.changelog);
			}

			const QString info_string = QStringLiteral("%0\n\n%1%2%3%4").arg(file_info.fileName()).arg(info.title).arg(info.local_cat).arg(info.title_id).arg(info.version);
			QString message = tr("Do you want to install this package?\n\n%0").arg(info_string);

			QMessageBox mb(QMessageBox::Icon::Question, tr("PKG Decrypter / Installer"), message, QMessageBox::Yes | QMessageBox::No, this);
			mb.setDefaultButton(QMessageBox::No);

			if (!info.changelog.isEmpty())
			{
				mb.setInformativeText(tr("To see the changelog, please click \"Show Details\"."));
				mb.setDetailedText(info.changelog);

				// Smartass hack to make the unresizeable message box wide enough for the changelog
				const int log_width = QLabel(info.changelog).sizeHint().width();
				while (QLabel(message).sizeHint().width() < log_width)
				{
					message += "          ";
				}

				mb.setText(message);
			}

			if (mb.exec() != QMessageBox::Yes)
			{
				gui_log.notice("PKG: Cancelled installation from drop.\n%s\n%s", info_string, info.changelog);
				return true;
			}
		}
	}

	// Install rap files if available
	int installed_rap_and_edat_count = 0;

	const auto install_filetype = [&installed_rap_and_edat_count, &file_paths](const std::string extension)
	{
		const QString pattern = QString(".*\\.%1").arg(QString::fromStdString(extension));
		for (const QString& file : file_paths.filter(QRegularExpression(pattern, QRegularExpression::PatternOption::CaseInsensitiveOption)))
		{
			const QFileInfo file_info(file);
			const std::string filename = file_info.fileName().toStdString();

			if (InstallFileInExData(extension, file, filename))
			{
				gui_log.success("Successfully copied %s file: %s", extension, filename);
				installed_rap_and_edat_count++;
			}
			else
			{
				gui_log.error("Could not copy %s file: %s", extension, filename);
			}
		}
	};

	if (!from_boot)
	{
		if (!m_gui_settings->GetBootConfirmation(this))
		{
			// Last chance to cancel the operation
			return true;
		}

		if (!Emu.IsStopped())
		{
			Emu.GracefulShutdown(false);
		}

		install_filetype("rap");
		install_filetype("edat");
	}

	if (installed_rap_and_edat_count > 0)
	{
		// Refresh game list since we probably unlocked some games now.
		m_game_list_frame->Refresh(true);
	}

	// Find remaining package files
	file_paths = file_paths.filter(QRegularExpression(".*\\.pkg", QRegularExpression::PatternOption::CaseInsensitiveOption));

	if (file_paths.isEmpty())
	{
		return true;
	}

	if (from_boot)
	{
		return HandlePackageInstallation(file_paths, true);
	}

	// Handle further installations with a timeout. Otherwise the source explorer instance is not usable during the following file processing.
	QTimer::singleShot(0, [this, paths = std::move(file_paths)]()
	{
		HandlePackageInstallation(paths, false);
	});

	return true;
}

bool main_window::HandlePackageInstallation(QStringList file_paths, bool from_boot)
{
	if (file_paths.empty())
	{
		return false;
	}

	std::vector<compat::package_info> packages;

	game_compatibility* compat = m_game_list_frame ? m_game_list_frame->GetGameCompatibility() : nullptr;

	if (file_paths.size() > 1)
	{
		// Let the user choose the packages to install and select the order in which they shall be installed.
		pkg_install_dialog dlg(file_paths, compat, this);
		connect(&dlg, &QDialog::accepted, this, [&packages, &dlg]()
		{
			packages = dlg.GetPathsToInstall();
		});
		dlg.exec();
	}
	else
	{
		packages.push_back(game_compatibility::GetPkgInfo(file_paths.front(), compat));
	}

	if (packages.empty())
	{
		return true;
	}

	if (!from_boot)
	{
		if (!m_gui_settings->GetBootConfirmation(this))
		{
			return true;
		}

		Emu.GracefulShutdown(false);
	}

	std::vector<std::string> path_vec;
	for (const compat::package_info& pkg : packages)
	{
		path_vec.push_back(pkg.path.toStdString());
	}
	gui_log.notice("About to install packages:\n%s", fmt::merge(path_vec, "\n"));

	progress_dialog pdlg(tr("RPCS3 Package Installer"), tr("Installing package, please wait..."), tr("Cancel"), 0, 1000, false, this);
	pdlg.setAutoClose(false);
	pdlg.show();

	package_install_result result = {};

	auto get_app_info = [](compat::package_info& package)
	{
		QString app_info = package.title; // This should always be non-empty

		if (!package.title_id.isEmpty() || !package.version.isEmpty())
		{
			app_info += QStringLiteral("\n");

			if (!package.title_id.isEmpty())
			{
				app_info += package.title_id;
			}

			if (!package.version.isEmpty())
			{
				if (!package.title_id.isEmpty())
				{
					app_info += " ";
				}

				app_info += tr("v.%0", "Package version for install progress dialog").arg(package.version);
			}
		}

		return app_info;
	};

	bool cancelled = false;

	std::deque<package_reader> readers;

	for (const compat::package_info& info : packages)
	{
		readers.emplace_back(info.path.toStdString());
	}

	std::deque<std::string> bootable_paths;

	// Run PKG unpacking asynchronously
	named_thread worker("PKG Installer", [&readers, &result, &bootable_paths]
	{
		result = package_reader::extract_data(readers, bootable_paths);
		return result.error == package_install_result::error_type::no_error;
	});

	pdlg.show();

	// Wait for the completion
	int reader_it = 0;
	int set_text = -1;

	qt_events_aware_op(5, [&, readers_size = ::narrow<int>(readers.size())]()
	{
		if (reader_it == readers_size || result.error != package_install_result::error_type::no_error)
		{
			// Exit loop
			return true;
		}

		if (pdlg.wasCanceled())
		{
			cancelled = true;

			for (package_reader& reader : readers)
			{
				reader.abort_extract();
			}

			// Exit loop
			return true;
		}

		// Update progress window
		const int progress = readers[reader_it].get_progress(pdlg.maximum());
		pdlg.SetValue(progress);

		if (set_text != reader_it)
		{
			pdlg.setLabelText(tr("Installing package (%0/%1), please wait...\n\n%2").arg(reader_it + 1).arg(readers_size).arg(get_app_info(packages[reader_it])));
			set_text = reader_it;
		}

		if (progress == pdlg.maximum())
		{
			reader_it++;
		}

		// Process events
		return false;
	});

	const bool success = worker();

	if (success)
	{
		pdlg.SetValue(pdlg.maximum());

		const u64 start_time = get_system_time();

		for (usz i = 0; i < packages.size(); i++)
		{
			const compat::package_info& package = ::at32(packages, i);
			const package_reader& reader = ::at32(readers, i);

			switch (reader.get_result())
			{
			case package_reader::result::success:
			{
				gui_log.success("Successfully installed %s (title_id=%s, title=%s, version=%s).", package.path, package.title_id, package.title, package.version);
				break;
			}
			case package_reader::result::not_started:
			case package_reader::result::started:
			case package_reader::result::aborted:
			{
				gui_log.notice("Aborted installation of %s (title_id=%s, title=%s, version=%s).", package.path, package.title_id, package.title, package.version);
				break;
			}
			case package_reader::result::error:
			{
				gui_log.error("Failed to install %s (title_id=%s, title=%s, version=%s).", package.path, package.title_id, package.title, package.version);
				break;
			}
			case package_reader::result::aborted_dirty:
			case package_reader::result::error_dirty:
			{
				gui_log.error("Partially installed %s (title_id=%s, title=%s, version=%s).", package.path, package.title_id, package.title, package.version);
				break;
			}
			}
		}

		std::map<std::string, QString> bootable_paths_installed; // -> title id

		for (usz index = 0; index < bootable_paths.size(); index++)
		{
			if (bootable_paths[index].empty())
			{
				continue;
			}

			bootable_paths_installed[bootable_paths[index]] = packages[index].title_id;
		}

		// Need to test here due to potential std::move later
		const bool installed_a_whole_package_without_new_software = bootable_paths_installed.empty() && !cancelled;

		if (!bootable_paths_installed.empty())
		{
			m_game_list_frame->AddRefreshedSlot([this, paths = std::move(bootable_paths_installed)](std::set<std::string>& claimed_paths) mutable
			{
				// Try to claim operaions on ID
				for (auto it = paths.begin(); it != paths.end();)
				{
					std::string resolved_path = Emu.GetCallbacks().resolve_path(it->first);

					if (resolved_path.empty() || claimed_paths.count(resolved_path))
					{
						it = paths.erase(it);
					}
					else
					{
						claimed_paths.emplace(std::move(resolved_path));
						it++;
					}
				}

				// Executes after PrecompileCachesFromInstalledPackages
				m_notify_batch_game_action_cb = [this, paths]() mutable
				{
					ShowOptionalGamePreparations(tr("Success!"), tr("Successfully installed software from package(s)!"), std::move(paths));
				};

				PrecompileCachesFromInstalledPackages(paths);
			});
		}

		m_game_list_frame->Refresh(true);

		std::this_thread::sleep_for(std::chrono::microseconds(100'000 - std::min<usz>(100'000, get_system_time() - start_time)));
		pdlg.hide();

		if (installed_a_whole_package_without_new_software)
		{
			m_gui_settings->ShowInfoBox(tr("Success!"), tr("Successfully installed software from package(s)!"), gui::ib_pkg_success, this);
		}
	}
	else
	{
		pdlg.hide();
		pdlg.SignalFailure();

		if (!cancelled)
		{
			const compat::package_info* package = nullptr;

			for (usz i = 0; i < readers.size() && !package; i++)
			{
				// Figure out what package failed the installation
				switch (readers[i].get_result())
				{
				case package_reader::result::success:
				case package_reader::result::not_started:
				case package_reader::result::started:
				case package_reader::result::aborted:
				case package_reader::result::aborted_dirty:
					break;
				case package_reader::result::error:
				case package_reader::result::error_dirty:
					package = &packages[i];
					break;
				}
			}

			ensure(package);

			if (result.error == package_install_result::error_type::app_version)
			{
				gui_log.error("Cannot install %s.", package->path);
				const bool has_expected = !result.version.expected.empty();
				const bool has_found = !result.version.found.empty();
				if (has_expected && has_found)
				{
					QMessageBox::warning(this, tr("Warning!"), tr("Package cannot be installed on top of the current data.\nUpdate is for version %1, but you have version %2.\n\nTried to install: %3")
							.arg(QString::fromStdString(result.version.expected)).arg(QString::fromStdString(result.version.found)).arg(package->path));
				}
				else if (has_expected)
				{
					QMessageBox::warning(this, tr("Warning!"), tr("Package cannot be installed on top of the current data.\nUpdate is for version %1, but you don't have any data installed.\n\nTried to install: %2")
							.arg(QString::fromStdString(result.version.expected)).arg(package->path));
				}
				else
				{
					// probably unreachable
					const QString found = has_found ? tr("version %1").arg(QString::fromStdString(result.version.found)) : tr("no data installed");
					QMessageBox::warning(this, tr("Warning!"), tr("Package cannot be installed on top of the current data.\nUpdate is for unknown version, but you have version %1.\n\nTried to install: %2")
							.arg(QString::fromStdString(result.version.expected)).arg(found).arg(package->path));
				}
			}
			else
			{
				gui_log.error("Failed to install %s.", package->path);
				QMessageBox::critical(this, tr("Failure!"), tr("Failed to install software from package:\n%1!"
					"\nThis is very likely caused by external interference from a faulty anti-virus software."
					"\nPlease add RPCS3 to your anti-virus\' whitelist or use better anti-virus software.").arg(package->path));
			}
		}
	}

	return success;
}

void main_window::ExtractMSELF()
{
	const QString path_last_mself = m_gui_settings->GetValue(gui::fd_ext_mself).toString();
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select MSELF To extract"), path_last_mself, tr("All mself files (*.mself *.MSELF);;All files (*.*)"));

	if (file_path.isEmpty())
	{
		return;
	}

	const QString dir = QFileDialog::getExistingDirectory(this, tr("Extraction Directory"), QString{}, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty())
	{
		m_gui_settings->SetValue(gui::fd_ext_mself, QFileInfo(file_path).path());
		extract_mself(file_path.toStdString(), dir.toStdString() + '/');
	}
}

void main_window::InstallPup(QString file_path)
{
	if (file_path.isEmpty())
	{
		const QString path_last_pup = m_gui_settings->GetValue(gui::fd_install_pup).toString();
		file_path = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To Install"), path_last_pup, tr("PS3 update file (PS3UPDAT.PUP);;All pup files (*.pup *.PUP);;All files (*.*)"));
	}
	else
	{
		if (QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Install firmware: %1?").arg(file_path),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			gui_log.notice("Firmware: Cancelled installation from drop. File: %s", file_path);
			return;
		}
	}

	if (!file_path.isEmpty())
	{
		// Handle the actual installation with a timeout. Otherwise the source explorer instance is not usable during the following file processing.
		QTimer::singleShot(0, [this, file_path]()
		{
			HandlePupInstallation(file_path);
		});
	}
}

void main_window::ExtractPup()
{
	const QString path_last_pup = m_gui_settings->GetValue(gui::fd_install_pup).toString();
	const QString file_path = QFileDialog::getOpenFileName(this, tr("Select PS3UPDAT.PUP To extract"), path_last_pup, tr("PS3 update file (PS3UPDAT.PUP);;All pup files (*.pup *.PUP);;All files (*.*)"));

	if (file_path.isEmpty())
	{
		return;
	}

	const QString dir = QFileDialog::getExistingDirectory(this, tr("Extraction Directory"), QString{}, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (!dir.isEmpty())
	{
		HandlePupInstallation(file_path, dir);
	}
}

void main_window::ExtractTar()
{
	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.GracefulShutdown(false);

	const QString path_last_tar = m_gui_settings->GetValue(gui::fd_ext_tar).toString();
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Select TAR To extract"), path_last_tar, tr("All tar files (*.tar *.TAR *.tar.aa.* *.TAR.AA.*);;All files (*.*)"));

	if (files.isEmpty())
	{
		return;
	}

	const QString dir = QFileDialog::getExistingDirectory(this, tr("Extraction Directory"), QString{}, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir.isEmpty())
	{
		return;
	}

	m_gui_settings->SetValue(gui::fd_ext_tar, QFileInfo(files[0]).path());

	progress_dialog pdlg(tr("TAR Extraction"), tr("Extracting encrypted TARs\nPlease wait..."), tr("Cancel"), 0, files.size(), false, this);
	pdlg.show();

	QString error;

	auto files_it = files.begin();
	int pdlg_progress = 0;

	qt_events_aware_op(0, [&]()
	{
		if (pdlg.wasCanceled() || files_it == files.end())
		{
			// Exit loop
			return true;
		}

		const QString& file = *files_it;

		// Do not abort on failure here, in case the user selected a wrong file in multi-selection while the rest are valid
		if (!extract_tar(file.toStdString(), dir.toStdString() + '/'))
		{
			if (error.isEmpty())
			{
				error = tr("The following TAR file(s) could not be extracted:");
			}

			error += "\n";
			error += file;
		}

		pdlg_progress++;
		pdlg.SetValue(pdlg_progress);

		files_it++;
		return false;
	});

	if (!error.isEmpty())
	{
		pdlg.hide();
		QMessageBox::critical(this, tr("TAR extraction failed"), error);
	}
}

void main_window::HandlePupInstallation(const QString& file_path, const QString& dir_path)
{
	const auto critical = [this](QString str)
	{
		Emu.CallFromMainThread([this, str = std::move(str)]()
		{
			QMessageBox::critical(this, tr("Firmware Installation Failed"), str);
		}, nullptr, false);
	};

	if (file_path.isEmpty())
	{
		gui_log.error("Error while installing firmware: provided path is empty.");
		critical(tr("Firmware installation failed: The provided path is empty."));
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.GracefulShutdown(false);

	m_gui_settings->SetValue(gui::fd_install_pup, QFileInfo(file_path).path());

	const std::string path = file_path.toStdString();

	fs::file pup_f(path);
	if (!pup_f)
	{
		gui_log.error("Error opening PUP file %s (%s)", path, fs::g_tls_error);
		critical(tr("Firmware installation failed: The selected firmware file couldn't be opened."));
		return;
	}

	pup_object pup(std::move(pup_f));

	switch (pup.operator pup_error())
	{
	case pup_error::header_read:
	{
		gui_log.error("%s", pup.get_formatted_error());
		critical(tr("Firmware installation failed: The provided file is empty."));
		return;
	}
	case pup_error::header_magic:
	{
		gui_log.error("Error while installing firmware: provided file is not a PUP file.");
		critical(tr("Firmware installation failed: The provided file is not a PUP file."));
		return;
	}
	case pup_error::expected_size:
	{
		gui_log.error("%s", pup.get_formatted_error());
		critical(tr("Firmware installation failed: The provided file is incomplete. Try redownloading it."));
		return;
	}
	case pup_error::header_file_count:
	case pup_error::file_entries:
	case pup_error::stream:
	{
		std::string error = "Error while installing firmware: PUP file is invalid.";

		if (!pup.get_formatted_error().empty())
		{
			fmt::append(error, "\n%s", pup.get_formatted_error());
		}

		gui_log.error("%s", error);
		critical(tr("Firmware installation failed: The provided file is corrupted."));
		return;
	}
	case pup_error::hash_mismatch:
	{
		gui_log.error("Error while installing firmware: Hash check failed.");
		critical(tr("Firmware installation failed: The provided file's contents are corrupted."));
		return;
	}
	case pup_error::ok: break;
	}

	fs::file update_files_f = pup.get_file(0x300);

	const usz update_files_size = update_files_f ? update_files_f.size() : 0;

	if (!update_files_size)
	{
		gui_log.error("Error while installing firmware: Couldn't find installation packages database.");
		critical(tr("Firmware installation failed: The provided file's contents are corrupted."));
		return;
	}

	fs::device_stat dev_stat{};
	if (!fs::statfs(g_cfg_vfs.get_dev_flash(), dev_stat))
	{
		gui_log.error("Error while installing firmware: Couldn't retrieve available disk space. ('%s')", g_cfg_vfs.get_dev_flash());
		critical(tr("Firmware installation failed: Couldn't retrieve available disk space."));
		return;
	}

	if (dev_stat.avail_free < update_files_size)
	{
		gui_log.error("Error while installing firmware: Out of disk space. ('%s', needed: %d bytes)", g_cfg_vfs.get_dev_flash(), update_files_size - dev_stat.avail_free);
		critical(tr("Firmware installation failed: Out of disk space."));
		return;
	}

	tar_object update_files(update_files_f);

	if (!dir_path.isEmpty())
	{
		// Extract only mode, extract direct TAR entries to a user directory

		if (!vfs::mount("/pup_extract", dir_path.toStdString() + '/'))
		{
			gui_log.error("Error while extracting firmware: Failed to mount '%s'", dir_path);
			critical(tr("Firmware extraction failed: VFS mounting failed."));
			return;
		}

		if (!update_files.extract("/pup_extract", true))
		{
			gui_log.error("Error while installing firmware: TAR contents are invalid.");
			critical(tr("Firmware installation failed: Firmware contents could not be extracted."));
		}

		gui_log.success("Extracted PUP file to %s", dir_path);
		return;
	}

	// In regular installation we select specfic entries from the main TAR which are prefixed with "dev_flash_"
	// Those entries are TAR as well, we extract their packed files from them and that's what installed in /dev_flash

	auto update_filenames = update_files.get_filenames();

	update_filenames.erase(std::remove_if(
		update_filenames.begin(), update_filenames.end(), [](const std::string& s) { return s.find("dev_flash_") == umax; }),
		update_filenames.end());

	if (update_filenames.empty())
	{
		gui_log.error("Error while installing firmware: No dev_flash_* packages were found.");
		critical(tr("Firmware installation failed: The provided file's contents are corrupted."));
		return;
	}

	static constexpr std::string_view cur_version = "4.92";

	std::string version_string;

	if (fs::file version = pup.get_file(0x100))
	{
		version_string = version.to_string();
	}

	if (const usz version_pos = version_string.find('\n'); version_pos != umax)
	{
		version_string.erase(version_pos);
	}

	if (version_string.empty())
	{
		gui_log.error("Error while installing firmware: No version data was found.");
		critical(tr("Firmware installation failed: The provided file's contents are corrupted."));
		return;
	}

	if (version_string < cur_version &&
		QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Old firmware detected.\nThe newest firmware version is %1 and you are trying to install version %2\nContinue installation?").arg(QString::fromUtf8(cur_version.data(), ::size32(cur_version)), QString::fromStdString(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
	{
		return;
	}

	if (const std::string installed = utils::get_firmware_version(); !installed.empty())
	{
		gui_log.warning("Reinstalling firmware: old=%s, new=%s", installed, version_string);

		if (QMessageBox::question(this, tr("RPCS3 Firmware Installer"), tr("Firmware of version %1 has already been installed.\nOverwrite current installation with version %2?").arg(QString::fromStdString(installed), QString::fromStdString(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
		{
			gui_log.warning("Reinstallation of firmware aborted.");
			return;
		}
	}

	// Remove possibly PS3 fonts from database
	QFontDatabase::removeAllApplicationFonts();

	progress_dialog pdlg(tr("RPCS3 Firmware Installer"), tr("Installing firmware version %1\nPlease wait...").arg(QString::fromStdString(version_string)), tr("Cancel"), 0, static_cast<int>(update_filenames.size()), false, this);
	pdlg.show();

	// Used by tar_object::extract() as destination directory
	vfs::mount("/dev_flash", g_cfg_vfs.get_dev_flash());

	// Synchronization variable
	atomic_t<uint> progress(0);
	{
		// Run asynchronously
		named_thread worker("Firmware Installer", [&]
		{
			for (const auto& update_filename : update_filenames)
			{
				auto update_file_stream = update_files.get_file(update_filename);

				if (update_file_stream->m_file_handler)
				{
					// Forcefully read all the data
					update_file_stream->m_file_handler->handle_file_op(*update_file_stream, 0, update_file_stream->get_size(umax), nullptr);
				}

				fs::file update_file = fs::make_stream(std::move(update_file_stream->data));

				SCEDecrypter self_dec(update_file);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3)
				{
					gui_log.error("Error while installing firmware: PUP contents are invalid.");
					critical(tr("Firmware installation failed: Firmware could not be decompressed"));
					progress = -1;
					return;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract())
				{
					gui_log.error("Error while installing firmware: TAR contents are invalid. (package=%s)", update_filename);
					critical(tr("The firmware contents could not be extracted."
						"\nThis is very likely caused by external interference from a faulty anti-virus software."
						"\nPlease add RPCS3 to your anti-virus\' whitelist or use better anti-virus software."));

					progress = -1;
					return;
				}

				if (!progress.try_inc(::narrow<uint>(update_filenames.size())))
				{
					// Installation was cancelled
					return;
				}
			}
		});

		// Wait for the completion
		qt_events_aware_op(5, [&]()
		{
			const uint value = progress.load();

			if (value >= update_filenames.size())
			{
				return true;
			}

			if (pdlg.wasCanceled())
			{
				progress = -1;
				return true;
			}

			// Update progress window
			pdlg.SetValue(static_cast<int>(value));
			return false;
		});

		// Join thread
		worker();
	}

	update_files_f.close();

	if (progress == update_filenames.size())
	{
		pdlg.SetValue(pdlg.maximum());
		std::this_thread::sleep_for(100ms);
	}

	// Update with newly installed PS3 fonts
	Q_EMIT RequestGlobalStylesheetChange();

	// Unmount
	Emu.Init();

	if (progress == update_filenames.size())
	{
		ui->bootVSHAct->setEnabled(fs::is_file(g_cfg_vfs.get_dev_flash() + "/vsh/module/vsh.self"));

		gui_log.success("Successfully installed PS3 firmware version %s.", version_string);
		m_gui_settings->ShowInfoBox(tr("Success!"), tr("Successfully installed PS3 firmware and LLE Modules!"), gui::ib_pup_success, this);

		CreateFirmwareCache();
	}
}

void main_window::DecryptSPRXLibraries()
{
	QString path_last_sprx = m_gui_settings->GetValue(gui::fd_decrypt_sprx).toString();

	if (!fs::is_dir(path_last_sprx.toStdString()))
	{
		// Default: redirect to userland firmware SPRX directory
		path_last_sprx = QString::fromStdString(g_cfg_vfs.get_dev_flash() + "sys/external");
	}

	const QStringList modules = QFileDialog::getOpenFileNames(this, tr("Select binary files"), path_last_sprx, tr("All Binaries (*.bin *.BIN *.self *.SELF *.sprx *.SPRX *.sdat *.SDAT *.edat *.EDAT);;"
		"BIN files (*.bin *.BIN);;SELF files (*.self *.SELF);;SPRX files (*.sprx *.SPRX);;SDAT/EDAT files (*.sdat *.SDAT *.edat *.EDAT);;All files (*.*)"));

	if (modules.isEmpty())
	{
		return;
	}

	m_gui_settings->SetValue(gui::fd_decrypt_sprx, QFileInfo(modules.first()).path());

	std::vector<std::string> vec_modules;
	for (const QString& mod : modules)
	{
		vec_modules.push_back(mod.toStdString());
	}

	auto iterate = std::make_shared<std::function<void(usz, usz)>>();
	const auto decrypter = std::make_shared<decrypt_binaries_t>(std::move(vec_modules));

	*iterate = [this, iterate, decrypter](usz mod_index, usz repeat_count)
	{
		const std::string& path = (*decrypter)[mod_index];
		const std::string filename = path.substr(path.find_last_of(fs::delim) + 1);

		const QString hint = tr("Hint: KLIC (KLicense key) is a 16-byte long string. (32 hexadecimal characters, can be prefixed with \"KLIC=0x\" from the log message)"
			"\nAnd is logged with some sceNpDrm* functions when the game/application which owns \"%0\" is running.").arg(QString::fromStdString(filename));

		if (repeat_count >= 2)
		{
			gui_log.error("Failed to decrypt %s with specified KLIC, retrying.\n%s", path, hint);
		}

		input_dialog* dlg = new input_dialog(39, "", tr("Enter KLIC of %0").arg(QString::fromStdString(filename)),
			repeat_count >= 2 ? tr("Decryption failed with provided KLIC.\n%0").arg(hint) : tr("Hexadecimal value."), "KLIC=0x00000000000000000000000000000000", this);

		QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
		mono.setPointSize(8);
		dlg->set_input_font(mono, true, '0');
		dlg->set_clear_button_enabled(false);
		dlg->set_button_enabled(QDialogButtonBox::StandardButton::Ok, false);
		dlg->set_validator(new QRegularExpressionValidator(QRegularExpression("^((((((K?L)?I)?C)?=)?0)?x)?[a-fA-F0-9]{0,32}$"), this)); // HEX only (with additional KLIC=0x prefix for convenience)
		dlg->setAttribute(Qt::WA_DeleteOnClose);

		connect(dlg, &input_dialog::text_changed, dlg, [dlg](const QString& text)
		{
			dlg->set_button_enabled(QDialogButtonBox::StandardButton::Ok, text.size() - (text.indexOf('x') + 1) == 32);
		});

		connect(dlg, &QDialog::accepted, this, [this, iterate, dlg, mod_index, decrypter, repeat_count]()
		{
			std::string text = dlg->get_input_text().toStdString();

			if (usz new_index = decrypter->decrypt(std::move(text)); !decrypter->done())
			{
				QTimer::singleShot(0, [iterate, mod_index, repeat_count, new_index]()
				{
					// Increase repeat count if "stuck" on the same file
					(*iterate)(new_index, new_index == mod_index ? repeat_count + 1 : 0);
				});
			}
		});

		connect(dlg, &QDialog::rejected, this, []()
		{
			gui_log.notice("User has cancelled entering KLIC.");
		});

		dlg->show();
	};

	if (usz new_index = decrypter->decrypt(); !decrypter->done())
	{
		(*iterate)(new_index, new_index == 0 ? 1 : 0);
	}
}

/** Needed so that when a backup occurs of window state in gui_settings, the state is current.
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState() const
{
	// Save gui settings
	m_gui_settings->SetValue(gui::mw_geometry, saveGeometry(), false);
	m_gui_settings->SetValue(gui::mw_windowState, saveState(), false);

	// NOTE:
	//
	// This method is also invoked in case the gui_application::Init() method failed ("false" was returned)
	// to initialize some modules leaving other modules uninitialized (NULL pointed).
	// So, the following checks on NULL pointer are provided before accessing the related module's object

	if (m_mw)
	{
		m_gui_settings->SetValue(gui::mw_mwState, m_mw->saveState(), true);
	}

	if (m_game_list_frame)
	{
		// Save column settings
		m_game_list_frame->SaveSettings();
	}

	if (m_debugger_frame)
	{
		// Save splitter state
		m_debugger_frame->SaveSettings();
	}
}

void main_window::RepaintThumbnailIcons()
{
	const QColor color = gui::utils::get_foreground_color();
	[[maybe_unused]] const QColor new_color = gui::utils::get_label_color("thumbnail_icon_color", color, color);

	[[maybe_unused]] const auto icon = [&new_color](const QString& path)
	{
		return gui::utils::get_colorized_icon(QPixmap::fromImage(gui::utils::get_opaque_image_area(path)), Qt::black, new_color);
	};
}

void main_window::RepaintToolBarIcons()
{
	const QColor color = gui::utils::get_foreground_color();

	std::map<QIcon::Mode, QColor> new_colors{};
	new_colors[QIcon::Normal] = gui::utils::get_label_color("toolbar_icon_color", color, color);
	new_colors[QIcon::Disabled] = gui::utils::get_label_color("toolbar_icon_color_disabled", Qt::gray, Qt::lightGray);
	new_colors[QIcon::Active] = gui::utils::get_label_color("toolbar_icon_color_active", color, color);
	new_colors[QIcon::Selected] = gui::utils::get_label_color("toolbar_icon_color_selected", color, color);

	const auto icon = [&new_colors](const QString& path)
	{
		return gui::utils::get_colorized_icon(QIcon(path), Qt::black, new_colors);
	};

	m_icon_play           = icon(":/Icons/play.png");
	m_icon_pause          = icon(":/Icons/pause.png");
	m_icon_restart        = icon(":/Icons/restart.png");
	m_icon_fullscreen_on  = icon(":/Icons/fullscreen.png");
	m_icon_fullscreen_off = icon(":/Icons/exit_fullscreen.png");

	ui->toolbar_config  ->setIcon(icon(":/Icons/configure.png"));
	ui->toolbar_controls->setIcon(icon(":/Icons/controllers.png"));
	ui->toolbar_open    ->setIcon(icon(":/Icons/open.png"));
	ui->toolbar_grid    ->setIcon(icon(":/Icons/grid.png"));
	ui->toolbar_list    ->setIcon(icon(":/Icons/list.png"));
	ui->toolbar_refresh ->setIcon(icon(":/Icons/refresh.png"));
	ui->toolbar_stop    ->setIcon(icon(":/Icons/stop.png"));

	ui->sysStopAct->setIcon(icon(":/Icons/stop.png"));
	ui->sysRebootAct->setIcon(m_icon_restart);

	if (Emu.IsRunning())
	{
		ui->toolbar_start->setIcon(m_icon_pause);
		ui->sysPauseAct->setIcon(m_icon_pause);
	}
	else if (Emu.IsStopped() && !Emu.GetBoot().empty())
	{
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->sysPauseAct->setIcon(m_icon_restart);
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->sysPauseAct->setIcon(m_icon_play);
	}

	if (isFullScreen())
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
	}
	else
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
	}

	const QColor& new_color = new_colors[QIcon::Normal];
	ui->sizeSlider->setStyleSheet(ui->sizeSlider->styleSheet().append("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(new_color.red()).arg(new_color.green()).arg(new_color.blue()).arg(new_color.alpha()));

	// resize toolbar elements

	const int tool_bar_height = ui->toolBar->sizeHint().height();

	for (const auto& act : ui->toolBar->actions())
	{
		if (act->isSeparator())
		{
			continue;
		}

		ui->toolBar->widgetForAction(act)->setMinimumWidth(tool_bar_height);
	}

	ui->sizeSliderContainer->setFixedWidth(tool_bar_height * 4);
	ui->mw_searchbar->setFixedWidth(tool_bar_height * 5);
}

void main_window::OnEmuRun(bool /*start_playtime*/)
{
	const QString title = GetCurrentTitle();
	const QString restart_tooltip = tr("Restart %0").arg(title);
	const QString pause_tooltip = tr("Pause %0").arg(title);
	const QString stop_tooltip = tr("Stop %0").arg(title);

	m_debugger_frame->EnableButtons(true);

	ui->sysPauseAct->setText(tr("&Pause"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(pause_tooltip);
	ui->toolbar_stop->setToolTip(stop_tooltip);

	EnableMenus(true);

	update_gui_pad_thread();
}

void main_window::OnEmuResume() const
{
	const QString title = GetCurrentTitle();
	const QString restart_tooltip = tr("Restart %0").arg(title);
	const QString pause_tooltip = tr("Pause %0").arg(title);
	const QString stop_tooltip = tr("Stop %0").arg(title);

	ui->sysPauseAct->setText(tr("&Pause"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setText(tr("Pause"));
	ui->toolbar_start->setToolTip(pause_tooltip);
	ui->toolbar_stop->setToolTip(stop_tooltip);
}

void main_window::OnEmuPause() const
{
	const QString title = GetCurrentTitle();
	const QString resume_tooltip = tr("Resume %0").arg(title);

	ui->sysPauseAct->setText(tr("&Resume"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(resume_tooltip);

	// Refresh game list in order to update time played
	if (m_game_list_frame)
	{
		m_game_list_frame->Refresh();
	}
}

void main_window::OnEmuStop()
{
	const QString title = GetCurrentTitle();
	const QString play_tooltip = tr("Play %0").arg(title);

	ui->sysPauseAct->setText(tr("&Play"));
	ui->sysPauseAct->setIcon(m_icon_play);

	EnableMenus(false);

	if (title.isEmpty())
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->toolbar_start->setText(tr("Play"));
		ui->toolbar_start->setToolTip(play_tooltip);
	}
	else
	{
		const QString restart_tooltip = tr("Restart %0").arg(title);

		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->toolbar_start->setText(tr("Restart"));
		ui->toolbar_start->setToolTip(restart_tooltip);
		ui->sysRebootAct->setEnabled(true);
	}

	ui->batchRemoveShaderCachesAct->setEnabled(true);
	ui->batchRemovePPUCachesAct->setEnabled(true);
	ui->batchRemoveSPUCachesAct->setEnabled(true);
	ui->removeHDD1CachesAct->setEnabled(true);
	ui->removeAllCachesAct->setEnabled(true);
	ui->removeSavestatesAct->setEnabled(true);
	ui->cleanUpGameListAct->setEnabled(true);

	ui->actionManage_Users->setEnabled(true);
	ui->confCamerasAct->setEnabled(true);
	ui->actionPS_Move_Tracker->setEnabled(true);

	// Refresh game list in order to update time played
	if (m_game_list_frame && m_is_list_mode)
	{
		m_game_list_frame->Refresh();
	}

	// Close kernel explorer if running
	if (m_kernel_explorer)
	{
		m_kernel_explorer->close();
	}

	// Close systen command dialog if running
	if (m_system_cmd_dialog)
	{
		m_system_cmd_dialog->close();
	}

	update_gui_pad_thread();
}

void main_window::OnEmuReady() const
{
	const QString title = GetCurrentTitle();
	const QString play_tooltip = tr("Play %0").arg(title);

	m_debugger_frame->EnableButtons(true);

	ui->sysPauseAct->setText(tr("&Play"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setText(tr("Play"));
	ui->toolbar_start->setToolTip(play_tooltip);

	EnableMenus(true);

	ui->actionManage_Users->setEnabled(false);
	ui->confCamerasAct->setEnabled(false);
	ui->actionPS_Move_Tracker->setEnabled(false);

	ui->batchRemoveShaderCachesAct->setEnabled(false);
	ui->batchRemovePPUCachesAct->setEnabled(false);
	ui->batchRemoveSPUCachesAct->setEnabled(false);
	ui->removeHDD1CachesAct->setEnabled(false);
	ui->removeAllCachesAct->setEnabled(false);
	ui->removeSavestatesAct->setEnabled(false);
	ui->cleanUpGameListAct->setEnabled(false);
}

void main_window::EnableMenus(bool enabled) const
{
	// Toolbar
	ui->toolbar_start->setEnabled(enabled);
	ui->toolbar_stop->setEnabled(enabled);

	// Emulation
	ui->sysPauseAct->setEnabled(enabled);
	ui->sysStopAct->setEnabled(enabled);
	ui->sysRebootAct->setEnabled(enabled);

	// Tools
	ui->toolskernel_explorerAct->setEnabled(enabled);
	ui->toolsmemory_viewerAct->setEnabled(enabled);
	ui->toolsRsxDebuggerAct->setEnabled(enabled);
	ui->toolsSystemCommandsAct->setEnabled(enabled);
	ui->actionCreate_RSX_Capture->setEnabled(enabled);
	ui->actionCreate_Savestate->setEnabled(enabled);
}

void main_window::OnAddBreakpoint(u32 addr) const
{
	if (m_debugger_frame)
	{
		m_debugger_frame->PerformAddBreakpointRequest(addr);
	}
}

void main_window::OnEnableDiscEject(bool enabled) const
{
	ui->ejectDiscAct->setEnabled(enabled);
}

void main_window::OnEnableDiscInsert(bool enabled) const
{
	ui->insertDiscAct->setEnabled(enabled);
}

void main_window::BootRecentAction(const QAction* act, bool is_savestate)
{
	if (Emu.IsRunning())
	{
		return;
	}

	recent_game_wrapper& rgw = is_savestate ? m_recent_save : m_recent_game;
	QMenu* menu = is_savestate ? ui->bootRecentSavestatesMenu : ui->bootRecentMenu;

	const QString pth = act->data().toString();
	const std::string path = pth.toStdString();
	QString name;
	bool contains_path = false;

	int idx = -1;
	for (int i = 0; i < rgw.entries.count(); i++)
	{
		const auto& entry = rgw.entries[i];
		if (entry.first == pth)
		{
			idx = i;
			contains_path = true;
			name = entry.second;
			break;
		}
	}

	// path is invalid: remove action from list return
	if ((contains_path && name.isEmpty()) || !fs::exists(path))
	{
		if (contains_path)
		{
			// clear menu of actions
			for (QAction* action : rgw.actions)
			{
				menu->removeAction(action);
			}

			// remove action from list
			rgw.entries.removeAt(idx);
			rgw.actions.removeAt(idx);

			m_gui_settings->SetValue(is_savestate ? gui::rs_entries : gui::rg_entries, gui_settings::List2Var(rgw.entries));

			gui_log.error("Recent Game not valid, removed from Boot Recent list: %s", path);

			// refill menu with actions
			for (int i = 0; i < rgw.actions.count(); i++)
			{
				rgw.actions[i]->setShortcut(QString("%0+%1").arg(is_savestate ? "Alt" : "Ctrl").arg(i + 1));
				rgw.actions[i]->setToolTip(::at32(rgw.entries, i).second + "\n" + ::at32(rgw.entries, i).first);
				menu->addAction(rgw.actions[i]);
			}

			gui_log.warning("Boot Recent list refreshed");
			return;
		}

		gui_log.error("Path invalid and not in m_rg_paths: %s", path);
		return;
	}

	gui_log.notice("Booting from recent games list...");
	Boot(path, "", true);
}

QAction* main_window::CreateRecentAction(const q_string_pair& entry, u32 sc_idx, bool is_savestate)
{
	recent_game_wrapper& rgw = is_savestate ? m_recent_save : m_recent_game;

	// if path is not valid remove from list
	if (entry.second.isEmpty() || (!QFileInfo(entry.first).isDir() && !QFileInfo(entry.first).isFile()))
	{
		if (rgw.entries.contains(entry))
		{
			gui_log.warning("Recent Game not valid, removing from Boot Recent list: %s", entry.first);

			const int idx = rgw.entries.indexOf(entry);
			rgw.entries.removeAt(idx);

			m_gui_settings->SetValue(is_savestate ? gui::rs_entries : gui::rg_entries, gui_settings::List2Var(rgw.entries));
		}
		return nullptr;
	}

	// if name is a path get filename
	QString shown_name = entry.second;
	if (QFileInfo(entry.second).isFile())
	{
		shown_name = entry.second.section('/', -1);
	}

	// create new action
	QAction* act = new QAction(shown_name, this);
	act->setData(entry.first);
	act->setToolTip(entry.second + "\n" + entry.first);
	act->setShortcut(QString("%0+%1").arg(is_savestate ? "Alt" : "Ctrl").arg(sc_idx));

	// truncate if too long
	if (shown_name.length() > 60)
	{
		act->setText(shown_name.left(27) + "(....)" + shown_name.right(27));
	}

	// connect boot
	connect(act, &QAction::triggered, this, [this, act, is_savestate](){ BootRecentAction(act, is_savestate); });

	return act;
}

void main_window::AddRecentAction(const q_string_pair& entry, bool is_savestate)
{
	QAction* freezeAction = is_savestate ? ui->freezeRecentSavestatesAct : ui->freezeRecentAct;

	// don't change list on freeze
	if (freezeAction->isChecked())
	{
		return;
	}

	// create new action, return if not valid
	QAction* act = CreateRecentAction(entry, 1, is_savestate);
	if (!act)
	{
		return;
	}

	recent_game_wrapper& rgw = is_savestate ? m_recent_save : m_recent_game;
	QMenu* menu = is_savestate ? ui->bootRecentSavestatesMenu : ui->bootRecentMenu;

	// clear menu of actions
	for (QAction* action : rgw.actions)
	{
		menu->removeAction(action);
	}

	// If path already exists, remove it in order to get it to beginning. Also remove duplicates.
	for (int i = rgw.entries.count() - 1; i >= 0; --i)
	{
		if (rgw.entries[i].first == entry.first)
		{
			rgw.entries.removeAt(i);
			rgw.actions.removeAt(i);
		}
	}

	// remove oldest action at the end if needed
	if (rgw.entries.count() == 9)
	{
		rgw.entries.removeLast();
		rgw.actions.removeLast();
	}
	else if (rgw.entries.count() > 9)
	{
		gui_log.error("Recent games entrylist too big");
	}

	if (rgw.entries.count() < 9)
	{
		// add new action at the beginning
		rgw.entries.prepend(entry);
		rgw.actions.prepend(act);
	}

	// refill menu with actions
	for (int i = 0; i < rgw.actions.count(); i++)
	{
		rgw.actions[i]->setShortcut(QString("%0+%1").arg(is_savestate ? "Alt" : "Ctrl").arg(i + 1));
		rgw.actions[i]->setToolTip(::at32(rgw.entries, i).second + "\n" + ::at32(rgw.entries, i).first);
		menu->addAction(rgw.actions[i]);
	}

	m_gui_settings->SetValue(is_savestate ? gui::rs_entries : gui::rg_entries, gui_settings::List2Var(rgw.entries));
}

void main_window::UpdateLanguageActions(const QStringList& language_codes, const QString& language_code)
{
	ui->languageMenu->clear();

	for (const auto& code : language_codes)
	{
		const QLocale locale      = QLocale(code);
		const QString locale_name = QLocale::languageToString(locale.language());

		// create new action
		QAction* act = new QAction(locale_name, this);
		act->setData(code);
		act->setToolTip(locale_name);
		act->setCheckable(true);
		act->setChecked(code == language_code);

		// connect to language changer
		connect(act, &QAction::triggered, this, [this, code]()
		{
			RequestLanguageChange(code);
		});

		ui->languageMenu->addAction(act);
	}
}

void main_window::UpdateFilterActions()
{
	ui->showCatHDDGameAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::HDD_Game, m_is_list_mode));
	ui->showCatDiscGameAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Disc_Game, m_is_list_mode));
	ui->showCatPS1GamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PS1_Game, m_is_list_mode));
	ui->showCatPS2GamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PS2_Game, m_is_list_mode));
	ui->showCatPSPGamesAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::PSP_Game, m_is_list_mode));
	ui->showCatHomeAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Home, m_is_list_mode));
	ui->showCatAudioVideoAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Media, m_is_list_mode));
	ui->showCatGameDataAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Data, m_is_list_mode));
	ui->showCatOSAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::OS, m_is_list_mode));
	ui->showCatUnknownAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Unknown_Cat, m_is_list_mode));
	ui->showCatOtherAct->setChecked(m_gui_settings->GetCategoryVisibility(Category::Others, m_is_list_mode));
}

void main_window::RepaintGui()
{
	if (m_game_list_frame)
	{
		m_game_list_frame->RepaintIcons(true);
	}

	if (m_log_frame)
	{
		m_log_frame->RepaintTextColors();
	}

	if (m_debugger_frame)
	{
		m_debugger_frame->ChangeColors();
	}

	RepaintToolBarIcons();
	RepaintThumbnailIcons();

	Q_EMIT RequestDialogRepaint();
}

void main_window::RetranslateUI(const QStringList& language_codes, const QString& language_code)
{
	UpdateLanguageActions(language_codes, language_code);

	ui->retranslateUi(this);

	if (m_game_list_frame)
	{
		m_game_list_frame->Refresh(true);
	}
}

void main_window::ShowTitleBars(bool show) const
{
	m_game_list_frame->SetTitleBarVisible(show);
	m_debugger_frame->SetTitleBarVisible(show);
	m_log_frame->SetTitleBarVisible(show);
}

void main_window::ShowOptionalGamePreparations(const QString& title, const QString& message, std::map<std::string, QString> bootable_paths)
{
	if (bootable_paths.empty())
	{
		m_gui_settings->ShowInfoBox(title, message, gui::ib_pkg_success, this);
		return;
	}

	QDialog* dlg = new QDialog(this);
	dlg->setObjectName("game_prepare_window");
	dlg->setWindowTitle(title);

	QVBoxLayout* vlayout = new QVBoxLayout(dlg);

	QCheckBox* desk_check = new QCheckBox(tr("Add desktop shortcut(s)"));
#ifdef _WIN32
	QCheckBox* quick_check = new QCheckBox(tr("Add Start menu shortcut(s)"));
#elif defined(__APPLE__)
	QCheckBox* quick_check = new QCheckBox(tr("Add dock shortcut(s)"));
#else
	QCheckBox* quick_check = new QCheckBox(tr("Add launcher shortcut(s)"));
#endif
	QLabel* label = new QLabel(tr("%1\nWould you like to install shortcuts to the installed software? (%2 new software detected)\n\n").arg(message).arg(bootable_paths.size()), dlg);

	vlayout->addWidget(label);
	vlayout->addStretch(10);
	vlayout->addWidget(desk_check);
	vlayout->addStretch(3);
	vlayout->addWidget(quick_check);
	vlayout->addStretch(3);

	QDialogButtonBox* btn_box = new QDialogButtonBox(QDialogButtonBox::Ok);

	vlayout->addWidget(btn_box);
	dlg->setLayout(vlayout);

	connect(btn_box, &QDialogButtonBox::accepted, this, [=, this, paths = std::move(bootable_paths)]()
	{
		const bool create_desktop_shortcuts = desk_check->isChecked();
		const bool create_app_shortcut = quick_check->isChecked();

		dlg->hide();
		dlg->accept();

		std::set<gui::utils::shortcut_location> locations;

#ifdef _WIN32
		locations.insert(gui::utils::shortcut_location::rpcs3_shortcuts);
#endif
		if (create_desktop_shortcuts)
		{
			locations.insert(gui::utils::shortcut_location::desktop);
		}

		if (create_app_shortcut)
		{
			locations.insert(gui::utils::shortcut_location::applications);
		}

		if (locations.empty())
		{
			return;
		}

		std::vector<game_info> game_data_shortcuts;

		for (const auto& [boot_path, title_id] : paths)
		{
			for (const game_info& gameinfo : m_game_list_frame->GetGameInfo())
			{
				if (gameinfo && gameinfo->info.serial == title_id.toStdString())
				{
					if (Emu.IsPathInsideDir(boot_path, gameinfo->info.path))
					{
						if (!locations.empty())
						{
							game_data_shortcuts.push_back(gameinfo);
						}
					}

					break;
				}
			}
		}

		if (!game_data_shortcuts.empty() && !locations.empty())
		{
			m_game_list_frame->CreateShortcuts(game_data_shortcuts, locations);
		}
	});

	dlg->setAttribute(Qt::WA_DeleteOnClose);
	dlg->open();
}


void main_window::PrecompileCachesFromInstalledPackages(const std::map<std::string, QString>& bootable_paths)
{
	std::vector<game_info> game_data;

	for (const auto& [boot_path, title_id] : bootable_paths)
	{
		for (const game_info& gameinfo : m_game_list_frame->GetGameInfo())
		{
			if (gameinfo && gameinfo->info.serial == title_id.toStdString())
			{
				if (Emu.IsPathInsideDir(boot_path, gameinfo->info.path))
				{
					game_data.push_back(gameinfo);
				}

				break;
			}
		}
	}

	if (!game_data.empty())
	{
		m_game_list_frame->BatchCreateCPUCaches(game_data, true);
	}
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	m_category_visible_act_group = new QActionGroup(this);
	m_category_visible_act_group->addAction(ui->showCatHDDGameAct);
	m_category_visible_act_group->addAction(ui->showCatDiscGameAct);
	m_category_visible_act_group->addAction(ui->showCatPS1GamesAct);
	m_category_visible_act_group->addAction(ui->showCatPS2GamesAct);
	m_category_visible_act_group->addAction(ui->showCatPSPGamesAct);
	m_category_visible_act_group->addAction(ui->showCatHomeAct);
	m_category_visible_act_group->addAction(ui->showCatAudioVideoAct);
	m_category_visible_act_group->addAction(ui->showCatGameDataAct);
	m_category_visible_act_group->addAction(ui->showCatOSAct);
	m_category_visible_act_group->addAction(ui->showCatUnknownAct);
	m_category_visible_act_group->addAction(ui->showCatOtherAct);
	m_category_visible_act_group->setExclusive(false);

	m_icon_size_act_group = new QActionGroup(this);
	m_icon_size_act_group->addAction(ui->setIconSizeTinyAct);
	m_icon_size_act_group->addAction(ui->setIconSizeSmallAct);
	m_icon_size_act_group->addAction(ui->setIconSizeMediumAct);
	m_icon_size_act_group->addAction(ui->setIconSizeLargeAct);

	m_list_mode_act_group = new QActionGroup(this);
	m_list_mode_act_group->addAction(ui->setlistModeListAct);
	m_list_mode_act_group->addAction(ui->setlistModeGridAct);
}

void main_window::CreateConnects()
{
	connect(ui->bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(ui->bootTestAct, &QAction::triggered, this, &main_window::BootTest);
	connect(ui->bootGameAct, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->bootVSHAct, &QAction::triggered, this, &main_window::BootVSH);
	connect(ui->actionopen_rsx_capture, &QAction::triggered, this, [this](){ BootRsxCapture(); });
	connect(ui->actionCreate_RSX_Capture, &QAction::triggered, this, []()
	{
		g_user_asked_for_frame_capture = true;
	});

	connect(ui->actionCreate_Savestate, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered savestate creation from utilities.");

		if (!g_cfg.savestate.suspend_emu)
		{
			Emu.after_kill_callback = []()
			{
				Emu.Restart();
			};

			// Make sure we keep the game window opened
			Emu.SetContinuousMode(true);
		}

		Emu.Kill(false, true);
	});

	connect(ui->actionCreate_Savestate_And_Exit, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered savestate creation and emulation stop from utilities.");
		Emu.Kill(false, true);
	});

	connect(ui->bootSavestateAct, &QAction::triggered, this, &main_window::BootSavestate);

	connect(ui->addGamesAct, &QAction::triggered, this, [this]()
	{
		if (!m_gui_settings->GetBootConfirmation(this))
		{
			return;
		}

		// Only select one folder for now
		QString dir = QFileDialog::getExistingDirectory(this, tr("Select a folder containing one or more games"), QString::fromStdString(fs::get_config_dir()), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
		if (dir.isEmpty())
		{
			return;
		}

		QStringList paths;
		paths << dir;
		AddGamesFromDirs(std::move(paths));
	});

	connect(ui->bootRecentMenu, &QMenu::aboutToShow, this, [this]()
	{
		// Enable/Disable Recent Games List
		const bool stopped = Emu.IsStopped();
		for (QAction* act : ui->bootRecentMenu->actions())
		{
			if (act != ui->freezeRecentAct && act != ui->clearRecentAct)
			{
				act->setEnabled(stopped);
			}
		}
	});

	connect(ui->bootRecentSavestatesMenu, &QMenu::aboutToShow, this, [this]()
	{
		// Enable/Disable Recent Savestates List
		const bool stopped = Emu.IsStopped();
		for (QAction* act : ui->bootRecentSavestatesMenu->actions())
		{
			if (act != ui->freezeRecentSavestatesAct && act != ui->clearRecentSavestatesAct)
			{
				act->setEnabled(stopped);
			}
		}
	});

	connect(ui->clearRecentAct, &QAction::triggered, this, [this]()
	{
		if (ui->freezeRecentAct->isChecked())
		{
			return;
		}
		m_recent_game.entries.clear();
		for (QAction* act : m_recent_game.actions)
		{
			ui->bootRecentMenu->removeAction(act);
		}
		m_recent_game.actions.clear();
		m_gui_settings->SetValue(gui::rg_entries, gui_settings::List2Var(q_pair_list()));
	});

	connect(ui->clearRecentSavestatesAct, &QAction::triggered, this, [this]()
	{
		if (ui->freezeRecentSavestatesAct->isChecked())
		{
			return;
		}
		m_recent_save.entries.clear();
		for (QAction* act : m_recent_save.actions)
		{
			ui->bootRecentSavestatesMenu->removeAction(act);
		}
		m_recent_save.actions.clear();
		m_gui_settings->SetValue(gui::rs_entries, gui_settings::List2Var(q_pair_list()));
	});

	connect(ui->freezeRecentAct, &QAction::triggered, this, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::rg_freeze, checked);
	});

	connect(ui->freezeRecentSavestatesAct, &QAction::triggered, this, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::rs_freeze, checked);
	});

	connect(ui->bootInstallPkgAct, &QAction::triggered, this, [this] {InstallPackages(); });
	connect(ui->bootInstallPupAct, &QAction::triggered, this, [this] {InstallPup(); });

	connect(this, &main_window::NotifyWindowCloseEvent, this, [this](bool closed)
	{
		if (!closed)
		{
			// Cancel the request
			m_requested_show_logs_on_exit = false;
			return;
		}

		if (!m_requested_show_logs_on_exit)
		{
			// Not requested
			return;
		}

		const std::string archived_path = fs::get_log_dir() + "RPCS3.log.gz";
		const std::string raw_file_path = fs::get_log_dir() + "RPCS3.log";

		fs::stat_t raw_stat{};
		fs::stat_t archived_stat{};

		if ((!fs::get_stat(raw_file_path, raw_stat) || raw_stat.is_directory) || (!fs::get_stat(archived_path, archived_stat) || archived_stat.is_directory) || (raw_stat.size == 0 && archived_stat.size == 0))
		{
			QMessageBox::warning(this, tr("Failed to locate log"), tr("Failed to locate log files.\nMake sure that RPCS3.log and RPCS3.log.gz are writable and can be created without permission issues."));
			return;
		}

		// Get new filename from title and title ID but simplified
		QString log_filename_q = QString::fromStdString(Emu.GetTitleID().empty() ? "RPCS3" : Emu.GetTitleAndTitleID());
		ensure(!log_filename_q.isEmpty());

		// Replace unfitting characters
		std::replace_if(log_filename_q.begin(), log_filename_q.end(), [](QChar c){ return !c.isLetterOrNumber() && c != QChar::Space && c != '[' && c != ']'; }, QChar::Space);
		log_filename_q = log_filename_q.simplified();

		const std::string log_filename = log_filename_q.toStdString();

		QString path_last_log = m_gui_settings->GetValue(gui::fd_save_log).toString();

		auto move_log = [](const std::string& from, const std::string& to)
		{
			if (from == to)
			{
				return false;
			}

			// Test writablity here to avoid closing the log with no *chance* of success
			if (fs::file test_writable{to, fs::write + fs::create}; !test_writable)
			{
				return false;
			}

			// Close and flush log file handle (!)
			// Cannot rename the file due to file management design
			logs::listener::close_all_prematurely();

			// Try to move it
			if (fs::rename(from, to, true))
			{
				return true;
			}

			// Try to copy it if fails
			if (fs::copy_file(from, to, true))
			{
				if (fs::file sync_fd{to, fs::write})
				{
					// Prevent data loss (expensive)
					sync_fd.sync();
				}

				fs::remove_file(from);
				return true;
			}

			return false;
		};

		if (archived_stat.size)
		{
			const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select RPCS3's log saving location (saving %0)").arg(QString::fromStdString(log_filename + ".log.gz")), path_last_log, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

			if (dir_path.isEmpty())
			{
				// Aborted - view the current location
				gui::utils::open_dir(archived_path);
				return;
			}

			const std::string dest_archived_path = dir_path.toStdString() + "/" + log_filename + ".log.gz";

			if (!Emu.GetTitleID().empty() && !dest_archived_path.empty() && move_log(archived_path, dest_archived_path))
			{
				m_gui_settings->SetValue(gui::fd_save_log, dir_path);
				gui_log.success("Moved log file to '%s'!", dest_archived_path);
				gui::utils::open_dir(dest_archived_path);
				return;
			}

			gui::utils::open_dir(archived_path);
			return;
		}

		const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select RPCS3's log saving location (saving %0)").arg(QString::fromStdString(log_filename + ".log")), path_last_log, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (dir_path.isEmpty())
		{
			// Aborted - view the current location
			gui::utils::open_dir(raw_file_path);
			return;
		}

		const std::string dest_raw_file_path = dir_path.toStdString() + "/" + log_filename + ".log";

		if (!Emu.GetTitleID().empty() && !dest_raw_file_path.empty() && move_log(raw_file_path, dest_raw_file_path))
		{
			m_gui_settings->SetValue(gui::fd_save_log, dir_path);
			gui_log.success("Moved log file to '%s'!", dest_raw_file_path);
			gui::utils::open_dir(dest_raw_file_path);
			return;
		}

		gui::utils::open_dir(raw_file_path);
	});

	connect(ui->exitAndSaveLogAct, &QAction::triggered, this, [this]()
	{
		m_requested_show_logs_on_exit = true;
		close();
	});
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);

	connect(ui->batchCreateCPUCachesAct, &QAction::triggered, m_game_list_frame, [list = m_game_list_frame]() { list->BatchCreateCPUCaches(); });
	connect(ui->batchRemoveCustomConfigurationsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveCustomConfigurations);
	connect(ui->batchRemoveCustomPadConfigurationsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveCustomPadConfigurations);
	connect(ui->batchRemoveShaderCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveShaderCaches);
	connect(ui->batchRemovePPUCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemovePPUCaches);
	connect(ui->batchRemoveSPUCachesAct, &QAction::triggered, m_game_list_frame, &game_list_frame::BatchRemoveSPUCaches);
	connect(ui->removeHDD1CachesAct, &QAction::triggered, this, &main_window::RemoveHDD1Caches);
	connect(ui->removeAllCachesAct, &QAction::triggered, this, &main_window::RemoveAllCaches);
	connect(ui->removeSavestatesAct, &QAction::triggered, this, &main_window::RemoveSavestates);
	connect(ui->cleanUpGameListAct, &QAction::triggered, this, &main_window::CleanUpGameList);

	connect(ui->removeFirmwareCacheAct, &QAction::triggered, this, &main_window::RemoveFirmwareCache);
	connect(ui->createFirmwareCacheAct, &QAction::triggered, this, &main_window::CreateFirmwareCache);

	connect(ui->sysPauseAct, &QAction::triggered, this, &main_window::OnPlayOrPause);
	connect(ui->sysStopAct, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered stop action in menu bar");
		Emu.GracefulShutdown(false, true);
	});
	connect(ui->sysRebootAct, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered restart action in menu bar");
		Emu.Restart();
	});

	connect(ui->ejectDiscAct, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered eject disc action in menu bar");
		Emu.EjectDisc();
	});
	connect(ui->insertDiscAct, &QAction::triggered, this, [this]()
	{
		gui_log.notice("User triggered insert disc action in menu bar");

		const QString path_last_game = m_gui_settings->GetValue(gui::fd_insert_disc).toString();
		const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select Disc Game Folder"), path_last_game, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

		if (dir_path.isEmpty())
		{
			return;
		}

		const game_boot_result result = Emu.InsertDisc(dir_path.toStdString());

		if (result != game_boot_result::no_errors)
		{
			QMessageBox::warning(this, tr("Failed to insert disc"), tr("Make sure that the emulation is running and that the selected path belongs to a valid disc game."));
			return;
		}

		m_gui_settings->SetValue(gui::fd_insert_disc, QFileInfo(dir_path).path());
	});

	const auto open_settings = [this](int tabIndex)
	{
		settings_dialog* dlg = new settings_dialog(m_gui_settings, m_emu_settings, tabIndex, this);
		connect(dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(dlg, &settings_dialog::GuiRepaintRequest, this, &main_window::RepaintGui);
		connect(dlg, &settings_dialog::EmuSettingsApplied, this, &main_window::NotifyEmuSettingsChange);
		connect(dlg, &settings_dialog::EmuSettingsApplied, this, &main_window::update_gui_pad_thread);
		connect(dlg, &settings_dialog::EmuSettingsApplied, m_log_frame, &log_frame::LoadSettings);
		dlg->setAttribute(Qt::WA_DeleteOnClose);
		dlg->open();
	};

	connect(ui->confCPUAct,    &QAction::triggered, this, [open_settings]() { open_settings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, this, [open_settings]() { open_settings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, this, [open_settings]() { open_settings(2); });
	connect(ui->confIOAct,     &QAction::triggered, this, [open_settings]() { open_settings(3); });
	connect(ui->confSystemAct, &QAction::triggered, this, [open_settings]() { open_settings(4); });
	connect(ui->confAdvAct,    &QAction::triggered, this, [open_settings]() { open_settings(6); });
	connect(ui->confEmuAct,    &QAction::triggered, this, [open_settings]() { open_settings(7); });
	connect(ui->confGuiAct,    &QAction::triggered, this, [open_settings]() { open_settings(8); });

	connect(ui->confShortcutsAct, &QAction::triggered, [this]()
	{
		shortcut_dialog dlg(m_gui_settings, this);
		connect(&dlg, &shortcut_dialog::saved, this, [this]()
		{
			m_shortcut_handler->update();
			NotifyShortcutHandlers();
		});
		dlg.exec();
	});

	const auto open_pad_settings = [this]
	{
		pad_settings_dialog dlg(m_gui_settings, this);
		dlg.exec();
	};

	connect(ui->confPadsAct, &QAction::triggered, this, open_pad_settings);

	connect(ui->confBuzzAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::buzz, this);
		dlg->show();
	});

	connect(ui->confGHLtarAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::ghltar, this);
		dlg->show();
	});

	connect(ui->confTurntableAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::turntable, this);
		dlg->show();
	});

	connect(ui->confUSIOAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::usio, this);
		dlg->show();
	});

	connect(ui->confPSMoveMouseAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::mousegem, this);
		dlg->show();
	});

	connect(ui->confPSMoveDS3Act, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::ds3gem, this);
		dlg->show();
	});

	connect(ui->confPSMoveAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::gem, this);
		dlg->show();
	});

	connect(ui->confGunCon3Act, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::guncon3, this);
		dlg->show();
	});

	connect(ui->confTopShotEliteAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::topshotelite, this);
		dlg->show();
	});

	connect(ui->confTopShotFearmasterAct, &QAction::triggered, this, [this]
	{
		emulated_pad_settings_dialog* dlg = new emulated_pad_settings_dialog(emulated_pad_settings_dialog::pad_type::topshotfearmaster, this);
		dlg->show();
	});

#ifndef HAVE_SDL3
	ui->confLogitechG27Act->setVisible(false);
#else
	connect(ui->confLogitechG27Act, &QAction::triggered, this, [this]
	{
		emulated_logitech_g27_settings_dialog* dlg = new emulated_logitech_g27_settings_dialog(this);
		dlg->show();
	});
#endif

	connect(ui->actionBasic_Mouse, &QAction::triggered, this, [this]
	{
		basic_mouse_settings_dialog* dlg = new basic_mouse_settings_dialog(this);
		dlg->show();
	});

#ifndef _WIN32
	ui->actionRaw_Mouse->setVisible(false);
#else
	connect(ui->actionRaw_Mouse, &QAction::triggered, this, [this]
	{
		raw_mouse_settings_dialog* dlg = new raw_mouse_settings_dialog(this);
		dlg->show();
	});
#endif

	connect(ui->confCamerasAct, &QAction::triggered, this, [this]()
	{
		camera_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confRPCNAct, &QAction::triggered, this, [this]()
	{
		rpcn_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confIPCAct, &QAction::triggered, this, [this]()
	{
		ipc_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confAutopauseManagerAct, &QAction::triggered, this, [this]()
	{
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confVFSDialogAct, &QAction::triggered, this, [this]()
	{
		vfs_dialog dlg(m_gui_settings, this);
		dlg.exec();
		ui->bootVSHAct->setEnabled(fs::is_file(g_cfg_vfs.get_dev_flash() + "vsh/module/vsh.self")); // dev_flash may have changed. Disable vsh if not present.
		m_game_list_frame->Refresh(true); // dev_hdd0 may have changed. Refresh just in case.
	});

	connect(ui->confSavedataManagerAct, &QAction::triggered, this, [this]
	{
		save_manager_dialog* save_manager = new save_manager_dialog(m_gui_settings, m_persistent_settings);
		connect(this, &main_window::RequestDialogRepaint, save_manager, &save_manager_dialog::HandleRepaintUiRequest);
		save_manager->show();
	});

	connect(ui->actionManage_Trophy_Data, &QAction::triggered, this, [this]
	{
		trophy_manager_dialog* trop_manager = new trophy_manager_dialog(m_gui_settings);
		connect(this, &main_window::RequestDialogRepaint, trop_manager, &trophy_manager_dialog::HandleRepaintUiRequest);
		trop_manager->show();
	});

	connect(ui->actionManage_Savestates, &QAction::triggered, this, [this]
	{
		savestate_manager_dialog* manager = new savestate_manager_dialog(m_gui_settings, m_game_list_frame->GetGameInfo());
		connect(this, &main_window::RequestDialogRepaint, manager, &savestate_manager_dialog::HandleRepaintUiRequest);
		connect(manager, &savestate_manager_dialog::RequestBoot, this, [this](const std::string& path) { Boot(path, "", true); });
		manager->show();
	});

	connect(ui->actionManage_Skylanders_Portal, &QAction::triggered, this, [this]
	{
		skylander_dialog* sky_diag = skylander_dialog::get_dlg(this);
		sky_diag->show();
	});

	connect(ui->actionManage_Infinity_Base, &QAction::triggered, this, [this]
	{
		infinity_dialog* inf_dlg = infinity_dialog::get_dlg(this);
		inf_dlg->show();
	});

	connect(ui->actionManage_Dimensions_ToyPad, &QAction::triggered, this, [this]
	{
		dimensions_dialog* dim_dlg = dimensions_dialog::get_dlg(this);
		dim_dlg->show();
	});

	connect(ui->actionManage_KamenRider_RideGate, &QAction::triggered, this, [this]
	{
		kamen_rider_dialog* kam_dlg = kamen_rider_dialog::get_dlg(this);
		kam_dlg->show();
	});

	connect(ui->actionManage_Cheats, &QAction::triggered, this, [this]
	{
		cheat_manager_dialog* cheat_manager = cheat_manager_dialog::get_dlg(this);
		cheat_manager->show();
 	});

	connect(ui->actionManage_Game_Patches, &QAction::triggered, this, [this]
	{
		patch_manager_dialog patch_manager(m_gui_settings, m_game_list_frame ? m_game_list_frame->GetGameInfo() : std::vector<game_info>{}, "", "", this);
		patch_manager.exec();
 	});

	connect(ui->patchCreatorAct, &QAction::triggered, this, [this]
	{
		patch_creator_dialog patch_creator(this);
		patch_creator.exec();
	});

	connect(ui->actionManage_Users, &QAction::triggered, this, [this]
	{
		user_manager_dialog user_manager(m_gui_settings, m_persistent_settings, this);
		user_manager.exec();
		m_game_list_frame->Refresh(true); // New user may have different games unlocked.
	});

	connect(ui->actionManage_Screenshots, &QAction::triggered, this, [this]
	{
		screenshot_manager_dialog* screenshot_manager = new screenshot_manager_dialog();
		screenshot_manager->show();
	});

	connect(ui->toolsCgDisasmAct, &QAction::triggered, this, [this]
	{
		cg_disasm_window* cgdw = new cg_disasm_window(m_gui_settings);
		cgdw->show();
	});

	connect(ui->actionLog_Viewer, &QAction::triggered, this, [this]
	{
		log_viewer* viewer = new log_viewer(m_gui_settings);
		viewer->show();
		viewer->show_log();
	});

	connect(ui->actionPS_Move_Tracker, &QAction::triggered, this, [this]
	{
		ps_move_tracker_dialog* dlg = new ps_move_tracker_dialog(this);
		dlg->open();
	});

	connect(ui->toolsCheckConfigAct, &QAction::triggered, this, [this]
	{
		const QString path_last_cfg = m_gui_settings->GetValue(gui::fd_cfg_check).toString();
		const QString file_path = QFileDialog::getOpenFileName(this, tr("Select rpcs3.log or config.yml"), path_last_cfg, tr("Log or Config files (*.log *.gz *.txt *.yml);;Log files (*.log *.gz);;Config Files (*.yml);;Text Files (*.txt);;All files (*.*)"));
		if (file_path.isEmpty())
		{
			// Aborted
			return;
		}

		const QFileInfo file_info(file_path);

		if (file_info.isExecutable() || !(file_path.endsWith(".log") || file_path.endsWith(".log.gz") || file_path.endsWith(".txt") || file_path.endsWith(".yml")))
		{
			if (QMessageBox::question(this, tr("Weird file!"), tr("This file seems to have an unexpected type:\n%0\n\nCheck anyway?").arg(file_path)) != QMessageBox::Yes)
			{
				return;
			}
		}

		bool failed = false;
		QString content;

		if (file_path.endsWith(".gz"))
		{
			if (fs::file file{file_path.toStdString()})
			{
				const std::vector<u8> decompressed = unzip(file.to_vector<u8>());
				content = QString::fromUtf8(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
			}
			else
			{
				failed = true;
			}
		}
		else if (QFile file(file_path); file.exists() && file.open(QIODevice::ReadOnly))
		{
			content = file.readAll();
		}
		else
		{
			failed = true;
		}

		if (failed)
		{
			QMessageBox::warning(this, tr("Failed to open file"), tr("The file could not be opened:\n%0").arg(file_path));
			return;
		}

		m_gui_settings->SetValue(gui::fd_cfg_check, file_info.path());

		config_checker* dlg = new config_checker(this, content, file_path.endsWith(".log") || file_path.endsWith(".log.gz"));
		dlg->open();
	});

	connect(ui->toolskernel_explorerAct, &QAction::triggered, this, [this]
	{
		if (!m_kernel_explorer)
		{
			m_kernel_explorer = new kernel_explorer(this);
			connect(m_kernel_explorer, &QDialog::finished, this, [this]() { m_kernel_explorer = nullptr; });
		}

		m_kernel_explorer->show();
	});

	connect(ui->toolsmemory_viewerAct, &QAction::triggered, this, [this]
	{
		if (!Emu.IsStopped())
			idm::make<memory_viewer_handle>(this, make_basic_ppu_disasm());
	});

	connect(ui->toolsRsxDebuggerAct, &QAction::triggered, this, [this]
	{
		rsx_debugger* rsx = new rsx_debugger(m_gui_settings);
		rsx->show();
	});

	connect(ui->toolsSystemCommandsAct, &QAction::triggered, this, [this]
	{
		if (Emu.IsStopped()) return;
		if (!m_system_cmd_dialog)
		{
			m_system_cmd_dialog = new system_cmd_dialog(this);
			connect(m_system_cmd_dialog, &QDialog::finished, this, [this]() { m_system_cmd_dialog = nullptr; });
		}
		m_system_cmd_dialog->show();
	});

	connect(ui->toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);

	connect(ui->toolsExtractMSELFAct, &QAction::triggered, this, &main_window::ExtractMSELF);

	connect(ui->toolsExtractPUPAct, &QAction::triggered, this, &main_window::ExtractPup);

	connect(ui->toolsExtractTARAct, &QAction::triggered, this, &main_window::ExtractTar);

	connect(ui->toolsVfsDialogAct, &QAction::triggered, this, [this]()
	{
		vfs_tool_dialog* dlg = new vfs_tool_dialog(this);
		dlg->show();
	});

	connect(ui->actionMusic_Player, &QAction::triggered, this, [this]()
	{
		music_player_dialog* dlg = new music_player_dialog(this);
		dlg->open();
	});

	connect(ui->showDebuggerAct, &QAction::triggered, this, [this](bool checked)
	{
		checked ? m_debugger_frame->show() : m_debugger_frame->hide();
		m_gui_settings->SetValue(gui::mw_debugger, checked);
	});

	connect(ui->showLogAct, &QAction::triggered, this, [this](bool checked)
	{
		checked ? m_log_frame->show() : m_log_frame->hide();
		m_gui_settings->SetValue(gui::mw_logger, checked);
	});

	connect(ui->showGameListAct, &QAction::triggered, this, [this](bool checked)
	{
		checked ? m_game_list_frame->show() : m_game_list_frame->hide();
		m_gui_settings->SetValue(gui::mw_gamelist, checked);
	});

	connect(ui->showTitleBarsAct, &QAction::triggered, this, [this](bool checked)
	{
		ShowTitleBars(checked);
		m_gui_settings->SetValue(gui::mw_titleBarsVisible, checked);
	});

	connect(ui->showToolBarAct, &QAction::triggered, this, [this](bool checked)
	{
		ui->toolBar->setVisible(checked);
		m_gui_settings->SetValue(gui::mw_toolBarVisible, checked);
	});

	connect(ui->showHiddenEntriesAct, &QAction::triggered, this, [this](bool checked)
	{
		m_gui_settings->SetValue(gui::gl_show_hidden, checked);
		m_game_list_frame->SetShowHidden(checked);
		m_game_list_frame->Refresh();
	});

	connect(ui->showCompatibilityInGridAct, &QAction::triggered, m_game_list_frame, &game_list_frame::SetShowCompatibilityInGrid);

	connect(ui->refreshGameListAct, &QAction::triggered, this, [this]
	{
		m_game_list_frame->Refresh(true);
	});

	const auto get_cats = [this](QAction* act, int& id) -> QStringList
	{
		QStringList categories;
		if      (act == ui->showCatHDDGameAct)    { categories.append(cat::cat_hdd_game);  id = Category::HDD_Game;    }
		else if (act == ui->showCatDiscGameAct)   { categories.append(cat::cat_disc_game); id = Category::Disc_Game;   }
		else if (act == ui->showCatPS1GamesAct)   { categories.append(cat::cat_ps1_game);  id = Category::PS1_Game;    }
		else if (act == ui->showCatPS2GamesAct)   { categories.append(cat::ps2_games);     id = Category::PS2_Game;    }
		else if (act == ui->showCatPSPGamesAct)   { categories.append(cat::psp_games);     id = Category::PSP_Game;    }
		else if (act == ui->showCatHomeAct)       { categories.append(cat::cat_home);      id = Category::Home;        }
		else if (act == ui->showCatAudioVideoAct) { categories.append(cat::media);         id = Category::Media;       }
		else if (act == ui->showCatGameDataAct)   { categories.append(cat::data);          id = Category::Data;        }
		else if (act == ui->showCatOSAct)         { categories.append(cat::os);            id = Category::OS;          }
		else if (act == ui->showCatUnknownAct)    { categories.append(cat::cat_unknown);   id = Category::Unknown_Cat; }
		else if (act == ui->showCatOtherAct)      { categories.append(cat::others);        id = Category::Others;      }
		else { gui_log.warning("categoryVisibleActGroup: category action not found"); }
		return categories;
	};

	connect(m_category_visible_act_group, &QActionGroup::triggered, this, [this, get_cats](QAction* act)
	{
		int id = 0;
		const QStringList categories = get_cats(act, id);

		if (!categories.isEmpty())
		{
			const bool checked = act->isChecked();
			m_game_list_frame->ToggleCategoryFilter(categories, checked);
			m_gui_settings->SetCategoryVisibility(id, checked, m_is_list_mode);
		}
	});

	connect(ui->menuGame_Categories, &QMenu::aboutToShow, ui->menuGame_Categories, [this, get_cats]()
	{
		const auto set_cat_count = [&](QAction* act, const QString& text)
		{
			int count = 0;
			int id = 0; // Unused
			const QStringList categories = get_cats(act, id);
			for (const game_info& game : m_game_list_frame->GetGameInfo())
			{
				if (game && categories.contains(QString::fromStdString(game->info.category))) count++;
			}
			act->setText(QString("%0 (%1)").arg(text).arg(count));
		};

		set_cat_count(ui->showCatHDDGameAct, tr("HDD Games"));
		set_cat_count(ui->showCatDiscGameAct, tr("Disc Games"));
		set_cat_count(ui->showCatPS1GamesAct, tr("PS1 Games"));
		set_cat_count(ui->showCatPS2GamesAct, tr("PS2 Games"));
		set_cat_count(ui->showCatPSPGamesAct, tr("PSP Games"));
		set_cat_count(ui->showCatHomeAct, tr("Home"));
		set_cat_count(ui->showCatAudioVideoAct, tr("Audio/Video"));
		set_cat_count(ui->showCatGameDataAct, tr("Game Data"));
		set_cat_count(ui->showCatOSAct, tr("Operating System"));
		set_cat_count(ui->showCatUnknownAct, tr("Unknown"));
		set_cat_count(ui->showCatOtherAct, tr("Other"));
	});

	connect(ui->updateAct, &QAction::triggered, this, [this]()
	{
#ifdef RPCS3_UPDATE_SUPPORTED
		m_updater.check_for_updates(false, false, false, this);
#else
		QMessageBox::warning(this, tr("Auto-updater"), tr("The auto-updater isn't available for your OS currently."));
#endif
	});

	connect(ui->welcomeAct, &QAction::triggered, this, [this]()
	{
		welcome_dialog* welcome = new welcome_dialog(m_gui_settings, true, this);
		welcome->open();
	});

	connect(ui->supportAct, &QAction::triggered, this, [this]
	{
		QDesktopServices::openUrl(QUrl("https://rpcs3.net/patreon"));
	});

	connect(ui->aboutAct, &QAction::triggered, this, [this]
	{
		about_dialog* dlg = new about_dialog(this);
		dlg->open();
	});

	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

	connect(m_icon_size_act_group, &QActionGroup::triggered, this, [this](QAction* act)
	{
		static const int index_small  = gui::get_Index(gui::gl_icon_size_small);
		static const int index_medium = gui::get_Index(gui::gl_icon_size_medium);

		int index;

		if (act == ui->setIconSizeTinyAct)
			index = 0;
		else if (act == ui->setIconSizeSmallAct)
			index = index_small;
		else if (act == ui->setIconSizeMediumAct)
			index = index_medium;
		else
			index = gui::gl_max_slider_pos;

		m_save_slider_pos = true;
		ResizeIcons(index);
	});

	connect(ui->actionPreferGameDataIcons, &QAction::triggered, m_game_list_frame, &game_list_frame::SetPreferGameDataIcons);
	connect(ui->showCustomIconsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::SetShowCustomIcons);
	connect(ui->playHoverGifsAct, &QAction::triggered, m_game_list_frame, &game_list_frame::SetPlayHoverGifs);

	connect(m_game_list_frame, &game_list_frame::RequestIconSizeChange, this, [this](const int& val)
	{
		const int idx = ui->sizeSlider->value() + val;
		m_save_slider_pos = true;
		ResizeIcons(idx);
	});

	connect(m_game_list_frame, &game_list_frame::RequestSaveStateManager, this, [this](const game_info& gameinfo)
	{
		savestate_manager_dialog* manager = new savestate_manager_dialog(m_gui_settings, std::vector<game_info>{gameinfo});
		connect(this, &main_window::RequestDialogRepaint, manager, &savestate_manager_dialog::HandleRepaintUiRequest);
		connect(manager, &savestate_manager_dialog::RequestBoot, this, [this, gameinfo](const std::string& path)
		{
			Boot(path, gameinfo->info.serial, false, false, cfg_mode::custom, "");
		});
		manager->show();
	});

	connect(m_list_mode_act_group, &QActionGroup::triggered, this, [this](QAction* act)
	{
		const bool is_list_act = act == ui->setlistModeListAct;
		if (is_list_act == m_is_list_mode)
			return;

		const int slider_pos = ui->sizeSlider->sliderPosition();
		ui->sizeSlider->setSliderPosition(m_other_slider_pos);
		SetIconSizeActions(m_other_slider_pos);
		m_other_slider_pos = slider_pos;

		m_is_list_mode = is_list_act;
		m_game_list_frame->SetListMode(m_is_list_mode);

		UpdateFilterActions();
	});

	connect(ui->toolbar_open, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, this, [this]() { m_game_list_frame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, this, []()
	{
		gui_log.notice("User triggered stop action in toolbar");
		Emu.GracefulShutdown(false);
	});
	connect(ui->toolbar_start, &QAction::triggered, this, &main_window::OnPlayOrPause);

	connect(ui->toolbar_fullscreen, &QAction::triggered, this, [this]
	{
		if (isFullScreen())
		{
			showNormal();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
		}
		else
		{
			showFullScreen();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
		}
	});

	connect(ui->toolbar_controls, &QAction::triggered, this, open_pad_settings);
	connect(ui->toolbar_config, &QAction::triggered, this, [=]() { open_settings(0); });
	connect(ui->toolbar_list, &QAction::triggered, this, [this]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, this, [this]() { ui->setlistModeGridAct->trigger(); });

	connect(ui->sizeSlider, &QSlider::valueChanged, this, &main_window::ResizeIcons);
	connect(ui->sizeSlider, &QSlider::sliderReleased, this, [this]
	{
		const int index = ui->sizeSlider->value();
		m_gui_settings->SetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid, index);
		SetIconSizeActions(index);
	});
	connect(ui->sizeSlider, &QSlider::actionTriggered, this, [this](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_slider_pos = true; // actionTriggered happens before the value was changed
		}
	});

	connect(ui->mw_searchbar, &QLineEdit::textChanged, m_game_list_frame, &game_list_frame::SetSearchText);
	connect(ui->mw_searchbar, &QLineEdit::returnPressed, m_game_list_frame, &game_list_frame::FocusAndSelectFirstEntryIfNoneIs);
	connect(m_game_list_frame, &game_list_frame::FocusToSearchBar, this, [this]() { ui->mw_searchbar->setFocus(); });

	connect(m_game_list_frame, &game_list_frame::NotifyBatchedGameActionFinished, this, [this]() mutable
	{
		if (m_notify_batch_game_action_cb)
		{
			m_notify_batch_game_action_cb();
			m_notify_batch_game_action_cb = {};
		}
	});
}

void main_window::CreateDockWindows()
{
	// new mainwindow widget because existing seems to be bugged for now
	m_mw = new QMainWindow();
	m_mw->setContextMenuPolicy(Qt::PreventContextMenu);

	m_game_list_frame = new game_list_frame(m_gui_settings, m_emu_settings, m_persistent_settings, m_mw);
	m_game_list_frame->setObjectName("gamelist");
	m_debugger_frame = new debugger_frame(m_gui_settings, m_mw);
	m_debugger_frame->setObjectName("debugger");
	m_log_frame = new log_frame(m_gui_settings, m_mw);
	m_log_frame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_game_list_frame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_log_frame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, m_debugger_frame);
	m_mw->setDockNestingEnabled(true);
	m_mw->resizeDocks({ m_log_frame }, { m_mw->sizeHint().height() / 10 }, Qt::Orientation::Vertical);
	setCentralWidget(m_mw);

	connect(m_log_frame, &log_frame::LogFrameClosed, this, [this]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_logger, false);
		}
	});

	connect(m_log_frame, &log_frame::PerformGoToOnDebugger, this, [this](const QString& text_argument, bool is_address, bool test_only, std::shared_ptr<bool> signal_accepted)
	{
		if (m_debugger_frame && m_debugger_frame->isVisible())
		{
			if (signal_accepted)
			{
				*signal_accepted = true;
			}

			if (!test_only)
			{
				if (is_address)
				{
					m_debugger_frame->PerformGoToRequest(text_argument);
				}
				else
				{
					m_debugger_frame->PerformGoToThreadRequest(text_argument);
				}
			}
		}
	});

	connect(m_debugger_frame, &debugger_frame::DebugFrameClosed, this, [this]()
	{
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_debugger, false);
		}
	});

	connect(m_game_list_frame, &game_list_frame::GameListFrameClosed, this, [this]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			m_gui_settings->SetValue(gui::mw_gamelist, false);
		}
	});

	connect(m_game_list_frame, &game_list_frame::NotifyGameSelection, this, [this](const game_info& game)
	{
		// Only change the button logic while the emulator is stopped.
		if (Emu.IsStopped())
		{
			QString tooltip;

			bool enable_play_buttons = true;

			if (game) // A game was selected
			{
				const std::string title_and_title_id = game->info.name + " [" + game->info.serial + "]";

				if (title_and_title_id == Emu.GetTitleAndTitleID()) // This should usually not cause trouble, but feel free to improve.
				{
					tooltip = tr("Restart %0").arg(QString::fromStdString(title_and_title_id));

					ui->toolbar_start->setIcon(m_icon_restart);
					ui->toolbar_start->setText(tr("Restart"));
				}
				else
				{
					tooltip = tr("Play %0").arg(QString::fromStdString(title_and_title_id));

					ui->toolbar_start->setIcon(m_icon_play);
					ui->toolbar_start->setText(tr("Play"));
				}
			}
			else if (m_selected_game) // No game was selected. Check if a game was selected before.
			{
				if (Emu.IsReady()) // Prefer games that are about to be booted ("Automatically start games" was set to off)
				{
					tooltip = tr("Play %0").arg(GetCurrentTitle());

					ui->toolbar_start->setIcon(m_icon_play);
				}
				else if (const std::string& path = Emu.GetLastBoot(); !path.empty()) // Restartable games
				{
					tooltip = tr("Restart %0").arg(GetCurrentTitle());

					ui->toolbar_start->setIcon(m_icon_restart);
					ui->toolbar_start->setText(tr("Restart"));
				}
				else if (!m_recent_game.actions.isEmpty()) // Get last played game
				{
					tooltip = tr("Play %0").arg(m_recent_game.actions.first()->text());
				}
				else
				{
					enable_play_buttons = false;
				}
			}
			else
			{
				enable_play_buttons = false;
			}

			ui->toolbar_start->setEnabled(enable_play_buttons);
			ui->sysPauseAct->setEnabled(enable_play_buttons);

			if (!tooltip.isEmpty())
			{
				ui->toolbar_start->setToolTip(tooltip);
			}
		}

		m_selected_game = game;
	});

	connect(m_game_list_frame, &game_list_frame::RequestBoot, this, [this](const game_info& game, cfg_mode config_mode, const std::string& config_path, const std::string& savestate)
	{
		Boot(savestate.empty() ? game->info.path : savestate, game->info.serial, false, false, config_mode, config_path);
	});

	connect(m_game_list_frame, &game_list_frame::NotifyEmuSettingsChange, this, &main_window::NotifyEmuSettingsChange);
}

void main_window::ConfigureGuiFromSettings()
{
	// Restore GUI state if needed. We need to if they exist.
	if (!restoreGeometry(m_gui_settings->GetValue(gui::mw_geometry).toByteArray()))
	{
		// By default, set the window to 70% of the screen and the debugger frame is hidden.
		m_debugger_frame->hide();
		resize(QGuiApplication::primaryScreen()->availableSize() * 0.7);
	}

	restoreState(m_gui_settings->GetValue(gui::mw_windowState).toByteArray());
	m_mw->restoreState(m_gui_settings->GetValue(gui::mw_mwState).toByteArray());

	ui->freezeRecentAct->setChecked(m_gui_settings->GetValue(gui::rg_freeze).toBool());
	ui->freezeRecentSavestatesAct->setChecked(m_gui_settings->GetValue(gui::rs_freeze).toBool());
	m_recent_game.entries = gui_settings::Var2List(m_gui_settings->GetValue(gui::rg_entries));
	m_recent_save.entries = gui_settings::Var2List(m_gui_settings->GetValue(gui::rs_entries));

	const auto update_recent_games_menu = [this](bool is_savestate)
	{
		recent_game_wrapper& rgw = is_savestate ? m_recent_save : m_recent_game;
		QMenu* menu = is_savestate ? ui->bootRecentSavestatesMenu : ui->bootRecentMenu;

		// clear recent games menu of actions
		for (QAction* act : rgw.actions)
		{
			menu->removeAction(act);
		}
		rgw.actions.clear();

		// Fill the recent games menu
		for (int i = 0; i < rgw.entries.count(); i++)
		{
			// adjust old unformatted entries (avoid duplication)
			rgw.entries[i] = gui::Recent_Game(rgw.entries[i].first, rgw.entries[i].second);

			// create new action
			QAction* act = CreateRecentAction(rgw.entries[i], i + 1, is_savestate);

			// add action to menu
			if (act)
			{
				rgw.actions.append(act);
				menu->addAction(act);
			}
			else
			{
				i--; // list count is now an entry shorter so we have to repeat the same index in order to load all other entries
			}
		}
	};
	update_recent_games_menu(false);
	update_recent_games_menu(true);

	ui->showLogAct->setChecked(m_gui_settings->GetValue(gui::mw_logger).toBool());
	ui->showGameListAct->setChecked(m_gui_settings->GetValue(gui::mw_gamelist).toBool());
	ui->showDebuggerAct->setChecked(m_gui_settings->GetValue(gui::mw_debugger).toBool());
	ui->showToolBarAct->setChecked(m_gui_settings->GetValue(gui::mw_toolBarVisible).toBool());
	ui->showTitleBarsAct->setChecked(m_gui_settings->GetValue(gui::mw_titleBarsVisible).toBool());

	m_debugger_frame->setVisible(ui->showDebuggerAct->isChecked());
	m_log_frame->setVisible(ui->showLogAct->isChecked());
	m_game_list_frame->setVisible(ui->showGameListAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	ShowTitleBars(ui->showTitleBarsAct->isChecked());

	ui->showHiddenEntriesAct->setChecked(m_gui_settings->GetValue(gui::gl_show_hidden).toBool());
	m_game_list_frame->SetShowHidden(ui->showHiddenEntriesAct->isChecked()); // prevent GetValue in m_game_list_frame->LoadSettings

	ui->showCompatibilityInGridAct->setChecked(m_gui_settings->GetValue(gui::gl_draw_compat).toBool());
	ui->actionPreferGameDataIcons->setChecked(m_gui_settings->GetValue(gui::gl_pref_gd_icon).toBool());
	ui->showCustomIconsAct->setChecked(m_gui_settings->GetValue(gui::gl_custom_icon).toBool());
	ui->playHoverGifsAct->setChecked(m_gui_settings->GetValue(gui::gl_hover_gifs).toBool());

	m_is_list_mode = m_gui_settings->GetValue(gui::gl_listMode).toBool();

	UpdateFilterActions();

	// handle icon size options
	if (m_is_list_mode)
		ui->setlistModeListAct->setChecked(true);
	else
		ui->setlistModeGridAct->setChecked(true);

	const int icon_size_index = m_gui_settings->GetValue(m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	m_other_slider_pos = m_gui_settings->GetValue(!m_is_list_mode ? gui::gl_iconSize : gui::gl_iconSizeGrid).toInt();
	ui->sizeSlider->setSliderPosition(icon_size_index);
	SetIconSizeActions(icon_size_index);

	// Handle log settings
	m_log_frame->LoadSettings();

	// Gamelist
	m_game_list_frame->LoadSettings();
}

void main_window::SetIconSizeActions(int idx) const
{
	static const int threshold_tiny = gui::get_Index((gui::gl_icon_size_small + gui::gl_icon_size_min) / 2);
	static const int threshold_small = gui::get_Index((gui::gl_icon_size_medium + gui::gl_icon_size_small) / 2);
	static const int threshold_medium = gui::get_Index((gui::gl_icon_size_max + gui::gl_icon_size_medium) / 2);

	if (idx < threshold_tiny)
		ui->setIconSizeTinyAct->setChecked(true);
	else if (idx < threshold_small)
		ui->setIconSizeSmallAct->setChecked(true);
	else if (idx < threshold_medium)
		ui->setIconSizeMediumAct->setChecked(true);
	else
		ui->setIconSizeLargeAct->setChecked(true);
}

void main_window::RemoveHDD1Caches()
{
	if (fs::remove_all(rpcs3::utils::get_hdd1_dir() + "caches", false))
	{
		QMessageBox::information(this, tr("HDD1 Caches Removed"), tr("HDD1 caches successfully removed"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Could not remove HDD1 caches"));
	}
}

void main_window::RemoveAllCaches()
{
	if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove all caches?")) != QMessageBox::Yes)
		return;

	const std::string cache_base_dir = rpcs3::utils::get_cache_dir();
	u64 caches_count = 0;
	u64 caches_removed = 0;

	for (const game_info& game : m_game_list_frame->GetGameInfo()) // Loop on detected games
	{
		if (game && QString::fromStdString(game->info.category) != cat::cat_ps3_os && fs::exists(cache_base_dir + game->info.serial)) // If not OS category and cache exists
		{
			caches_count++;

			if (fs::remove_all(cache_base_dir + game->info.serial))
			{
				caches_removed++;
			}
		}
	}

	if (caches_count == caches_removed)
	{
		QMessageBox::information(this, tr("Caches Removed"), tr("%0 cache(s) successfully removed").arg(caches_removed));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Could not remove %0 of %1 cache(s)").arg(caches_count - caches_removed).arg(caches_count));
	}

	RemoveHDD1Caches();
}

void main_window::RemoveSavestates()
{
	if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove savestates?")) != QMessageBox::Yes)
		return;

	if (fs::remove_all(fs::get_config_dir() + "savestates", false))
	{
		QMessageBox::information(this, tr("Savestates Removed"), tr("Savestates successfully removed"));
	}
	else
	{
		QMessageBox::warning(this, tr("Error"), tr("Could not remove savestates"));
	}
}

void main_window::CleanUpGameList()
{
	if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove invalid game paths from game list?\n"
		"Undetectable games (zombies) as well as corrupted games will be removed from the game list file (games.yml)")) != QMessageBox::Yes)
	{
		return;
	}

	// List of serials (title id) to remove in "games.yml" file (if any)
	std::vector<std::string> serials_to_remove_from_yml{};

	for (const auto& [serial, path] : Emu.GetGamesConfig().get_games()) // Loop on game list file
	{
		bool found = false;

		for (const game_info& game : m_game_list_frame->GetGameInfo()) // Loop on detected games
		{
			// If Disc Game and its serial is found in game list file
			if (game && QString::fromStdString(game->info.category) == cat::cat_disc_game && game->info.serial == serial)
			{
				found = true;
				break;
			}
		}

		if (!found) // If serial not found, add it to the removal list
		{
			serials_to_remove_from_yml.push_back(serial);
		}
	}

	// Remove the found serials (title id) in "games.yml" file (if any)
	QMessageBox::information(this, tr("Summary"), tr("%0 game(s) removed from game list").arg(Emu.RemoveGames(serials_to_remove_from_yml)));
}

void main_window::RemoveFirmwareCache()
{
	const std::string cache_dir = rpcs3::utils::get_cache_dir();

	if (!fs::is_dir(cache_dir))
		return;

	if (QMessageBox::question(this, tr("Confirm Removal"), tr("Remove firmware cache?")) != QMessageBox::Yes)
		return;

	u32 caches_removed = 0;
	u32 caches_total = 0;

	const QStringList filter{ QStringLiteral("ppu-*-lib*.sprx")};

	QDirIterator dir_iter(QString::fromStdString(cache_dir), filter, QDir::Dirs | QDir::NoDotAndDotDot);

	while (dir_iter.hasNext())
	{
		const QString path = dir_iter.next();

		if (QDir(path).removeRecursively())
		{
			++caches_removed;
			gui_log.notice("Removed firmware cache: %s", path);
		}
		else
		{
			gui_log.warning("Could not remove firmware cache: %s", path);
		}

		++caches_total;
	}

	const bool success = caches_total == caches_removed;

	if (success)
		gui_log.success("Removed firmware cache in %s", cache_dir);
	else
		gui_log.fatal("Only %d/%d firmware caches could be removed in %s", caches_removed, caches_total, cache_dir);

	return;
}

void main_window::CreateFirmwareCache()
{
	if (Emu.IsBootingRestricted())
	{
		return;
	}

	if (!m_gui_settings->GetBootConfirmation(this))
	{
		return;
	}

	Emu.GracefulShutdown(false);
	Emu.SetForceBoot(true);

	if (const game_boot_result error = Emu.BootGame(g_cfg_vfs.get_dev_flash() + "sys", "", true);
		error != game_boot_result::no_errors)
	{
		gui_log.error("Creating firmware cache failed: reason: %s", error);
	}
}

void main_window::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (isFullScreen())
	{
		if (event->button() == Qt::LeftButton)
		{
			showNormal();
			ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
		}
	}
}

/** Override the Qt close event to have the emulator stop and the application die.
*/
void main_window::closeEvent(QCloseEvent* closeEvent)
{
	if (!m_gui_settings->GetBootConfirmation(this, gui::ib_confirm_exit))
	{
		Q_EMIT NotifyWindowCloseEvent(false);
		closeEvent->ignore();
		return;
	}

	// Cleanly stop and quit the emulator.
	if (!Emu.IsStopped())
	{
		Emu.GracefulShutdown(false);
	}

	SaveWindowState();

	// Flush logs here as well
	logs::listener::sync_all();

	Q_EMIT NotifyWindowCloseEvent(true);

	Emu.Quit(true);
}

/**
Add valid disc games to gamelist (games.yml)
@param paths = dir paths to scan for game
*/
void main_window::AddGamesFromDirs(QStringList&& paths)
{
	if (paths.isEmpty())
	{
		return;
	}

	// Obtain list of previously existing entries under the specificied parent paths for comparison
	std::unordered_set<std::string> existing;

	for (const game_info& game : m_game_list_frame->GetGameInfo())
	{
		if (game)
		{
			for (const auto& dir_path : paths)
			{
				if (dir_path.startsWith(game->info.path.c_str()) && fs::exists(game->info.path))
				{
					existing.insert(game->info.path);
					break;
				}
			}
		}
	}

	u32 games_added = 0;

	for (const QString& path : paths)
	{
		games_added += Emu.AddGamesFromDir(path.toStdString());
	}

	if (games_added)
	{
		gui_log.notice("AddGamesFromDirs added %d new entries", games_added);
	}

	m_game_list_frame->AddRefreshedSlot([this, paths = std::move(paths), existing = std::move(existing)](std::set<std::string>& claimed_paths)
	{
		// Execute followup operations only for newly added entries under the specified paths
		std::map<std::string, QString> paths_added; // -> title id

		for (const game_info& game : m_game_list_frame->GetGameInfo())
		{
			if (game && !existing.contains(game->info.path))
			{
				for (const auto& dir_path : paths)
				{
					if (Emu.IsPathInsideDir(game->info.path, dir_path.toStdString()))
					{
						// Try to claim operation on directory path

						std::string resolved_path = Emu.GetCallbacks().resolve_path(game->info.path);

						if (!resolved_path.empty() && !claimed_paths.count(resolved_path))
						{
							claimed_paths.emplace(game->info.path);
							paths_added.emplace(game->info.path, QString::fromStdString(game->info.serial));
						}

						break;
					}
				}
			}
		}

		if (paths_added.empty())
		{
			QMessageBox::information(this, tr("Nothing to add!"), tr("Could not find any new software."));
		}
		else
		{
			ShowOptionalGamePreparations(tr("Success!"), tr("Successfully added software to game list from path(s)!"), std::move(paths_added));
		}
	});

	m_game_list_frame->Refresh(true);
}

/**
Check data for valid file types and cache their paths if necessary
@param md = data containing file urls
@param drop_paths = flag for path caching
@returns validity of file type
*/
main_window::drop_type main_window::IsValidFile(const QMimeData& md, QStringList* drop_paths)
{
	if (drop_paths)
	{
		drop_paths->clear();
	}

	drop_type type = drop_type::drop_error;

	QList<QUrl> list = md.urls(); // get list of all the dropped file urls

	// Try to cache the data for half a second
	if (m_drop_file_timestamp != umax && m_drop_file_url_list == list && get_system_time() - m_drop_file_timestamp < 500'000)
	{
		if (drop_paths)
		{
			for (auto&& url : m_drop_file_url_list)
			{
				drop_paths->append(url.toLocalFile());
			}
		}

		return m_drop_file_cached_drop_type;
	}

	m_drop_file_url_list = std::move(list);

	auto set_result = [this](drop_type type)
	{
		m_drop_file_timestamp = get_system_time();
		m_drop_file_cached_drop_type = type;
		return type;
	};

	for (auto&& url : m_drop_file_url_list) // check each file in url list for valid type
	{
		const QString path = url.toLocalFile(); // convert url to filepath
		const QFileInfo info(path);

		const QString suffix_lo = info.suffix().toLower();

		// check for directories first, only valid if all other paths led to directories until now.
		if (info.isDir())
		{
			if (type != drop_type::drop_dir && type != drop_type::drop_error)
			{
				return set_result(drop_type::drop_error);
			}

			type = drop_type::drop_dir;
		}
		else if (!info.exists())
		{
			// If does not exist (anymore), ignore it
			continue;
		}
		else if (info.size() < 0x4)
		{
			return set_result(drop_type::drop_error);
		}
		else if (info.suffix() == "PUP")
		{
			if (m_drop_file_url_list.size() != 1)
			{
				return set_result(drop_type::drop_error);
			}

			type = drop_type::drop_pup;
		}
		else if (info.fileName().toLower() == "param.sfo")
		{
			if (type != drop_type::drop_psf && type != drop_type::drop_error)
			{
				return set_result(drop_type::drop_error);
			}

			type = drop_type::drop_psf;
		}
		else if (suffix_lo == "pkg")
		{
			if (type != drop_type::drop_rap_edat_pkg && type != drop_type::drop_error)
			{
				return set_result(drop_type::drop_error);
			}

			type = drop_type::drop_rap_edat_pkg;
		}
		else if (suffix_lo == "rap" || suffix_lo == "edat")
		{
			if (info.size() < 0x10 || (type != drop_type::drop_rap_edat_pkg && type != drop_type::drop_error))
			{
				return set_result(drop_type::drop_error);
			}

			type = drop_type::drop_rap_edat_pkg;
		}
		else if (m_drop_file_url_list.size() == 1)
		{
			if (suffix_lo == "rrc" || path.toLower().endsWith(".rrc.gz"))
			{
				type = drop_type::drop_rrc;
			}
			// The emulator allows to execute ANY filetype, just not from drag-and-drop because it is confusing to users
			else if (path.toLower().endsWith(".savestat.gz") || path.toLower().endsWith(".savestat.zst") || suffix_lo == "savestat" || suffix_lo == "sprx" || suffix_lo == "self" || suffix_lo == "bin" || suffix_lo == "prx" || suffix_lo == "elf" || suffix_lo == "o")
			{
				type = drop_type::drop_game;
			}
			else
			{
				return set_result(drop_type::drop_error);
			}
		}
		else
		{
			return set_result(drop_type::drop_error);
		}

		if (drop_paths) // we only need to know the paths on drop
		{
			drop_paths->append(path);
		}
	}

	return set_result(type);
}

void main_window::dropEvent(QDropEvent* event)
{
	event->accept();

	QStringList drop_paths;

	switch (IsValidFile(*event->mimeData(), &drop_paths)) // get valid file paths and drop type
	{
	case drop_type::drop_error:
	{
		event->ignore();
		break;
	}
	case drop_type::drop_rap_edat_pkg: // install the packages
	{
		InstallPackages(drop_paths);
		break;
	}
	case drop_type::drop_pup: // install the firmware
	{
		InstallPup(drop_paths.first());
		break;
	}
	case drop_type::drop_psf: // Display PARAM.SFO content
	{
		for (const auto& psf : drop_paths)
		{
			const std::string psf_path = psf.toStdString();
			std::string info = fmt::format("Dropped PARAM.SFO '%s':\n\n%s", psf_path, psf::load(psf_path).sfo);

			gui_log.success("%s", info);
			info.erase(info.begin(), info.begin() + info.find_first_of('\''));

			QMessageBox mb(QMessageBox::Information, tr("PARAM.SFO Information"), QString::fromStdString(info), QMessageBox::Ok, this);
			mb.setTextInteractionFlags(Qt::TextSelectableByMouse);
			mb.exec();
		}

		break;
	}
	case drop_type::drop_dir: // import valid games to gamelist (games.yaml)
	{
		AddGamesFromDirs(std::move(drop_paths));
		break;
	}
	case drop_type::drop_game: // import valid games to gamelist (games.yaml)
	{
		if (Emu.IsBootingRestricted())
		{
			return;
		}

		if (!m_gui_settings->GetBootConfirmation(this))
		{
			return;
		}

		Emu.GracefulShutdown(false);

		const std::string path = drop_paths.first().toStdString();

		if (const auto error = Emu.BootGame(path, "", true); error != game_boot_result::no_errors)
		{
			gui_log.error("Boot failed: reason: %s, path: %s", error, path);
			show_boot_error(error);
			return;
		}

		if (is_savestate_compatible(path))
		{
			gui_log.success("Savestate Boot from drag and drop done: %s", path);
			AddRecentAction(gui::Recent_Game(QString::fromStdString(path), QString::fromStdString(Emu.GetTitleAndTitleID())), true);
		}
		else
		{
			gui_log.success("Elf Boot from drag and drop done: %s", path);
			AddRecentAction(gui::Recent_Game(QString::fromStdString(Emu.GetBoot()), QString::fromStdString(Emu.GetTitleAndTitleID())), false);
		}

		m_game_list_frame->Refresh(true);
		break;
	}
	case drop_type::drop_rrc: // replay a rsx capture file
	{
		BootRsxCapture(drop_paths.first().toStdString());
		break;
	}
	}
}

void main_window::dragEnterEvent(QDragEnterEvent* event)
{
	event->setAccepted(IsValidFile(*event->mimeData()) != drop_type::drop_error);
}

void main_window::dragMoveEvent(QDragMoveEvent* event)
{
	event->setAccepted(IsValidFile(*event->mimeData()) != drop_type::drop_error);
}

void main_window::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}
