#pragma once

#include <QObject>
#include "util/atomic.hpp"

class curl_handle;
class progress_dialog;

class downloader : public QObject
{
	Q_OBJECT

public:
	downloader(QWidget* parent = nullptr);

	void start(const std::string& url, bool follow_location, bool show_progress_dialog, const QString& progress_dialog_title = "", bool keep_progress_dialog_open = false, int expected_size = -1);
	size_t update_buffer(char* data, size_t size);

	void update_progress_dialog(const QString& title);
	void close_progress_dialog();

	progress_dialog* get_progress_dialog() const;

	static std::string get_hash(const char* data, size_t size, bool lower_case);

private Q_SLOTS:
	void handle_buffer_update(int size, int max);

Q_SIGNALS:
	void signal_download_error(const QString& error);
	void signal_download_finished(const QByteArray& data);
	void signal_buffer_update(int size, int max);

private:
	QWidget* m_parent = nullptr;

	curl_handle* m_curl = nullptr;
	QByteArray m_curl_buf;
	atomic_t<bool> m_curl_abort = false;
	atomic_t<bool> m_curl_success = false;
	double m_actual_download_size = -1.0;

	progress_dialog* m_progress_dialog = nullptr;
	atomic_t<bool> m_keep_progress_dialog_open = false;
	QString m_progress_dialog_title;
};
