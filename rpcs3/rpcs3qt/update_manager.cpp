#include "update_manager.h"
#include "progress_dialog.h"
#include "localized.h"
#include "rpcs3_version.h"
#include "downloader.h"
#include "gui_settings.h"
#include "Utilities/File.h"
#include "Emu/System.h"
#include "Crypto/utils.h"
#include "util/logs.hpp"
#include "util/types.hpp"
#include "util/sysinfo.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDateTime>
#include <QMessageBox>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

#if defined(_WIN32) || defined(__APPLE__)
#include <7z.h>
#include <7zAlloc.h>
#include <7zCrc.h>
#include <7zFile.h>
#endif

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <CpuArch.h>

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include "Utilities/StrUtil.h"
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

LOG_CHANNEL(update_log, "UPDATER");

update_manager::update_manager(QObject* parent, std::shared_ptr<gui_settings> gui_settings)
	: QObject(parent), m_gui_settings(std::move(gui_settings))
{
}

void update_manager::check_for_updates(bool automatic, bool check_only, bool auto_accept, QWidget* parent)
{
	update_log.notice("Checking for updates: automatic=%d, check_only=%d, auto_accept=%d", automatic, check_only, auto_accept);

	m_update_message.clear();
	m_changelog.clear();

	if (automatic)
	{
		// Don't check for updates on local builds
		if (rpcs3::is_local_build())
		{
			update_log.notice("Skipped automatic update check: this is a local build");
			return;
		}
#ifdef __linux__
		// Don't check for updates on startup if RPCS3 is not running from an AppImage.
		if (!::getenv("APPIMAGE"))
		{
			update_log.notice("Skipped automatic update check: this is not an AppImage");
			return;
		}
#endif
	}

	m_parent     = parent;
	m_downloader = new downloader(parent);

	connect(m_downloader, &downloader::signal_download_error, this, [this, automatic](const QString& /*error*/)
	{
		if (!automatic)
		{
			QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
		}
	});

	connect(m_downloader, &downloader::signal_download_finished, this, [this, automatic, check_only, auto_accept](const QByteArray& data)
	{
		const bool result_json = handle_json(automatic, check_only, auto_accept, data);

		if (!result_json)
		{
			// The progress dialog is configured to stay open, so we need to close it manually if the download succeeds.
			m_downloader->close_progress_dialog();

			if (!automatic)
			{
				QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
			}
		}

		Q_EMIT signal_update_available(result_json && !m_update_message.isEmpty());
	});

	const utils::OS_version os = utils::get_OS_version();

	const std::string url = fmt::format("https://update.rpcs3.net/?api=v3&c=%s&os_type=%s&os_arch=%s&os_version=%i.%i.%i",
		rpcs3::get_commit_and_hash().second, os.type, os.arch, os.version_major, os.version_minor, os.version_patch);

	m_downloader->start(url, true, !automatic, tr("Checking For Updates"), true);
}

