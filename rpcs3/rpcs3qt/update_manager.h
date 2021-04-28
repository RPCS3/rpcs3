#pragma once

#include "util/types.hpp"
#include <QObject>
#include <QByteArray>

#include <string>

class downloader;

class update_manager final :  public QObject
{
	Q_OBJECT

private:
	downloader* m_downloader = nullptr;
	QWidget* m_parent        = nullptr;

	// This message is empty if there is no download available
	QString m_update_message;

	std::string m_request_url;
	std::string m_expected_hash;
	u64 m_expected_size = 0;

	bool handle_json(bool automatic, bool check_only, const QByteArray& data);
	bool handle_rpcs3(const QByteArray& data);

public:
	update_manager() = default;
	void check_for_updates(bool automatic, bool check_only, QWidget* parent = nullptr);
	void update();

Q_SIGNALS:
	void signal_update_available(bool update_available);
};
