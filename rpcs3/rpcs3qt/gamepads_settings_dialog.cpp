#include "gamepads_settings_dialog.h"
#include "pad_settings_dialog.h"
#include "../Emu/Io/PadHandler.h"
#include "../ds4_pad_handler.h"
#ifdef _WIN32
#include "../xinput_pad_handler.h"
#include "../mm_joystick_handler.h"
#elif HAVE_LIBEVDEV
#include "../evdev_joystick_handler.h"
#endif
#include "../keyboard_pad_handler.h"
#include "../Emu/Io/Null/NullPadHandler.h"

#include <QJsonObject>
#include <QJsonDocument>
#include <QInputDialog>
#include <QMessageBox>

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

// taken from https://stackoverflow.com/a/30818424/8353754
// because size policies won't work as expected (see similar bugs in Qt bugtracker)
inline void resizeComboBoxView(QComboBox* combo)
{
	int max_width = 0;
	QFontMetrics fm(combo->font());
	for (int i = 0; i < combo->count(); ++i)
	{
		int width = fm.width(combo->itemText(i));
		if (width > max_width) max_width = width;
	}
	if (combo->view()->minimumWidth() < max_width)
	{
		// add scrollbar width and margin
		max_width += combo->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		max_width += combo->view()->autoScrollMargin();
		combo->view()->setMinimumWidth(max_width);
	}
};

