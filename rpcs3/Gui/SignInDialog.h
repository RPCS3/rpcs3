#pragma once

#include "Emu/SysCalls/Modules/cellNetCtl.h"

class SignInDialogFrame : public SignInDialogInstance
{
	std::unique_ptr<wxDialog> m_dialog;
	/*wxGauge* m_gauge1;
	wxGauge* m_gauge2;
	wxStaticText* m_text1;
	wxStaticText* m_text2;
	wxButton* m_button_ok;
	wxButton* m_button_yes;
	wxButton* m_button_no;
	wxStaticText* m_text;
	wxSizer* m_sizer1;
	wxSizer* m_buttons;*/

public:
	virtual void Create() override;
	virtual void Destroy() override;
	//virtual void ProgressBarSetMsg(u32 progressBarIndex, std::string msg) override;
	//virtual void ProgressBarReset(u32 progressBarIndex) override;
	//virtual void ProgressBarInc(u32 progressBarIndex, u32 delta) override;
};
