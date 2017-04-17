#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QButtonGroup>

#include "stdafx.h"
#include "padsettingsdialog.h"

// TODO: rewrite with std::chrono or wxTimer
#include <time.h>

#define PADBUTTONWIDTH 50

PadSettingsDialog::PadSettingsDialog(QWidget *parent) : QDialog(parent)
{
    // Left Analog Stick
	QGroupBox *roundStickL = new QGroupBox(tr("Left Analog Stick"));
	QVBoxLayout *roundStickLVBox = new QVBoxLayout;
	QHBoxLayout *roundStickLHBox1 = new QHBoxLayout;
	QHBoxLayout *roundStickLHBox2 = new QHBoxLayout;
	QHBoxLayout *roundStickLHBox3 = new QHBoxLayout;
	b_up_lstick = new QPushButton(tr("W"));
	b_left_lstick = new QPushButton(tr("A"));
	b_right_lstick = new QPushButton(tr("D"));
	b_down_lstick = new QPushButton(tr("S"));
	roundStickLHBox1->addWidget(b_up_lstick);
	roundStickLHBox2->addWidget(b_left_lstick);
	roundStickLHBox2->addWidget(b_right_lstick);
	roundStickLHBox3->addWidget(b_down_lstick);
	roundStickLVBox->addLayout(roundStickLHBox1);
	roundStickLVBox->addLayout(roundStickLHBox2);
	roundStickLVBox->addLayout(roundStickLHBox3);
	roundStickL->setLayout(roundStickLVBox);
	b_up_lstick->setFixedWidth(PADBUTTONWIDTH);
	b_left_lstick->setFixedWidth(PADBUTTONWIDTH);
	b_right_lstick->setFixedWidth(PADBUTTONWIDTH);
	b_down_lstick->setFixedWidth(PADBUTTONWIDTH);

    // D-Pad
    QGroupBox *roundPadControls = new QGroupBox(tr("D-Pad"));
	QVBoxLayout *roundPadControlsVBox = new QVBoxLayout;
	QHBoxLayout *roundPadControlsHBox1 = new QHBoxLayout;
	QHBoxLayout *roundPadControlsHBox2 = new QHBoxLayout;
	QHBoxLayout *roundPadControlsHBox3 = new QHBoxLayout;
	b_up = new QPushButton(tr("Up"));
	b_left = new QPushButton(tr("Left"));
	b_right = new QPushButton(tr("Right"));
	b_down = new QPushButton(tr("Down"));
	roundPadControlsHBox1->addWidget(b_up);
	roundPadControlsHBox2->addWidget(b_left);
	roundPadControlsHBox2->addWidget(b_right);
	roundPadControlsHBox3->addWidget(b_down);
	roundPadControlsVBox->addLayout(roundPadControlsHBox1);
	roundPadControlsVBox->addLayout(roundPadControlsHBox2);
	roundPadControlsVBox->addLayout(roundPadControlsHBox3);
	roundPadControls->setLayout(roundPadControlsVBox);
	b_up->setFixedWidth(PADBUTTONWIDTH);
	b_left->setFixedWidth(PADBUTTONWIDTH);
	b_right->setFixedWidth(PADBUTTONWIDTH);
	b_down->setFixedWidth(PADBUTTONWIDTH);
	
	// Left Shifts
	QGroupBox *roundPadShiftsL = new QGroupBox(tr("Left Shifts"));
	QGroupBox *roundPadL1 = new QGroupBox(tr("L1"));
	QGroupBox *roundPadL2 = new QGroupBox(tr("L2"));
	QGroupBox *roundPadL3 = new QGroupBox(tr("L3"));
	QVBoxLayout *roundPadShiftsLVbox = new QVBoxLayout;
	QVBoxLayout *roundPadL1Vbox = new QVBoxLayout;
	QVBoxLayout *roundPadL2Vbox = new QVBoxLayout;
	QVBoxLayout *roundPadL3Vbox = new QVBoxLayout;
	b_shift_l1 = new QPushButton(tr("Q"));
	b_shift_l2 = new QPushButton(tr("R"));
	b_shift_l3 = new QPushButton(tr("F"));
	roundPadL1Vbox->addWidget(b_shift_l1);
	roundPadL2Vbox->addWidget(b_shift_l2);
	roundPadL3Vbox->addWidget(b_shift_l3);
	roundPadL1->setLayout(roundPadL1Vbox);
	roundPadL2->setLayout(roundPadL2Vbox);
	roundPadL3->setLayout(roundPadL3Vbox);
	roundPadShiftsLVbox->addWidget(roundPadL1);
	roundPadShiftsLVbox->addWidget(roundPadL2);
	roundPadShiftsLVbox->addWidget(roundPadL3);
	roundPadShiftsL->setLayout(roundPadShiftsLVbox);
	b_shift_l1->setFixedWidth(PADBUTTONWIDTH);
	b_shift_l2->setFixedWidth(PADBUTTONWIDTH);
	b_shift_l3->setFixedWidth(PADBUTTONWIDTH);

    // Start / Select
    QGroupBox *roundPadSystem = new QGroupBox(tr("System"));
    QGroupBox *roundPadSelect = new QGroupBox(tr("Select"));
    QGroupBox *roundPadStart = new QGroupBox(tr("Start"));
	QVBoxLayout *roundPadSystemVbox = new QVBoxLayout;
	QVBoxLayout *roundPadSelectVbox = new QVBoxLayout;
	QVBoxLayout *roundPadStartVbox = new QVBoxLayout;
	b_select = new QPushButton(tr("Space"));
	b_start = new QPushButton(tr("Enter"));
	roundPadSelectVbox->addWidget(b_select);
	roundPadStartVbox->addWidget(b_start);
	roundPadSelect->setLayout(roundPadSelectVbox);
	roundPadStart->setLayout(roundPadStartVbox);
	roundPadSystemVbox->addWidget(roundPadSelect);
	roundPadSystemVbox->addWidget(roundPadStart);
	roundPadSystem->setLayout(roundPadSystemVbox);
	b_select->setFixedWidth(PADBUTTONWIDTH);
	b_start->setFixedWidth(PADBUTTONWIDTH);

    // Right Shifts
    QGroupBox *roundPadShiftsR = new QGroupBox(tr("Right Shifts"));
    QGroupBox *roundPadR1 = new QGroupBox(tr("R1"));
    QGroupBox *roundPadR2 = new QGroupBox(tr("R2"));
    QGroupBox *roundPadR3 = new QGroupBox(tr("R3"));
	QVBoxLayout *roundPadShiftsRVbox = new QVBoxLayout;
	QVBoxLayout *roundPadR1Vbox = new QVBoxLayout;
	QVBoxLayout *roundPadR2Vbox = new QVBoxLayout;
	QVBoxLayout *roundPadR3Vbox = new QVBoxLayout;
	b_shift_r1 = new QPushButton(tr("E"));
	b_shift_r2 = new QPushButton(tr("T"));
	b_shift_r3 = new QPushButton(tr("G"));
	roundPadR1Vbox->addWidget(b_shift_r1);
	roundPadR2Vbox->addWidget(b_shift_r2);
	roundPadR3Vbox->addWidget(b_shift_r3);
	roundPadR1->setLayout(roundPadR1Vbox);
	roundPadR2->setLayout(roundPadR2Vbox);
	roundPadR3->setLayout(roundPadR3Vbox);
    roundPadShiftsRVbox->addWidget(roundPadR1);
    roundPadShiftsRVbox->addWidget(roundPadR2);
    roundPadShiftsRVbox->addWidget(roundPadR3);
    roundPadShiftsR->setLayout(roundPadShiftsRVbox);
	b_shift_r1->setFixedWidth(PADBUTTONWIDTH);
	b_shift_r2->setFixedWidth(PADBUTTONWIDTH);
	b_shift_r3->setFixedWidth(PADBUTTONWIDTH);

    // Action buttons
	QGroupBox *roundPadButtons = new QGroupBox(tr("Buttons"));
	QGroupBox *roundPadSquare = new QGroupBox(tr("Square"));
	QGroupBox *roundPadCross = new QGroupBox(tr("Cross"));
	QGroupBox *roundPadCircle = new QGroupBox(tr("Circle"));
	QGroupBox *roundPadTriangle = new QGroupBox(tr("Triangle"));
	QVBoxLayout *roundPadButtonsVBox = new QVBoxLayout;
	QHBoxLayout *roundPadButtonsHBox1 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsHBox2 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsHBox3 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsVBox21 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsVBox22 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsVBox23 = new QHBoxLayout;
	QHBoxLayout *roundPadButtonsVBox24 = new QHBoxLayout;
	b_triangle = new QPushButton(tr("V"));
	b_square = new QPushButton(tr("Z"));
	b_circle = new QPushButton(tr("C"));
	b_cross = new QPushButton(tr("X"));
	roundPadButtonsVBox21->addWidget(b_triangle);
	roundPadButtonsVBox22->addWidget(b_square);
	roundPadButtonsVBox23->addWidget(b_circle);
	roundPadButtonsVBox24->addWidget(b_cross);
	roundPadTriangle->setLayout(roundPadButtonsVBox21);
	roundPadSquare->setLayout(roundPadButtonsVBox22);
	roundPadCircle->setLayout(roundPadButtonsVBox23);
	roundPadCross->setLayout(roundPadButtonsVBox24);
	roundPadButtonsHBox1->addWidget(roundPadTriangle);
	roundPadButtonsHBox2->addWidget(roundPadSquare);
	roundPadButtonsHBox2->addWidget(roundPadCircle);
	roundPadButtonsHBox3->addWidget(roundPadCross);
	roundPadButtonsVBox->addLayout(roundPadButtonsHBox1);
	roundPadButtonsVBox->addLayout(roundPadButtonsHBox2);
	roundPadButtonsVBox->addLayout(roundPadButtonsHBox3);
	roundPadButtons->setLayout(roundPadButtonsVBox);
	b_triangle->setFixedWidth(PADBUTTONWIDTH);
	b_square->setFixedWidth(PADBUTTONWIDTH);
	b_circle->setFixedWidth(PADBUTTONWIDTH);
	b_cross->setFixedWidth(PADBUTTONWIDTH);

    // Right Analog Stick
    QGroupBox *roundStickR = new QGroupBox(tr("Right Analog Stick"));
	QVBoxLayout *roundStickRVBox = new QVBoxLayout;
	QHBoxLayout *roundStickRHBox1 = new QHBoxLayout;
	QHBoxLayout *roundStickRHBox2 = new QHBoxLayout;
	QHBoxLayout *roundStickRHBox3 = new QHBoxLayout;
	b_up_rstick = new QPushButton(tr("PgUp"));
	b_left_rstick = new QPushButton(tr("Home"));
	b_right_rstick = new QPushButton(tr("End"));
	b_down_rstick = new QPushButton(tr("PgDown"));
	roundStickRHBox1->addWidget(b_up_rstick);
	roundStickRHBox2->addWidget(b_left_rstick);
	roundStickRHBox2->addWidget(b_right_rstick);
	roundStickRHBox3->addWidget(b_down_rstick);
	roundStickRVBox->addLayout(roundStickRHBox1);
	roundStickRVBox->addLayout(roundStickRHBox2);
	roundStickRVBox->addLayout(roundStickRHBox3);
	roundStickR->setLayout(roundStickRVBox);
	b_up_rstick->setFixedWidth(PADBUTTONWIDTH);
	b_left_rstick->setFixedWidth(PADBUTTONWIDTH);
	b_right_rstick->setFixedWidth(PADBUTTONWIDTH);
	b_down_rstick->setFixedWidth(PADBUTTONWIDTH);

    // Buttons
    b_reset = new QPushButton(tr("By default"));

    b_ok = new QPushButton(tr("OK"));

    b_cancel = new QPushButton(tr("Cancel"));
	b_cancel->setDefault(true);
    connect(b_cancel, &QAbstractButton::clicked, this, &QWidget::close);

	// Handling
	QButtonGroup *padButtons = new QButtonGroup(this);
	padButtons->addButton(b_left_lstick, 1);
	padButtons->addButton(b_down_lstick, 2);
	padButtons->addButton(b_right_lstick, 3);
	padButtons->addButton(b_up_lstick, 4);
	
	padButtons->addButton(b_left, 5);
	padButtons->addButton(b_down, 6);
	padButtons->addButton(b_right, 7);
	padButtons->addButton(b_up, 8);
	
	padButtons->addButton(b_shift_l1, 9);
	padButtons->addButton(b_shift_l2, 10);
	padButtons->addButton(b_shift_l3, 11);
	
	padButtons->addButton(b_start, 12);
	padButtons->addButton(b_select, 13);
	
	padButtons->addButton(b_shift_r1, 14);
	padButtons->addButton(b_shift_r2, 15);
	padButtons->addButton(b_shift_r3, 16);
	
	padButtons->addButton(b_square, 17);
	padButtons->addButton(b_cross, 18);
	padButtons->addButton(b_circle, 19);
	padButtons->addButton(b_triangle, 20);
	
	padButtons->addButton(b_left_rstick, 21);
	padButtons->addButton(b_down_rstick, 22);
	padButtons->addButton(b_right_rstick, 23);
	padButtons->addButton(b_up_rstick, 24);
	
	padButtons->addButton(b_reset, 25);
	padButtons->addButton(b_ok, 26);
	padButtons->addButton(b_cancel, 27);

	connect(padButtons, SIGNAL(buttonClicked(int)), this, SLOT(OnPadButtonClicked(int)));

	// Main layout
	QHBoxLayout *hbox1 = new QHBoxLayout;
	hbox1->addWidget(roundStickL);
	hbox1->addWidget(roundPadControls);
	hbox1->addWidget(roundPadShiftsL);
	hbox1->addWidget(roundPadSystem);
	hbox1->addWidget(roundPadShiftsR);
	hbox1->addWidget(roundPadButtons);
	hbox1->addWidget(roundStickR);

	QHBoxLayout *hbox2 = new QHBoxLayout;
	hbox2->addWidget(b_reset);
	hbox2->addStretch();
	hbox2->addWidget(b_ok);
	hbox2->addWidget(b_cancel);

	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addLayout(hbox1);
	vbox->addLayout(hbox2);
	setLayout(vbox);

	setWindowTitle(tr("Input Settings"));
}

