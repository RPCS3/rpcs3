#pragma once

#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/Modules/cellOskDialog.h"
#include "Emu/System.h"
#include "Emu/Cell/lv2/sys_time.h"

#include "custom_dialog.h"

class osk_dialog_frame : public QObject, public OskDialogBase
{
	Q_OBJECT

public:
	osk_dialog_frame();
	~osk_dialog_frame();
	virtual void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit) override;

private:
	custom_dialog* m_dialog = nullptr;
};
