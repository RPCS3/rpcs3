#include "custom_dialog.h"

custom_dialog::custom_dialog(bool disableCancel, QWidget* parent)
	: QDialog(parent), m_disable_cancel(disableCancel)
{
	if (m_disable_cancel)
	{
		setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
	}
}

void custom_dialog::keyPressEvent(QKeyEvent* event)
{
	// this won't work with Alt+F4, the window still closes
	if (m_disable_cancel && event->key() == Qt::Key_Escape)
	{
		event->ignore();
	}
	else
	{
		QDialog::keyPressEvent(event);
	}
}

void custom_dialog::closeEvent(QCloseEvent* event)
{
	// spontaneous: don't close on external system level events like Alt+F4
	if (m_disable_cancel && event->spontaneous())
	{
		event->ignore();
	}
	else
	{
		QDialog::closeEvent(event);
	}
}
