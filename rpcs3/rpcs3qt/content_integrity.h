#pragma once

#include "downloader.h"

#include "Loader/content_validation.h"

class content_integrity : public QObject
{
	Q_OBJECT

public:
	// Handles download for the content integrity database
	content_integrity(QWidget* parent, content_file_type file_type);

	// Downloads and writes the database to file
	void download();

private Q_SLOTS:
	void handle_download_finished(const QByteArray& content);
	void handle_download_canceled();
	void handle_download_error(const QString& error);

private:
	QByteArray read_json(const QByteArray& data, bool after_download);

	content_file_type m_file_type;
	QString m_file_path;
	std::string m_url;
	std::string m_data_prefix;
	downloader* m_downloader = nullptr;
};
