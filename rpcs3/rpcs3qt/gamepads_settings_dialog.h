#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QListView>

#include "../Emu/System.h"
#include "../../Utilities/Config.h"
#include "../../Utilities/File.h"
#include "../Emu/Io/PadHandler.h"

class gamepads_settings_dialog : public QDialog
{
	const int MAX_PLAYERS = 7;

protected:
	std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);
	void ChangeInputType(int player);
	void ClickConfigButton(int player);
	void SaveExit();
	void CancelExit();

protected:
	QComboBox *co_inputtype[7];
	QComboBox *co_deviceID[7];
	QComboBox *co_profile[7];
	QPushButton *bu_config[7];
	QPushButton *bu_new_profile[7];

public:
	gamepads_settings_dialog(QWidget* parent);
	~gamepads_settings_dialog() = default;
};