void PadSettingsDialog::keyPressEvent(QKeyEvent *keyEvent)
{
	m_key_pressed = true;

	switch (m_button_id)
	{
	case id_pad_lstick_left: g_kbpad_config.left_stick_left = keyEvent->key(); break;
	case id_pad_lstick_down: g_kbpad_config.left_stick_down = keyEvent->key(); break;
	case id_pad_lstick_right: g_kbpad_config.left_stick_right = keyEvent->key(); break;
	case id_pad_lstick_up: g_kbpad_config.left_stick_up = keyEvent->key(); break;
	
	case id_pad_left: g_kbpad_config.left = keyEvent->key(); break;
	case id_pad_down: g_kbpad_config.down = keyEvent->key(); break;
	case id_pad_right: g_kbpad_config.right = keyEvent->key(); break;
	case id_pad_up: g_kbpad_config.up = keyEvent->key(); break;
	
	case id_pad_l1: g_kbpad_config.l1 = keyEvent->key(); break;
	case id_pad_l2: g_kbpad_config.l2 = keyEvent->key(); break;
	case id_pad_l3: g_kbpad_config.l3 = keyEvent->key(); break;
	
	case id_pad_start: g_kbpad_config.start = keyEvent->key(); break;
	case id_pad_select: g_kbpad_config.select = keyEvent->key(); break;
	
	case id_pad_r1: g_kbpad_config.r1 = keyEvent->key(); break;
	case id_pad_r2: g_kbpad_config.r2 = keyEvent->key(); break;
	case id_pad_r3: g_kbpad_config.r3 = keyEvent->key(); break;
	
	case id_pad_square: g_kbpad_config.square = keyEvent->key(); break;
	case id_pad_cross: g_kbpad_config.cross = keyEvent->key(); break;
	case id_pad_circle: g_kbpad_config.circle = keyEvent->key(); break;
	case id_pad_triangle: g_kbpad_config.triangle = keyEvent->key(); break;
	
	case id_pad_rstick_left: g_kbpad_config.right_stick_left = keyEvent->key(); break;
	case id_pad_rstick_down: g_kbpad_config.right_stick_down = keyEvent->key(); break;
	case id_pad_rstick_right: g_kbpad_config.right_stick_right = keyEvent->key(); break;
	case id_pad_rstick_up: g_kbpad_config.right_stick_up = keyEvent->key(); break;

	case 0: break;
	default: LOG_ERROR(HLE, "Unknown button ID: %d", m_button_id); break;
	}

	SwitchButtons(true);  // enable all buttons
	m_button_id = 0; // reset current button id
	m_key_pressed = false;
	UpdateLabel();
}

