#include "stdafx.h"

#include <QApplication>
#include <QThread>

#include "downloader.h"
#include "curl_handle.h"
#include "progress_dialog.h"

LOG_CHANNEL(network_log, "NETWORK");

size_t curl_write_cb_compat(char* ptr, size_t /*size*/, size_t nmemb, void* userdata)
{
	downloader* download = reinterpret_cast<downloader*>(userdata);
	return download->update_buffer(ptr, nmemb);
}

downloader::downloader(QWidget* parent)
	: QObject(parent)
	, m_parent(parent)
{
	m_curl = new curl_handle(this);
}

void downloader::start(const std::string& url, bool follow_location, bool show_progress_dialog, const QString& progress_dialog_title, bool keep_progress_dialog_open, int exptected_size)
{
	connect(this, &downloader::signal_buffer_update, this, &downloader::handle_buffer_update);

	m_keep_progress_dialog_open = keep_progress_dialog_open;
	m_curl_buf.clear();
	m_curl_abort = false;

	curl_easy_setopt(m_curl->get_curl(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(m_curl->get_curl(), CURLOPT_WRITEFUNCTION, curl_write_cb_compat);
	curl_easy_setopt(m_curl->get_curl(), CURLOPT_WRITEDATA, this);
	curl_easy_setopt(m_curl->get_curl(), CURLOPT_FOLLOWLOCATION, follow_location ? 1 : 0);

	const auto thread = QThread::create([this]
	{
		const auto result = curl_easy_perform(m_curl->get_curl());
		m_curl_success = result == CURLE_OK;

		if (!m_curl_success && !m_curl_abort)
		{
			const std::string error = "Curl error: " + std::string{ curl_easy_strerror(result) };
			network_log.error("%s", error);
			Q_EMIT signal_download_error(QString::fromStdString(error));
		}
	});

	connect(thread, &QThread::finished, this, [this]()
	{
		if (m_curl_abort)
		{
			return;
		}

		if (m_progress_dialog && (!m_keep_progress_dialog_open || !m_curl_success))
		{
			m_progress_dialog->close();
			m_progress_dialog = nullptr;
		}

		if (m_curl_success)
		{
			Q_EMIT signal_download_finished(m_curl_buf);
		}
	});

	if (show_progress_dialog)
	{
		const int maximum = exptected_size > 0 ? exptected_size : 100;

		if (m_progress_dialog)
		{
			m_progress_dialog->setWindowTitle(progress_dialog_title);
			m_progress_dialog->setAutoClose(!m_keep_progress_dialog_open);
			m_progress_dialog->setMaximum(maximum);
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
				close_progress_dialog();
			});
			connect(m_progress_dialog, &QProgressDialog::finished, m_progress_dialog, &QProgressDialog::deleteLater);
		}
	}

	thread->setObjectName("Compat Update");
	thread->start();
}

void downloader::update_progress_dialog(const QString& title)
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
		m_progress_dialog->close();
		m_progress_dialog = nullptr;
	}
}

progress_dialog* downloader::get_progress_dialog() const
{
	return m_progress_dialog;
}

size_t downloader::update_buffer(char* data, size_t size)
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
		if (curl_easy_getinfo(m_curl->get_curl(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &m_actual_download_size) == CURLE_OK && m_actual_download_size > 0)
		{
			max = static_cast<int>(m_actual_download_size);
		}
	}

	Q_EMIT signal_buffer_update(static_cast<int>(new_size), max);

	return size;
}

void downloader::handle_buffer_update(int size, int max)
{
	if (m_curl_abort)
	{
		return;
	}

	if (m_progress_dialog)
	{
		m_progress_dialog->setMaximum(max > 0 ? max : m_progress_dialog->maximum());
		m_progress_dialog->setValue(size);
		QApplication::processEvents();
	}
}
