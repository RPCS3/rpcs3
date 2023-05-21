#include "progress_dialog.h"

#include <QApplication>
#include <QLabel>

progress_dialog::progress_dialog(const QString& windowTitle, const QString& labelText, const QString& cancelButtonText, int minimum, int maximum, bool delete_on_close, QWidget* parent, Qt::WindowFlags flags)
	: QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, flags)
{
	setWindowTitle(windowTitle);
	setMinimumSize(QLabel("This is the very length of the progressdialog due to hidpi reasons.").sizeHint().width(), sizeHint().height());
	setValue(0);
	setWindowModality(Qt::WindowModal);

	if (delete_on_close)
	{
		connect(this, &QProgressDialog::canceled, this, &QProgressDialog::deleteLater);
	}

	// Try to find a window handle first
	QWindow* handle = windowHandle();

	for (QWidget* ancestor = this; !handle && ancestor;)
	{
		ancestor = static_cast<QWidget*>(ancestor->parent());
		if (ancestor) handle = ancestor->windowHandle();
	}

	m_progress_indicator = std::make_unique<progress_indicator>(minimum, maximum);
	m_progress_indicator->show(handle);
}

progress_dialog::~progress_dialog()
{
}

void progress_dialog::SetRange(int min, int max)
{
	m_progress_indicator->set_range(min, max);

	QProgressDialog::setRange(min, max);
}

void progress_dialog::SetValue(int progress)
{
	const int value = std::clamp(progress, minimum(), maximum());

	m_progress_indicator->set_value(value);

	QProgressDialog::setValue(value);
}

void progress_dialog::SignalFailure() const
{
	m_progress_indicator->signal_failure();

	QApplication::beep();
}
