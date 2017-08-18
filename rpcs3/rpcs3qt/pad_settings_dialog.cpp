#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QButtonGroup>

#include "stdafx.h"
#include "pad_settings_dialog.h"
#include "ui_pad_settings_dialog.h"

// TODO: rewrite with std::chrono or QTimer
#include <time.h>

static const int PadButtonWidth = 60;

extern keyboard_pad_config g_kbpad_config;

pad_settings_dialog::pad_settings_dialog(QWidget *parent) : QDialog(parent), ui(new Ui::pad_settings_dialog)
{
	ui->setupUi(this);

	ui->b_cancel->setDefault(true);
	connect(ui->b_cancel, &QAbstractButton::clicked, this, &QWidget::close);

	// Handling
	QButtonGroup *padButtons = new QButtonGroup(this);
	padButtons->addButton(ui->b_left_lstick, 1);
	padButtons->addButton(ui->b_down_lstick, 2);
	padButtons->addButton(ui->b_right_lstick, 3);
	padButtons->addButton(ui->b_up_lstick, 4);
	
	padButtons->addButton(ui->b_left, 5);
	padButtons->addButton(ui->b_down, 6);
	padButtons->addButton(ui->b_right, 7);
	padButtons->addButton(ui->b_up, 8);
	
	padButtons->addButton(ui->b_shift_l1, 9);
	padButtons->addButton(ui->b_shift_l2, 10);
	padButtons->addButton(ui->b_shift_l3, 11);
	
	padButtons->addButton(ui->b_start, 12);
	padButtons->addButton(ui->b_select, 13);
	
	padButtons->addButton(ui->b_shift_r1, 14);
	padButtons->addButton(ui->b_shift_r2, 15);
	padButtons->addButton(ui->b_shift_r3, 16);
	
	padButtons->addButton(ui->b_square, 17);
	padButtons->addButton(ui->b_cross, 18);
	padButtons->addButton(ui->b_circle, 19);
	padButtons->addButton(ui->b_triangle, 20);
	
	padButtons->addButton(ui->b_left_rstick, 21);
	padButtons->addButton(ui->b_down_rstick, 22);
	padButtons->addButton(ui->b_right_rstick, 23);
	padButtons->addButton(ui->b_up_rstick, 24);
	
	padButtons->addButton(ui->b_reset, 25);
	padButtons->addButton(ui->b_ok, 26);
	padButtons->addButton(ui->b_cancel, 27);

	connect(padButtons, SIGNAL(buttonClicked(int)), this, SLOT(OnPadButtonClicked(int)));

	g_kbpad_config.load();
	UpdateLabel();

	ui->l_controller->setMaximumSize(ui->gb_description->sizeHint().width(), ui->l_controller->maximumHeight() * ui->gb_description->sizeHint().width() / ui->l_controller->maximumWidth());
	layout()->setSizeConstraint(QLayout::SetFixedSize);
}

pad_settings_dialog::~pad_settings_dialog()
{
	delete ui;
}

