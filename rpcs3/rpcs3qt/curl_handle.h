#pragma once

#include <QObject>

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif

#ifdef _MSC_VER
#pragma warning(push, 0)
#include <curl/curl.h>
#pragma warning(pop)
#else
#include <curl/curl.h>
#endif

class curl_handle : public QObject
{
private:
	CURL* m_curl = nullptr;

public:
	curl_handle(QObject* parent = nullptr);
	~curl_handle();

	CURL* get_curl();
};
