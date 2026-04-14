#pragma once

#include "gui_settings.h"
#include "downloader.h"

class iso_integrity : public QObject
{
	Q_OBJECT

private:
	QString m_filepath;
	downloader* m_downloader = nullptr;

	QByteArray read_json(const QByteArray& data, bool after_download);

public:
	// Handles download for the ISO integrity database
	iso_integrity(const std::shared_ptr<gui_settings>& settings, QWidget* parent);

	// Downloads and writes the database to file
	void download();

private Q_SLOTS:
	void handle_download_finished(const QByteArray& content);
	void handle_download_canceled();
	void handle_download_error(const QString& error);
};
