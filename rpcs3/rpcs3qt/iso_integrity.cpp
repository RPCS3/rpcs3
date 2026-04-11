#include "iso_integrity.h"
#include "gui_settings.h"

#include "EMU/system_utils.hpp"

LOG_CHANNEL(compat_log, "Compat");

iso_integrity::iso_integrity(QWidget* parent)
	: QObject(parent)
{
	m_downloader = new downloader(parent);

	connect(m_downloader, &downloader::signal_download_finished, this, &iso_integrity::handle_download_finished);
	connect(m_downloader, &downloader::signal_download_canceled, this, &iso_integrity::handle_download_canceled);
	connect(m_downloader, &downloader::signal_download_error, this, &iso_integrity::handle_download_error);
}

void iso_integrity::handle_download_finished(const QByteArray& content)
{
	compat_log.notice("Database download finished");

	QString path = QString::fromStdString(rpcs3::utils::get_redump_db_path());

	// Write database to file
	QFile file(path);

	if (file.exists())
	{
		compat_log.notice("Database file found: %s", path);
	}

	if (!file.open(QIODevice::WriteOnly))
	{
		compat_log.error("Failed to write database to file: %s", path);
		return;
	}

	file.write(content);
	file.close();

	compat_log.success("Database written to file: %s", path);
}

void iso_integrity::handle_download_canceled()
{
	compat_log.notice("Database download canceled");
}

void iso_integrity::handle_download_error(const QString& error)
{
	compat_log.error("", error.toStdString().c_str());
}

void iso_integrity::download()
{
	const std::string url = rpcs3::utils::get_redump_db_download_url();

	compat_log.notice("Starting database download from: %s", url);

	m_downloader->start(url, true, true, true, tr("Downloading database"));
}