void pad_settings_dialog::keyPressEvent(QKeyEvent *keyEvent)
{
	m_key_pressed = true;

	cfg::int32* entry = nullptr;

	switch (m_button_id)
	{
	case id_pad_lstick_left: entry = &g_kbpad_config.left_stick_left; break;
	case id_pad_lstick_down: entry = &g_kbpad_config.left_stick_down; break;
	case id_pad_lstick_right: entry = &g_kbpad_config.left_stick_right; break;
	case id_pad_lstick_up: entry = &g_kbpad_config.left_stick_up; break;
	
	case id_pad_left: entry = &g_kbpad_config.left; break;
	case id_pad_down: entry = &g_kbpad_config.down; break;
	case id_pad_right: entry = &g_kbpad_config.right; break;
	case id_pad_up: entry = &g_kbpad_config.up; break;
	
	case id_pad_l1: entry = &g_kbpad_config.l1; break;
	case id_pad_l2: entry = &g_kbpad_config.l2; break;
	case id_pad_l3: entry = &g_kbpad_config.l3; break;
	
	case id_pad_start: entry = &g_kbpad_config.start; break;
	case id_pad_select: entry = &g_kbpad_config.select; break;
	
	case id_pad_r1: entry = &g_kbpad_config.r1; break;
	case id_pad_r2: entry = &g_kbpad_config.r2; break;
	case id_pad_r3: entry = &g_kbpad_config.r3; break;
	
	case id_pad_square: entry = &g_kbpad_config.square; break;
	case id_pad_cross: entry = &g_kbpad_config.cross; break;
	case id_pad_circle: entry = &g_kbpad_config.circle; break;
	case id_pad_triangle: entry = &g_kbpad_config.triangle; break;
	case id_pad_rstick_left: entry = &g_kbpad_config.right_stick_left; break;
	case id_pad_rstick_down: entry = &g_kbpad_config.right_stick_down; break;
	case id_pad_rstick_right: entry = &g_kbpad_config.right_stick_right; break;
	case id_pad_rstick_up: entry = &g_kbpad_config.right_stick_up; break;

	case 0: break;
	default: LOG_ERROR(HLE, "Unknown button ID: %d", m_button_id); break;
	}

	if (entry)
	{
		// TODO: do not modify config
		entry->from_string(std::to_string(keyEvent->key()));
	}

	SwitchButtons(true);  // enable all buttons
	m_button_id = 0; // reset current button id
	m_key_pressed = false;
	UpdateLabel();
}

void pad_settings_dialog::UpdateLabel()
{
	// Get button labels from .ini
	ui->b_up_lstick->setText(GetKeyName(g_kbpad_config.left_stick_up));
	ui->b_down_lstick->setText(GetKeyName(g_kbpad_config.left_stick_down));
	ui->b_left_lstick->setText(GetKeyName(g_kbpad_config.left_stick_left));
	ui->b_right_lstick->setText(GetKeyName(g_kbpad_config.left_stick_right));
	
	ui->b_up->setText(GetKeyName(g_kbpad_config.up));
	ui->b_down->setText(GetKeyName(g_kbpad_config.down));
	ui->b_left->setText(GetKeyName(g_kbpad_config.left));
	ui->b_right->setText(GetKeyName(g_kbpad_config.right));
	
	ui->b_shift_l1->setText(GetKeyName(g_kbpad_config.l1));
	ui->b_shift_l2->setText(GetKeyName(g_kbpad_config.l2));
	ui->b_shift_l3->setText(GetKeyName(g_kbpad_config.l3));
	
	ui->b_start->setText(GetKeyName(g_kbpad_config.start));
	ui->b_select->setText(GetKeyName(g_kbpad_config.select));
	
	ui->b_shift_r1->setText(GetKeyName(g_kbpad_config.r1));
	ui->b_shift_r2->setText(GetKeyName(g_kbpad_config.r2));
	ui->b_shift_r3->setText(GetKeyName(g_kbpad_config.r3));
	
	ui->b_square->setText(GetKeyName(g_kbpad_config.square));
	ui->b_cross->setText(GetKeyName(g_kbpad_config.cross));
	ui->b_circle->setText(GetKeyName(g_kbpad_config.circle));
	ui->b_triangle->setText(GetKeyName(g_kbpad_config.triangle));
	
	ui->b_up_rstick->setText(GetKeyName(g_kbpad_config.right_stick_up));
	ui->b_down_rstick->setText(GetKeyName(g_kbpad_config.right_stick_down));
	ui->b_left_rstick->setText(GetKeyName(g_kbpad_config.right_stick_left));
	ui->b_right_rstick->setText(GetKeyName(g_kbpad_config.right_stick_right));
}

