#pragma once

#include <QObject>

#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#include <curl/curl.h>

class curl_handle : public QObject
{
private:
	CURL* m_curl = nullptr;

public:
	curl_handle(QObject* parent = nullptr);
	~curl_handle();

	CURL* get_curl();
};
