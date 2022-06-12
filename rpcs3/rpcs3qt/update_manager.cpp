#include "update_manager.h"
#include "progress_dialog.h"
#include "localized.h"
#include "rpcs3_version.h"
#include "downloader.h"
#include "Utilities/StrUtil.h"
#include "Utilities/File.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Crypto/utils.h"
#include "util/logs.hpp"

#include <QApplication>
#include <QDateTime>
#include <QMessageBox>
#include <QLabel>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <CpuArch.h>
#include <7z.h>
#include <7zAlloc.h>
#include <7zCrc.h>
#include <7zFile.h>

#define PATH_MAX MAX_PATH

#else
#include <unistd.h>
#include <sys/stat.h>
#endif

LOG_CHANNEL(update_log, "UPDATER");

void update_manager::check_for_updates(bool automatic, bool check_only, bool auto_accept, QWidget* parent)
{
	m_update_message.clear();
	m_changelog.clear();

#ifdef __linux__
	if (automatic && !::getenv("APPIMAGE"))
	{
		// Don't check for updates on startup if RPCS3 is not running from an AppImage.
		return;
	}
#endif

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

	const std::string url = "https://update.rpcs3.net/?api=v2&c=" + rpcs3::get_commit_and_hash().second;
	m_downloader->start(url, true, !automatic, tr("Checking For Updates"), true);
}

