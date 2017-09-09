#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>

#include "../Emu/System.h"
#include "../../Utilities/Config.h"
#include "../../Utilities/File.h"
#include "../Emu/Io/PadHandler.h"

struct input_config final : cfg::node
{
	const std::string cfg_name = fs::get_config_dir() + "/config_input.yml";

	cfg::_enum<pad_handler> player_input[7]{
		{ this, "Player 1 Input", pad_handler::keyboard },
		{ this, "Player 2 Input", pad_handler::null },
		{ this, "Player 3 Input", pad_handler::null },
		{ this, "Player 4 Input", pad_handler::null },
		{ this, "Player 5 Input", pad_handler::null },
		{ this, "Player 6 Input", pad_handler::null },
		{ this, "Player 7 Input", pad_handler::null } };

	cfg::string player1{ this, "Player 1 Device", "Keyboard" };
	cfg::string player2{ this, "Player 2 Device", "Default Null Device" };
	cfg::string player3{ this, "Player 3 Device", "Default Null Device" };
	cfg::string player4{ this, "Player 4 Device", "Default Null Device" };
	cfg::string player5{ this, "Player 5 Device", "Default Null Device" };
	cfg::string player6{ this, "Player 6 Device", "Default Null Device" };
	cfg::string player7{ this, "Player 7 Device", "Default Null Device" };

	cfg::string *player_device[7]{ &player1, &player2, &player3, &player4, &player5, &player6, &player7 }; // Thanks gcc!

	bool load()
	{
		if (fs::file cfg_file{ cfg_name, fs::read })
		{
			return from_string(cfg_file.to_string());
		}

		return false;
	}

	void save()
	{
		fs::file(cfg_name, fs::rewrite).write(to_string());
	}

	bool exist()
	{
		return fs::is_file(cfg_name);
	}
};

extern input_config input_cfg;

class gamepads_settings_dialog : public QDialog
{
protected:
	std::shared_ptr<PadHandlerBase> GetHandler(pad_handler type);
	void ChangeInputType(int player);
	void ChangeDevice(int player);
	void ClickConfigButton(int player);
	void SaveExit();
	void CancelExit();

protected:
	QComboBox *co_inputtype[7];
	QComboBox *co_deviceID[7];
	QPushButton *bu_config[7];

public:
	gamepads_settings_dialog(QWidget* parent);
	~gamepads_settings_dialog() = default;
};
