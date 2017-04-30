#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

#include "misctab.h"
#include "mainwindow.h"

MiscTab::MiscTab(QWidget *parent) : QWidget(parent)
{
	// Left Widgets
	QCheckBox *exitOnStop = new QCheckBox(tr("Exit RPCS3 when process finishes"));
	QCheckBox *alwaysStart = new QCheckBox(tr("Always start after boot"));
	QCheckBox *apSystemcall = new QCheckBox(tr("Auto Pause at System Call"));
	QCheckBox *apFunctioncall = new QCheckBox(tr("Auto Pause at Function Call"));

	// Right Widgets
	QGroupBox *gb_stylesheets = new QGroupBox(tr("Stylesheets"));
	QVBoxLayout *vbox_stylesheets = new QVBoxLayout;
	QHBoxLayout *hbox_stylesheets = new QHBoxLayout;
	QComboBox *combo_stylesheets = new QComboBox();
	QPushButton *pb_apply = new QPushButton(tr("Apply"));
	QPushButton *pb_reset = new QPushButton(tr("Reset"));

	// Left layout
	QVBoxLayout *vbox_left = new QVBoxLayout;
	vbox_left->addWidget(exitOnStop);
	vbox_left->addWidget(alwaysStart);
	vbox_left->addWidget(apSystemcall);
	vbox_left->addWidget(apFunctioncall);
	vbox_left->addStretch(1);

	// Right layout
	QVBoxLayout *vbox_right = new QVBoxLayout;
	hbox_stylesheets->addWidget(pb_apply);
	hbox_stylesheets->addWidget(pb_reset);
	vbox_stylesheets->addWidget(combo_stylesheets);
	vbox_stylesheets->addLayout(hbox_stylesheets);
	gb_stylesheets->setLayout(vbox_stylesheets);
	vbox_right->addWidget(gb_stylesheets);
	vbox_right->addStretch(1);

	// Main Layout
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox_left);
	hbox->addLayout(vbox_right);
	setLayout(hbox);

	connect(pb_apply, &QAbstractButton::clicked, this, &MiscTab::OnApply);
	connect(pb_reset, &QAbstractButton::clicked, this, &MiscTab::OnReset);
	connect(combo_stylesheets, static_cast<void (QComboBox::*)(int index)>(&QComboBox::currentIndexChanged), this, &MiscTab::OnStylesheetChanged);
}

void MiscTab::OnApply()
{
	
}

void MiscTab::OnReset()
{
	
}

void MiscTab::OnStylesheetChanged()
{
	
}
