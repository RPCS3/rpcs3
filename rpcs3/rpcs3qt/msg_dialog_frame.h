#ifndef MSGDIALOG_H
#define MSGDIALOG_H

#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellmsgdialog.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
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

class msg_dialog_frame : public QObject, public MsgDialogBase
{
	Q_OBJECT

	QDialog* m_dialog =nullptr;
	QLabel* m_text = nullptr;
	QLabel* m_text1 = nullptr;
	QLabel* m_text2 = nullptr;
	QPushButton* m_button_ok = nullptr;
	QPushButton* m_button_yes = nullptr;
	QPushButton* m_button_no = nullptr;
	QProgressBar* m_gauge1 = nullptr;
	QProgressBar* m_gauge2 = nullptr;
	QFormLayout* m_layout = nullptr;
	QHBoxLayout* m_hBoxLayout1 = nullptr;
	QHBoxLayout* m_hBoxLayout2 = nullptr;
	QHBoxLayout* m_hBoxLayout3 = nullptr;


	QDialog* osk_dialog = nullptr;
	QHBoxLayout* osk_hBoxLayout = nullptr;
	QFormLayout* osk_layout = nullptr;
	QPushButton* osk_button_ok = nullptr;
	QLineEdit* osk_input = nullptr;
	char16_t* osk_text_return;

public:
	msg_dialog_frame(QWindow* taskbarTarget);
	~msg_dialog_frame();
	virtual void Create(const std::string& msg) override;
	virtual void CreateOsk(const std::string& msg, char16_t* osk_text) override;
	virtual void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) override;
	virtual void ProgressBarReset(u32 progressBarIndex) override;
	virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) override;
private:
	QWindow* m_taskbarTarget;	// Window which will be targeted by custom taskbars.
};

#endif // !MSGDIALOG_H
