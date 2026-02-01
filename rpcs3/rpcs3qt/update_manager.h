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

	struct changelog_data
	{
		QString version;
		QString title;
	};

	struct update_info
	{
		bool update_found = false;
		bool hash_found = false;
		qint64 diff_msec = 0;
		QString cur_date;
		QString lts_date;
		QString old_version;
		QString new_version;
		std::vector<changelog_data> changelog;
	};

	update_info m_update_info {};

	bool m_dialog_deferred = false;
	bool m_initialization_complete = true;

	std::string m_request_url;
	std::string m_expected_hash;
	u64 m_expected_size = 0;

	bool handle_json(bool automatic, bool check_only, bool auto_accept, const QByteArray& data, bool initialization_complete);
	bool handle_rpcs3(const QByteArray& data, bool auto_accept);

public:
	update_manager(QObject* parent, std::shared_ptr<gui_settings> gui_settings);
	void check_for_updates(bool automatic, bool check_only, bool auto_accept, QWidget* parent = nullptr, bool initialization_complete = true);
	void update(bool auto_accept);
	void set_initialization_complete(bool complete);
	void show_pending_update_dialog();

Q_SIGNALS:
	void signal_update_available(bool update_available);
};