void PadSettingsDialog::UpdateLabel()
{
	// Get button labels from .ini
	b_up_lstick->setText(GetKeyName(g_kbpad_config.left_stick_up));
	b_down_lstick->setText(GetKeyName(g_kbpad_config.left_stick_down));
	b_left_lstick->setText(GetKeyName(g_kbpad_config.left_stick_left));
	b_right_lstick->setText(GetKeyName(g_kbpad_config.left_stick_right));
	
	b_up->setText(GetKeyName(g_kbpad_config.up));
	b_down->setText(GetKeyName(g_kbpad_config.down));
	b_left->setText(GetKeyName(g_kbpad_config.left));
	b_right->setText(GetKeyName(g_kbpad_config.right));
	
	b_shift_l1->setText(GetKeyName(g_kbpad_config.l1));
	b_shift_l2->setText(GetKeyName(g_kbpad_config.l2));
	b_shift_l3->setText(GetKeyName(g_kbpad_config.l3));
	
	b_start->setText(GetKeyName(g_kbpad_config.start));
	b_select->setText(GetKeyName(g_kbpad_config.select));
	
	b_shift_r1->setText(GetKeyName(g_kbpad_config.r1));
	b_shift_r2->setText(GetKeyName(g_kbpad_config.r2));
	b_shift_r3->setText(GetKeyName(g_kbpad_config.r3));
	
	b_square->setText(GetKeyName(g_kbpad_config.square));
	b_cross->setText(GetKeyName(g_kbpad_config.cross));
	b_circle->setText(GetKeyName(g_kbpad_config.circle));
	b_triangle->setText(GetKeyName(g_kbpad_config.triangle));
	
	b_up_rstick->setText(GetKeyName(g_kbpad_config.right_stick_up));
	b_down_rstick->setText(GetKeyName(g_kbpad_config.right_stick_down));
	b_left_rstick->setText(GetKeyName(g_kbpad_config.right_stick_left));
	b_right_rstick->setText(GetKeyName(g_kbpad_config.right_stick_right));
}

