#include <QApplication>
#include <QThread>

#include "downloader.h"
#include "curl_handle.h"
#include "progress_dialog.h"

#include "util/logs.hpp"

LOG_CHANNEL(network_log, "NET");

usz curl_write_cb_compat(char* ptr, usz /*size*/, usz nmemb, void* userdata)
{
	downloader* download = static_cast<downloader*>(userdata);
	return download->update_buffer(ptr, nmemb);
}

downloader::downloader(QWidget* parent)
	: QObject(parent)
	, m_parent(parent)
	, m_curl(new rpcs3::curl::curl_handle())
{
}

downloader::~downloader()
{
	if (m_thread && m_thread->isRunning())
	{
		m_curl_abort = true;
		m_thread->wait();
	}
}

void downloader::start(const std::string& url, bool follow_location, bool show_progress_dialog, const QString& progress_dialog_title, bool keep_progress_dialog_open, int expected_size)
{
	network_log.notice("Starting download from URL: %s", url);

	if (m_thread)
	{
		if (m_thread->isRunning())
		{
			m_curl_abort = true;
			m_thread->wait();
		}
		m_thread->deleteLater();
	}

	m_keep_progress_dialog_open = keep_progress_dialog_open;
	m_curl_buf.clear();
	m_curl_abort = false;

	CURLcode err = curl_easy_setopt(m_curl->get_curl(), CURLOPT_URL, url.c_str());
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_URL, %s) error: %s", url, curl_easy_strerror(err));

	err = curl_easy_setopt(m_curl->get_curl(), CURLOPT_WRITEFUNCTION, curl_write_cb_compat);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_WRITEFUNCTION, curl_write_cb_compat) error: %s", curl_easy_strerror(err));

	err = curl_easy_setopt(m_curl->get_curl(), CURLOPT_WRITEDATA, this);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_WRITEDATA) error: %s", curl_easy_strerror(err));

	err = curl_easy_setopt(m_curl->get_curl(), CURLOPT_FOLLOWLOCATION, follow_location ? 1 : 0);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_FOLLOWLOCATION, %d) error: %s", follow_location, curl_easy_strerror(err));

	m_thread = QThread::create([this]
	{
		// Reset error buffer before we call curl_easy_perform
		m_curl->reset_error_buffer();

		const CURLcode result = curl_easy_perform(m_curl->get_curl());
		m_curl_success = result == CURLE_OK;

		if (!m_curl_success && !m_curl_abort)
		{
			const std::string error = fmt::format("curl_easy_perform(): %s", m_curl->get_verbose_error(result));
			network_log.error("%s", error);
			Q_EMIT signal_download_error(QString::fromStdString(error));
		}
	});

	connect(m_thread, &QThread::finished, this, [this]()
	{
		if (m_curl_abort)
		{
			network_log.notice("Download aborted");
			return;
		}

		if (!m_keep_progress_dialog_open || !m_curl_success)
		{
			close_progress_dialog();
		}

		if (m_curl_success)
		{
			network_log.notice("Download finished");
			Q_EMIT signal_download_finished(m_curl_buf);
		}
	});

	// The downloader's signals are expected to be disconnected and customized before start is called.
	// Therefore we need to (re)connect its signal(s) here and not in the constructor.
	connect(this, &downloader::signal_buffer_update, this, &downloader::handle_buffer_update);

	if (show_progress_dialog)
	{
		const int maximum = expected_size > 0 ? expected_size : 100;

		if (m_progress_dialog)
		{
			m_progress_dialog->setWindowTitle(progress_dialog_title);
			m_progress_dialog->setAutoClose(!m_keep_progress_dialog_open);
			m_progress_dialog->SetRange(0, maximum);
		}
		else
		{
			m_progress_dialog = new progress_dialog(progress_dialog_title, tr("Please wait..."), tr("Abort"), 0, maximum, true, m_parent);
			m_progress_dialog->setAutoReset(false);
			m_progress_dialog->setAutoClose(!m_keep_progress_dialog_open);
			m_progress_dialog->show();

			// Handle abort
			connect(m_progress_dialog, &QProgressDialog::canceled, this, [this]()
			{
				m_curl_abort = true;
				m_progress_dialog = nullptr; // The progress dialog deletes itself on close
				Q_EMIT signal_download_canceled();
			});
			connect(m_progress_dialog, &QProgressDialog::finished, this, [this]()
			{
				m_progress_dialog = nullptr; // The progress dialog deletes itself on close
			});
		}
	}

	m_thread->setObjectName("Download Thread");
	m_thread->setParent(this);
	m_thread->start();
}

void downloader::update_progress_dialog(const QString& title) const
{
	if (m_progress_dialog)
	{
		m_progress_dialog->setWindowTitle(title);
	}
}

void downloader::close_progress_dialog()
{
	if (m_progress_dialog)
	{
		m_progress_dialog->accept();
		m_progress_dialog = nullptr;
	}
}

progress_dialog* downloader::get_progress_dialog() const
{
	return m_progress_dialog;
}

usz downloader::update_buffer(char* data, usz size)
{
	if (m_curl_abort)
	{
		return 0;
	}

	const auto old_size = m_curl_buf.size();
	const auto new_size = old_size + size;
	m_curl_buf.resize(static_cast<int>(new_size));
	memcpy(m_curl_buf.data() + old_size, data, size);

	int max = 0;

	if (m_actual_download_size < 0)
	{
		if (curl_easy_getinfo(m_curl->get_curl(), CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &m_actual_download_size) == CURLE_OK && m_actual_download_size > 0)
		{
			max = static_cast<int>(m_actual_download_size);
		}
	}

	Q_EMIT signal_buffer_update(static_cast<int>(new_size), max);

	return size;
}

void downloader::handle_buffer_update(int size, int max) const
{
	if (m_curl_abort)
	{
		return;
	}

	if (m_progress_dialog)
	{
		m_progress_dialog->SetRange(0, max > 0 ? max : m_progress_dialog->maximum());
		m_progress_dialog->SetValue(size);
		QApplication::processEvents();
	}
}
