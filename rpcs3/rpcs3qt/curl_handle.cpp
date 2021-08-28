#include "curl_handle.h"
#include "Emu/system_utils.hpp"
#include "util/logs.hpp"

#ifdef _WIN32
#include "Utilities/StrUtil.h"
#endif

LOG_CHANNEL(network_log, "NET");

curl_handle::curl_handle(QObject* parent) : QObject(parent)
{
	m_curl = curl_easy_init();

#ifdef _WIN32
	// This shouldn't be needed on linux
	const std::string path_to_cert = rpcs3::utils::get_exe_dir() + "cacert.pem";
	const std::string ansi_path = utf8_path_to_ansi_path(path_to_cert);

	const CURLcode err = curl_easy_setopt(m_curl, CURLOPT_CAINFO, ansi_path.data());
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
