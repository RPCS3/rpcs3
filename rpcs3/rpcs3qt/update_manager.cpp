#include "stdafx.h"
#include "update_manager.h"
#include "progress_dialog.h"
#include "rpcs3_version.h"
#include "Utilities/StrUtil.h"
#include "Crypto/sha256.h"
#include "Emu/System.h"

#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>

#include <sstream>
#include <iomanip>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <CpuArch.h>
#include <7z.h>
#include <7zAlloc.h>
#include <7zBuf.h>
#include <7zCrc.h>
#include <7zFile.h>
#include <7zVersion.h>

#define PATH_MAX MAX_PATH

#else
#include <unistd.h>
#include <sys/stat.h>
#endif

LOG_CHANNEL(update_log, "UPDATER");

size_t curl_write_cb(char* ptr, size_t /*size*/, size_t nmemb, void* userdata)
{
	update_manager* upd_mgr = reinterpret_cast<update_manager*>(userdata);
	return upd_mgr->update_buffer(ptr, nmemb);
}

update_manager::update_manager()
{
	m_curl = curl_easy_init();

#ifdef _WIN32
	// This shouldn't be needed on linux
	curl_easy_setopt(m_curl, CURLOPT_CAINFO, "cacert.pem");
#endif
}

size_t update_manager::update_buffer(char* data, size_t size)
{
	if (m_curl_abort)
	{
		return 0;
	}

	const auto old_size = m_curl_buf.size();
	const auto new_size = old_size + size;
	m_curl_buf.resize(static_cast<int>(new_size));
	memcpy(m_curl_buf.data() + old_size, data, size);

	if (m_progress_dialog && m_update_dialog)
	{
		m_progress_dialog->setValue(static_cast<int>(new_size));
	}

	return size;
}

void update_manager::check_for_updates(bool automatic, QWidget* parent)
{
#ifdef __linux__
	if (automatic && !::getenv("APPIMAGE"))
	{
		// Don't check for updates on startup if RPCS3 is not running from an AppImage.
		return;
	}
#endif

	m_parent        = parent;
	m_curl_abort    = false;
	m_update_dialog = false;
	m_curl_buf.clear();

	m_progress_dialog = new progress_dialog(tr("Checking For Updates"), tr("Please wait..."), tr("Abort"), 0, 100, true, parent);
	m_progress_dialog->setAutoClose(false);
	m_progress_dialog->setAutoReset(false);
	m_progress_dialog->show();

	connect(m_progress_dialog, &QProgressDialog::canceled, [&]() { m_curl_abort = true; });
	connect(m_progress_dialog, &QProgressDialog::finished, m_progress_dialog, &QProgressDialog::deleteLater);

	const std::string request_url = m_update_url + rpcs3::get_commit_and_hash().second;
	curl_easy_setopt(m_curl, CURLOPT_URL, request_url.c_str());
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

	bool result_json = false;

	if (const auto curl_result = curl_easy_perform(m_curl); curl_result != CURLE_OK)
	{
		update_log.error("Curl error(query): %s", curl_easy_strerror(curl_result));
	}
	else
	{
		result_json = handle_json(automatic);
	}

	if (!result_json && !m_curl_abort)
	{
		if (!automatic)
		{
			QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
		}

		if (m_progress_dialog)
		{
			m_progress_dialog->close();
			m_progress_dialog = nullptr;
		}
	}
}

