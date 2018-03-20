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
		{ "Playable", { 0, "", "#1ebc61", QObject::tr(u8"可玩"),         QObject::tr(u8"可以從頭到尾執行的遊戲。") } },
		{ "Ingame",   { 1, "", "#f9b32f", QObject::tr(u8"遊戲中"),           QObject::tr(u8"不完善的遊戲，有嚴重故障或性能不足。") } },
		{ "Intro",    { 2, "", "#e08a1e", QObject::tr(u8"介紹"),            QObject::tr(u8"顯示圖像但不會超過遊戲選單。") } },
		{ "Loadable", { 3, "", "#e74c3c", QObject::tr(u8"可讀取"),         QObject::tr(u8"在視窗標題上僅顯示框速率但黑畫面的遊戲。") } },
		{ "Nothing",  { 4, "", "#455556", QObject::tr(u8"沒有"),          QObject::tr(u8"不正確初始化遊戲，根本不讀取或使仿真器崩潰。") } },
		{ "NoResult", { 5, "", "",        QObject::tr(u8"未找到結果"), QObject::tr(u8"此相容性資料庫中未發現遊戲或應用程式項目。") } },
		{ "NoData",   { 6, "", "",        QObject::tr(u8"資料庫遺失"), QObject::tr(u8"右鍵點擊此處並下載最新的資料庫。\n確定您已連結到網際網路。") } },
		{ "Download", { 7, "", "",        QObject::tr(u8"提取..."),    QObject::tr(u8"下載相容性資料庫。 請稍候...") } }
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
