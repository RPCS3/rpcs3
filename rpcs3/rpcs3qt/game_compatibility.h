#pragma once

#include <memory>

#include <QPainter>
#include <QJsonObject>

#define NOMINMAX
#define CURL_STATICLIB
#include <curl/curl.h>

class gui_settings;
class progress_dialog;

struct compat_status
{
	int index;
	QString date;
	QString color;
	QString text;
	QString tooltip;
	QString latest_version;
};

class game_compatibility : public QObject
{
	Q_OBJECT

private:
	const std::map<QString, compat_status> Status_Data =
	{
		{ "Playable", { 0, "", "#1ebc61", tr("Playable"),         tr("Games that can be properly played from start to finish") } },
		{ "Ingame",   { 1, "", "#f9b32f", tr("Ingame"),           tr("Games that either can't be finished, have serious glitches or have insufficient performance") } },
		{ "Intro",    { 2, "", "#e08a1e", tr("Intro"),            tr("Games that display image but don't make it past the menus") } },
		{ "Loadable", { 3, "", "#e74c3c", tr("Loadable"),         tr("Games that display a black screen with a framerate on the window's title") } },
		{ "Nothing",  { 4, "", "#455556", tr("Nothing"),          tr("Games that don't initialize properly, not loading at all and/or crashing the emulator") } },
		{ "NoResult", { 5, "", "",        tr("No results found"), tr("There is no entry for this game or application in the compatibility database yet.") } },
		{ "NoData",   { 6, "", "",        tr("Database missing"), tr("Right click here and download the current database.\nMake sure you are connected to the internet.") } },
		{ "Download", { 7, "", "",        tr("Retrieving..."),    tr("Downloading the compatibility database. Please wait...") } }
	};
	int m_timer_count = 0;
	QString m_filepath;
	std::string m_url;
	std::atomic<bool> m_curl_abort = false;
	double m_actual_dwnld_size = -1.0;
	CURL *m_curl = nullptr;
	QByteArray m_curl_buf;
	progress_dialog* m_progress_dialog = nullptr;
	std::shared_ptr<gui_settings> m_xgui_settings;
	std::map<std::string, compat_status> m_compat_database;

	/** Creates new map from the database */
	bool ReadJSON(const QJsonObject& json_data, bool after_download);

public:
	/** Handles reads, writes and downloads for the compatibility database */
	game_compatibility(std::shared_ptr<gui_settings> settings);

	/** Reads database. If online set to true: Downloads and writes the database to file */
	void RequestCompatibility(bool online = false);

	/** Returns the compatibility status for the requested title */
	compat_status GetCompatibility(const std::string& title_id);

	/** Returns the data for the requested status */
	compat_status GetStatusData(const QString& status);

	size_t update_buffer(char* data, size_t size);

Q_SIGNALS:
	void DownloadStarted();
	void DownloadFinished();
	void DownloadError(const QString& error);
};

class compat_pixmap : public QPixmap
{
public:
	compat_pixmap(const QColor& color, qreal pixel_ratio) : QPixmap(16 * pixel_ratio, 16 * pixel_ratio)
	{
		fill(Qt::transparent);

		QPainter painter(this);
		setDevicePixelRatio(pixel_ratio);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(Qt::NoPen);
		painter.setBrush(color);
		painter.drawEllipse(0, 0, width(), height());
	}
};