bool update_manager::handle_json(bool automatic)
{
	const QJsonObject json_data = QJsonDocument::fromJson(m_curl_buf).object();
	const int return_code             = json_data["return_code"].toInt(-255);

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
			if (QMessageBox::question(m_progress_dialog, tr("Auto-updater"), tr("You're currently using a custom or PR build.\n\nDo you want to update to the latest official RPCS3 version?"),
			        QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			{
				m_progress_dialog->close();
				return true;
			}

			hash_found = false;
		}
		else
		{
			return false;
		}
	}

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
	    (hash_found && (!json_data["current_build"].isObject() || !json_data["current_build"]["version"].isString() || !json_data["current_build"]["datetime"].isString())))
	{
		update_log.error("Some information seems unavailable");
		return false;
	}

	if (hash_found && return_code == 0)
	{
		update_log.success("RPCS3 is up to date!");
		m_progress_dialog->close();

		if (!automatic)
			QMessageBox::information(m_parent, tr("Auto-updater"), tr("Your version is already up to date!"));

		return true;
	}

	if (hash_found)
	{
		// Calculates how old the build is
		std::string cur_date = json_data["current_build"]["datetime"].toString().toStdString();
		std::string lts_date = latest["datetime"].toString().toStdString();

		tm cur_tm, lts_tm;
		QString timediff = "";

		auto time_from_str = [](const std::string& str, const std::string& format, tm* tm) -> bool
		{
			update_log.notice("Converting string: %s", str);
			std::istringstream input(str);
			input.imbue(std::locale(setlocale(LC_TIME, "C")));
			input >> std::get_time(tm, format.c_str());

			return !input.fail();
		};

		if (time_from_str(cur_date, "%Y-%m-%d %H:%M:%S", &cur_tm) && time_from_str(lts_date, "%Y-%m-%d %H:%M:%S", &lts_tm))
		{
			time_t cur_time = mktime(&cur_tm);
			time_t lts_time = mktime(&lts_tm);

			s64 u_timediff = static_cast<s64>(std::difftime(lts_time, cur_time));
			update_log.notice("Current: %lld, latest: %lld, difference: %lld", static_cast<s64>(cur_time), static_cast<s64>(lts_time), u_timediff);

			timediff = tr("Your version is %1 day(s), %2 hour(s) and %3 minute(s) old.").arg(u_timediff / (60 * 60 * 24)).arg((u_timediff / (60 * 60)) % 24).arg((u_timediff / 60) % 60);
		}

		const QString to_show = tr("A new version of RPCS3 is available!\n\nCurrent version: %1\nLatest version: %2\n%3\n\nDo you want to update?")
		                            .arg(json_data["current_build"]["version"].toString())
		                            .arg(latest["version"].toString())
		                            .arg(timediff);
		if (QMessageBox::question(m_progress_dialog, tr("Update Available"), to_show, QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		{
			m_progress_dialog->close();
			return true;
		}
	}

	m_expected_hash = latest[os]["checksum"].toString().toStdString();
	m_expected_size = latest[os]["size"].toInt();

	m_progress_dialog->setWindowTitle(tr("Downloading Update"));

	// Download RPCS3
	m_progress_dialog->setMaximum(m_expected_size);
	m_progress_dialog->setValue(0);
	m_update_dialog = true;

	const std::string request_url = latest[os]["download"].toString().toStdString();
	curl_easy_setopt(m_curl, CURLOPT_URL, request_url.c_str());
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);

	m_curl_buf.clear();

	QThread::create([this, automatic]
	{
		bool result_rpcs3 = false;

		if (const auto curl_result = curl_easy_perform(m_curl); curl_result != CURLE_OK)
		{
			update_log.error("Curl error(download): %s", curl_easy_strerror(curl_result));
		}
		else
		{
			result_rpcs3 = handle_rpcs3();
		}
		
		if (!result_rpcs3 && !m_curl_abort)
		{
			if (!automatic)
			{
				QMessageBox::warning(m_parent, tr("Auto-updater"), tr("An error occurred during the auto-updating process.\nCheck the log for more information."));
			}

			if (m_progress_dialog)
			{
				m_progress_dialog->close();
				m_progress_dialog = nullptr;
			}
		}
	})->start();

	return true;
}

bool update_manager::handle_rpcs3()
{
	if (m_expected_size != m_curl_buf.size() + 0u)
	{
		update_log.error("Download size mismatch: %d expected: %d", m_curl_buf.size(), m_expected_size);
		return false;
	}

	u8 res_hash[32];
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts_ret(&ctx, 0);
	mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const unsigned char*>(m_curl_buf.data()), m_curl_buf.size());
	mbedtls_sha256_finish_ret(&ctx, res_hash);

	std::string res_hash_string("0000000000000000000000000000000000000000000000000000000000000000");
	for (size_t index = 0; index < 32; index++)
	{
		constexpr auto pal               = "0123456789ABCDEF";
		res_hash_string[index * 2]       = pal[res_hash[index] >> 4];
		res_hash_string[(index * 2) + 1] = pal[res_hash[index] & 15];
	}

	if (m_expected_hash != res_hash_string)
	{
		update_log.error("Hash mismatch: %s expected: %s", res_hash_string, m_expected_hash);
		return false;
	}

	std::string replace_path;

