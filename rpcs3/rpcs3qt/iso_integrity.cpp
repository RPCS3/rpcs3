#include "iso_integrity.h"
#include "gui_settings.h"

#include "Emu/system_utils.hpp"

#include <QJsonDocument>
#include <QJsonObject>

LOG_CHANNEL(compat_log, "Compat");

QByteArray iso_integrity::read_json(const QByteArray& data, bool after_download)
{
	QJsonParseError error {};
	const QJsonDocument json_document = QJsonDocument::fromJson(data, &error);

	if (!json_document.isObject())
	{
		compat_log.error("ISO Integrity database error - Invalid JSON: '%s'", error.errorString());
		return {};
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

			compat_log.error("%s: return code %d", error_message, return_code);
		}
		else
		{
			compat_log.error("ISO Integrity database error - Invalid: return code %d", return_code);
		}

		return {};
	}

	if (!json_data["redump"].isString())
	{
		compat_log.error("ISO Integrity database error - Unusable Redump string");
		return {};
	}

	return QByteArray().fromStdString(json_data["redump"].toString().toStdString());
};

iso_integrity::iso_integrity(const std::shared_ptr<gui_settings>& settings, QWidget* parent)
	: QObject(parent)
{
	m_filepath = settings->GetSettingsDir() + QString::fromStdString(rpcs3::utils::get_redump_db_filename());
	m_downloader = new downloader(parent);

	connect(m_downloader, &downloader::signal_download_finished, this, &iso_integrity::handle_download_finished);
	connect(m_downloader, &downloader::signal_download_canceled, this, &iso_integrity::handle_download_canceled);
	connect(m_downloader, &downloader::signal_download_error, this, &iso_integrity::handle_download_error);
}

void iso_integrity::download()
{
	const std::string url = rpcs3::utils::get_redump_db_download_url();

	compat_log.notice("Starting database download from: %s", url);

	m_downloader->start(url, true, true, true, tr("Downloading database"));
}

void iso_integrity::handle_download_finished(const QByteArray& content)
{
	compat_log.notice("Database download finished");

	// Write database to file
	if (QByteArray data = read_json(content, true); !data.isEmpty())
	{
		QFile file(m_filepath);

		if (file.exists())
		{
			compat_log.notice("Database file found: %s", m_filepath);
		}

		if (!file.open(QIODevice::WriteOnly))
		{
			compat_log.error("Failed to write database to file: %s", m_filepath);
			return;
		}

		file.write(data);
		file.close();

		compat_log.success("Database written to file: %s", m_filepath);
	}
}

void iso_integrity::handle_download_canceled()
{
	compat_log.notice("Database download canceled");
}

void iso_integrity::handle_download_error(const QString& error)
{
	compat_log.error("", error.toStdString().c_str());
}
