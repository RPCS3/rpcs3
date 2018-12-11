#include <QGroupBox>
#include <QPushButton>

#include "qt_utils.h"
#include "custom_pad_settings_dialog.h"
#include "ui_custom_pad_settings_dialog.h"

#include "Emu/Io/Null/NullPadHandler.h"

#include "keyboard_pad_handler.h"
#include "ds4_pad_handler.h"
#ifdef _WIN32
#include "xinput_pad_handler.h"
#include "mm_joystick_handler.h"
#endif
#ifdef HAVE_LIBEVDEV
#include "evdev_joystick_handler.h"
#endif

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

inline bool CreateConfigFile(const QString& dir, const QString& name)
{
	QString input_dir = qstr(fs::get_config_dir()) + "/InputConfigs/";
	if (!QDir().mkdir(input_dir) && !QDir().exists(input_dir))
	{
		LOG_ERROR(GENERAL, "Failed to create dir %s", sstr(input_dir));
		return false;
	}
	if (!QDir().mkdir(dir) && !QDir().exists(dir))
	{
		LOG_ERROR(GENERAL, "Failed to create dir %s", sstr(dir));
		return false;
	}

	QString filename = dir + name + ".yml";
	QFile new_file(filename);

	if (!new_file.open(QIODevice::WriteOnly))
	{
		LOG_ERROR(GENERAL, "Failed to create file %s", sstr(filename));
		return false;
	}

	new_file.close();
	return true;
};

custom_pad_settings_dialog::custom_pad_settings_dialog(QWidget *parent, const GameInfo *game) :
    QDialog(parent),
    ui(new Ui::custom_pad_settings_dialog)
{
    ui->setupUi(this);

	// load input config
	g_cfg_input.from_default();
	g_cfg_input.load(fs::get_config_dir() + "data/" + game->serial);
	setWindowTitle(tr("Gamepads Settings: [%0] %1").arg(qstr(game->serial)).arg(qstr(game->name)));

	// Fill input type combobox
	std::vector<std::string> str_inputs = g_cfg_input.player[0]->handler.to_list();
	for (int i = 0; i < 7; i++)
	{
		QComboBox *chooseHandler = this->findChild<QComboBox *>("chooseHandler_j" + QString::number(i));
		QComboBox *chooseDevice = this->findChild<QComboBox *>("chooseDevice_j" + QString::number(i));
		QComboBox *chooseProfile = this->findChild<QComboBox *>("chooseProfile_j" + QString::number(i));

		for (int index = 0; index < str_inputs.size(); index++)
		{
			chooseHandler->addItem(qstr(str_inputs[index]));
		}

		// Combobox: Input type
		connect(chooseHandler, &QComboBox::currentTextChanged, this, &custom_pad_settings_dialog::ChangeInputType);

		// Combobox: Devices
		connect(chooseDevice, &QComboBox::currentTextChanged, this, &custom_pad_settings_dialog::ChangeDeviceType);

		// Combobox: Profiles
		connect(chooseProfile, &QComboBox::currentTextChanged,  this, &custom_pad_settings_dialog::ChangeProfileBox);
	}

	// Cancel Button
	connect(ui->b_cancel, &QAbstractButton::clicked, this, &custom_pad_settings_dialog::CancelExit);

	// Save Button
	connect(ui->b_ok, &QAbstractButton::clicked, this, &custom_pad_settings_dialog::SaveExit);

	// Refresh Button
	connect(ui->b_refresh, &QPushButton::clicked, this, &custom_pad_settings_dialog::RefreshInputTypes);

	RefreshInputTypes();

	show();
}

custom_pad_settings_dialog::~custom_pad_settings_dialog()
{
    delete ui;
}