#ifdef _WIN32
	// Get executable path
	wchar_t orig_path[32767];
	GetModuleFileNameW(nullptr, orig_path, sizeof(orig_path) / 2);
#endif

#ifdef __linux__

	const char* appimage_path = ::getenv("APPIMAGE");
	if (appimage_path != nullptr)
	{
		replace_path = appimage_path;
		update_log.notice("Found AppImage path: %s", appimage_path);
	}
	else
	{
		update_log.warning("Failed to find AppImage path");
		char exe_path[PATH_MAX];
		ssize_t len = ::readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);

		if (len == -1)
		{
			update_log.error("Failed to find executable path");
			return false;
		}

		exe_path[len] = '\0';
		update_log.trace("Found exec path: %s", exe_path);

		replace_path = exe_path;
	}

	m_progress_dialog->setWindowTitle(tr("Updating RPCS3"));

	// Move the appimage/exe and replace with new appimage
	std::string move_dest = replace_path + "_old";
	fs::rename(replace_path, move_dest, true);
	fs::file new_appimage(replace_path, fs::read + fs::write + fs::create + fs::trunc);
	if (!new_appimage)
	{
		update_log.error("Failed to create new AppImage file: %s", replace_path);
		return false;
	}
	if (new_appimage.write(m_curl_buf.data(), m_curl_buf.size()) != m_curl_buf.size() + 0u)
	{
		update_log.error("Failed to write new AppImage file: %s", replace_path);
		return false;
	}
	if (fchmod(new_appimage.get_handle(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1)
	{
		update_log.error("Failed to chmod rwxrxrx %s", replace_path);
		return false;
	}
	new_appimage.close();

	update_log.success("Successfully updated %s!", replace_path);

#elif defined(_WIN32)

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
	if (tmpfile.write(m_curl_buf.data(), m_curl_buf.size()) != m_curl_buf.size())
	{
		update_log.error("Failed to write temporary file: %s", tmpfile_path);
		return false;
	}
	tmpfile.close();

	m_progress_dialog->setWindowTitle(tr("Updating RPCS3"));

	// 7z stuff (most of this stuff is from 7z Util sample and has been reworked to be more stl friendly)

	ISzAlloc allocImp;
	ISzAlloc allocTempImp;

	CFileInStream archiveStream;
	CLookToRead2 lookStream;
	CSzArEx db;
	SRes res;
	UInt16 temp_u16[PATH_MAX];
	u8 temp_u8[PATH_MAX];
	const size_t kInputBufSize = static_cast<size_t>(1u << 18u);
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
		lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, kInputBufSize);
		if (!lookStream.buf)
			res = SZ_ERROR_MEM;
		else
		{
			lookStream.bufSize    = kInputBufSize;
			lookStream.realStream = &archiveStream.vt;
			LookToRead2_Init(&lookStream);
		}
	}

	CrcGenerateTable();
	SzArEx_Init(&db);

	auto error_free7z = [&]() {
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
	size_t outBufferSize = 0;

	// Creates temp folder for moving active files
	std::string tmp_folder = Emulator::GetEmuDir() + m_tmp_folder;
	fs::create_dir(tmp_folder);

	for (UInt32 i = 0; i < db.NumFiles; i++)
	{
		size_t offset           = 0;
		size_t outSizeProcessed = 0;
		size_t len;
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
		for (size_t index = 0; index < len; index++)
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
		std::string name((char*)temp_u8);

		name = Emulator::GetEmuDir() + name;

		if (!isDir)
		{
			res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp, &allocTempImp);
			if (res != SZ_OK)
				break;
		}

		if (size_t pos = name.find_last_of('/'); pos != umax)
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
#endif

	m_progress_dialog->close();
	QMessageBox::information(m_parent, tr("Auto-updater"), tr("Update successful!"));

#ifdef _WIN32
	int ret = _wexecl(orig_path, orig_path, nullptr);
#else
	int ret = execl(replace_path.c_str(), replace_path.c_str(), nullptr);
#endif
	if (ret == -1)
	{
		update_log.error("Relaunching failed with result: %d(%s)", ret, strerror(errno));
		return false;
	}

	return true;
}
