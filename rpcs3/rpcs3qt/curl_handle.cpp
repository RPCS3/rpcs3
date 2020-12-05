#include "stdafx.h"
#include "curl_handle.h"
#include "Emu/System.h"

#ifdef _WIN32
#include "Utilities/StrUtil.h"
#endif

curl_handle::curl_handle(QObject* parent) : QObject(parent)
{
	m_curl = curl_easy_init();

#ifdef _WIN32
	// This shouldn't be needed on linux
	const std::string path_to_cert = Emulator::GetExeDir() + "cacert.pem";
	const std::string ansi_path = utf8_path_to_ansi_path(path_to_cert);

	curl_easy_setopt(m_curl, CURLOPT_CAINFO, ansi_path.data());
#endif
}

curl_handle::~curl_handle()
{
	curl_easy_cleanup(m_curl);
}

CURL* curl_handle::get_curl()
{
	return m_curl;
}