void custom_pad_settings_dialog::ChangeInputType()
{
	bool force_enable = false; // enable configs even with disconnected devices
	QComboBox *chooseHandler = qobject_cast<QComboBox*>(sender());
	const int player = std::atoi(&chooseHandler->objectName().toStdString().back());
	QComboBox *chooseDevice = this->findChild<QComboBox *>("chooseDevice_j" + QString::number(player));
	QComboBox *chooseProfile = this->findChild<QComboBox *>("chooseProfile_j" + QString::number(player));

	const std::string handler = sstr(chooseHandler->currentText());
	const std::string device = g_cfg_input.player[player]->device.to_string();
	const std::string profile = g_cfg_input.player[player]->profile.to_string();

	// Change this player's current handler
	if (!g_cfg_input.player[player]->handler.from_string(handler))
	{
		// Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert input string: %s", handler);
		return;
	}

	chooseDevice->clear();
	chooseProfile->clear();

	// Get this player's current handler and it's currently available devices
	std::shared_ptr<PadHandlerBase> current_handler = GetHandler(g_cfg_input.player[player]->handler);
	const std::vector<std::string> list_devices = current_handler->ListDevices();

	// Refill the device combobox with currently available devices
	switch (current_handler->m_type)
	{
#ifdef _WIN32
	case pad_handler::xinput:
	{
		const QString name_string = qstr(current_handler->name_string());
		for (int i = 0; i < current_handler->max_devices(); i++)
		{
			chooseDevice->addItem(name_string + QString::number(i), i);
		}
		force_enable = true;
		break;
	}
#endif
	default:
	{
		for (int i = 0; i < list_devices.size(); i++)
		{
			chooseDevice->addItem(qstr(list_devices[i]), i);
		}
		break;
	}
	}

	// Handle empty device list
	bool config_enabled = force_enable || (current_handler->m_type != pad_handler::null && list_devices.size() > 0);
	chooseDevice->setEnabled(config_enabled);

	if (config_enabled)
	{
		chooseDevice->setCurrentText(qstr(device));

		QString profile_dir = qstr(PadHandlerBase::get_config_dir(current_handler->m_type));
		QStringList profiles = gui::utils::get_dir_entries(QDir(profile_dir), QStringList() << "*.yml");

		if (profiles.isEmpty())
		{
			QString def_name = "Default Profile";
			if (CreateConfigFile(profile_dir, def_name))
			{
				chooseProfile->addItem(def_name);
			}
			else
			{
				config_enabled = false;
			}
		}
		else
		{
			for (const auto& prof : profiles)
			{
				chooseProfile->addItem(prof);
			}
			chooseProfile->setCurrentText(qstr(profile));
		}
	}
	else
	{
		chooseProfile->addItem(tr("No Profiles"));
		chooseDevice->addItem(tr("No Device Detected"), -1);
	}

	// enable configuration and profile list if possible
	chooseProfile->setEnabled(config_enabled);
}

void custom_pad_settings_dialog::ChangeDeviceType(const QString& dev)
{
	if (dev.isEmpty())
	{
		return;
	}
	std::string device_name = sstr(dev);
	const int player = std::atoi(&sender()->objectName().toStdString().back());
	if (!g_cfg_input.player[player]->device.from_string(device_name))
	{
		// Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert device string: %s", device_name);
		return;
	}
}

void custom_pad_settings_dialog::ChangeProfileBox(const QString& prof)
{
	if (prof.isEmpty())
	{
		return;
	}
	std::string profile = sstr(prof);
	const int player = std::atoi(&sender()->objectName().toStdString().back());
	if (!g_cfg_input.player[player]->profile.from_string(profile))
	{
		// Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert profile string: %s", profile);
		return;
	}
}

void custom_pad_settings_dialog::RefreshInputTypes()
{
	// Set the current input type from config.
	for (int i = 0; i < 7; i++)
	{
		QComboBox *chooseHandler = this->findChild<QComboBox *>("chooseHandler_j" + QString::number(i));
		chooseHandler->setCurrentText(qstr(g_cfg_input.player[i]->handler.to_string()));
	}
}

void custom_pad_settings_dialog::SaveExit()
{
	// Check for invalid selection
	for (int i = 0; i < 7; i++)
	{
		QComboBox *chooseDevice = this->findChild<QComboBox *>("chooseDevice_j" + QString::number(i));
		if (!chooseDevice->isEnabled() || chooseDevice->currentIndex() < 0)
		{
			g_cfg_input.player[i]->handler.from_default();
			g_cfg_input.player[i]->device.from_default();
			g_cfg_input.player[i]->profile.from_default();
		}
	}

	g_cfg_input.save();

	QDialog::accept();
}

void custom_pad_settings_dialog::CancelExit()
{
	// Reloads config from file or defaults
	g_cfg_input.from_default();
	g_cfg_input.load();

	QDialog::reject();
}

std::shared_ptr<PadHandlerBase> custom_pad_settings_dialog::GetHandler(pad_handler type)
{
	std::shared_ptr<PadHandlerBase> ret_handler;

	switch (type)
	{
	case pad_handler::null:
		ret_handler = std::make_unique<NullPadHandler>();
		break;
	case pad_handler::keyboard:
		ret_handler = std::make_unique<keyboard_pad_handler>();
		break;
	case pad_handler::ds4:
		ret_handler = std::make_unique<ds4_pad_handler>();
		break;
#ifdef _WIN32
	case pad_handler::xinput:
		ret_handler = std::make_unique<xinput_pad_handler>();
		break;
	case pad_handler::mm:
		ret_handler = std::make_unique<mm_joystick_handler>();
		break;
#endif
#ifdef HAVE_LIBEVDEV
	case pad_handler::evdev:
		ret_handler = std::make_unique<evdev_joystick_handler>();
		break;
#endif
	}

	return ret_handler;
}