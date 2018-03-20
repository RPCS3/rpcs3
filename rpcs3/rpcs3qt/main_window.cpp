
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QDockWidget>
#include <QDesktopWidget>
#include <QMimeData>

#include "vfs_dialog.h"
#include "save_manager_dialog.h"
#include "trophy_manager_dialog.h"
#include "kernel_explorer.h"
#include "game_list_frame.h"
#include "debugger_frame.h"
#include "log_frame.h"
#include "settings_dialog.h"
#include "auto_pause_settings_dialog.h"
#include "cg_disasm_window.h"
#include "memory_string_searcher.h"
#include "memory_viewer_panel.h"
#include "rsx_debugger.h"
#include "main_window.h"
#include "emu_settings.h"
#include "about_dialog.h"
#include "gamepads_settings_dialog.h"
#include "progress_dialog.h"

#include <thread>

#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"

#include "Crypto/unpkg.h"
#include "Crypto/unself.h"

#include "Loader/PUP.h"
#include "Loader/TAR.h"
#include "Loader/PSF.h"

#include "Utilities/Thread.h"
#include "Utilities/StrUtil.h"

#include "rpcs3_version.h"
#include "Utilities/sysinfo.h"

#include "ui_main_window.h"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

main_window::main_window(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget *parent)
	: QMainWindow(parent), guiSettings(guiSettings), emuSettings(emuSettings), m_sys_menu_opened(false), ui(new Ui::main_window)
{
}

main_window::~main_window()
{
	delete ui;
}

auto Pause = []()
{
	if (Emu.IsReady()) Emu.Run();
	else if (Emu.IsPaused()) Emu.Resume();
	else if (Emu.IsRunning()) Emu.Pause();
	else if (!Emu.GetBoot().empty()) Emu.Load();
};

/* An init method is used so that RPCS3App can create the necessary connects before calling init (specifically the stylesheet connect).
 * Simplifies logic a bit.
 */
void main_window::Init()
{
	ui->setupUi(this);

	setAcceptDrops(true);

	// hide utilities from the average user
	ui->menuUtilities->menuAction()->setVisible(guiSettings->GetValue(gui::m_showDebugTab).toBool());

	// add toolbar widgets (crappy Qt designer is not able to)
	ui->toolBar->setObjectName("mw_toolbar");
	ui->sizeSlider->setRange(0, gui::gl_max_slider_pos);
	ui->sizeSlider->setSliderPosition(guiSettings->GetValue(gui::gl_iconSize).toInt());
	ui->toolBar->addWidget(ui->sizeSliderContainer);
	ui->toolBar->addSeparator();
	ui->toolBar->addWidget(ui->mw_searchbar);

	// for highdpi resize toolbar icons and height dynamically
	// choose factors to mimic Gui-Design in main_window.ui
	// TODO: in case Qt::AA_EnableHighDpiScaling is enabled in main.cpp we only need the else branch
#ifdef _WIN32
	const int toolBarHeight = menuBar()->sizeHint().height() * 1.5;
	ui->toolBar->setIconSize(QSize(toolBarHeight, toolBarHeight));
#else
	const int toolBarHeight = ui->toolBar->iconSize().height();
#endif
	ui->sizeSliderContainer->setFixedWidth(toolBarHeight * 5);
	ui->sizeSlider->setFixedHeight(toolBarHeight * 0.65f);

	CreateActions();
	CreateDockWindows();
	CreateConnects();

	setMinimumSize(350, minimumSizeHint().height());    // seems fine on win 10
	setWindowTitle(QString::fromStdString("RPCS3 " + rpcs3::version.to_string()));

	Q_EMIT RequestGlobalStylesheetChange(guiSettings->GetCurrentStylesheetPath());
	ConfigureGuiFromSettings(true);

#ifdef BRANCH
	if ("RPCS3/rpcs3/master"s != STRINGIZE(BRANCH) && ""s != STRINGIZE(BRANCH))
#else
	if (false)
#endif
	{
		LOG_WARNING(GENERAL, u8"實驗性建置警告! 建置來源: " STRINGIZE(BRANCH));

		QMessageBox msg;
		msg.setWindowTitle(u8"實驗性建置警告");
		msg.setWindowIcon(m_appIcon);
		msg.setIcon(QMessageBox::Critical);
		msg.setTextFormat(Qt::RichText);
		msg.setText(u8"請您理解，這一個版本非 RPCS3 官方所發布的版本。<br>此建置可能會破壞遊戲的變化，甚至 <b>毀損</b> 您的資料。<br>建議下載使用官方版本 <a href='https://rpcs3.net/download'>RPCS3 網站</a>.<br><br>建置來源: " STRINGIZE(BRANCH) u8"<br>是否仍要使用此版本?");
		msg.addButton(tr(u8"是"),QMessageBox::AcceptRole);
		msg.addButton(tr(u8"否"),QMessageBox::RejectRole);
		msg.setDefaultButton(QMessageBox::No);

		if (msg.exec() == QMessageBox::RejectRole)
		{
			std::exit(EXIT_SUCCESS);
		}
	}
}

