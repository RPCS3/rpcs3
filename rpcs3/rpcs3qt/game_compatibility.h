#pragma once

#include <memory>

#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTableWidget>
#include <QProgressDialog>
#include <QTimer>

#include "gui_settings.h"

class game_compatibility : public QObject
{
	Q_OBJECT

	const std::map<QString, Compat_Status> Status_Data =
	{
		{ "Playable", { "", "#2ecc71", QObject::tr("Playable"),         QObject::tr("Games that can be properly played from start to finish") } },
		{ "Ingame",   { "", "#f1c40f", QObject::tr("Ingame"),           QObject::tr("Games that go somewhere but not far enough to be considered playable") } },
		{ "Intro",    { "", "#f39c12", QObject::tr("Intro"),            QObject::tr("Games that only display some screens") } },
		{ "Loadable", { "", "#e74c3c", QObject::tr("Loadable"),         QObject::tr("Games that display a black screen with an active framerate") } },
		{ "Nothing",  { "", "#2c3e50", QObject::tr("Nothing"),          QObject::tr("Games that show nothing") } },
		{ "NoResult", { "", "",        QObject::tr("No results found"), QObject::tr("There is no entry for this game or application in the compatibility database yet.") } },
		{ "NoData",   { "", "",        QObject::tr("Database missing"), QObject::tr("Right click here and download the current database.\nMake sure you are connected to the internet.") } },
		{ "Download", { "", "",        QObject::tr("Retrieving..."),    QObject::tr("Downloading the compatibility database. Please wait...") } }
	};
	int m_timer_count = 0;
	QString m_filepath;
	QString m_url;
	QNetworkRequest m_network_request;
	std::shared_ptr<gui_settings> m_xgui_settings;
	std::unique_ptr<QTimer> m_progress_timer;
	std::unique_ptr<QProgressDialog> m_progress_dialog;
	std::unique_ptr<QNetworkAccessManager> m_network_access_manager;
	std::map<std::string, Compat_Status> m_compat_database;

public:
	/** Handles reads, writes and downloads for the compatibility database */
	game_compatibility(std::shared_ptr<gui_settings> settings);

	/** Reads database. If online set to true: Downloads and writes the database to file */
	void RequestCompatibility(bool online = false);

	/** Returns the compatibility status for the requested title */
	Compat_Status GetCompatibility(const std::string& title_id);

	/** Returns the data for the requested status */
	Compat_Status GetStatusData(const QString& status);

Q_SIGNALS:
	void DownloadStarted();
	void DownloadFinished();
	void DownloadError(const QString& error);
};

class compat_pixmap : public QPixmap
{
public:
	compat_pixmap(const QColor& color) : QPixmap(16, 16)
	{
		fill(Qt::transparent);

		QPainter painter(this);
		painter.setPen(color);
		painter.setBrush(color);
		painter.drawEllipse(0, 0, 15, 15);
	}
};
