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
		{ "Playable", { 0, "", "#1ebc61", QObject::tr(u8"\u53EF\u73A9"),         QObject::tr(u8"\u53EF\u4EE5\u5F9E\u982D\u5230\u5C3E\u57F7\u884C\u7684\u904A\u6232\u3002") } },
		{ "Ingame",   { 1, "", "#f9b32f", QObject::tr(u8"\u904A\u6232\u4E2D"),           QObject::tr(u8"\u4E0D\u5B8C\u5584\u7684\u904A\u6232\uFF0C\u6709\u56B4\u91CD\u6545\u969C\u6216\u6027\u80FD\u4E0D\u8DB3\u3002") } },
		{ "Intro",    { 2, "", "#e08a1e", QObject::tr(u8"\u4ECB\u7D39"),            QObject::tr(u8"\u986F\u793A\u5716\u50CF\u4F46\u4E0D\u6703\u8D85\u904E\u904A\u6232\u9078\u55AE\u3002") } },
		{ "Loadable", { 3, "", "#e74c3c", QObject::tr(u8"\u53EF\u8B80\u53D6"),         QObject::tr(u8"\u5728\u8996\u7A97\u6A19\u984C\u4E0A\u50C5\u986F\u793A\u6846\u901F\u7387\u4F46\u9ED1\u756B\u9762\u7684\u904A\u6232\u3002") } },
		{ "Nothing",  { 4, "", "#455556", QObject::tr(u8"\u6C92\u6709"),          QObject::tr(u8"\u4E0D\u6B63\u78BA\u521D\u59CB\u5316\u904A\u6232\uFF0C\u6839\u672C\u4E0D\u8B80\u53D6\u6216\u4F7F\u4EFF\u771F\u5668\u5D29\u6F70\u3002") } },
		{ "NoResult", { 5, "", "",        QObject::tr(u8"\u672A\u627E\u5230\u7D50\u679C"), QObject::tr(u8"\u6B64\u76F8\u5BB9\u6027\u8CC7\u6599\u5EAB\u4E2D\u672A\u767C\u73FE\u904A\u6232\u6216\u61C9\u7528\u7A0B\u5F0F\u9805\u76EE\u3002") } },
		{ "NoData",   { 6, "", "",        QObject::tr(u8"\u8CC7\u6599\u5EAB\u907A\u5931"), QObject::tr(u8"\u53F3\u9375\u9EDE\u64CA\u6B64\u8655\u4E26\u4E0B\u8F09\u6700\u65B0\u7684\u8CC7\u6599\u5EAB\u3002\n\u78BA\u5B9A\u60A8\u5DF2\u9023\u7D50\u5230\u7DB2\u969B\u7DB2\u8DEF\u3002") } },
		{ "Download", { 7, "", "",        QObject::tr(u8"\u63D0\u53D6..."),    QObject::tr(u8"\u4E0B\u8F09\u76F8\u5BB9\u6027\u8CC7\u6599\u5EAB\u3002 \u8ACB\u7A0D\u5019...") } }
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
