#pragma once
#include "Dialog.h"

enum MessageDialogFlags
{
	MessageDialog_Button_Enter   = 0x1,
	MessageDialog_Button_Back    = 0x2,

	MessageDialog_Controls_YesNo = 0x4,
};

class MessageDialog : public Dialog
{
	MessageDialog(std::string message, std::string title, int flags, char* icon);
};