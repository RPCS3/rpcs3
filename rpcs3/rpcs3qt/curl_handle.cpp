#include "curl_handle.h"
#include "Emu/system_utils.hpp"
#include "util/logs.hpp"

#ifdef _WIN32
#include "Utilities/StrUtil.h"
#endif

LOG_CHANNEL(network_log, "NET");

namespace rpcs3::curl
{

curl_handle::curl_handle(QObject* parent) : QObject(parent)
{
	reset_error_buffer();

	m_curl = curl_easy_init();

	CURLcode err = curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_error_buffer.data());
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_ERRORBUFFER): %s", curl_easy_strerror(err));

	m_uses_error_buffer = err == CURLE_OK;

	err = curl_easy_setopt(m_curl, CURLOPT_VERBOSE, s_curl_verbose);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_VERBOSE, %d): %s", s_curl_verbose, curl_easy_strerror(err));

#ifdef _WIN32
	// This shouldn't be needed on linux
	const std::string path_to_cert = rpcs3::utils::get_exe_dir() + "cacert.pem";
	const std::string ansi_path = utf8_path_to_ansi_path(path_to_cert);

	err = curl_easy_setopt(m_curl, CURLOPT_CAINFO, ansi_path.data());
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_CAINFO, %s) error: %s", ansi_path, curl_easy_strerror(err));
#endif
}

curl_handle::~curl_handle()
{
	curl_easy_cleanup(m_curl);
}

CURL* curl_handle::get_curl() const
{
	return m_curl;
}

void curl_handle::reset_error_buffer()
{
	ensure(m_error_buffer.size() == CURL_ERROR_SIZE);
	m_error_buffer[0] = 0;
}

std::string curl_handle::get_verbose_error(CURLcode code)
{
	if (m_uses_error_buffer)
	{
		ensure(m_error_buffer.size() == CURL_ERROR_SIZE);
		if (m_error_buffer[0])
		{
			return fmt::format("Curl error (%d): %s\nDetails: %s", static_cast<int>(code), curl_easy_strerror(code), m_error_buffer.data());
		}
	}

	return fmt::format("Curl error (%d): %s", static_cast<int>(code), curl_easy_strerror(code));
}

}
