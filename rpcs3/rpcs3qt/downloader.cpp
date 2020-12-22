#include <QApplication>
#include <QThread>

#include "downloader.h"
#include "curl_handle.h"
#include "progress_dialog.h"

#include "Crypto/sha256.h"
#include "util/logs.hpp"

LOG_CHANNEL(network_log, "NETWORK");

usz curl_write_cb_compat(char* ptr, usz /*size*/, usz nmemb, void* userdata)
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

std::string downloader::get_hash(const char* data, usz size, bool lower_case)
{
	u8 res_hash[32];
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts_ret(&ctx, 0);
	mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const unsigned char*>(data), size);
	mbedtls_sha256_finish_ret(&ctx, res_hash);

	std::string res_hash_string("0000000000000000000000000000000000000000000000000000000000000000");

	for (usz index = 0; index < 32; index++)
	{
		const auto pal                   = lower_case ? "0123456789abcdef" : "0123456789ABCDEF";
		res_hash_string[index * 2]       = pal[res_hash[index] >> 4];
		res_hash_string[(index * 2) + 1] = pal[res_hash[index] & 15];
	}

	return res_hash_string;
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