void PadSettingsDialog::ResetParameters()
{
	g_kbpad_config.left_stick_up = g_kbpad_config.left_stick_up.def;
	g_kbpad_config.left_stick_down = g_kbpad_config.left_stick_down.def;
	g_kbpad_config.left_stick_left = g_kbpad_config.left_stick_left.def;
	g_kbpad_config.left_stick_right = g_kbpad_config.left_stick_right.def;
	
	g_kbpad_config.up = g_kbpad_config.up.def;
	g_kbpad_config.down = g_kbpad_config.down.def;
	g_kbpad_config.left = g_kbpad_config.left.def;
	g_kbpad_config.right = g_kbpad_config.right.def;
	
	g_kbpad_config.l1 = g_kbpad_config.l1.def;
	g_kbpad_config.l2 = g_kbpad_config.l2.def;
	g_kbpad_config.l3 = g_kbpad_config.l3.def;
	
	g_kbpad_config.start = g_kbpad_config.start.def;
	g_kbpad_config.select = g_kbpad_config.select.def;
	
	g_kbpad_config.r1 = g_kbpad_config.r1.def;
	g_kbpad_config.r2 = g_kbpad_config.r2.def;
	g_kbpad_config.r3 = g_kbpad_config.r3.def;
	
	g_kbpad_config.square = g_kbpad_config.square.def;
	g_kbpad_config.cross = g_kbpad_config.cross.def;
	g_kbpad_config.circle = g_kbpad_config.circle.def;
	g_kbpad_config.triangle = g_kbpad_config.triangle.def;
	
	g_kbpad_config.right_stick_up = g_kbpad_config.right_stick_up.def;
	g_kbpad_config.right_stick_down = g_kbpad_config.right_stick_down.def;
	g_kbpad_config.right_stick_left = g_kbpad_config.right_stick_left.def;
	g_kbpad_config.right_stick_right = g_kbpad_config.right_stick_right.def;
}

