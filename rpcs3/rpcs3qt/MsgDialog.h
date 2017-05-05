#ifndef MSG_DIALOG_H
#define MSG_DIALOG_H

#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"
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

class MsgDialogFrame : public QWidget, public MsgDialogBase
{
	Q_OBJECT

	QDialog* m_dialog;
	QLabel* m_text;
	QLabel* m_text1;
	QLabel* m_text2;
	QPushButton* m_button_ok;
	QPushButton* m_button_yes;
	QPushButton* m_button_no;
	QProgressBar* m_gauge1;
	QProgressBar* m_gauge2;
	QFormLayout* m_layout;
	QHBoxLayout* m_hBoxLayout1;
	QHBoxLayout* m_hBoxLayout2;
	QHBoxLayout* m_hBoxLayout3;


	QInputDialog* osk_dialog;
	QHBoxLayout* osk_hBoxLayout;
	QFormLayout* osk_layout;
	QPushButton* osk_button_ok;
	QLineEdit* osk_input;
	char16_t* osk_text_return;

public:
	MsgDialogFrame();
	virtual void Create(const std::string& msg) override;
	virtual void CreateOsk(const std::string& msg, char16_t* osk_text) override;
	virtual void ProgressBarSetMsg(u32 progressBarIndex, const std::string& msg) override;
	virtual void ProgressBarReset(u32 progressBarIndex) override;
	virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) override;

private:
	void closeEvent(QCloseEvent *event);
};

#endif
