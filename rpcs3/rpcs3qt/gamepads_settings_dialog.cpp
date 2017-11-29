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
#include "../Emu/System.h"

#include <QJsonObject>
#include <QJsonDocument>

input_config input_cfg;

inline std::string sstr(const QString& _in) { return _in.toStdString(); }
constexpr auto qstr = QString::fromStdString;

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
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QVBoxLayout *dialog_layout = new QVBoxLayout();
	QHBoxLayout *all_players = new QHBoxLayout();

	input_cfg.from_default();
	input_cfg.load();

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

		QHBoxLayout *button_layout = new QHBoxLayout();
		bu_config[i] = new QPushButton(tr("Config"));
		bu_config[i]->setEnabled(false);
		bu_config[i]->setFixedSize(bu_config[i]->sizeHint());
		button_layout->addSpacing(bu_config[i]->sizeHint().width()*0.50f);
		button_layout->addWidget(bu_config[i]);
		button_layout->addSpacing(bu_config[i]->sizeHint().width()*0.50f);
		ppad_layout->addLayout(button_layout);

		grp_player->setLayout(ppad_layout);
		grp_player->setFixedSize(grp_player->sizeHint());

		// fill comboboxes after setting the groupbox's size to prevent stretch
		std::vector<std::string> str_inputs = input_cfg.player_input[0].to_list();
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
			co_inputtype[i]->setCurrentText(qstr(input_cfg.player_input[i].to_string()));
			// Device will be empty on some rare occasions, so fill them by force
			ChangeInputType(i);
		}
	};

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		connect(co_inputtype[i], &QComboBox::currentTextChanged, [=] { ChangeInputType(i); });
		connect(co_deviceID[i], &QComboBox::currentTextChanged, [=](const QString& dev)
		{
			std::string device = sstr(dev);
			if (!input_cfg.player_device[i]->from_string(device))
			{
				//Something went wrong
				LOG_ERROR(GENERAL, "Failed to convert device string: %s", device);
				return;
			}
		});
		connect(bu_config[i], &QAbstractButton::clicked, [=] { ClickConfigButton(i); });
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
			input_cfg.player_input[i].from_default();
			input_cfg.player_device[i]->from_default();
		}
	}

	input_cfg.save();

	QDialog::accept();
}

void gamepads_settings_dialog::CancelExit()
{
	//Reloads config from file or defaults
	input_cfg.from_default();
	input_cfg.load();

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
	std::string device = input_cfg.player_device[player]->to_string();

	// Change this player's current handler
	if (!input_cfg.player_input[player].from_string(handler))
	{
		//Something went wrong
		LOG_ERROR(GENERAL, "Failed to convert input string:%s", handler);
		return;
	}

	// Get this player's current handler and it's currently available devices
	std::shared_ptr<PadHandlerBase> cur_pad_handler = GetHandler(input_cfg.player_input[player]);
	std::vector<std::string> list_devices = cur_pad_handler->ListDevices();

	// Refill the device combobox with currently available devices
	co_deviceID[player]->clear();
	for (int i = 0; i < list_devices.size(); i++)
	{
		co_deviceID[player]->addItem(qstr(list_devices[i]), i);
	}

	// Handle empty device list
	if (list_devices.size() == 0)
	{
		co_deviceID[player]->addItem(tr("No Device Detected"), -1);
		co_deviceID[player]->setEnabled(false);
	}
	else
	{
		co_deviceID[player]->setEnabled(true);
		co_deviceID[player]->setCurrentText(qstr(device));
	}

	// Update view and enable configuration if possible
	resizeComboBoxView(co_deviceID[player]);
	bu_config[player]->setEnabled(cur_pad_handler->has_config());
}

void gamepads_settings_dialog::ClickConfigButton(int player)
{
	// Get this player's current handler and open its pad settings dialog
	std::shared_ptr<PadHandlerBase> cur_pad_handler = GetHandler(input_cfg.player_input[player]);
	if (cur_pad_handler->has_config())
	{
		std::string device = sstr(co_deviceID[player]->currentText());
		pad_settings_dialog dlg(device, cur_pad_handler);
		dlg.exec();
	}
}
