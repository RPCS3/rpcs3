#include "content_integrity.h"
#include "gui_settings.h"

#include "Emu/system_utils.hpp"

#include <QJsonDocument>
#include <QJsonObject>

LOG_CHANNEL(sys_log, "INTEGRITY");

content_integrity::content_integrity(QWidget* parent, content_file_type file_type)
	: QObject(parent), m_file_type(file_type)
{
	switch (m_file_type)
	{
	case content_file_type::ISO:
		m_file_path = QString::fromStdString(rpcs3::utils::get_redump_db_path());
		m_url = rpcs3::utils::get_redump_db_download_url();
		m_data_prefix = "redump";
		break;
	case content_file_type::PSN_CONTENT:
		m_file_path = QString::fromStdString(rpcs3::utils::get_psn_content_db_path());
		m_url = rpcs3::utils::get_psn_content_db_download_url();
		m_data_prefix = "nointro";
		break;
	case content_file_type::PSN_DLC:
		m_file_path = QString::fromStdString(rpcs3::utils::get_psn_dlc_db_path());
		m_url = rpcs3::utils::get_psn_dlc_db_download_url();
		m_data_prefix = "nointro";
		break;
	case content_file_type::PSN_UPDATE:
		m_file_path = QString::fromStdString(rpcs3::utils::get_psn_update_db_path());
		m_url = rpcs3::utils::get_psn_update_db_download_url();
		m_data_prefix = "nointro";
		break;
	}

	m_downloader = new downloader(parent);

	connect(m_downloader, &downloader::signal_download_finished, this, &content_integrity::handle_download_finished);
	connect(m_downloader, &downloader::signal_download_canceled, this, &content_integrity::handle_download_canceled);
	connect(m_downloader, &downloader::signal_download_error, this, &content_integrity::handle_download_error);
}

void content_integrity::download()
{
	sys_log.notice("Starting database download from: %s", m_url);

	m_downloader->start(m_url, true, true, true, tr("Downloading database"));
}

void content_integrity::handle_download_finished(const QByteArray& content)
{
	sys_log.notice("Database download finished");

	// Write database to file
	if (QByteArray data = read_json(content, true); !data.isEmpty())
	{
		QFile file(m_file_path);

		if (file.exists())
		{
			sys_log.notice("Database file found: %s", m_file_path);
		}

		if (!file.open(QIODevice::WriteOnly))
		{
			sys_log.error("Failed to write database to file: %s", m_file_path);
			return;
		}

		file.write(data);
		file.close();

		sys_log.success("Database written to file: %s", m_file_path);
	}
}

void content_integrity::handle_download_canceled()
{
	sys_log.notice("Database download canceled");
}

void content_integrity::handle_download_error(const QString& error)
{
	sys_log.error("", error.toStdString().c_str());
}

QByteArray content_integrity::read_json(const QByteArray& data, bool after_download)
{
	QJsonParseError error{};
	const QJsonDocument json_document = QJsonDocument::fromJson(data, &error);

	if (!json_document.isObject())
	{
		sys_log.error("Integrity database error - Invalid JSON: '%s'", error.errorString());
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

			sys_log.error("%s: return code %d", error_message, return_code);
		}
		else
		{
			sys_log.error("Integrity database error - Invalid: return code %d", return_code);
		}

		return {};
	}

	if (!json_data[m_data_prefix.c_str()].isString())
	{
		sys_log.error("Integrity database error - Unusable string");
		return {};
	}

	return QByteArray().fromStdString(json_data[m_data_prefix.c_str()].toString().toStdString());
};