void PadSettingsDialog::UpdateTimerLabel(const u32 id)
{
	// Lambda used to update label. The 47 is magical.
	auto UpdateLabel = [=](QPushButton* target) {
		target->setText(QString::number(m_seconds + 47));
	};

	switch (id)
	{
	case id_pad_lstick_left: UpdateLabel(b_left_lstick); break;
	case id_pad_lstick_down: UpdateLabel(b_down_lstick); break;
	case id_pad_lstick_right: UpdateLabel(b_right_lstick); break;
	case id_pad_lstick_up: UpdateLabel(b_up_lstick); break;

	case id_pad_left: UpdateLabel(b_left); break;
	case id_pad_down: UpdateLabel(b_down); break;
	case id_pad_right: UpdateLabel(b_right); break;
	case id_pad_up: UpdateLabel(b_up); break;

	case id_pad_l1: UpdateLabel(b_shift_l1); break;
	case id_pad_l2: UpdateLabel(b_shift_l2); break;
	case id_pad_l3: UpdateLabel(b_shift_l3); break;

	case id_pad_start: UpdateLabel(b_start); break;
	case id_pad_select: UpdateLabel(b_select); break;

	case id_pad_r1: UpdateLabel(b_shift_r1); break;
	case id_pad_r2: UpdateLabel(b_shift_r2); break;
	case id_pad_r3: UpdateLabel(b_shift_r3); break;

	case id_pad_square: UpdateLabel(b_square); break;
	case id_pad_cross: UpdateLabel(b_cross); break;
	case id_pad_circle: UpdateLabel(b_circle); break;
	case id_pad_triangle: UpdateLabel(b_triangle); break;

	case id_pad_rstick_left: UpdateLabel(b_left_rstick); break;
	case id_pad_rstick_down: UpdateLabel(b_down_rstick); break;
	case id_pad_rstick_right: UpdateLabel(b_right_rstick); break;
	case id_pad_rstick_up: UpdateLabel(b_up_rstick); break;

	default: LOG_ERROR(HLE, "Unknown button ID: %d", id); break;
	}
}

