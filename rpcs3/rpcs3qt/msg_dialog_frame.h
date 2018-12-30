#pragma once

#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/lv2/sys_time.h"

#include <QPushButton>
#include <QStaticText>
#include <QDialog>
#include <QInputDialog>
#include <QFormLayout>
#include <QProgressBar>
#include <QLabel>
#include <QButtonGroup>
#include <QLineEdit>
#include <QKeyEvent>

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QWinTHumbnailToolbar>
#include <QWinTHumbnailToolbutton>
#elif HAVE_QTDBUS
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusConnection>
#endif

class custom_dialog;

class msg_dialog_frame : public QObject, public MsgDialogBase
{
	Q_OBJECT

private:
#ifdef _WIN32
	QWinTaskbarButton* m_tb_button = nullptr;
	QWinTaskbarProgress* m_tb_progress = nullptr;
#elif HAVE_QTDBUS
	int m_progress_value = 0;
#endif
	custom_dialog* m_dialog =nullptr;
	QLabel* m_text = nullptr;
	QLabel* m_text1 = nullptr;
	QLabel* m_text2 = nullptr;
	QPushButton* m_button_ok = nullptr;
	QPushButton* m_button_yes = nullptr;
	QPushButton* m_button_no = nullptr;
	QProgressBar* m_gauge1 = nullptr;
	QProgressBar* m_gauge2 = nullptr;

	QWindow* m_taskbarTarget; // Window which will be targeted by custom taskbars.

	custom_dialog* m_osk_dialog = nullptr;
	char16_t* m_osk_text_return;

	int m_gauge_max = 0;

public:
	msg_dialog_frame(QWindow* taskbarTarget);
	~msg_dialog_frame();
	virtual void Create(const std::string& msg, const std::string& title = "") override;
	virtual void CreateOsk(const std::string& msg, char16_t* osk_text, u32 charlimit) override;
	virtual void SetMsg(const std::string& msg) override;
	virtual void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) override;
	virtual void ProgressBarReset(u32 progressBarIndex) override;
	virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) override;
	virtual void ProgressBarSetLimit(u32 index, u32 limit) override;
#ifdef HAVE_QTDBUS
private:
	void UpdateProgress(int progress, bool disable = false);
#endif
};

class custom_dialog : public QDialog
{
	Q_OBJECT

	bool m_disable_cancel;
public:
	explicit custom_dialog(bool disableCancel, QWidget* parent = 0)
		: QDialog(parent), m_disable_cancel(disableCancel)
	{
		if (m_disable_cancel)
		{
			setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
		}
	}
private:
	void keyPressEvent(QKeyEvent* event) override
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
	void closeEvent(QCloseEvent* event) override
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
};
