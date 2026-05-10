#pragma once

#include "util/types.hpp"
#include "Emu/Cell/Modules/cellOskDialog.h"

#include <QObject>

#include <string>

class custom_dialog;

class osk_dialog_frame : public QObject, public OskDialogBase
{
	Q_OBJECT

public:
	osk_dialog_frame() = default;
	~osk_dialog_frame();
	void Create(const osk_params& params) override;
	void Close(s32 status) override;
	void Clear(bool clear_all_data) override;
	void SetText(const std::u16string& text) override;
	void Insert(const std::u16string& text) override;

private:
	void SetOskText(const QString& text);

	custom_dialog* m_dialog = nullptr;
	QString m_text_old;
};