bool update_manager::handle_json(bool automatic, bool check_only, bool auto_accept, const QByteArray& data)
{
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
#else
	update_log.error("Your OS isn't currently supported by the auto-updater");
	return false;
#endif

	// Check that every bit of info we need is there
	if (!latest[os].isObject() || !latest[os]["download"].isString() || !latest[os]["size"].isDouble() || !latest[os]["checksum"].isString() || !latest["version"].isString() ||
	    !latest["datetime"].isString() ||
	    (hash_found && (!current.isObject() || !current["version"].isString() || !current["datetime"].isString())))
	{
		update_log.error("Some information seems unavailable");
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

	update_log.notice("Current: %s, latest: %s, difference: %lld ms", cur_str.toStdString(), lts_str.toStdString(), diff_msec);

	const Localized localized;

	m_new_version = latest["version"].toString().toStdString();

	if (hash_found)
	{
		m_old_version = current["version"].toString().toStdString();

		if (diff_msec < 0)
		{
			// This usually means that the current version was marked as broken and won't be shipped anymore, so we need to downgrade to avoid certain bugs.
			m_update_message = tr("A better version of RPCS3 is available!\n\nCurrent version: %0 (%1)\nBetter version: %2 (%3)\n\nDo you want to update?")
				.arg(current["version"].toString())
				.arg(cur_str)
				.arg(latest["version"].toString())
				.arg(lts_str);
		}
		else
		{
			m_update_message = tr("A new version of RPCS3 is available!\n\nCurrent version: %0 (%1)\nLatest version: %2 (%3)\nYour version is %4 behind.\n\nDo you want to update?")
				.arg(current["version"].toString())
				.arg(cur_str)
				.arg(latest["version"].toString())
				.arg(lts_str)
				.arg(localized.GetVerboseTimeByMs(diff_msec, true));
		}
	}
	else
	{
		m_old_version = fmt::format("%s-%s-%s", rpcs3::get_full_branch(), rpcs3::get_branch(), rpcs3::get_version().to_string());

		m_update_message = tr("You're currently using a custom or PR build.\n\nLatest version: %0 (%1)\nThe latest version is %2 old.\n\nDo you want to update to the latest official RPCS3 version?")
			.arg(latest["version"].toString())
			.arg(lts_str)
			.arg(localized.GetVerboseTimeByMs(std::abs(diff_msec), true));
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
		m_downloader->close_progress_dialog();
		return true;
	}

	update(auto_accept);
	return true;
}

void update_manager::update(bool auto_accept)
{
	ensure(m_downloader);

	if (!auto_accept)
	{
		if (m_update_message.isEmpty())
		{
			m_downloader->close_progress_dialog();
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

		if (!changelog_content.isEmpty())
		{
			mb.setInformativeText(tr("To see the changelog, please click \"Show Details\"."));
			mb.setDetailedText(tr("Changelog:\n\n%0").arg(changelog_content));

			// Smartass hack to make the unresizeable message box wide enough for the changelog
			const int changelog_width = QLabel(changelog_content).sizeHint().width();
			while (QLabel(m_update_message).sizeHint().width() < changelog_width)
			{
				m_update_message += "          ";
			}
			mb.setText(m_update_message);
		}

		if (mb.exec() == QMessageBox::No)
		{
			m_downloader->close_progress_dialog();
			return;
		}
	}

	if (!Emu.IsStopped())
	{
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

	m_downloader->start(m_request_url, true, true, tr("Downloading Update"), true, m_expected_size);
}

bool update_manager::handle_rpcs3(const QByteArray& data, bool auto_accept)
{
	m_downloader->update_progress_dialog(tr("Updating RPCS3"));

#ifdef __APPLE__
	update_log.error("Unsupported operating system.");
	return false;
#else

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

#ifdef _WIN32

	// Get executable path
	const std::string orig_path = rpcs3::utils::get_exe_dir() + "rpcs3.exe";

	std::wstring wchar_orig_path;
	const auto tmp_size = MultiByteToWideChar(CP_UTF8, 0, orig_path.c_str(), -1, nullptr, 0);
	wchar_orig_path.resize(tmp_size);
	MultiByteToWideChar(CP_UTF8, 0, orig_path.c_str(), -1, wchar_orig_path.data(), tmp_size);

	char temp_path[PATH_MAX];

	GetTempPathA(sizeof(temp_path) - 1, temp_path);
	temp_path[PATH_MAX - 1] = 0;

	std::string tmpfile_path = temp_path;
	tmpfile_path += "\\rpcs3_update.7z";

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

	// 7z stuff (most of this stuff is from 7z Util sample and has been reworked to be more stl friendly)

	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	CFileInStream archiveStream;
	CLookToRead2 lookStream;
	CSzArEx db;
	SRes res;
	UInt16 temp_u16[PATH_MAX];
	u8 temp_u8[PATH_MAX];
	const usz kInputBufSize = static_cast<usz>(1u << 18u);
	const ISzAlloc g_Alloc     = {SzAlloc, SzFree};

	allocImp     = g_Alloc;
	allocTempImp = g_Alloc;

	if (InFile_Open(&archiveStream.file, tmpfile_path.c_str()))
	{
		update_log.error("Failed to open temporary storage file: %s", tmpfile_path);
		return false;
	}

	FileInStream_CreateVTable(&archiveStream);
	LookToRead2_CreateVTable(&lookStream, False);
	lookStream.buf = nullptr;

	res = SZ_OK;
	{
		lookStream.buf = static_cast<Byte*>(ISzAlloc_Alloc(&allocImp, kInputBufSize));
		if (!lookStream.buf)
			res = SZ_ERROR_MEM;
		else
		{
			lookStream.bufSize    = kInputBufSize;
			lookStream.realStream = &archiveStream.vt;
			LookToRead2_Init(&lookStream)
		}
	}

	CrcGenerateTable();
	SzArEx_Init(&db);

	auto error_free7z = [&]()
	{
		SzArEx_Free(&db, &allocImp);
		ISzAlloc_Free(&allocImp, lookStream.buf);

		File_Close(&archiveStream.file);

		switch (res)
		{
		case SZ_OK: break;
		case SZ_ERROR_UNSUPPORTED: update_log.error("7z decoder doesn't support this archive"); break;
		case SZ_ERROR_MEM: update_log.error("7z decoder failed to allocate memory"); break;
		case SZ_ERROR_CRC: update_log.error("7z decoder CRC error"); break;
		default: update_log.error("7z decoder error: %d", static_cast<u64>(res)); break;
		}
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

	UInt32 blockIndex    = 0xFFFFFFFF;
	Byte* outBuffer      = nullptr;
	usz outBufferSize = 0;

	// Creates temp folder for moving active files
	const std::string tmp_folder = rpcs3::utils::get_emu_dir() + "rpcs3_old/";
	fs::create_dir(tmp_folder);

	for (UInt32 i = 0; i < db.NumFiles; i++)
	{
		usz offset           = 0;
		usz outSizeProcessed = 0;
		usz len;
		unsigned isDir = SzArEx_IsDir(&db, i);
		len            = SzArEx_GetFileNameUtf16(&db, i, nullptr);

		if (len >= PATH_MAX)
		{
			update_log.error("7z decoder error: filename longer or equal to PATH_MAX");
			error_free7z();
			return false;
		}

		SzArEx_GetFileNameUtf16(&db, i, temp_u16);
		memset(temp_u8, 0, sizeof(temp_u8));
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
		const std::string name = rpcs3::utils::get_emu_dir() + std::string(reinterpret_cast<char*>(temp_u8));

		if (!isDir)
		{
			res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
			if (res != SZ_OK)
				break;
		}

		if (const usz pos = name.find_last_of('/'); pos != umax)
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

		fs::file outfile(name, fs::read + fs::write + fs::create + fs::trunc);
		if (!outfile)
		{
			// File failed to open, probably because in use, rename existing file and try again
			const auto pos = name.find_last_of('/');
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
	}

	error_free7z();
	if (res)
		return false;

#else

	std::string replace_path = rpcs3::utils::get_executable_path();
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
	if (new_appimage.write(data.data(), data.size()) != data.size() + 0u)
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
		QMessageBox::information(m_parent, tr("Auto-updater"), tr("Update successful!\nRPCS3 will now restart."));
	}

	Emu.GracefulShutdown(false);
	Emu.CleanUp();

#ifdef _WIN32
	const int ret = _wexecl(wchar_orig_path.data(), wchar_orig_path.data(), L"--updating", nullptr);
#else
	// execv is used for compatibility with checkrt
	const char * const params[3] = { replace_path.c_str(), "--updating", nullptr };
	const int ret = execv(replace_path.c_str(), const_cast<char * const *>(&params[0]));
#endif
	if (ret == -1)
	{
		update_log.error("Relaunching failed with result: %d(%s)", ret, strerror(errno));
		return false;
	}

	return true;
#endif //def __APPLE__
}
