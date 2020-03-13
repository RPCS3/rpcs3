#include "game_compatibility.h"
#include "gui_settings.h"
#include "progress_dialog.h"

#include <QMessageBox>
#include <QJsonDocument>
#include <QThread>

LOG_CHANNEL(compat_log, "Compat");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

size_t curl_write_cb_compat(char* ptr, size_t /*size*/, size_t nmemb, void* userdata)
{
	game_compatibility* gm_cmp = reinterpret_cast<game_compatibility*>(userdata);
	return gm_cmp->update_buffer(ptr, nmemb);
}

game_compatibility::game_compatibility(std::shared_ptr<gui_settings> settings) : m_xgui_settings(settings)
{
	m_filepath = m_xgui_settings->GetSettingsDir() + "/compat_database.dat";
	m_url = "https://rpcs3.net/compatibility?api=v1&export";

	m_curl = curl_easy_init();

	RequestCompatibility();
}

size_t game_compatibility::update_buffer(char* data, size_t size)
{
	if (m_curl_abort)
	{
		return 0;
	}

	const auto old_size = m_curl_buf.size();
	const auto new_size = old_size + size;
	m_curl_buf.resize(static_cast<int>(new_size));
	memcpy(m_curl_buf.data() + old_size, data, size);

	if (m_actual_dwnld_size < 0)
	{
		if (curl_easy_getinfo(m_curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &m_actual_dwnld_size) == CURLE_OK && m_actual_dwnld_size > 0)
		{
			if (m_progress_dialog)
				m_progress_dialog->setMaximum(static_cast<int>(m_actual_dwnld_size));
		}
	}

	if (m_progress_dialog)
		m_progress_dialog->setValue(static_cast<int>(new_size));
	
	return size;
}

bool game_compatibility::ReadJSON(const QJsonObject& json_data, bool after_download)
{
	int return_code = json_data["return_code"].toInt();

	if (return_code < 0)
	{
		if (after_download)
		{
			std::string error_message;
			switch (return_code)
			{
			case -1:
				error_message = "Server Error - Internal Error";
				break;
			case -2:
				error_message = "Server Error - Maintenance Mode";
				break;
			default:
				error_message = "Server Error - Unknown Error";
				break;
			}
			compat_log.error("%s: return code %d", error_message, return_code);
			Q_EMIT DownloadError(qstr(error_message) + " " + QString::number(return_code));
		}
		else
		{
			compat_log.error("Database Error - Invalid: return code %d", return_code);
		}
		return false;
	}

	if (!json_data["results"].isObject())
	{
		compat_log.error("Database Error - No Results found");
		return false;
	}

	m_compat_database.clear();

	QJsonObject json_results = json_data["results"].toObject();

	// Retrieve status data for every valid entry
	for (const auto& key : json_results.keys())
	{
		if (!json_results[key].isObject())
		{
			compat_log.error("Database Error - Unusable object %s", sstr(key));
			continue;
		}

		QJsonObject json_result = json_results[key].toObject();

		// Retrieve compatibility information from json
		compat_status status = Status_Data.at(json_result.value("status").toString("NoResult"));

		// Add date if possible
		status.date = json_result.value("date").toString();

		// Add latest version if possible
		status.latest_version = json_result.value("update").toString();

		// Add status to map
		m_compat_database.emplace(std::pair<std::string, compat_status>(sstr(key), status));
	}

	return true;
}

void game_compatibility::RequestCompatibility(bool online)
{
	if (!online)
	{
		// Retrieve database from file
		QFile file(m_filepath);

		if (!file.exists())
		{
			compat_log.notice("Database file not found: %s", sstr(m_filepath));
			return;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			compat_log.error("Database Error - Could not read database from file: %s", sstr(m_filepath));
			return;
		}

		QByteArray data = file.readAll();
		file.close();

		compat_log.notice("Finished reading database from file: %s", sstr(m_filepath));

		// Create new map from database
		ReadJSON(QJsonDocument::fromJson(data).object(), online);

		return;
	}

	compat_log.notice("Beginning compatibility database download from: %s", m_url);

	// Show Progress
	m_progress_dialog = new progress_dialog(tr("Downloading Database"), tr("Please wait ..."), tr("Abort"), 0, 100, true);
	m_progress_dialog->show();

	curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, curl_write_cb_compat);
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);

	m_curl_buf.clear();
	m_curl_abort = false;

	// Handle abort
	connect(m_progress_dialog, &QProgressDialog::canceled, [this] { m_curl_abort = true; });
	connect(m_progress_dialog, &QProgressDialog::finished, m_progress_dialog, &QProgressDialog::deleteLater);

	QThread::create([&]
	{
		const auto result = curl_easy_perform(m_curl);

		if (m_progress_dialog)
		{
			m_progress_dialog->close();
			m_progress_dialog = nullptr;
		}

		if (result != CURLE_OK)
		{
			Q_EMIT DownloadError(qstr("Curl error: ") + qstr(curl_easy_strerror(result)));
			return;
		}

		compat_log.notice("Database download finished");

		// Create new map from database and write database to file if database was valid
		if (ReadJSON(QJsonDocument::fromJson(m_curl_buf).object(), online))
		{
			// We have a new database in map, therefore refresh gamelist to new state
			Q_EMIT DownloadFinished();

			// Write database to file
			QFile file(m_filepath);

			if (file.exists())
			{
				compat_log.notice("Database file found: %s", sstr(m_filepath));
			}

			if (!file.open(QIODevice::WriteOnly))
			{
				compat_log.error("Database Error - Could not write database to file: %s", sstr(m_filepath));
				return;
			}

			file.write(m_curl_buf);
			file.close();

			compat_log.success("Wrote database to file: %s", sstr(m_filepath));
		}
	})->start();

	// We want to retrieve a new database, therefore refresh gamelist and indicate that
	Q_EMIT DownloadStarted();
}

compat_status game_compatibility::GetCompatibility(const std::string& title_id)
{
	if (m_compat_database.empty())
	{
		return Status_Data.at("NoData");
	}
	else if (m_compat_database.count(title_id) > 0)
	{
		return m_compat_database[title_id];
	}

	return Status_Data.at("NoResult");
}

compat_status game_compatibility::GetStatusData(const QString& status)
{
	return Status_Data.at(status);
}