void main_window::CreateThumbnailToolbar()
{
#ifdef _WIN32
	m_thumb_bar = new QWinThumbnailToolBar(this);
	m_thumb_bar->setWindow(windowHandle());

	m_thumb_playPause = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_playPause->setToolTip(tr(u8"暫停"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
	m_thumb_playPause->setEnabled(false);

	m_thumb_stop = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_stop->setToolTip(tr(u8"停止"));
	m_thumb_stop->setIcon(m_icon_thumb_stop);
	m_thumb_stop->setEnabled(false);

	m_thumb_restart = new QWinThumbnailToolButton(m_thumb_bar);
	m_thumb_restart->setToolTip(tr(u8"重啟"));
	m_thumb_restart->setIcon(m_icon_thumb_restart);
	m_thumb_restart->setEnabled(false);

	m_thumb_bar->addButton(m_thumb_playPause);
	m_thumb_bar->addButton(m_thumb_stop);
	m_thumb_bar->addButton(m_thumb_restart);

	RepaintThumbnailIcons();

	connect(m_thumb_stop, &QWinThumbnailToolButton::clicked, [=]() { Emu.Stop(); });
	connect(m_thumb_restart, &QWinThumbnailToolButton::clicked, [=]() { Emu.Restart(); });
	connect(m_thumb_playPause, &QWinThumbnailToolButton::clicked, Pause);
#endif
}

// returns appIcon
QIcon main_window::GetAppIcon()
{
	return m_appIcon;
}

// loads the appIcon from path and embeds it centered into an empty square icon
void main_window::SetAppIconFromPath(const std::string& path)
{
	// get Icon for the gs_frame from path. this handles presumably all possible use cases
	QString qpath = qstr(path);
	std::string icon_list[] = { "/ICON0.PNG", "/PS3_GAME/ICON0.PNG" };
	std::string path_list[] = { path, sstr(qpath.section("/", 0, -2)), sstr(qpath.section("/", 0, -3)) };
	for (std::string pth : path_list)
	{
		if (!fs::is_dir(pth)) continue;

		for (std::string ico : icon_list)
		{
			ico = pth + ico;
			if (fs::is_file(ico))
			{
				// load the image from path. It will most likely be a rectangle
				QImage source = QImage(qstr(ico));
				int edgeMax = std::max(source.width(), source.height());

				// create a new transparent image with square size and same format as source (maybe handle other formats than RGB32 as well?)
				QImage::Format format = source.format() == QImage::Format_RGB32 ? QImage::Format_ARGB32 : source.format();
				QImage dest = QImage(edgeMax, edgeMax, format);
				dest.fill(QColor("transparent"));

				// get the location to draw the source image centered within the dest image.
				QPoint destPos = source.width() > source.height() ? QPoint(0, (source.width() - source.height()) / 2)
				                                                  : QPoint((source.height() - source.width()) / 2, 0);

				// Paint the source into/over the dest
				QPainter painter(&dest);
				painter.drawImage(destPos, source);
				painter.end();

				// set Icon
				m_appIcon = QIcon(QPixmap::fromImage(dest));
				return;
			}
		}
	}
	// if nothing was found reset the icon to default
	m_appIcon = QApplication::windowIcon();
}

void main_window::Boot(const std::string& path, bool direct, bool add_only)
{
	SetAppIconFromPath(path);
	Emu.SetForceBoot(true);
	Emu.Stop();

	if (Emu.BootGame(path, direct, add_only))
	{
		LOG_SUCCESS(LOADER, u8"啟動完成。");
		const std::string serial = Emu.GetTitleID().empty() ? "" : "[" + Emu.GetTitleID() + "] ";
		AddRecentAction(gui::Recent_Game(qstr(Emu.GetBoot()), qstr(serial + Emu.GetTitle())));
	}
	else
	{
		LOG_ERROR(GENERAL, u8"啟動失敗: %s", path);
	}

	m_gameListFrame->Refresh(true);
}

void main_window::BootElf()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QString path_last_ELF = guiSettings->GetValue(gui::fd_boot_elf).toString();
	QString filePath = QFileDialog::getOpenFileName(this, tr(u8"選擇 (S)ELF 啟動"), path_last_ELF, tr(
		u8"(S)ELF 檔案 (*BOOT.BIN *.elf *.self);;"
		u8"ELF 檔案 (BOOT.BIN *.elf);;"
		u8"SELF 檔案 (EBOOT.BIN *.self);;"
		u8"BOOT 檔案 (*BOOT.BIN);;"
		u8"BIN 檔案 (*.bin);;"
		u8"全部檔案 (*.*)"), 
		Q_NULLPTR, QFileDialog::DontResolveSymlinks);

	if (filePath == NULL)
	{
		if (stopped) Emu.Resume();
		return;
	}

	// If we resolved the filepath earlier we would end up setting the last opened dir to the unwanted
	// game folder in case of having e.g. a Game Folder with collected links to elf files.
	// Don't set last path earlier in case of cancelled dialog
	guiSettings->SetValue(gui::fd_boot_elf, filePath);
	const std::string path = sstr(QFileInfo(filePath).canonicalFilePath());

	LOG_NOTICE(LOADER, u8"從 BootElf 啟動...");
	Boot(path, true);
}

void main_window::BootGame()
{
	bool stopped = false;

	if (Emu.IsRunning())
	{
		Emu.Pause();
		stopped = true;
	}

	QString path_last_Game = guiSettings->GetValue(gui::fd_boot_game).toString();
	QString dirPath = QFileDialog::getExistingDirectory(this, tr(u8"選擇遊戲資料夾"), path_last_Game, QFileDialog::ShowDirsOnly);

	if (dirPath == NULL)
	{
		if (stopped) Emu.Resume();
		return;
	}

	guiSettings->SetValue(gui::fd_boot_game, QFileInfo(dirPath).path());
	const std::string path = sstr(dirPath);

	LOG_NOTICE(LOADER, u8"從 BootGame 啟動...");
	Boot(path);
}

void main_window::InstallPkg(const QString& dropPath)
{
	QString filePath = dropPath;

	if (filePath.isEmpty())
	{
		QString path_last_PKG = guiSettings->GetValue(gui::fd_install_pkg).toString();
		filePath = QFileDialog::getOpenFileName(this, tr(u8"選擇 PKG 安裝"), path_last_PKG, tr(u8"PKG 檔案 (*.pkg);;全部檔案 (*.*)"));
	}
	else
	{
		if (QMessageBox::question(this, tr(u8"PKG 解密/安裝程式"), tr(u8"安裝軟體包: %1?").arg(filePath),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			LOG_NOTICE(LOADER, u8"PKG: 從拖放取消安裝。 檔案: %s", sstr(filePath));
			return;
		}
	}

	if (filePath.isEmpty())
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	guiSettings->SetValue(gui::fd_install_pkg, QFileInfo(filePath).path());
	const std::string fileName = sstr(QFileInfo(filePath).fileName());
	const std::string path = sstr(filePath);

	progress_dialog pdlg(0, 1000, this);
	pdlg.setWindowTitle(tr(u8"RPCS3 軟體包安裝程式"));
	pdlg.setLabelText(tr(u8"安裝軟體包 ... 請稍候 ..."));
	pdlg.setCancelButtonText(tr(u8"取消"));
	pdlg.setWindowModality(Qt::WindowModal);
	pdlg.setFixedWidth(QLabel("This is the very length of the progressdialog due to hidpi reasons.").sizeHint().width());
	pdlg.show();

	// Synchronization variable
	atomic_t<double> progress(0.);
	{
		// Run PKG unpacking asynchronously
		scope_thread worker(u8"PKG 安裝程式", [&]
		{
			if (pkg_install(path, progress))
			{
				progress = 1.;
				return;
			}

			progress = -1.;
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < 1.)
		{
			if (pdlg.wasCanceled())
			{
				progress -= 1.;
			}

			// Update progress window
			double pval = progress;
			pval < 0 ? pval += 1. : pval;
			pdlg.SetValue(static_cast<int>(pval * pdlg.maximum()));
			QCoreApplication::processEvents();
		}

		if (progress > 0.)
		{
			pdlg.SetValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress >= 1.)
	{
		m_gameListFrame->Refresh(true);
		LOG_SUCCESS(GENERAL, u8"成功安裝 %s.", fileName);
		guiSettings->ShowInfoBox(gui::ib_pkg_success, tr(u8"成功!"), tr(u8"成功安裝軟體!"), this);
	}
}

void main_window::InstallPup(const QString& dropPath)
{
	QString filePath = dropPath;

	if (filePath.isEmpty())
	{
		QString path_last_PUP = guiSettings->GetValue(gui::fd_install_pup).toString();
		filePath = QFileDialog::getOpenFileName(this, tr(u8"選擇 PS3UPDAT.PUP 安裝"), path_last_PUP, tr(u8"PS3 更新檔案 (PS3UPDAT.PUP)"));
	}
	else
	{
		if (QMessageBox::question(this, tr(u8"RPCS3 韌體安裝程式"), tr(u8"安裝韌體: %1?").arg(filePath),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			LOG_NOTICE(LOADER, u8"韌體: 從拖放取消安裝。 檔案: %s", sstr(filePath));
			return;
		}
	}

	if (filePath == NULL)
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	guiSettings->SetValue(gui::fd_install_pup, QFileInfo(filePath).path());
	const std::string path = sstr(filePath);

	fs::file pup_f(path);
	pup_object pup(pup_f);
	if (!pup)
	{
		LOG_ERROR(GENERAL, u8"安裝韌體時錯誤: PUP 檔案無效。");
		QMessageBox::critical(this, tr(u8"失敗!"), tr(u8"安裝韌體時錯誤: PUP 檔案無效。"));
		return;
	}

	fs::file update_files_f = pup.get_file(0x300);
	tar_object update_files(update_files_f);
	auto updatefilenames = update_files.get_filenames();

	updatefilenames.erase(std::remove_if(
		updatefilenames.begin(), updatefilenames.end(), [](std::string s) { return s.find("dev_flash_") == std::string::npos; }),
		updatefilenames.end());

	std::string version_string = pup.get_file(0x100).to_string();
	version_string.erase(version_string.find('\n'));

	const std::string cur_version = "4.82";

	if (version_string < cur_version &&
		QMessageBox::question(this, tr(u8"RPCS3 韌體安裝程式"), tr(u8"檢測到舊版韌體。\n最新的韌體版本是 %1 並且您正在嘗試安裝 %2 版本\n您想繼續安裝嗎?").arg(qstr(cur_version), qstr(version_string)),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
	{
		return;
	}

	progress_dialog pdlg(0, static_cast<int>(updatefilenames.size()), this);
	pdlg.setWindowTitle(tr(u8"RPCS3 韌體安裝程式"));
	pdlg.setLabelText(tr(u8"安裝韌體 %1 版本\n請稍候...").arg(qstr(version_string)));
	pdlg.setCancelButtonText(tr(u8"取消"));
	pdlg.setWindowModality(Qt::WindowModal);
	pdlg.setFixedWidth(QLabel("This is the very length of the progressdialog due to hidpi reasons.").sizeHint().width());
	pdlg.show();

	// Synchronization variable
	atomic_t<int> progress(0);
	{
		// Run asynchronously
		scope_thread worker(u8"韌體安裝程式", [&]
		{
			for (const auto& updatefilename : updatefilenames)
			{
				if (progress == -1) break;

				fs::file updatefile = update_files.get_file(updatefilename);

				SCEDecrypter self_dec(updatefile);
				self_dec.LoadHeaders();
				self_dec.LoadMetadata(SCEPKG_ERK, SCEPKG_RIV);
				self_dec.DecryptData();

				auto dev_flash_tar_f = self_dec.MakeFile();
				if (dev_flash_tar_f.size() < 3)
				{
					LOG_ERROR(GENERAL, u8"安裝韌體時錯誤: PUP 內容無效。");
					QMessageBox::critical(this, tr(u8"失敗!"), tr(u8"安裝韌體時錯誤: PUP 內容無效。"));
					progress = -1;
				}

				tar_object dev_flash_tar(dev_flash_tar_f[2]);
				if (!dev_flash_tar.extract(Emu.GetEmuDir()))
				{
					LOG_ERROR(GENERAL, u8"安裝韌體時錯誤: TAR 內容無效。");
					QMessageBox::critical(this, tr(u8"失敗!"), tr(u8"安裝韌體時錯誤: TAR 內容無效。"));
					progress = -1;
				}

				if (progress >= 0)
					progress += 1;
			}
		});

		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), std::abs(progress) < pdlg.maximum())
		{
			if (pdlg.wasCanceled())
			{
				progress = -1;
				break;
			}
			// Update progress window
			pdlg.SetValue(static_cast<int>(progress));
			QCoreApplication::processEvents();
		}

		update_files_f.close();
		pup_f.close();

		if (progress > 0)
		{
			pdlg.SetValue(pdlg.maximum());
			std::this_thread::sleep_for(100ms);
		}
	}

	if (progress > 0)
	{
		LOG_SUCCESS(GENERAL, u8"成功安裝 PS3 韌體 %s 版本。", version_string);
		guiSettings->ShowInfoBox(gui::ib_pup_success, tr(u8"成功!"), tr(u8"已成功安裝 PS3 韌體與 LLE 模組!"), this);
	}
}

// This is ugly, but PS3 headers shall not be included there.
extern void sysutil_send_system_cmd(u64 status, u64 param);

void main_window::DecryptSPRXLibraries()
{
	QString path_last_SPRX = guiSettings->GetValue(gui::fd_decrypt_sprx).toString();
	QStringList modules = QFileDialog::getOpenFileNames(this, tr(u8"選擇 SPRX 檔案"), path_last_SPRX, tr(u8"SPRX 檔案 (*.sprx)"));

	if (modules.isEmpty())
	{
		return;
	}

	Emu.SetForceBoot(true);
	Emu.Stop();

	guiSettings->SetValue(gui::fd_decrypt_sprx, QFileInfo(modules.first()).path());

	LOG_NOTICE(GENERAL, u8"解密 SPRX 庫...");

	for (const QString& module : modules)
	{
		std::string prx_path = sstr(module);
		const std::string& prx_dir = fs::get_parent_dir(prx_path);

		fs::file elf_file(prx_path);

		if (elf_file && elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::size_t prx_ext_pos = prx_path.find_last_of('.');
			const std::string& prx_name = prx_path.substr(prx_dir.size());

			elf_file = decrypt_self(std::move(elf_file));

			prx_path.erase(prx_path.size() - 4, 1); // change *.sprx to *.prx

			if (elf_file)
			{
				if (fs::file new_file{ prx_path, fs::rewrite })
				{
					new_file.write(elf_file.to_string());
					LOG_SUCCESS(GENERAL, u8"解密 %s", prx_dir + prx_name);
				}
				else
				{
					LOG_ERROR(GENERAL, u8"無法建立 %s", prx_path);
				}
			}
			else
			{
				LOG_ERROR(GENERAL, u8"無法解密 %s", prx_dir + prx_name);
			}
		}
	}

	LOG_NOTICE(GENERAL, u8"完成解密所有的 SPRX 庫。");
}

/** Needed so that when a backup occurs of window state in guisettings, the state is current.
* Also, so that on close, the window state is preserved.
*/
void main_window::SaveWindowState()
{
	// Save gui settings
	guiSettings->SetValue(gui::mw_geometry, saveGeometry());
	guiSettings->SetValue(gui::mw_windowState, saveState());
	guiSettings->SetValue(gui::mw_mwState, m_mw->saveState());

	// Save column settings
	m_gameListFrame->SaveSettings();
	// Save splitter state
	m_debuggerFrame->SaveSettings();
}

void main_window::RepaintThumbnailIcons()
{
	QColor newColor = gui::get_Label_Color("thumbnail_icon_color");

	auto icon = [&newColor](const QString& path)
	{
		return gui_settings::colorizedIcon(QPixmap::fromImage(gui_settings::GetOpaqueImageArea(path)), gui::mw_tool_icon_color, newColor);
	};

#ifdef _WIN32
	if (!m_thumb_bar) return;

	m_icon_thumb_play = icon(":/Icons/play.png");
	m_icon_thumb_pause = icon(":/Icons/pause.png");
	m_icon_thumb_stop = icon(":/Icons/stop.png");
	m_icon_thumb_restart = icon(":/Icons/restart.png");

	m_thumb_playPause->setIcon(Emu.IsRunning() ? m_icon_thumb_pause : m_icon_thumb_play);
	m_thumb_stop->setIcon(m_icon_thumb_stop);
	m_thumb_restart->setIcon(m_icon_thumb_restart);
#endif
}

void main_window::RepaintToolBarIcons()
{
	QColor newColor;

	if (guiSettings->GetValue(gui::m_enableUIColors).toBool())
	{
		newColor = guiSettings->GetValue(gui::mw_toolIconColor).value<QColor>();
	}
	else
	{
		newColor = gui::get_Label_Color("toolbar_icon_color");
	}

	auto icon = [&newColor](const QString& path)
	{
		return gui_settings::colorizedIcon(QIcon(path), gui::mw_tool_icon_color, newColor);
	};

	m_icon_play           = icon(":/Icons/play.png");
	m_icon_pause          = icon(":/Icons/pause.png");
	m_icon_stop           = icon(":/Icons/stop.png");
	m_icon_restart        = icon(":/Icons/restart.png");
	m_icon_fullscreen_on  = icon(":/Icons/fullscreen.png");
	m_icon_fullscreen_off = icon(":/Icons/fullscreen_invert.png");

	ui->toolbar_config  ->setIcon(icon(":/Icons/configure.png"));
	ui->toolbar_controls->setIcon(icon(":/Icons/controllers.png"));
	ui->toolbar_disc    ->setIcon(icon(":/Icons/disc.png"));
	ui->toolbar_grid    ->setIcon(icon(":/Icons/grid.png"));
	ui->toolbar_list    ->setIcon(icon(":/Icons/list.png"));
	ui->toolbar_refresh ->setIcon(icon(":/Icons/refresh.png"));
	ui->toolbar_snap    ->setIcon(icon(":/Icons/screenshot.png"));
	ui->toolbar_sort    ->setIcon(icon(":/Icons/sort.png"));
	ui->toolbar_stop    ->setIcon(icon(":/Icons/stop.png"));

	if (Emu.IsRunning())
	{
		ui->toolbar_start->setIcon(m_icon_pause);
	}
	else if (Emu.IsStopped() && !Emu.GetBoot().empty())
	{
		ui->toolbar_start->setIcon(m_icon_restart);
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
	}

	if (isFullScreen())
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_off);
	}
	else
	{
		ui->toolbar_fullscreen->setIcon(m_icon_fullscreen_on);
	}

	ui->sizeSlider->setStyleSheet(ui->sizeSlider->styleSheet().append("QSlider::handle:horizontal{ background: rgba(%1, %2, %3, %4); }")
		.arg(newColor.red()).arg(newColor.green()).arg(newColor.blue()).arg(newColor.alpha()));
}

void main_window::OnEmuRun()
{
	m_debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr(u8"暫停仿真"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr(u8"暫停\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setToolTip(tr(u8"暫停仿真"));
	EnableMenus(true);
}

void main_window::OnEmuResume()
{
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr(u8"暫停仿真"));
	m_thumb_playPause->setIcon(m_icon_thumb_pause);
#endif
	ui->sysPauseAct->setText(tr(u8"暫停\tCtrl+P"));
	ui->sysPauseAct->setIcon(m_icon_pause);
	ui->toolbar_start->setIcon(m_icon_pause);
	ui->toolbar_start->setToolTip(tr(u8"暫停仿真"));
}

void main_window::OnEmuPause()
{
#ifdef _WIN32
	m_thumb_playPause->setToolTip(tr(u8"繼續仿真"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(tr(u8"繼續\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setToolTip(tr(u8"繼續仿真"));
}

void main_window::OnEmuStop()
{
	m_debuggerFrame->EnableButtons(false);
	m_debuggerFrame->ClearBreakpoints();

	ui->sysPauseAct->setText(Emu.IsReady() ? tr(u8"啟動\tCtrl+E") : tr(u8"繼續\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(Emu.IsReady() ? tr(u8"啟動仿真") : tr(u8"繼續仿真"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	EnableMenus(false);
	if (!Emu.GetBoot().empty())
	{
		ui->toolbar_start->setEnabled(true);
		ui->toolbar_start->setIcon(m_icon_restart);
		ui->toolbar_start->setToolTip(tr(u8"重啟仿真"));
		ui->sysRebootAct->setEnabled(true);
#ifdef _WIN32
		m_thumb_restart->setEnabled(true);
#endif
	}
	else
	{
		ui->toolbar_start->setIcon(m_icon_play);
		ui->toolbar_start->setToolTip(Emu.IsReady() ? tr(u8"啟動仿真") : tr(u8"繼續仿真"));
	}
}

void main_window::OnEmuReady()
{
	m_debuggerFrame->EnableButtons(true);
#ifdef _WIN32
	m_thumb_playPause->setToolTip(Emu.IsReady() ? tr(u8"啟動仿真") : tr(u8"繼續仿真"));
	m_thumb_playPause->setIcon(m_icon_thumb_play);
#endif
	ui->sysPauseAct->setText(Emu.IsReady() ? tr(u8"啟動\tCtrl+E") : tr(u8"繼續\tCtrl+E"));
	ui->sysPauseAct->setIcon(m_icon_play);
	ui->toolbar_start->setIcon(m_icon_play);
	ui->toolbar_start->setToolTip(Emu.IsReady() ? tr(u8"啟動仿真") : tr(u8"繼續仿真"));
	EnableMenus(true);
}

void main_window::EnableMenus(bool enabled)
{
	// Thumbnail Buttons
#ifdef _WIN32
	m_thumb_playPause->setEnabled(enabled);
	m_thumb_stop->setEnabled(enabled);
	m_thumb_restart->setEnabled(enabled);
#endif

	// Toolbar
	ui->toolbar_start->setEnabled(enabled);
	ui->toolbar_stop->setEnabled(enabled);

	// Emulation
	ui->sysPauseAct->setEnabled(enabled);
	ui->sysStopAct->setEnabled(enabled);
	ui->sysRebootAct->setEnabled(enabled);

	// PS3 Commands
	ui->sysSendOpenMenuAct->setEnabled(enabled);
	ui->sysSendExitAct->setEnabled(enabled);

	// Tools
	ui->toolskernel_explorerAct->setEnabled(enabled);
	ui->toolsmemory_viewerAct->setEnabled(enabled);
	ui->toolsRsxDebuggerAct->setEnabled(enabled);
	ui->toolsStringSearchAct->setEnabled(enabled);
}

void main_window::BootRecentAction(const QAction* act)
{
	if (Emu.IsRunning())
	{
		return;
	}

	const QString pth = act->data().toString();
	const std::string path = sstr(pth);
	QString nam;
	bool containsPath = false;

	int idx = -1;
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		if (m_rg_entries.at(i).first == pth)
		{
			idx = i;
			containsPath = true;
			nam = m_rg_entries.at(idx).second;
		}
	}

	// path is invalid: remove action from list return
	if ((containsPath && nam.isEmpty()) || (!QFileInfo(pth).isDir() && !QFileInfo(pth).isFile()))
	{
		if (containsPath)
		{
			// clear menu of actions
			for (auto act : m_recentGameActs)
			{
				ui->bootRecentMenu->removeAction(act);
			}

			// remove action from list
			m_rg_entries.removeAt(idx);
			m_recentGameActs.removeAt(idx);

			guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));

			LOG_ERROR(GENERAL, u8"近期遊戲無效，從近期啟動清單刪除: %s", path);

			// refill menu with actions
			for (int i = 0; i < m_recentGameActs.count(); i++)
			{
				m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i + 1));
				m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
				ui->bootRecentMenu->addAction(m_recentGameActs[i]);
			}

			LOG_WARNING(GENERAL, u8"更新近期啟動清單");
			return;
		}

		LOG_ERROR(GENERAL, u8"路徑無效，不在 m_rg_paths: %s", path);
		return;
	}

	LOG_NOTICE(LOADER, u8"從近期遊戲清單啟動...");
	Boot(path, true);
};

QAction* main_window::CreateRecentAction(const q_string_pair& entry, const uint& sc_idx)
{
	// if path is not valid remove from list
	if (entry.second.isEmpty() || (!QFileInfo(entry.first).isDir() && !QFileInfo(entry.first).isFile()))
	{
		if (m_rg_entries.contains(entry))
		{
			LOG_ERROR(GENERAL, u8"近期遊戲無效，從近期啟動清單刪除: %s", sstr(entry.first));

			int idx = m_rg_entries.indexOf(entry);
			m_rg_entries.removeAt(idx);

			guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));
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
	act->setToolTip(entry.second);
	act->setShortcut(tr("Ctrl+%1").arg(sc_idx));

	// truncate if too long
	if (shown_name.length() > 60)
	{
		act->setText(shown_name.left(27) + "(....)" + shown_name.right(27));
	}

	// connect boot
	connect(act, &QAction::triggered, [=]() {BootRecentAction(act); });

	return act;
};

void main_window::AddRecentAction(const q_string_pair& entry)
{
	// don't change list on freeze
	if (ui->freezeRecentAct->isChecked())
	{
		return;
	}

	// create new action, return if not valid
	QAction* act = CreateRecentAction(entry, 1);
	if (!act)
	{
		return;
	}

	// clear menu of actions
	for (auto act : m_recentGameActs)
	{
		ui->bootRecentMenu->removeAction(act);
	}

	// if path already exists, remove it in order to get it to beginning
	if (m_rg_entries.contains(entry))
	{
		int idx = m_rg_entries.indexOf(entry);
		m_rg_entries.removeAt(idx);
		m_recentGameActs.removeAt(idx);
	}

	// remove oldest action at the end if needed
	if (m_rg_entries.count() == 9)
	{
		m_rg_entries.removeLast();
		m_recentGameActs.removeLast();
	}
	else if (m_rg_entries.count() > 9)
	{
		LOG_ERROR(LOADER, u8"近期遊戲名單表過大");
	}

	if (m_rg_entries.count() < 9)
	{
		// add new action at the beginning
		m_rg_entries.prepend(entry);
		m_recentGameActs.prepend(act);
	}

	// refill menu with actions
	for (int i = 0; i < m_recentGameActs.count(); i++)
	{
		m_recentGameActs[i]->setShortcut(tr("Ctrl+%1").arg(i+1));
		m_recentGameActs[i]->setToolTip(m_rg_entries.at(i).second);
		ui->bootRecentMenu->addAction(m_recentGameActs[i]);
	}

	guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(m_rg_entries));
}

void main_window::RepaintGui()
{
	if (m_gameListFrame)
	{
		m_gameListFrame->RepaintIcons(true);
	}

	if (m_logFrame)
	{
		m_logFrame->RepaintTextColors();
	}

	if (m_debuggerFrame)
	{
		m_debuggerFrame->ChangeColors();
	}

	RepaintToolbar();
	RepaintToolBarIcons();
	RepaintThumbnailIcons();
}

void main_window::RepaintToolbar()
{
	if (guiSettings->GetValue(gui::m_enableUIColors).toBool())
	{
		QColor tbc = guiSettings->GetValue(gui::mw_toolBarColor).value<QColor>();

		ui->toolBar->setStyleSheet(gui::stylesheet + QString(
			"QToolBar { background-color: rgba(%1, %2, %3, %4); }"
			"QToolBar::separator {background-color: rgba(%5, %6, %7, %8); width: 1px; margin-top: 2px; margin-bottom: 2px;}"
			"QSlider { background-color: rgba(%1, %2, %3, %4); }"
			"QLineEdit { background-color: rgba(%1, %2, %3, %4); }")
			.arg(tbc.red()).arg(tbc.green()).arg(tbc.blue()).arg(tbc.alpha())
			.arg(tbc.red() - 20).arg(tbc.green() - 20).arg(tbc.blue() - 20).arg(tbc.alpha() - 20)
		);
	}
	else
	{
		ui->toolBar->setStyleSheet(gui::stylesheet);
	}
}

void main_window::CreateActions()
{
	ui->exitAct->setShortcuts(QKeySequence::Quit);

	ui->toolbar_start->setEnabled(false);
	ui->toolbar_stop->setEnabled(false);

	m_categoryVisibleActGroup = new QActionGroup(this);
	m_categoryVisibleActGroup->addAction(ui->showCatHDDGameAct);
	m_categoryVisibleActGroup->addAction(ui->showCatDiscGameAct);
	m_categoryVisibleActGroup->addAction(ui->showCatHomeAct);
	m_categoryVisibleActGroup->addAction(ui->showCatAudioVideoAct);
	m_categoryVisibleActGroup->addAction(ui->showCatGameDataAct);
	m_categoryVisibleActGroup->addAction(ui->showCatUnknownAct);
	m_categoryVisibleActGroup->addAction(ui->showCatOtherAct);
	m_categoryVisibleActGroup->setExclusive(false);

	m_iconSizeActGroup = new QActionGroup(this);
	m_iconSizeActGroup->addAction(ui->setIconSizeTinyAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeSmallAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeMediumAct);
	m_iconSizeActGroup->addAction(ui->setIconSizeLargeAct);

	m_listModeActGroup = new QActionGroup(this);
	m_listModeActGroup->addAction(ui->setlistModeListAct);
	m_listModeActGroup->addAction(ui->setlistModeGridAct);
}

void main_window::CreateConnects()
{
	connect(ui->bootElfAct, &QAction::triggered, this, &main_window::BootElf);
	connect(ui->bootGameAct, &QAction::triggered, this, &main_window::BootGame);

	connect(ui->bootRecentMenu, &QMenu::aboutToShow, [=]
	{
		// Enable/Disable Recent Games List
		const bool stopped = Emu.IsStopped();
		for (auto act : ui->bootRecentMenu->actions())
		{
			if (act != ui->freezeRecentAct && act != ui->clearRecentAct)
			{
				act->setEnabled(stopped);
			}
		}
	});

	connect(ui->clearRecentAct, &QAction::triggered, [this]
	{
		if (ui->freezeRecentAct->isChecked()) { return; }
		m_rg_entries.clear();
		for (auto act : m_recentGameActs)
		{
			ui->bootRecentMenu->removeAction(act);
		}
		m_recentGameActs.clear();
		guiSettings->SetValue(gui::rg_entries, guiSettings->List2Var(q_pair_list()));
	});

	connect(ui->freezeRecentAct, &QAction::triggered, [=](bool checked)
	{
		guiSettings->SetValue(gui::rg_freeze, checked);
	});

	connect(ui->bootInstallPkgAct, &QAction::triggered, [this] {InstallPkg(); });
	connect(ui->bootInstallPupAct, &QAction::triggered, [this] {InstallPup(); });
	connect(ui->exitAct, &QAction::triggered, this, &QWidget::close);
	connect(ui->sysPauseAct, &QAction::triggered, Pause);
	connect(ui->sysStopAct, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->sysRebootAct, &QAction::triggered, [=]() { Emu.Restart(); });

	connect(ui->sysSendOpenMenuAct, &QAction::triggered, [=]
	{
		sysutil_send_system_cmd(m_sys_menu_opened ? 0x0132 /* CELL_SYSUTIL_SYSTEM_MENU_CLOSE */ : 0x0131 /* CELL_SYSUTIL_SYSTEM_MENU_OPEN */, 0);
		m_sys_menu_opened = !m_sys_menu_opened;
		ui->sysSendOpenMenuAct->setText(tr("Send &%0 system menu cmd").arg(m_sys_menu_opened ? tr("close") : tr("open")));
	});

	connect(ui->sysSendExitAct, &QAction::triggered, [=]
	{
		sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0);
	});

	auto openSettings = [=](int tabIndex)
	{
		settings_dialog dlg(guiSettings, emuSettings, tabIndex, this);
		connect(&dlg, &settings_dialog::GuiSettingsSaveRequest, this, &main_window::SaveWindowState);
		connect(&dlg, &settings_dialog::GuiSettingsSyncRequest, this, &main_window::ConfigureGuiFromSettings);
		connect(&dlg, &settings_dialog::GuiStylesheetRequest, this, &main_window::RequestGlobalStylesheetChange);
		connect(&dlg, &settings_dialog::GuiRepaintRequest, this, &main_window::RepaintGui);
		dlg.exec();
	};

	connect(ui->confCPUAct,    &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->confGPUAct,    &QAction::triggered, [=]() { openSettings(1); });
	connect(ui->confAudioAct,  &QAction::triggered, [=]() { openSettings(2); });
	connect(ui->confIOAct,     &QAction::triggered, [=]() { openSettings(3); });
	connect(ui->confSystemAct, &QAction::triggered, [=]() { openSettings(4); });

	connect(ui->confPadsAct, &QAction::triggered, this, [=]
	{
		gamepads_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confAutopauseManagerAct, &QAction::triggered, [=]
	{
		auto_pause_settings_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->confVFSDialogAct, &QAction::triggered, [=]
	{
		vfs_dialog dlg(guiSettings, emuSettings, this);
		dlg.exec();
		m_gameListFrame->Refresh(true); // dev-hdd0 may have changed. Refresh just in case.
	});

	connect(ui->confSavedataManagerAct, &QAction::triggered, [=]
	{
		save_manager_dialog* sdid = new save_manager_dialog(guiSettings);
		sdid->show();
	});

	connect(ui->actionManage_Trophy_Data, &QAction::triggered, [=]
	{
		trophy_manager_dialog* trop_manager = new trophy_manager_dialog(guiSettings);
		trop_manager->show();
	});

	connect(ui->toolsCgDisasmAct, &QAction::triggered, [=]
	{
		cg_disasm_window* cgdw = new cg_disasm_window(guiSettings);
		cgdw->show();
	});

	connect(ui->toolskernel_explorerAct, &QAction::triggered, [=]
	{
		kernel_explorer* kernelExplorer = new kernel_explorer(this);
		kernelExplorer->show();
	});

	connect(ui->toolsmemory_viewerAct, &QAction::triggered, [=]
	{
		memory_viewer_panel* mvp = new memory_viewer_panel(this);
		mvp->show();
	});

	connect(ui->toolsRsxDebuggerAct, &QAction::triggered, [=]
	{
		rsx_debugger* rsx = new rsx_debugger(this);
		rsx->show();
	});

	connect(ui->toolsStringSearchAct, &QAction::triggered, [=]
	{
		memory_string_searcher* mss = new memory_string_searcher(this);
		mss->show();
	});

	connect(ui->toolsDecryptSprxLibsAct, &QAction::triggered, this, &main_window::DecryptSPRXLibraries);

	connect(ui->showDebuggerAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_debuggerFrame->show() : m_debuggerFrame->hide();
		guiSettings->SetValue(gui::mw_debugger, checked);
	});

	connect(ui->showLogAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_logFrame->show() : m_logFrame->hide();
		guiSettings->SetValue(gui::mw_logger, checked);
	});

	connect(ui->showGameListAct, &QAction::triggered, [=](bool checked)
	{
		checked ? m_gameListFrame->show() : m_gameListFrame->hide();
		guiSettings->SetValue(gui::mw_gamelist, checked);
	});

	connect(ui->showToolBarAct, &QAction::triggered, [=](bool checked)
	{
		ui->toolBar->setVisible(checked);
		guiSettings->SetValue(gui::mw_toolBarVisible, checked);
	});

	connect(ui->refreshGameListAct, &QAction::triggered, [=]
	{
		m_gameListFrame->Refresh(true);
	});

	connect(m_categoryVisibleActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		QStringList categories;
		int id;
		const bool& checked = act->isChecked();

		if      (act == ui->showCatHDDGameAct)    categories += category::non_disc_games, id = Category::Non_Disc_Game;
		else if (act == ui->showCatDiscGameAct)   categories += category::disc_Game, id = Category::Disc_Game;
		else if (act == ui->showCatHomeAct)       categories += category::home, id = Category::Home;
		else if (act == ui->showCatAudioVideoAct) categories += category::media, id = Category::Media;
		else if (act == ui->showCatGameDataAct)   categories += category::data, id = Category::Data;
		else if (act == ui->showCatUnknownAct)    categories += category::unknown, id = Category::Unknown_Cat;
		else if (act == ui->showCatOtherAct)      categories += category::others, id = Category::Others;
		else LOG_WARNING(GENERAL, "categoryVisibleActGroup: category action not found");

		m_gameListFrame->ToggleCategoryFilter(categories, checked);
		guiSettings->SetCategoryVisibility(id, checked);
	});

	connect(ui->aboutAct, &QAction::triggered, [this]
	{
		about_dialog dlg(this);
		dlg.exec();
	});

	connect(ui->aboutQtAct, &QAction::triggered, qApp, &QApplication::aboutQt);

	auto resizeIcons = [=](const int& index)
	{
		if (ui->sizeSlider->value() != index)
		{
			ui->sizeSlider->setSliderPosition(index);
		}
		if (m_save_slider_pos)
		{
			m_save_slider_pos = false;
			guiSettings->SetValue(gui::gl_iconSize, index);
		}
		m_gameListFrame->ResizeIcons(index);
	};

	connect(m_iconSizeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		int index;

		if (act == ui->setIconSizeTinyAct)
			index = 0;
		else if (act == ui->setIconSizeSmallAct)
			index = gui::get_Index(gui::gl_icon_size_small);
		else if (act == ui->setIconSizeMediumAct)
			index = gui::get_Index(gui::gl_icon_size_medium);
		else
			index = gui::gl_max_slider_pos;

		m_save_slider_pos = true;
		resizeIcons(index);
	});

	connect (m_gameListFrame, &game_list_frame::RequestIconSizeChange, [=](const int& val)
	{
		const int idx = ui->sizeSlider->value() + val;
		m_save_slider_pos = true;
		SetIconSizeActions(idx);
		resizeIcons(idx);
	});

	connect(m_listModeActGroup, &QActionGroup::triggered, [=](QAction* act)
	{
		bool isList = act == ui->setlistModeListAct;
		m_gameListFrame->SetListMode(isList);
		m_categoryVisibleActGroup->setEnabled(isList);
	});

	connect(ui->toolbar_disc, &QAction::triggered, this, &main_window::BootGame);
	connect(ui->toolbar_refresh, &QAction::triggered, [=]() { m_gameListFrame->Refresh(true); });
	connect(ui->toolbar_stop, &QAction::triggered, [=]() { Emu.Stop(); });
	connect(ui->toolbar_start, &QAction::triggered, Pause);

	connect(ui->toolbar_fullscreen, &QAction::triggered, [=]
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

	connect(ui->toolbar_controls, &QAction::triggered, [=]() { gamepads_settings_dialog dlg(this); dlg.exec(); });
	connect(ui->toolbar_config, &QAction::triggered, [=]() { openSettings(0); });
	connect(ui->toolbar_list, &QAction::triggered, [=]() { ui->setlistModeListAct->trigger(); });
	connect(ui->toolbar_grid, &QAction::triggered, [=]() { ui->setlistModeGridAct->trigger(); });

	connect(ui->sizeSlider, &QSlider::valueChanged, resizeIcons);
	connect(ui->sizeSlider, &QSlider::sliderReleased, this, [&] { guiSettings->SetValue(gui::gl_iconSize, ui->sizeSlider->value()); });
	connect(ui->sizeSlider, &QSlider::actionTriggered, [&](int action)
	{
		if (action != QAbstractSlider::SliderNoAction && action != QAbstractSlider::SliderMove)
		{	// we only want to save on mouseclicks or slider release (the other connect handles this)
			m_save_slider_pos = true; // actionTriggered happens before the value was changed
		}
	});

	connect(ui->mw_searchbar, &QLineEdit::textChanged, m_gameListFrame, &game_list_frame::SetSearchText);
}

void main_window::CreateDockWindows()
{
	// new mainwindow widget because existing seems to be bugged for now
	m_mw = new QMainWindow();
	m_mw->setContextMenuPolicy(Qt::PreventContextMenu);

	m_gameListFrame = new game_list_frame(guiSettings, emuSettings, m_mw);
	m_gameListFrame->setObjectName("gamelist");
	m_debuggerFrame = new debugger_frame(guiSettings, m_mw);
	m_debuggerFrame->setObjectName("debugger");
	m_logFrame = new log_frame(guiSettings, m_mw);
	m_logFrame->setObjectName("logger");

	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_gameListFrame);
	m_mw->addDockWidget(Qt::LeftDockWidgetArea, m_logFrame);
	m_mw->addDockWidget(Qt::RightDockWidgetArea, m_debuggerFrame);
	m_mw->setDockNestingEnabled(true);
	setCentralWidget(m_mw);

	connect(m_logFrame, &log_frame::LogFrameClosed, [=]()
	{
		if (ui->showLogAct->isChecked())
		{
			ui->showLogAct->setChecked(false);
			guiSettings->SetValue(gui::mw_logger, false);
		}
	});

	connect(m_debuggerFrame, &debugger_frame::DebugFrameClosed, [=]()
	{
		if (ui->showDebuggerAct->isChecked())
		{
			ui->showDebuggerAct->setChecked(false);
			guiSettings->SetValue(gui::mw_debugger, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::GameListFrameClosed, [=]()
	{
		if (ui->showGameListAct->isChecked())
		{
			ui->showGameListAct->setChecked(false);
			guiSettings->SetValue(gui::mw_gamelist, false);
		}
	});

	connect(m_gameListFrame, &game_list_frame::RequestBoot, [this](const std::string& path){ Boot(path); });
}

void main_window::ConfigureGuiFromSettings(bool configure_all)
{
	// Restore GUI state if needed. We need to if they exist.
	QByteArray geometry = guiSettings->GetValue(gui::mw_geometry).toByteArray();
	if (geometry.isEmpty() == false)
	{
		restoreGeometry(geometry);
	}
	else
	{	// By default, set the window to 70% of the screen and the debugger frame is hidden.
		m_debuggerFrame->hide();

		QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
		resize(defaultSize);
	}

	restoreState(guiSettings->GetValue(gui::mw_windowState).toByteArray());
	m_mw->restoreState(guiSettings->GetValue(gui::mw_mwState).toByteArray());

	ui->freezeRecentAct->setChecked(guiSettings->GetValue(gui::rg_freeze).toBool());
	m_rg_entries = guiSettings->Var2List(guiSettings->GetValue(gui::rg_entries));

	// clear recent games menu of actions
	for (auto act : m_recentGameActs)
	{
		ui->bootRecentMenu->removeAction(act);
	}
	m_recentGameActs.clear();
	// Fill the recent games menu
	for (int i = 0; i < m_rg_entries.count(); i++)
	{
		// adjust old unformatted entries (avoid duplication)
		m_rg_entries[i] = gui::Recent_Game(m_rg_entries[i].first, m_rg_entries[i].second);

		// create new action
		QAction* act = CreateRecentAction(m_rg_entries[i], i + 1);

		// add action to menu
		if (act)
		{
			m_recentGameActs.append(act);
			ui->bootRecentMenu->addAction(act);
		}
		else
		{
			i--; // list count is now an entry shorter so we have to repeat the same index in order to load all other entries
		}
	}

	ui->showLogAct->setChecked(guiSettings->GetValue(gui::mw_logger).toBool());
	ui->showGameListAct->setChecked(guiSettings->GetValue(gui::mw_gamelist).toBool());
	ui->showDebuggerAct->setChecked(guiSettings->GetValue(gui::mw_debugger).toBool());
	ui->showToolBarAct->setChecked(guiSettings->GetValue(gui::mw_toolBarVisible).toBool());

	m_debuggerFrame->setVisible(ui->showDebuggerAct->isChecked());
	m_logFrame->setVisible(ui->showLogAct->isChecked());
	m_gameListFrame->setVisible(ui->showGameListAct->isChecked());
	ui->toolBar->setVisible(ui->showToolBarAct->isChecked());

	RepaintToolbar();

	ui->showCatHDDGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Non_Disc_Game));
	ui->showCatDiscGameAct->setChecked(guiSettings->GetCategoryVisibility(Category::Disc_Game));
	ui->showCatHomeAct->setChecked(guiSettings->GetCategoryVisibility(Category::Home));
	ui->showCatAudioVideoAct->setChecked(guiSettings->GetCategoryVisibility(Category::Media));
	ui->showCatGameDataAct->setChecked(guiSettings->GetCategoryVisibility(Category::Data));
	ui->showCatUnknownAct->setChecked(guiSettings->GetCategoryVisibility(Category::Unknown_Cat));
	ui->showCatOtherAct->setChecked(guiSettings->GetCategoryVisibility(Category::Others));

	SetIconSizeActions(guiSettings->GetValue(gui::gl_iconSize).toInt());

	bool isListMode = guiSettings->GetValue(gui::gl_listMode).toBool();
	if (isListMode)
		ui->setlistModeListAct->setChecked(true);
	else
		ui->setlistModeGridAct->setChecked(true);
	m_categoryVisibleActGroup->setEnabled(isListMode);

	if (configure_all)
	{
		// Handle log settings
		m_logFrame->LoadSettings();

		// Gamelist
		m_gameListFrame->LoadSettings();
	}
}

void main_window::SetIconSizeActions(int idx)
{
	if (idx < gui::get_Index((gui::gl_icon_size_small + gui::gl_icon_size_min) / 2))
		ui->setIconSizeTinyAct->setChecked(true);
	else if (idx < gui::get_Index((gui::gl_icon_size_medium + gui::gl_icon_size_small) / 2))
		ui->setIconSizeSmallAct->setChecked(true);
	else if (idx < gui::get_Index((gui::gl_icon_size_max + gui::gl_icon_size_medium) / 2))
		ui->setIconSizeMediumAct->setChecked(true);
	else
		ui->setIconSizeLargeAct->setChecked(true);
}

void main_window::keyPressEvent(QKeyEvent *keyEvent)
{
	if (((keyEvent->modifiers() & Qt::AltModifier) && keyEvent->key() == Qt::Key_Return) || (isFullScreen() && keyEvent->key() == Qt::Key_Escape))
	{
		ui->toolbar_fullscreen->trigger();
	}

	if (keyEvent->modifiers() & Qt::ControlModifier)
	{
		switch (keyEvent->key())
		{
		case Qt::Key_E: if (Emu.IsPaused()) Emu.Resume(); else if (Emu.IsReady()) Emu.Run(); return;
		case Qt::Key_P: if (Emu.IsRunning()) Emu.Pause(); return;
		case Qt::Key_S: if (!Emu.IsStopped()) Emu.Stop(); return;
		case Qt::Key_R: if (!Emu.GetBoot().empty()) Emu.Restart(); return;
		}
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

/** Override the Qt close event to have the emulator stop and the application die.  May add a warning dialog in future.
*/
void main_window::closeEvent(QCloseEvent* closeEvent)
{
	Q_UNUSED(closeEvent);

	// Cleanly stop the emulator.
	Emu.Stop();

	SaveWindowState();

	// I need the gui settings to sync, and that means having the destructor called as guiSetting's parent is main_window.
	setAttribute(Qt::WA_DeleteOnClose);
	QMainWindow::close();

	// It's possible to have other windows open, like games.  So, force the application to die.
	QApplication::quit();
}

/**
Add valid disc games to gamelist (games.yml)
@param path = dir path to scan for game
*/
void main_window::AddGamesFromDir(const QString& path)
{
	if (!QFileInfo(path).isDir()) return;

	const std::string s_path = sstr(path);

	// search dropped path first or else the direct parent to an elf is wrongly skipped
	if (Emu.BootGame(s_path, false, true))
	{
		LOG_NOTICE(GENERAL, "Returned from game addition by drag and drop: %s", s_path);
	}

	// search direct subdirectories, that way we can drop one folder containing all games
	for (const auto& entry : fs::dir(s_path))
	{
		if (entry.name == "." || entry.name == "..") continue;

		const std::string pth = s_path + "/" + entry.name;

		if (!QFileInfo(qstr(pth)).isDir()) continue;

		if (Emu.BootGame(pth, false, true))
		{
			LOG_NOTICE(GENERAL, "Returned from game addition by drag and drop: %s", pth);
		}
	}
}

/**
Check data for valid file types and cache their paths if necessary
@param md = data containing file urls
@param savePaths = flag for path caching
@returns validity of file type
*/
int main_window::IsValidFile(const QMimeData& md, QStringList* dropPaths)
{
	int dropType = drop_type::drop_error;

	const QList<QUrl> list = md.urls(); // get list of all the dropped file urls

	for (auto&& url : list) // check each file in url list for valid type
	{
		const QString path = url.toLocalFile(); // convert url to filepath

		const QFileInfo info = path;

		// check for directories first, only valid if all other paths led to directories until now.
		if (info.isDir())
		{
			if (dropType != drop_type::drop_dir && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_dir;
		}
		else if (info.fileName() == "PS3UPDAT.PUP")
		{
			if (list.size() != 1)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_pup;
		}
		else if (info.suffix().toLower() == "pkg")
		{
			if (dropType != drop_type::drop_pkg && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_pkg;
		}
		else if (info.suffix() == "rap")
		{
			if (dropType != drop_type::drop_rap && dropType != drop_type::drop_error)
			{
				return drop_type::drop_error;
			}

			dropType = drop_type::drop_rap;
		}
		else if (list.size() == 1)
		{
			dropType = drop_type::drop_game;
		}
		else
		{
			return drop_type::drop_error;
		}

		if (dropPaths) // we only need to know the paths on drop
		{
			dropPaths->append(path);
		}
	}

	return dropType;
}

void main_window::dropEvent(QDropEvent* event)
{
	QStringList dropPaths;

	switch (IsValidFile(*event->mimeData(), &dropPaths)) // get valid file paths and drop type
	{
	case drop_type::drop_error:
		break;
	case drop_type::drop_pkg: // install the packages
		for (const auto& path : dropPaths)
		{
			InstallPkg(path);
		}
		break;
	case drop_type::drop_pup: // install the firmware
		InstallPup(dropPaths.first());
		break;
	case drop_type::drop_rap: // import rap files to exdata dir
		for (const auto& rap : dropPaths)
		{
			const std::string rapname = sstr(QFileInfo(rap).fileName());

			// TODO: use correct user ID once User Manager is implemented
			if (!fs::copy_file(sstr(rap), fmt::format("%s/home/%s/exdata/%s", Emu.GetHddDir(), "00000001", rapname), false))
			{
				LOG_WARNING(GENERAL, u8"無法拖放複製 rap 檔案: %s", rapname);
			}
			else
			{
				LOG_SUCCESS(GENERAL, u8"成功拖放複製了 rap 檔案: %s", rapname);
			}
		}
		break;
	case drop_type::drop_dir: // import valid games to gamelist (games.yaml)
		for (const auto& path : dropPaths)
		{
			AddGamesFromDir(path);
		}
		m_gameListFrame->Refresh(true);
		break;
	case drop_type::drop_game: // import valid games to gamelist (games.yaml)
		if (Emu.BootGame(sstr(dropPaths.first()), true))
		{
			LOG_SUCCESS(GENERAL, u8"拖放 Elf 啟動完成: %s", sstr(dropPaths.first()));
		}
		m_gameListFrame->Refresh(true);
		break;
	default:
		LOG_WARNING(GENERAL, u8"無效的拖放類型於遊戲清單放入事件");
		break;
	}
}

void main_window::dragEnterEvent(QDragEnterEvent* event)
{
	if (IsValidFile(*event->mimeData()))
	{
		event->accept();
	}
}

void main_window::dragMoveEvent(QDragMoveEvent* event)
{
	if (IsValidFile(*event->mimeData()))
	{
		event->accept();
	}
}

void main_window::dragLeaveEvent(QDragLeaveEvent* event)
{
	event->accept();
}
