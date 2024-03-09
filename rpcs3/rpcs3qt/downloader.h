#pragma once

#include <QObject>
#include "util/atomic.hpp"

namespace rpcs3
{
	namespace curl
	{
		class curl_handle;
	}
}

class progress_dialog;

class downloader : public QObject
{
	Q_OBJECT

public:
	explicit downloader(QWidget* parent = nullptr);
	~downloader();

	void start(const std::string& url, bool follow_location, bool show_progress_dialog, const QString& progress_dialog_title = "", bool keep_progress_dialog_open = false, int expected_size = -1);
	usz update_buffer(char* data, usz size);

	void update_progress_dialog(const QString& title) const;
	void close_progress_dialog();

	progress_dialog* get_progress_dialog() const;

private Q_SLOTS:
	void handle_buffer_update(int size, int max) const;

Q_SIGNALS:
	void signal_download_error(const QString& error);
	void signal_download_finished(const QByteArray& data);
	void signal_download_canceled();
	void signal_buffer_update(int size, int max);

private:
	QWidget* m_parent = nullptr;

	std::unique_ptr<rpcs3::curl::curl_handle> m_curl;
	QByteArray m_curl_buf;
	atomic_t<bool> m_curl_abort = false;
	atomic_t<bool> m_curl_success = false;
	double m_actual_download_size = -1.0;

	progress_dialog* m_progress_dialog = nullptr;
	atomic_t<bool> m_keep_progress_dialog_open = false;
	QString m_progress_dialog_title;

	QThread* m_thread = nullptr;
};