gamepads_settings_dialog::gamepads_settings_dialog(QWidget* parent)
	: QDialog(parent)
{
	setWindowTitle(tr("Gamepads Settings"));

	// read tooltips from json
	QFile json_file(":/Json/tooltips.json");
	json_file.open(QIODevice::ReadOnly | QIODevice::Text);
	QJsonObject json_input = QJsonDocument::fromJson(json_file.readAll()).object().value("input").toObject();
	json_file.close();

	QVBoxLayout *dialog_layout = new QVBoxLayout();
	QHBoxLayout *all_players = new QHBoxLayout();

	g_cfg_input.from_default();
	g_cfg_input.load();

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		QGroupBox *grp_player = new QGroupBox(QString(tr("Player %1").arg(i+1)));

		QVBoxLayout *ppad_layout = new QVBoxLayout();

		co_inputtype[i] = new QComboBox();
		co_inputtype[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		co_inputtype[i]->view()->setTextElideMode(Qt::ElideNone);
#ifdef WIN32
		co_inputtype[i]->setToolTip(json_input["padHandlerBox"].toString());
#else
		co_inputtype[i]->setToolTip(json_input["padHandlerBox_Linux"].toString());
#endif
		ppad_layout->addWidget(co_inputtype[i]);

		co_deviceID[i] = new QComboBox();
		co_deviceID[i]->setEnabled(false);
		co_deviceID[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		co_deviceID[i]->view()->setTextElideMode(Qt::ElideNone);	
		ppad_layout->addWidget(co_deviceID[i]);

		co_profile[i] = new QComboBox();
		co_profile[i]->setEnabled(false);
		co_profile[i]->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
		co_profile[i]->view()->setTextElideMode(Qt::ElideNone);
		ppad_layout->addWidget(co_profile[i]);

		QHBoxLayout *button_layout = new QHBoxLayout();
		bu_new_profile[i] = new QPushButton(tr("Add Profile"));
		bu_new_profile[i]->setEnabled(false);
		bu_config[i] = new QPushButton(tr("Configure"));
		bu_config[i]->setEnabled(false);
		button_layout->setContentsMargins(0,0,0,0);
		button_layout->addWidget(bu_config[i]);
		button_layout->addWidget(bu_new_profile[i]);
		ppad_layout->addLayout(button_layout);

		grp_player->setLayout(ppad_layout);
		grp_player->setFixedSize(grp_player->sizeHint());

		// fill comboboxes after setting the groupbox's size to prevent stretch
		std::vector<std::string> str_inputs = g_cfg_input.player[0]->handler.to_list();
		for (int index = 0; index < str_inputs.size(); index++)
		{
			co_inputtype[i]->addItem(qstr(str_inputs[index]));
		}
		resizeComboBoxView(co_inputtype[i]);

		all_players->addWidget(grp_player);

		if (i == 3)
		{
			dialog_layout->addLayout(all_players);
			all_players = new QHBoxLayout();
			all_players->addStretch();
		}
	}

	all_players->addStretch();
	dialog_layout->addLayout(all_players);

	QHBoxLayout *buttons_layout = new QHBoxLayout();
	QPushButton *ok_button = new QPushButton(tr("OK"));
	QPushButton *cancel_button = new QPushButton(tr("Cancel"));
	QPushButton *refresh_button = new QPushButton(tr("Refresh"));
	buttons_layout->addWidget(ok_button);
	buttons_layout->addWidget(refresh_button);
	buttons_layout->addWidget(cancel_button);
	buttons_layout->addStretch();
	dialog_layout->addLayout(buttons_layout);

	setLayout(dialog_layout);
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	auto configure_combos = [=]
	{
		//Set the values from config
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			// No extra loops are necessary because setCurrentText does it for us
			co_inputtype[i]->setCurrentText(qstr(g_cfg_input.player[i]->handler.to_string()));
			// Device will be empty on some rare occasions, so fill them by force
			ChangeInputType(i);
		}
	};

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		connect(co_inputtype[i], &QComboBox::currentTextChanged, [=]
		{
			ChangeInputType(i);
		});
		connect(co_deviceID[i], &QComboBox::currentTextChanged, [=](const QString& dev)
		{
			std::string device = sstr(dev);
			if (!g_cfg_input.player[i]->device.from_string(device))
			{
				//Something went wrong
				LOG_ERROR(GENERAL, "Failed to convert device string: %s", device);
				return;
			}
		});
		connect(co_profile[i], &QComboBox::currentTextChanged, [=](const QString& prof)
		{
			std::string profile = sstr(prof);
			if (!g_cfg_input.player[i]->profile.from_string(profile))
			{
				//Something went wrong 
				LOG_ERROR(GENERAL, "Failed to convert profile string: %s", profile);
				return;
			}
		});
		connect(bu_config[i], &QAbstractButton::clicked, [=]
		{
			ClickConfigButton(i);
		});
		connect(bu_new_profile[i], &QAbstractButton::clicked, [=]
		{
			QInputDialog* dialog = new QInputDialog(this);
			dialog->setWindowTitle(tr("Choose a unique name"));
			dialog->setLabelText(tr("Profile Name: "));
			dialog->setFixedSize(500, 100);

			while (dialog->exec() != QDialog::Rejected)
			{
				QString friendlyName = dialog->textValue();
				if (friendlyName == "")
				{
					QMessageBox::warning(this, tr("Error"), tr("Name cannot be empty"));
					continue;
				}
				if (friendlyName.contains("."))
				{
					QMessageBox::warning(this, tr("Error"), tr("Must choose a name without '.'"));
					continue;
				}
				if (co_profile[i]->findText(friendlyName) != -1)
				{
					QMessageBox::warning(this, tr("Error"), tr("Please choose a non-existing name"));
					continue;
				}
				if (CreateConfigFile(qstr(PadHandlerBase::get_config_dir(g_cfg_input.player[i]->handler)), friendlyName))
				{
					co_profile[i]->addItem(friendlyName);
					co_profile[i]->setCurrentText(friendlyName);
				}
				break;
			}
		});
	}
	connect(ok_button, &QPushButton::pressed, this, &gamepads_settings_dialog::SaveExit);
	connect(cancel_button, &QPushButton::pressed, this, &gamepads_settings_dialog::CancelExit);
	connect(refresh_button, &QPushButton::pressed, [=] { configure_combos(); });

	configure_combos();
}

void gamepads_settings_dialog::SaveExit()
{
	//Check for invalid selection
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (co_deviceID[i]->currentData() == -1)
		{
			g_cfg_input.player[i]->handler.from_default();
			g_cfg_input.player[i]->device.from_default();
			g_cfg_input.player[i]->profile.from_default();
		}
	}

	g_cfg_input.save();

	QDialog::accept();
}

