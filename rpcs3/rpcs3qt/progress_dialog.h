﻿#pragma once

#include "stdafx.h"

#include <QProgressDialog>

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

class progress_dialog : public QProgressDialog
{
public:
	progress_dialog(const QString &windowTitle, const QString &labelText, const QString &cancelButtonText, int minimum, int maximum, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
	~progress_dialog();
	void SetValue(int progress);
	void SignalFailure();

private:
#ifdef _WIN32
	std::unique_ptr<QWinTaskbarButton> m_tb_button = nullptr;
	QWinTaskbarProgress* m_tb_progress = nullptr;
#elif HAVE_QTDBUS
	void UpdateProgress(int progress, bool disable = false);
#endif
};
