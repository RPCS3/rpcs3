#pragma once

#include "stdafx.h"
#include <QtNetwork>

class progress_dialog;

class update_manager : public QObject
{
	Q_OBJECT

private:
	const std::string m_update_url = "https://update.rpcs3.net/?api=v1&c=";
	const std::string m_tmp_folder = "/rpcs3_old/";

private:
	progress_dialog* m_progress_dialog = nullptr;
	QWidget* m_parent                  = nullptr;

	QNetworkAccessManager m_manager;

	std::string m_expected_hash;
	u64 m_expected_size = 0;

private:
	bool handle_reply(QNetworkReply* reply, const std::function<bool(update_manager& man, const QByteArray&, bool)>& func, bool automatic, const std::string& message);
	bool handle_json(const QByteArray& json_data, bool automatic);
	bool handle_rpcs3(const QByteArray& rpcs3_data, bool automatic);

public:
	update_manager();
	void check_for_updates(bool automatic, QWidget* parent = nullptr);

private Q_SLOTS:
	void handle_error(QNetworkReply::NetworkError error);
};
