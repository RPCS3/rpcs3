#include "stdafx.h"
#include "config_database.h"
#include "gui_settings.h"
#include "downloader.h"
#include "Emu/system_config.h"

LOG_CHANNEL(gui_log, "GUI");

config_database::config_database(QWidget* parent)
	: QObject(parent)
{
	m_filepath = gui_settings::GetSettingsDir() + "config_database.dat";
	request_config_database();

	if (parent)
	{
		m_downloader = new downloader(parent);
		connect(m_downloader, &downloader::signal_download_error, this, &config_database::handle_download_error);
		connect(m_downloader, &downloader::signal_download_finished, this, &config_database::handle_download_finished);
		connect(m_downloader, &downloader::signal_download_canceled, this, &config_database::handle_download_canceled);
	}
}

config_database::~config_database()
{
}

bool config_database::has_config(const std::string& title_id) const
{
	return m_config_database.contains(title_id);
}

std::optional<std::string> config_database::get_config(const std::string& title_id)
{
	if (!m_config_database.contains(title_id))
	{
		gui_log.error("Config database does not contain '%s'", title_id);
		return std::nullopt;
	}

	QFile file(m_filepath);

	if (!file.exists())
	{
		gui_log.error("Config database file not found: %s", m_filepath);
		return std::nullopt;
	}

	if (!file.open(QIODevice::ReadOnly))
	{
		gui_log.error("Config database error - Could not read database from file: %s", m_filepath);
		return std::nullopt;
	}

	const QByteArray content = file.readAll();
	file.close();

	return read_json(content, false, title_id);
}

void config_database::request_config_database(bool online)
{
	if (!online)
	{
		// Retrieve database from file
		QFile file(m_filepath);

		if (!file.exists())
		{
			gui_log.notice("Config database file not found: %s", m_filepath);
			return;
		}

		if (!file.open(QIODevice::ReadOnly))
		{
			gui_log.error("Config database error - Could not read database from file: %s", m_filepath);
			return;
		}

		const QByteArray content = file.readAll();
		file.close();

		gui_log.notice("Finished reading config database from file: %s", m_filepath);

		// Create new set from database
		read_json(content, online);

		return;
	}

	const std::string url = "https://api.rpcs3.net/config/?api=v1";
	gui_log.notice("Beginning config database download from: %s", url);

	ensure(m_downloader)->start(url, true, true, true, tr("Downloading Config Database"));

	Q_EMIT download_started();
}

void config_database::handle_download_error(const QString& error)
{
	Q_EMIT download_error(error);
}

void config_database::handle_download_finished(const QByteArray& content)
{
	gui_log.notice("Config database download finished");

	// Create new map from database and write database to file if database was valid
	if (read_json(content, true))
	{
		// Write database to file
		QFile file(m_filepath);

		if (file.exists())
		{
			gui_log.notice("Config database file found: %s", m_filepath);
		}

		if (!file.open(QIODevice::WriteOnly))
		{
			gui_log.error("Config database error - Could not write database to file: %s", m_filepath);
			return;
		}

		file.write(content);
		file.close();

		gui_log.success("Wrote config database to file: %s", m_filepath);
	}

	Q_EMIT download_finished();
}

void config_database::handle_download_canceled()
{
	Q_EMIT download_canceled();
}

std::optional<std::string> config_database::read_json(const QByteArray& data, bool after_download, const std::string& serial)
{
	QJsonParseError error {};
	const QJsonDocument json_document = QJsonDocument::fromJson(data, &error);

	if (!json_document.isObject())
	{
		gui_log.error("Config database error - Invalid JSON: '%s'", error.errorString());
		return std::nullopt;
	}

	const QJsonObject json_data = json_document.object();
	const int return_code = json_data["return_code"].toInt(-255);

	if (return_code < 0)
	{
		if (after_download)
		{
			std::string error_message;
			switch (return_code)
			{
			case -1: error_message = "Server Error - Internal Error"; break;
			case -2: error_message = "Server Error - Maintenance Mode"; break;
			case -255: error_message = "Server Error - Return code not found"; break;
			default: error_message = "Server Error - Unknown Error"; break;
			}
			gui_log.error("%s: return code %d", error_message, return_code);
			Q_EMIT download_error(QString::fromStdString(error_message) + " " + QString::number(return_code));
		}
		else
		{
			gui_log.error("Config database error - Invalid: return code %d", return_code);
		}
		return std::nullopt;
	}

	if (!json_data["games"].isObject())
	{
		gui_log.error("Config database error - No games found");
		return std::nullopt;
	}

	std::unique_ptr<cfg_root> config = std::make_unique<cfg_root>();

	const QJsonObject json_games = json_data["games"].toObject();

	const auto validate = [&json_games, &config](const QString& serial) -> std::optional<std::string>
	{
		if (!json_games[serial].isObject())
		{
			gui_log.error("Config database error - Unusable object %s", serial);
			return std::nullopt;
		}

		const QJsonObject game = json_games[serial].toObject();
		if (!game["config"].isString())
		{
			gui_log.error("Config database error - Unusable game string %s (config missing)", serial);
			return std::nullopt;
		}

		const std::string content = game["config"].toString().toStdString();

		// Verify config
		if (!config->validate(content))
		{
			gui_log.error("Config database error - Invalid config for %s", serial);
			return std::nullopt;
		}

		return content;
	};

	if (serial.empty())
	{
		m_config_database.clear();

		// Retrieve status data for every valid entry
		for (const QString& serial : json_games.keys())
		{
			if (validate(serial))
			{
				// Add title to set
				m_config_database.insert(serial.toStdString());
			}
		}

		return std::string();
	}

	return validate(QString::fromStdString(serial));
}
