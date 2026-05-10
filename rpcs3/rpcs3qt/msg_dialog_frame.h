#pragma once

#include "util/types.hpp"
#include "Emu/Cell/Modules/cellMsgDialog.h"
#include "progress_indicator.h"

#include <QProgressBar>
#include <QLabel>

#include <string>

class custom_dialog;

class msg_dialog_frame : public QObject, public MsgDialogBase
{
	Q_OBJECT

private:
	custom_dialog* m_dialog = nullptr;
	QLabel* m_text = nullptr;
	QLabel* m_text1 = nullptr;
	QLabel* m_text2 = nullptr;
	QProgressBar* m_gauge1 = nullptr;
	QProgressBar* m_gauge2 = nullptr;

	int m_gauge_max = 0;
	std::unique_ptr<progress_indicator> m_progress_indicator;

public:
	msg_dialog_frame() = default;
	~msg_dialog_frame();
	void Create(const std::string& msg, const std::string& title = "") override;
	void Close(bool success) override;
	void SetMsg(const std::string& msg) override;
	void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) override;
	void ProgressBarReset(u32 progressBarIndex) override;
	void ProgressBarInc(u32 progressBarIndex, u32 delta) override;
	void ProgressBarSetValue(u32 progressBarIndex, u32 value) override;
	void ProgressBarSetLimit(u32 index, u32 limit) override;
};
