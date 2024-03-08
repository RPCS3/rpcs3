#pragma once

#include <array>

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

namespace rpcs3::curl
{
inline bool g_curl_verbose = false;

class curl_handle
{
public:
	explicit curl_handle();
	~curl_handle();

	CURL* get_curl() const;

	operator CURL*() const
	{
		return get_curl();
	}

	void reset_error_buffer();
	std::string get_verbose_error(CURLcode code) const;

private:
	CURL* m_curl = nullptr;
	bool m_uses_error_buffer = false;
	std::array<char, CURL_ERROR_SIZE> m_error_buffer;
};

}
