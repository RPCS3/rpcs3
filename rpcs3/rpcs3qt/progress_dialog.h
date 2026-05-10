#pragma once

#include "progress_indicator.h"

#include <QProgressDialog>

class progress_dialog : public QProgressDialog
{
public:
	progress_dialog(const QString& windowTitle, const QString& labelText, const QString& cancelButtonText, int minimum, int maximum, bool delete_on_close, QWidget* parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
	~progress_dialog();
	void SetRange(int min, int max);
	void SetValue(int progress);
	void SetDeleteOnClose();
	void SignalFailure() const;

private:
	std::unique_ptr<progress_indicator> m_progress_indicator;
};