bool update_manager::handle_json(bool automatic, bool check_only, bool auto_accept, const QByteArray& data)
{
	update_log.notice("Download of update info finished. automatic=%d, check_only=%d, auto_accept=%d", automatic, check_only, auto_accept);

	const QJsonObject json_data = QJsonDocument::fromJson(data).object();
	const int return_code       = json_data["return_code"].toInt(-255);

	bool hash_found = true;

	if (return_code < 0)
	{
		std::string error_message;
		switch (return_code)
		{
		case -1: error_message = "Hash not found(Custom/PR build)"; break;
		case -2: error_message = "Server Error - Maintenance Mode"; break;
		case -3: error_message = "Server Error - Illegal Search"; break;
		case -255: error_message = "Server Error - Return code not found"; break;
		default: error_message = "Server Error - Unknown Error"; break;
		}

		if (return_code != -1)
			update_log.error("Update error: %s return code: %d", error_message, return_code);
		else
			update_log.warning("Update error: %s return code: %d", error_message, return_code);

		// If a user clicks "Check for Updates" with a custom build ask him if he's sure he wants to update to latest version
		if (!automatic && return_code == -1)
		{
			hash_found = false;
		}
		else
		{
			return false;
		}
	}

	const auto& current = json_data["current_build"];
	const auto& latest = json_data["latest_build"];

	if (!latest.isObject())
	{
		update_log.error("JSON doesn't contain latest_build section");
		return false;
	}

	QString os;

#ifdef _WIN32
	os = "windows";
#elif defined(__linux__)
	os = "linux";
#elif defined(__APPLE__)
	os = "mac";
#else
	update_log.error("Your OS isn't currently supported by the auto-updater");
	return false;
#endif

	// Check that every bit of info we need is there
	const auto check_json = [](bool cond, std::string_view msg) -> bool
	{
		if (cond) return true;
		update_log.error("%s", msg);
		return false;
	};
	if (!(check_json(latest[os].isObject(), fmt::format("Node 'latest_build: %s' not found", os)) &&
	      check_json(latest[os]["download"].isString(), fmt::format("Node 'latest_build: %s: download' not found or not a string", os)) &&
	      check_json(latest[os]["size"].isDouble(), fmt::format("Node 'latest_build: %s: size' not found or not a double", os)) &&
	      check_json(latest[os]["checksum"].isString(), fmt::format("Node 'latest_build: %s: checksum' not found or not a string", os)) &&
	      check_json(latest["version"].isString(), "Node 'latest_build: version' not found or not a string") &&
	      check_json(latest["datetime"].isString(), "Node 'latest_build: datetime' not found or not a string")
	     ) ||
	     (hash_found && !(
	      check_json(current.isObject(), "JSON doesn't contain current_build section") &&
	      check_json(current["version"].isString(), "Node 'current_build: datetime' not found or not a string") &&
	      check_json(current["datetime"].isString(), "Node 'current_build: version' not found or not a string")
	     )))
	{
		return false;
	}

	if (hash_found && return_code == 0)
	{
		update_log.success("RPCS3 is up to date!");
		m_downloader->close_progress_dialog();

		if (!automatic)
			QMessageBox::information(m_parent, tr("Auto-updater"), tr("Your version is already up to date!"));

		return true;
	}

	// Calculate how old the build is
	const QString date_fmt = QStringLiteral("yyyy-MM-dd hh:mm:ss");

	const QDateTime cur_date = hash_found ? QDateTime::fromString(current["datetime"].toString(), date_fmt) : QDateTime::currentDateTimeUtc();
	const QDateTime lts_date = QDateTime::fromString(latest["datetime"].toString(), date_fmt);

	const QString cur_str = cur_date.toString(date_fmt);
	const QString lts_str = lts_date.toString(date_fmt);

	const qint64 diff_msec = cur_date.msecsTo(lts_date);

	update_log.notice("Current: %s, latest: %s, difference: %lld ms", cur_str, lts_str, diff_msec);

	const Localized localized;

	const QString new_version = latest["version"].toString();
	m_new_version = new_version.toStdString();
	const QString support_message = tr("<br>You can empower our project at <a href=\"https://rpcs3.net/patreon\">RPCS3 Patreon</a>.<br>");

	if (hash_found)
	{
		const QString old_version = current["version"].toString();
		m_old_version = old_version.toStdString();

		if (diff_msec < 0)
		{
			// This usually means that the current version was marked as broken and won't be shipped anymore, so we need to downgrade to avoid certain bugs.
			m_update_message = tr("A better version of RPCS3 is available!<br><br>Current version: %0 (%1)<br>Better version: %2 (%3)<br>%4<br>Do you want to update?")
				.arg(old_version)
				.arg(cur_str)
				.arg(new_version)
				.arg(lts_str)
				.arg(support_message);
		}
		else
		{
			m_update_message = tr("A new version of RPCS3 is available!<br><br>Current version: %0 (%1)<br>Latest version: %2 (%3)<br>Your version is %4 behind.<br>%5<br>Do you want to update?")
				.arg(old_version)
				.arg(cur_str)
				.arg(new_version)
				.arg(lts_str)
				.arg(localized.GetVerboseTimeByMs(diff_msec, true))
				.arg(support_message);
		}
	}
	else
	{
		m_old_version = fmt::format("%s-%s-%s", rpcs3::get_full_branch(), rpcs3::get_branch(), rpcs3::get_version().to_string());

		m_update_message = tr("You're currently using a custom or PR build.<br><br>Latest version: %0 (%1)<br>The latest version is %2 old.<br>%3<br>Do you want to update to the latest official RPCS3 version?")
			.arg(new_version)
			.arg(lts_str)
			.arg(localized.GetVerboseTimeByMs(std::abs(diff_msec), true))
			.arg(support_message);
	}

	m_request_url   = latest[os]["download"].toString().toStdString();
	m_expected_hash = latest[os]["checksum"].toString().toStdString();
	m_expected_size = latest[os]["size"].toInt();

	if (!m_request_url.starts_with("https://github.com/RPCS3/rpcs3"))
	{
		update_log.fatal("Bad url: %s", m_request_url);
		return false;
	}

	update_log.notice("Update found: %s", m_request_url);

	if (!auto_accept)
	{
		if (automatic && m_gui_settings->GetValue(gui::ib_skip_version).toString() == new_version)
		{
			update_log.notice("Skipping automatic update notification for version '%s' due to user preference", new_version);
			m_downloader->close_progress_dialog();
			return true;
		}

		const auto& changelog = json_data["changelog"];

		if (changelog.isArray())
		{
			for (const QJsonValue& changelog_entry : changelog.toArray())
			{
				if (changelog_entry.isObject())
				{
					changelog_data entry;

					if (QJsonValue version = changelog_entry["version"]; version.isString())
					{
						entry.version = version.toString();
					}
					else
					{
						entry.version = tr("N/A");
						update_log.notice("JSON changelog entry does not contain a version string.");
					}

					if (QJsonValue title = changelog_entry["title"]; title.isString())
					{
						entry.title = title.toString();
					}
					else
					{
						entry.title = tr("N/A");
						update_log.notice("JSON changelog entry does not contain a title string.");
					}

					m_changelog.push_back(entry);
				}
				else
				{
					update_log.error("JSON changelog entry is not an object.");
				}
			}
		}
		else if (changelog.isObject())
		{
			update_log.error("JSON changelog is not an array.");
		}
		else
		{
			update_log.notice("JSON does not contain a changelog section.");
		}
	}

	if (check_only)
	{
		update_log.notice("Update postponed. Check only is active");
		m_downloader->close_progress_dialog();
		return true;
	}

	update(auto_accept);
	return true;
}

