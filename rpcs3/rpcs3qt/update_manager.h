#pragma once

#include "util/types.hpp"
#include <QObject>
#include <QByteArray>

#include <string>

class downloader;
class gui_settings;

class update_manager final :  public QObject
{
	Q_OBJECT

private:
	downloader* m_downloader = nullptr;
	QWidget* m_parent        = nullptr;

	std::shared_ptr<gui_settings> m_gui_settings;

	// This message is empty if there is no download available
	QString m_update_message;

	struct changelog_data
	{
		QString version;
		QString title;
	};
	std::vector<changelog_data> m_changelog;

	std::string m_request_url;
	std::string m_expected_hash;
	std::string m_old_version;
	std::string m_new_version;
	u64 m_expected_size = 0;

	bool handle_json(bool automatic, bool check_only, bool auto_accept, const QByteArray& data);
	bool handle_rpcs3(const QByteArray& data, bool auto_accept);

public:
	update_manager(QObject* parent, std::shared_ptr<gui_settings> gui_settings);
	void check_for_updates(bool automatic, bool check_only, bool auto_accept, QWidget* parent = nullptr);
	void update(bool auto_accept);

Q_SIGNALS:
	void signal_update_available(bool update_available);
};
