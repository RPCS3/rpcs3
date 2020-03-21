﻿#pragma once

#include "stdafx.h"
#define NOMINMAX
#define CURL_STATICLIB
#include <curl/curl.h>
#include <QObject>
#include <QByteArray>

class progress_dialog;

class update_manager final :  public QObject
{
	Q_OBJECT

private:
	const std::string m_update_url = "https://update.rpcs3.net/?api=v1&c=";
	const std::string m_tmp_folder = "/rpcs3_old/";

	std::atomic<bool> m_update_dialog  = false;
	progress_dialog* m_progress_dialog = nullptr;
	QWidget* m_parent                  = nullptr;

	CURL *m_curl = nullptr;
	QByteArray m_curl_buf;
	std::atomic<bool> m_curl_abort = false;
	std::atomic<bool> m_curl_result = false;

	std::string m_expected_hash;
	u64 m_expected_size = 0;

	bool handle_json(bool automatic);
	bool handle_rpcs3();

public:
	update_manager();
	void check_for_updates(bool automatic, QWidget* parent = nullptr);
	size_t update_buffer(char* data, size_t size);

Q_SIGNALS:
	void signal_buffer_update(int size);

private Q_SLOTS:
	void handle_buffer_update(int size);
};