void update_manager::update(bool auto_accept)
{
	update_log.notice("Updating with auto_accept=%d", auto_accept);

	ensure(m_downloader);

	if (!auto_accept)
	{
		if (m_update_message.isEmpty())
		{
			// This can happen if we abort the check_for_updates download. Just check again in this case.
			update_log.notice("Aborting update: Update message is empty. Trying again...");
			m_downloader->close_progress_dialog();
			check_for_updates(false, false, false, m_parent);
			return;
		}

		QString changelog_content;

		for (const changelog_data& entry : m_changelog)
		{
			if (!changelog_content.isEmpty())
				changelog_content.append('\n');
			changelog_content.append(tr("• %0: %1").arg(entry.version, entry.title));
		}

		QMessageBox mb(QMessageBox::Icon::Question, tr("Update Available"), m_update_message, QMessageBox::Yes | QMessageBox::No, m_downloader->get_progress_dialog() ? m_downloader->get_progress_dialog() : m_parent);
		mb.setTextFormat(Qt::RichText);
		mb.setCheckBox(new QCheckBox(tr("Don't show again for this version")));

		if (!changelog_content.isEmpty())
		{
			mb.setInformativeText(tr("To see the changelog, please click \"Show Details\"."));
			mb.setDetailedText(tr("Changelog:\n\n%0").arg(changelog_content));

			// Smartass hack to make the unresizeable message box wide enough for the changelog
			const int changelog_width = QLabel(changelog_content).sizeHint().width();
			if (QLabel(m_update_message).sizeHint().width() < changelog_width)
			{
				m_update_message += " &nbsp;";
				while (QLabel(m_update_message).sizeHint().width() < changelog_width)
				{
					m_update_message += "&nbsp;";
				}
			}

			mb.setText(m_update_message);
		}

		update_log.notice("Asking user for permission to update...");

		if (mb.exec() == QMessageBox::No)
		{
			update_log.notice("Aborting update: User declined update");

			if (mb.checkBox()->isChecked())
			{
				update_log.notice("User requested to skip further automatic update notifications for version '%s'", m_new_version);
				m_gui_settings->SetValue(gui::ib_skip_version, QString::fromStdString(m_new_version));
			}

			m_downloader->close_progress_dialog();
			return;
		}
	}

	if (!Emu.IsStopped())
	{
		update_log.notice("Aborting update: Emulation is running...");
		m_downloader->close_progress_dialog();
		QMessageBox::warning(m_parent, tr("Auto-updater"), tr("Please stop the emulation before trying to update."));
		return;
	}

	m_downloader->disconnect();

	connect(m_downloader, &downloader::signal_download_error, this, [this](const QString& /*error*/)
	{
		QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
	});

	connect(m_downloader, &downloader::signal_download_finished, this, [this, auto_accept](const QByteArray& data)
	{
		const bool result_json = handle_rpcs3(data, auto_accept);

		if (!result_json)
		{
			// The progress dialog is configured to stay open, so we need to close it manually if the download succeeds.
			m_downloader->close_progress_dialog();

			QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
		}

		Q_EMIT signal_update_available(false);
	});

	update_log.notice("Downloading update...");
	m_downloader->start(m_request_url, true, true, tr("Downloading Update"), true, m_expected_size);
}

