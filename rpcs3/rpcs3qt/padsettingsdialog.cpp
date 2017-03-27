#ifdef QT_UI

#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "padsettingsdialog.h"

PadSettingsDialog::PadSettingsDialog(QWidget *parent) : QDialog(parent)
{
    // Left Analog Stick
    QGroupBox *roundStickL = new QGroupBox(tr("Left Analog Stick"));

    // D-Pad
    QGroupBox *roundPadControls = new QGroupBox(tr("D-Pad"));

    // Left Shifts
    QGroupBox *roundPadShiftsL = new QGroupBox(tr("Left Shifts"));
    QGroupBox *roundPadL1 = new QGroupBox(tr("L1"));
    QGroupBox *roundPadL2 = new QGroupBox(tr("L2"));
    QGroupBox *roundPadL3 = new QGroupBox(tr("L3"));

    QVBoxLayout *roundPadShiftsLVbox = new QVBoxLayout;
    roundPadShiftsLVbox->addWidget(roundPadL1);
    roundPadShiftsLVbox->addWidget(roundPadL2);
    roundPadShiftsLVbox->addWidget(roundPadL3);
    roundPadShiftsL->setLayout(roundPadShiftsLVbox);

    // Start / Select
    QGroupBox *roundPadSystem = new QGroupBox(tr("System"));
    QGroupBox *roundPadSelect = new QGroupBox(tr("Select"));
    QGroupBox *roundPadStart = new QGroupBox(tr("Start"));

    QVBoxLayout *roundPadSystemVbox = new QVBoxLayout;
    roundPadSystemVbox->addWidget(roundPadSelect);
    roundPadSystemVbox->addWidget(roundPadStart);
    roundPadSystem->setLayout(roundPadSystemVbox);

    // Right Shifts
    QGroupBox *roundPadShiftsR = new QGroupBox(tr("Right Shifts"));
    QGroupBox *roundPadR1 = new QGroupBox(tr("R1"));
    QGroupBox *roundPadR2 = new QGroupBox(tr("R2"));
    QGroupBox *roundPadR3 = new QGroupBox(tr("R3"));

    QVBoxLayout *roundPadShiftsRVbox = new QVBoxLayout;
    roundPadShiftsRVbox->addWidget(roundPadR1);
    roundPadShiftsRVbox->addWidget(roundPadR2);
    roundPadShiftsRVbox->addWidget(roundPadR3);
    roundPadShiftsR->setLayout(roundPadShiftsRVbox);

    // Action buttons
    QGroupBox *roundPadButtons = new QGroupBox(tr("Buttons"));
    QGroupBox *roundPadSquare = new QGroupBox(tr("Square"));
    QGroupBox *roundPadCross = new QGroupBox(tr("Cross"));
    QGroupBox *roundPadCircle = new QGroupBox(tr("Circle"));
    QGroupBox *roundPadTriangle = new QGroupBox(tr("Triangle"));

    QHBoxLayout *roundPadButtonsHbox = new QHBoxLayout;
    roundPadButtonsHbox->addWidget(roundPadSquare);
    roundPadButtonsHbox->addWidget(roundPadCircle);

    QVBoxLayout *roundPadButtonsVbox = new QVBoxLayout;
    roundPadButtonsVbox->addWidget(roundPadTriangle);
    roundPadButtonsVbox->addLayout(roundPadButtonsHbox);
    roundPadButtonsVbox->addWidget(roundPadCross);
    roundPadButtons->setLayout(roundPadButtonsVbox);

    // Right Analog Stick
    QGroupBox *roundStickR = new QGroupBox(tr("Right Analog Stick"));

    // Buttons
    QPushButton *defaultButton = new QPushButton(tr("By default"));

    QPushButton *okButton = new QPushButton(tr("OK"));
    connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);

    QPushButton *cancelButton = new QPushButton(tr("Cancel"));
    cancelButton->setDefault(true);
    connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);

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
    hbox2->addWidget(defaultButton);
    hbox2->addStretch();
    hbox2->addWidget(okButton);
    hbox2->addWidget(cancelButton);

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addLayout(hbox1);
    vbox->addLayout(hbox2);
    setLayout(vbox);

    setWindowTitle(tr("PAD Settings"));
}

#endif // QT_UI
