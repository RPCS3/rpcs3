#pragma once

#include "downloader.h"

class iso_integrity : public QObject
{
	Q_OBJECT

private:
	downloader* m_downloader = nullptr;

public:
	// Handles download for the ISO integrity database
	iso_integrity(QWidget* parent);

	// Downloads and writes the database to file
	void download();

private Q_SLOTS:
	void handle_download_finished(const QByteArray& content);
	void handle_download_canceled();
	void handle_download_error(const QString& error);
};
