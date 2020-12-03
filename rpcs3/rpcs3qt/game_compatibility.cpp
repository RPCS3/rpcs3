#include "game_compatibility.h"
#include "gui_settings.h"
#include "downloader.h"

#include <QApplication>
#include <QMessageBox>
#include <QJsonDocument>

LOG_CHANNEL(compat_log, "Compat");

constexpr auto qstr = QString::fromStdString;
inline std::string sstr(const QString& _in) { return _in.toStdString(); }

game_compatibility::game_compatibility(std::shared_ptr<gui_settings> settings, QWidget* parent)
	: QObject(parent)
	, m_gui_settings(settings)
{
	m_filepath = m_gui_settings->GetSettingsDir() + "/compat_database.dat";
	m_downloader = new downloader(parent);
	RequestCompatibility();

	connect(m_downloader, &downloader::signal_download_error, this, &game_compatibility::handle_download_error);
	connect(m_downloader, &downloader::signal_download_finished, this, &game_compatibility::handle_download_finished);
}

void game_compatibility::handle_download_error(const QString& error)
{
	Q_EMIT DownloadError(error);
}

void game_compatibility::handle_download_finished(const QByteArray& data)
{
	compat_log.notice("Database download finished");

	// Create new map from database and write database to file if database was valid
	if (ReadJSON(QJsonDocument::fromJson(data).object(), true))
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

		file.write(data);
		file.close();

		compat_log.success("Wrote database to file: %s", sstr(m_filepath));
	}
}

bool game_compatibility::ReadJSON(const QJsonObject& json_data, bool after_download)
{
	const int return_code = json_data["return_code"].toInt();

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

	const std::string url = "https://rpcs3.net/compatibility?api=v1&export";
	compat_log.notice("Beginning compatibility database download from: %s", url);

	m_downloader->start(url, true, true, tr("Downloading Database"));

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
