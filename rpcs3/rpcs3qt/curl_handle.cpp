#include "stdafx.h"
#include "curl_handle.h"
#include "util/logs.hpp"

#ifdef _WIN32
#include "Utilities/StrUtil.h"
#endif

LOG_CHANNEL(network_log, "NET");

namespace rpcs3::curl
{

curl_handle::curl_handle()
{
	reset_error_buffer();

	m_curl = curl_easy_init();

	CURLcode err = curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_error_buffer.data());
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_ERRORBUFFER): %s", curl_easy_strerror(err));

	m_uses_error_buffer = err == CURLE_OK;

	err = curl_easy_setopt(m_curl, CURLOPT_VERBOSE, g_curl_verbose);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_VERBOSE, %d): %s", g_curl_verbose, curl_easy_strerror(err));

#ifdef _WIN32
	// Tell curl to use the native CA store for certificate verification
	err = curl_easy_setopt(m_curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
	if (err != CURLE_OK) network_log.error("curl_easy_setopt(CURLOPT_SSL_OPTIONS): %s", curl_easy_strerror(err));
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

std::string curl_handle::get_verbose_error(CURLcode code) const
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

#ifdef _WIN32
// Functions exported from our user_settings.h in WolfSSL, implemented in RPCS3
extern "C"
{

FILE* wolfSSL_fopen_utf8(const char* name, const char* mode)
{
	return _wfopen(utf8_to_wchar(name).c_str(), utf8_to_wchar(mode).c_str());
}

int wolfSSL_stat_utf8(const char* path, struct _stat* buffer)
{
	return _wstat(utf8_to_wchar(path).c_str(), buffer);
}

}
#endif
