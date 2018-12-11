#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <QEvent>
#include <QLabel>

#include "Emu/Io/PadHandler.h"
#include "Emu/GameInfo.h"

namespace Ui
{
	class custom_pad_settings_dialog;
}

class custom_pad_settings_dialog : public QDialog
{
    Q_OBJECT

public:
    explicit custom_pad_settings_dialog(QWidget *parent = nullptr, const GameInfo *game = nullptr);
	~custom_pad_settings_dialog();

private Q_SLOTS:
	void RefreshInputTypes();
	void ChangeInputType();
	void ChangeDeviceType(const QString & dev);
	void ChangeProfileBox(const QString& prof);
	/** Save the Pad Configuration to the current Pad Handler Config File */
	void SaveExit();
	void CancelExit();

private:
    Ui::custom_pad_settings_dialog *ui;

	std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);
};