bool update_manager::handle_rpcs3(const QByteArray& data, bool auto_accept)
{
	update_log.notice("Download of update file finished. Updating rpcs3 with auto_accept=%d", auto_accept);

	m_downloader->update_progress_dialog(tr("Updating RPCS3"));

	if (m_expected_size != static_cast<u64>(data.size()))
	{
		update_log.error("Download size mismatch: %d expected: %d", data.size(), m_expected_size);
		return false;
	}

	if (const std::string res_hash_string = sha256_get_hash(data.data(), data.size(), false);
		m_expected_hash != res_hash_string)
	{
		update_log.error("Hash mismatch: %s expected: %s", res_hash_string, m_expected_hash);
		return false;
	}

#if defined(_WIN32) || defined(__APPLE__)

	// Get executable path
	const std::string exe_dir = fs::get_executable_dir();
	const std::string orig_path = fs::get_executable_path();
#ifdef _WIN32
	const std::wstring wchar_orig_path = utf8_to_wchar(orig_path);
	const std::string tmpfile_path = fs::get_temp_dir() + "\\rpcs3_update.7z";
#else
	const std::string tmpfile_path = fs::get_temp_dir() + "rpcs3_update.7z";
#endif

	update_log.notice("Writing temporary update file: %s", tmpfile_path);

	fs::file tmpfile(tmpfile_path, fs::read + fs::write + fs::create + fs::trunc);
	if (!tmpfile)
	{
		update_log.error("Failed to create temporary file: %s", tmpfile_path);
		return false;
	}
	if (tmpfile.write(data.data(), data.size()) != static_cast<u64>(data.size()))
	{
		update_log.error("Failed to write temporary file: %s", tmpfile_path);
		return false;
	}
	tmpfile.close();

	update_log.notice("Unpacking update file: %s", tmpfile_path);

	// 7z stuff (most of this stuff is from 7z Util sample and has been reworked to be more stl friendly)

	CFileInStream archiveStream{};
	CLookToRead2 lookStream{};
	CSzArEx db;
	UInt16 temp_u16[PATH_MAX];
	u8 temp_u8[PATH_MAX];
	const usz kInputBufSize = static_cast<usz>(1u << 18u);
	const ISzAlloc g_Alloc = {SzAlloc, SzFree};

	ISzAlloc allocImp     = g_Alloc;
	ISzAlloc allocTempImp = g_Alloc;

	const auto WRes_to_string = [](WRes res)
	{
#ifdef _WIN32
		return fmt::format("0x%x='%s'", res, std::system_category().message(HRESULT_FROM_WIN32(res)));
#else
		return fmt::format("0x%x='%s'", res, strerror(res));
#endif
	};

#ifdef _WIN32
	const std::wstring tmpfile_path_w = utf8_to_wchar(tmpfile_path);
	if (const WRes res = InFile_OpenW(&archiveStream.file, tmpfile_path_w.c_str()))
	{
		update_log.error("Failed to open temporary storage file: '%s' (error=%s)", tmpfile_path_w.c_str(), WRes_to_string(res));
		return false;
	}
#else
	if (const WRes res = InFile_Open(&archiveStream.file, tmpfile_path.c_str()))
	{
		update_log.error("Failed to open temporary storage file: '%s' (error=%s)", tmpfile_path, WRes_to_string(res));
		return false;
	}
#endif

	FileInStream_CreateVTable(&archiveStream);
	LookToRead2_CreateVTable(&lookStream, False);

	SRes res = SZ_OK;

	lookStream.buf = static_cast<Byte*>(ISzAlloc_Alloc(&allocImp, kInputBufSize));
	if (!lookStream.buf)
	{
		res = SZ_ERROR_MEM;
	}
	else
	{
		lookStream.bufSize    = kInputBufSize;
		lookStream.realStream = &archiveStream.vt;
	}

	CrcGenerateTable();
	SzArEx_Init(&db);

	const auto error_free7z = [&]()
	{
		SzArEx_Free(&db, &allocImp);
		ISzAlloc_Free(&allocImp, lookStream.buf);

		const WRes res2 = File_Close(&archiveStream.file);
		if (res2) update_log.warning("7z failed to close file (error=%s)", WRes_to_string(res2));
		if (res) update_log.error("7z decoder error: %s", WRes_to_string(res));
	};

	if (res != SZ_OK)
	{
		error_free7z();
		return false;
	}

	res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
	if (res != SZ_OK)
	{
		error_free7z();
		return false;
	}

	UInt32 blockIndex = 0xFFFFFFFF;
	Byte* outBuffer   = nullptr;
	usz outBufferSize = 0;

#ifdef _WIN32
	// Create temp folder for moving active files
	const std::string tmp_folder = exe_dir + "rpcs3_old/";
#else
	// Create temp folder for extracting the new app
	const std::string tmp_folder = fs::get_temp_dir() + "rpcs3_new/";
#endif
	fs::create_dir(tmp_folder);

	for (UInt32 i = 0; i < db.NumFiles; i++)
	{
		usz offset           = 0;
		usz outSizeProcessed = 0;
		const bool isDir     = SzArEx_IsDir(&db, i);
		[[maybe_unused]] const DWORD attribs = SzBitWithVals_Check(&db.Attribs, i) ? db.Attribs.Vals[i] : 0;
#ifdef _WIN32
		// This is commented out for now as we shouldn't need it and symlinks
		// aren't well supported on Windows. Left in case it is needed in the future.
		// const bool is_symlink = (attribs & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
		const bool is_symlink = false;
#else
		const DWORD permissions = (attribs >> 16) & (S_IRWXU | S_IRWXG | S_IRWXO);
		const bool is_symlink = (attribs & FILE_ATTRIBUTE_UNIX_EXTENSION) != 0 && S_ISLNK(attribs >> 16);
#endif
		const usz len = SzArEx_GetFileNameUtf16(&db, i, nullptr);

		if (len >= PATH_MAX)
		{
			update_log.error("7z decoder error: filename longer or equal to PATH_MAX");
			error_free7z();
			return false;
		}

		SzArEx_GetFileNameUtf16(&db, i, temp_u16);
		std::memset(temp_u8, 0, sizeof(temp_u8));
		// Simplistic conversion to UTF-8
		for (usz index = 0; index < len; index++)
		{
			if (temp_u16[index] > 0xFF)
			{
				update_log.error("7z decoder error: Failed to convert UTF-16 to UTF-8");
				error_free7z();
				return false;
			}

			temp_u8[index] = static_cast<u8>(temp_u16[index]);
		}
		temp_u8[len] = 0;
		const std::string archived_name = std::string(reinterpret_cast<char*>(temp_u8));
#ifdef __APPLE__
		const std::string name = tmp_folder + archived_name;
#else
		const std::string name = exe_dir + archived_name;
#endif

		if (!isDir)
		{
			res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
			if (res != SZ_OK)
				break;
		}

		if (const usz pos = name.find_last_of(fs::delim); pos != umax)
		{
			update_log.trace("Creating path: %s", name.substr(0, pos));
			fs::create_path(name.substr(0, pos));
		}

		if (isDir)
		{
			update_log.trace("Creating dir: %s", name);
			fs::create_dir(name);
			continue;
		}

		if (is_symlink)
		{
			const std::string link_target(reinterpret_cast<const char*>(outBuffer + offset), outSizeProcessed);
			update_log.trace("Creating symbolic link: %s -> %s", name, link_target);
			fs::create_symlink(name, link_target);
			continue;
		}

		fs::file outfile(name, fs::read + fs::write + fs::create + fs::trunc);
		if (!outfile)
		{
			// File failed to open, probably because in use, rename existing file and try again
			const auto pos = name.find_last_of(fs::delim);
			std::string filename;
			if (pos == umax)
				filename = name;
			else
				filename = name.substr(pos + 1);

			// Moving to temp is not an option on windows as it will fail if the disk is different
			// So we create a folder in config dir and move stuff there
			const std::string rename_target = tmp_folder + filename;
			update_log.trace("Renaming %s to %s", name, rename_target);
			if (!fs::rename(name, rename_target, true))
			{
				update_log.error("Failed to rename %s to %s", name, rename_target);
				res = SZ_ERROR_FAIL;
				break;
			}

			outfile.open(name, fs::read + fs::write + fs::create + fs::trunc);
			if (!outfile)
			{
				update_log.error("Can not open output file %s", name);
				res = SZ_ERROR_FAIL;
				break;
			}
		}

		if (outfile.write(outBuffer + offset, outSizeProcessed) != outSizeProcessed)
		{
			update_log.error("Can not write output file: %s", name);
			res = SZ_ERROR_FAIL;
			break;
		}
		outfile.close();

#ifndef _WIN32
		// Apply correct file permissions.
		chmod(name.c_str(), permissions);
#endif
	}

	error_free7z();
	if (res)
		return false;

	update_log.success("Update successful!");

#else

	std::string replace_path = fs::get_executable_path();
	if (replace_path.empty())
	{
		return false;
	}

	// Move the appimage/exe and replace with new appimage
	const std::string move_dest = replace_path + "_old";
	if (!fs::rename(replace_path, move_dest, true))
	{
		// Simply log error for now
		update_log.error("Failed to move old AppImage file: %s (%s)", replace_path, fs::g_tls_error);
	}
	fs::file new_appimage(replace_path, fs::read + fs::write + fs::create + fs::trunc);
	if (!new_appimage)
	{
		update_log.error("Failed to create new AppImage file: %s (%s)", replace_path, fs::g_tls_error);
		return false;
	}
	if (new_appimage.write(data.data(), data.size()) != data.size() + 0ull)
	{
		update_log.error("Failed to write new AppImage file: %s", replace_path);
		return false;
	}
	if (fchmod(new_appimage.get_handle(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
	{
		update_log.error("Failed to chmod rwxrxrx %s (%s)", replace_path, strerror(errno));
		return false;
	}
	new_appimage.close();

	update_log.success("Successfully updated %s!", replace_path);

#endif

	m_downloader->close_progress_dialog();

	// Add new version to log file
	if (fs::file update_file{fs::get_config_dir() + "update_history.log", fs::create + fs::write + fs::append})
	{
		const std::string update_time = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss").toStdString();
		const std::string entry = fmt::format("%s: Updated from \"%s\" to \"%s\"", update_time, m_old_version, m_new_version);
		update_file.write(fmt::format("%s\n", entry));
		update_log.notice("Added entry '%s' to update_history.log", entry);
	}
	else
	{
		update_log.error("Failed to append version to update_history.log");
	}

	if (!auto_accept)
	{
		m_gui_settings->ShowInfoBox(tr("Auto-updater"), tr("Update successful!<br>RPCS3 will now restart.<br>"), gui::ib_restart_hint, m_parent);
		m_gui_settings->sync(); // Make sure to sync before terminating RPCS3
	}

	Emu.GracefulShutdown(false);
	Emu.CleanUp();

#ifdef _WIN32
	update_log.notice("Relaunching %s with _wexecl", wchar_to_utf8(wchar_orig_path));
	const int ret = _wexecl(wchar_orig_path.data(), wchar_orig_path.data(), L"--updating", nullptr);
#elif defined(__APPLE__)
	// Execute helper script to replace the app and relaunch
	const std::string helper_script = fmt::format("%s/Contents/Resources/update_helper.sh", orig_path);
	const std::string extracted_app = fmt::format("%s/RPCS3.app", tmp_folder);
	update_log.notice("Executing update helper script: '%s %s %s'", helper_script, extracted_app, orig_path);
	const int ret = execl(helper_script.c_str(), helper_script.c_str(), extracted_app.c_str(), orig_path.c_str(), nullptr);
#else
	// execv is used for compatibility with checkrt
	update_log.notice("Relaunching %s with execv", replace_path);
	const char * const params[3] = { replace_path.c_str(), "--updating", nullptr };
	const int ret = execv(replace_path.c_str(), const_cast<char * const *>(&params[0]));
#endif
	if (ret == -1)
	{
		update_log.error("Relaunching failed with result: %d(%s)", ret, strerror(errno));
		return false;
	}

	return true;
}
