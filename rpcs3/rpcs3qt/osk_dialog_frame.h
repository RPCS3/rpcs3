#pragma once

#include "stdafx.h"
#include "Emu/Cell/Modules/cellOskDialog.h"

#include <QObject>

class custom_dialog;

class osk_dialog_frame : public QObject, public OskDialogBase
{
	Q_OBJECT

public:
	osk_dialog_frame();
	~osk_dialog_frame();
	virtual void Create(const std::string& title, const std::u16string& message, char16_t* init_text, u32 charlimit, u32 prohibit_flags, u32 panel_flag, u32 first_view_panel) override;
	virtual void Close(bool accepted) override;

private:
	void SetOskText(const QString& text);

	custom_dialog* m_dialog = nullptr;
	QString m_text_old;
};