void pad_settings_dialog::UpdateTimerLabel(const u32 id)
{
	// Lambda used to update label. The 47 is magical.
	auto UpdateLabel = [=](QPushButton* target) {
		target->setText(QString::number(m_seconds + 47));
	};

	switch (id)
	{
	case id_pad_lstick_left: UpdateLabel(ui->b_left_lstick); break;
	case id_pad_lstick_down: UpdateLabel(ui->b_down_lstick); break;
	case id_pad_lstick_right: UpdateLabel(ui->b_right_lstick); break;
	case id_pad_lstick_up: UpdateLabel(ui->b_up_lstick); break;

	case id_pad_left: UpdateLabel(ui->b_left); break;
	case id_pad_down: UpdateLabel(ui->b_down); break;
	case id_pad_right: UpdateLabel(ui->b_right); break;
	case id_pad_up: UpdateLabel(ui->b_up); break;

	case id_pad_l1: UpdateLabel(ui->b_shift_l1); break;
	case id_pad_l2: UpdateLabel(ui->b_shift_l2); break;
	case id_pad_l3: UpdateLabel(ui->b_shift_l3); break;

	case id_pad_start: UpdateLabel(ui->b_start); break;
	case id_pad_select: UpdateLabel(ui->b_select); break;

	case id_pad_r1: UpdateLabel(ui->b_shift_r1); break;
	case id_pad_r2: UpdateLabel(ui->b_shift_r2); break;
	case id_pad_r3: UpdateLabel(ui->b_shift_r3); break;

	case id_pad_square: UpdateLabel(ui->b_square); break;
	case id_pad_cross: UpdateLabel(ui->b_cross); break;
	case id_pad_circle: UpdateLabel(ui->b_circle); break;
	case id_pad_triangle: UpdateLabel(ui->b_triangle); break;

	case id_pad_rstick_left: UpdateLabel(ui->b_left_rstick); break;
	case id_pad_rstick_down: UpdateLabel(ui->b_down_rstick); break;
	case id_pad_rstick_right: UpdateLabel(ui->b_right_rstick); break;
	case id_pad_rstick_up: UpdateLabel(ui->b_up_rstick); break;

	default: LOG_ERROR(HLE, "Unknown button ID: %d", id); break;
	}
}

void pad_settings_dialog::SwitchButtons(const bool IsEnabled)
{
	ui->b_up_lstick->setEnabled(IsEnabled);
	ui->b_down_lstick->setEnabled(IsEnabled);
	ui->b_left_lstick->setEnabled(IsEnabled);
	ui->b_right_lstick->setEnabled(IsEnabled);

	ui->b_up->setEnabled(IsEnabled);
	ui->b_down->setEnabled(IsEnabled);
	ui->b_left->setEnabled(IsEnabled);
	ui->b_right->setEnabled(IsEnabled);

	ui->b_shift_l1->setEnabled(IsEnabled);
	ui->b_shift_l2->setEnabled(IsEnabled);
	ui->b_shift_l3->setEnabled(IsEnabled);

	ui->b_start->setEnabled(IsEnabled);
	ui->b_select->setEnabled(IsEnabled);

	ui->b_shift_r1->setEnabled(IsEnabled);
	ui->b_shift_r2->setEnabled(IsEnabled);
	ui->b_shift_r3->setEnabled(IsEnabled);

	ui->b_square->setEnabled(IsEnabled);
	ui->b_cross->setEnabled(IsEnabled);
	ui->b_circle->setEnabled(IsEnabled);
	ui->b_triangle->setEnabled(IsEnabled);

	ui->b_up_rstick->setEnabled(IsEnabled);
	ui->b_down_rstick->setEnabled(IsEnabled);
	ui->b_left_rstick->setEnabled(IsEnabled);
	ui->b_right_rstick->setEnabled(IsEnabled);

	ui->b_ok->setEnabled(IsEnabled);
	ui->b_cancel->setEnabled(IsEnabled);
	ui->b_reset->setEnabled(IsEnabled);
}

