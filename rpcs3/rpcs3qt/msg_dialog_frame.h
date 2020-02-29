#pragma once

#include "stdafx.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include <QProgressBar>
#include <QLabel>

#ifdef _WIN32
#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
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
	custom_dialog* m_dialog = nullptr;
	QLabel* m_text = nullptr;
	QLabel* m_text1 = nullptr;
	QLabel* m_text2 = nullptr;
	QProgressBar* m_gauge1 = nullptr;
	QProgressBar* m_gauge2 = nullptr;

	int m_gauge_max = 0;

public:
	msg_dialog_frame();
	~msg_dialog_frame();
	virtual void Create(const std::string& msg, const std::string& title = "") override;
	virtual void Close(bool success) override;
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