void PadSettingsDialog::SwitchButtons(const bool IsEnabled)
{
	b_up_lstick->setEnabled(IsEnabled);
	b_down_lstick->setEnabled(IsEnabled);
	b_left_lstick->setEnabled(IsEnabled);
	b_right_lstick->setEnabled(IsEnabled);

	b_up->setEnabled(IsEnabled);
	b_down->setEnabled(IsEnabled);
	b_left->setEnabled(IsEnabled);
	b_right->setEnabled(IsEnabled);

	b_shift_l1->setEnabled(IsEnabled);
	b_shift_l2->setEnabled(IsEnabled);
	b_shift_l3->setEnabled(IsEnabled);

	b_start->setEnabled(IsEnabled);
	b_select->setEnabled(IsEnabled);

	b_shift_r1->setEnabled(IsEnabled);
	b_shift_r2->setEnabled(IsEnabled);
	b_shift_r3->setEnabled(IsEnabled);

	b_square->setEnabled(IsEnabled);
	b_cross->setEnabled(IsEnabled);
	b_circle->setEnabled(IsEnabled);
	b_triangle->setEnabled(IsEnabled);

	b_up_rstick->setEnabled(IsEnabled);
	b_down_rstick->setEnabled(IsEnabled);
	b_left_rstick->setEnabled(IsEnabled);
	b_right_rstick->setEnabled(IsEnabled);

	b_ok->setEnabled(IsEnabled);
	b_cancel->setEnabled(IsEnabled);
	b_reset->setEnabled(IsEnabled);
}

void PadSettingsDialog::RunTimer(const u32 seconds, const u32 id)
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

void PadSettingsDialog::Init(const u32 max_connect)
{
	memset(&m_info, 0, sizeof(PadInfo));
	m_info.max_connect = max_connect;
	LoadSettings();
	m_info.now_connect = std::min(m_pads.size(), (size_t)max_connect);
}

void PadSettingsDialog::LoadSettings()
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

const QString PadSettingsDialog::GetKeyName(const u32 keyCode)
{
	//TODO what about numpad?
	return QKeySequence(keyCode).toString();
}

void PadSettingsDialog::OnPadButtonClicked(int id)
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
		case id_reset_parameters: ResetParameters(); UpdateLabel(); break;
		case id_ok: g_kbpad_config.save(); QDialog::accept(); break;
		case id_cancel: break;
		}
	}
}
