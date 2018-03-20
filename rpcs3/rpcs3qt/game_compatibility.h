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
		{ "Playable", { 0, "", "#1ebc61", QObject::tr(u8"�i��"),         QObject::tr(u8"�i�H�q�Y������檺�C���C") } },
		{ "Ingame",   { 1, "", "#f9b32f", QObject::tr(u8"�C����"),           QObject::tr(u8"���������C���A���Y���G�٩Ωʯण���C") } },
		{ "Intro",    { 2, "", "#e08a1e", QObject::tr(u8"����"),            QObject::tr(u8"��ܹϹ������|�W�L�C�����C") } },
		{ "Loadable", { 3, "", "#e74c3c", QObject::tr(u8"�iŪ��"),         QObject::tr(u8"�b�������D�W����ܮسt�v���µe�����C���C") } },
		{ "Nothing",  { 4, "", "#455556", QObject::tr(u8"�S��"),          QObject::tr(u8"�����T��l�ƹC���A�ڥ���Ū���Ψϥ�u���Y��C") } },
		{ "NoResult", { 5, "", "",        QObject::tr(u8"����쵲�G"), QObject::tr(u8"���ۮe�ʸ�Ʈw�����o�{�C�������ε{�����ءC") } },
		{ "NoData",   { 6, "", "",        QObject::tr(u8"��Ʈw��"), QObject::tr(u8"�k���I�����B�äU���̷s����Ʈw�C\n�T�w�z�w�s������ں����C") } },
		{ "Download", { 7, "", "",        QObject::tr(u8"����..."),    QObject::tr(u8"�U���ۮe�ʸ�Ʈw�C �еy��...") } }
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