void pad_settings_dialog::RunTimer(const u32 seconds, const u32 id)
{
	m_seconds = seconds;
	clock_t t1, t2;
	t1 = t2 = clock() / CLOCKS_PER_SEC;
	while (m_seconds)
	{
		if (t1 / CLOCKS_PER_SEC + 1 <= (t2 = clock()) / CLOCKS_PER_SEC)
		{
			UpdateTimerLabel(id);
			m_seconds--;
			t1 = t2;
		}

		if (m_key_pressed)
		{
			m_seconds = 0;
			break;
		}
	}
}

void pad_settings_dialog::Init(const u32 max_connect)
{
	memset(&m_info, 0, sizeof(PadInfo));
	m_info.max_connect = max_connect;
	LoadSettings();
	m_info.now_connect = std::min((u32)m_pads.size(), max_connect);
}

void pad_settings_dialog::LoadSettings()
{
	g_kbpad_config.load();

	//Fixed assign change, default is both sensor and press off
	m_pads.emplace_back(
		CELL_PAD_STATUS_CONNECTED | CELL_PAD_STATUS_ASSIGN_CHANGES,
		CELL_PAD_SETTING_PRESS_OFF | CELL_PAD_SETTING_SENSOR_OFF,
		CELL_PAD_CAPABILITY_PS3_CONFORMITY | CELL_PAD_CAPABILITY_PRESS_MODE,
		CELL_PAD_DEV_TYPE_STANDARD);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.left, CELL_PAD_CTRL_LEFT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.down, CELL_PAD_CTRL_DOWN);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.right, CELL_PAD_CTRL_RIGHT);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.up, CELL_PAD_CTRL_UP);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.start, CELL_PAD_CTRL_START);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.r3, CELL_PAD_CTRL_R3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.l3, CELL_PAD_CTRL_L3);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL1, g_kbpad_config.select, CELL_PAD_CTRL_SELECT);

	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.square, CELL_PAD_CTRL_SQUARE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.cross, CELL_PAD_CTRL_CROSS);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.circle, CELL_PAD_CTRL_CIRCLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.triangle, CELL_PAD_CTRL_TRIANGLE);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r1, CELL_PAD_CTRL_R1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l1, CELL_PAD_CTRL_L1);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.r2, CELL_PAD_CTRL_R2);
	m_pads[0].m_buttons.emplace_back(CELL_PAD_BTN_OFFSET_DIGITAL2, g_kbpad_config.l2, CELL_PAD_CTRL_L2);

	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_X, g_kbpad_config.left_stick_left, g_kbpad_config.left_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_LEFT_Y, g_kbpad_config.left_stick_up, g_kbpad_config.left_stick_down);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_X, g_kbpad_config.right_stick_left, g_kbpad_config.right_stick_right);
	m_pads[0].m_sticks.emplace_back(CELL_PAD_BTN_OFFSET_ANALOG_RIGHT_Y, g_kbpad_config.right_stick_up, g_kbpad_config.right_stick_down);
}

const QString pad_settings_dialog::GetKeyName(const u32 keyCode)
{
	//TODO what about numpad?
	return QKeySequence(keyCode).toString();
}

void pad_settings_dialog::OnPadButtonClicked(int id)
{
	if (id != id_reset_parameters && id != id_ok)
	{
		m_button_id = id;
		SwitchButtons(false); // disable all buttons, needed for using Space, Enter and other specific buttons
		//RunTimer(3, event.GetId()); // TODO: Currently, timer disabled. Use by later, have some strange problems
		//SwitchButtons(true); // needed, if timer enabled
		UpdateLabel();
	}

	else
	{
		switch (id)
		{
		case id_reset_parameters: g_kbpad_config.from_default(); UpdateLabel(); break;
		case id_ok: g_kbpad_config.save(); QDialog::accept(); break;
		case id_cancel: break;
		}
	}
}