void gamepads_settings_dialog::CancelExit()
{
	//Reloads config from file or defaults
	g_cfg_input.from_default();
	g_cfg_input.load();

	QDialog::accept();
}

std::shared_ptr<PadHandlerBase> gamepads_settings_dialog::GetHandler(pad_handler type)
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
#ifdef _MSC_VER
	case pad_handler::xinput:
		ret_handler = std::make_unique<xinput_pad_handler>();
		break;
#endif
#ifdef _WIN32
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

void gamepads_settings_dialog::ChangeInputType(int player)
{
	std::string handler = sstr(co_inputtype[player]->currentText());
	std::string device = g_cfg_input.player[player]->device.to_string();
	std::string profile = g_cfg_input.player[player]->profile.to_string();

	// Change this player's current handler
	if (!g_cfg_input.player[player]->handler.from_string(handler))
	{
		//Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert input string:%s", handler);
		return;
	}

	// Get this player's current handler and it's currently available devices
	std::shared_ptr<PadHandlerBase> cur_pad_handler = GetHandler(g_cfg_input.player[player]->handler);
	std::vector<std::string> list_devices = cur_pad_handler->ListDevices();

	// Refill the device combobox with currently available devices
	co_deviceID[player]->clear();

	bool force_enable = true; // enable configs even with disconnected devices

	switch (cur_pad_handler->m_type)
	{
#ifdef _MSC_VER
	case pad_handler::xinput:
	{
		QString name_string = qstr(cur_pad_handler->name_string());
		for (int i = 0; i < cur_pad_handler->max_devices(); i++)
		{
			co_deviceID[player]->addItem(name_string + QString::number(i), i);
		}
		break;
	}
#endif
	default:
		for (int i = 0; i < list_devices.size(); i++)
		{
			co_deviceID[player]->addItem(qstr(list_devices[i]), i);
		}
		force_enable = false;
		break;
	}

	// Handle empty device list
	bool device_found = list_devices.size() > 0;
	co_deviceID[player]->setEnabled(force_enable || device_found);

	if (force_enable || device_found)
	{
		co_deviceID[player]->setCurrentText(qstr(device));
	}
	else
	{
		co_deviceID[player]->addItem(tr("No Device Detected"), -1);
	}

	bool config_enabled = force_enable || (device_found && cur_pad_handler->has_config());
	co_profile[player]->clear();

	// update profile list if possible
	if (config_enabled)
	{
		QString s_profile_dir = qstr(PadHandlerBase::get_config_dir(cur_pad_handler->m_type));
		QStringList profiles = gui_settings::GetDirEntries(QDir(s_profile_dir), QStringList() << "*.yml");

		if (profiles.isEmpty())
		{
			QString def_name = "Default Profile";
			if (!CreateConfigFile(s_profile_dir, def_name))
			{
				config_enabled = false;
			}
			else
			{
				co_profile[player]->addItem(def_name);
				co_profile[player]->setCurrentText(def_name);
			}
		}
		else
		{
			for (const auto& prof : profiles)
			{
				co_profile[player]->addItem(prof);
			}
			co_profile[player]->setCurrentText(qstr(profile));
		}
	}

	if (!config_enabled)
		co_profile[player]->addItem(tr("No Profiles"));

	// enable configuration and profile list if possible
	bu_config[player]->setEnabled(config_enabled);
	bu_new_profile[player]->setEnabled(config_enabled);
	co_profile[player]->setEnabled(config_enabled);

	// update view
	resizeComboBoxView(co_deviceID[player]);
	resizeComboBoxView(co_profile[player]);
}

void gamepads_settings_dialog::ClickConfigButton(int player)
{
	// Get this player's current handler and open its pad settings dialog
	std::shared_ptr<PadHandlerBase> cur_pad_handler = GetHandler(g_cfg_input.player[player]->handler);
	if (cur_pad_handler->has_config())
	{
		std::string device = sstr(co_deviceID[player]->currentText());
		std::string profile = sstr(co_profile[player]->currentText());
		pad_settings_dialog dlg(device, profile, cur_pad_handler);
		dlg.exec();
	}
}
