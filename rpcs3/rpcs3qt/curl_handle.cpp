#include "stdafx.h"
#include "curl_handle.h"
#include "Emu/System.h"

curl_handle::curl_handle(QObject* parent) : QObject(parent)
{
	m_curl = curl_easy_init();

#ifdef _WIN32
	// This shouldn't be needed on linux
	const std::string path_to_cert = Emulator::GetEmuDir() + "cacert.pem";
	curl_easy_setopt(m_curl, CURLOPT_CAINFO, path_to_cert.c_str());
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
