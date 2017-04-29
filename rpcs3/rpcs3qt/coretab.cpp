#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include "coretab.h"

CoreTab::CoreTab(QWidget *parent) : QWidget(parent)
{
	// PPU Decoder
	QGroupBox *ppuDecoder = new QGroupBox(tr("PPU Decoder"));
	QRadioButton *ppuRadio1 = new QRadioButton(tr("Interpreter (precise)"));
	ppuRadio1->setEnabled(false); // TODO
	QRadioButton *ppuRadio2 = new QRadioButton(tr("Interpreter (fast)"));
	QRadioButton *ppuRadio3 = new QRadioButton(tr("Recompiler (LLVM)"));
	QVBoxLayout *ppuVbox = new QVBoxLayout;

	ppuVbox->addWidget(ppuRadio1);
	ppuVbox->addWidget(ppuRadio2);
	ppuVbox->addWidget(ppuRadio3);
	ppuDecoder->setLayout(ppuVbox);

	// SPU Decoder
	QGroupBox *spuDecoder = new QGroupBox(tr("SPU Decoder"));

	QRadioButton *spuRadio1 = new QRadioButton(tr("Interpreter (precise)"));
	QRadioButton *spuRadio2 = new QRadioButton(tr("Interpreter (fast)"));
	QRadioButton *spuRadio3 = new QRadioButton(tr("Recompiler (ASMJIT)"));
	QRadioButton *spuRadio4 = new QRadioButton(tr("Recompiler (LLVM)"));
	spuRadio4->setEnabled(false); // TODO

	QVBoxLayout *spuVbox = new QVBoxLayout;
	spuVbox->addWidget(spuRadio1);
	spuVbox->addWidget(spuRadio2);
	spuVbox->addWidget(spuRadio3);
	spuVbox->addWidget(spuRadio4);
	spuDecoder->setLayout(spuVbox);

	// Checkboxes
	QCheckBox *hookStFunc = new QCheckBox(tr("Hook static functions"));

	// Load libraries
	QGroupBox *lle = new QGroupBox(tr("Load libraries"));

	QButtonGroup *libModeBG = new QButtonGroup(this);
	QRadioButton* automaticRB = new QRadioButton("Auto-Load required libs (recommended)", lle);
	QRadioButton* manualRB = new QRadioButton("Manually select required libs", lle);
	QRadioButton* automanualRB = new QRadioButton("Auto-Load + manually selected", lle);
	QRadioButton* liblv2RB = new QRadioButton("Load liblv2.sprx only", lle);
	libModeBG->addButton(automaticRB);
	libModeBG->addButton(manualRB);
	libModeBG->addButton(automanualRB);
	libModeBG->addButton(liblv2RB);

	lleList = new QListWidget;
	searchBox = new QLineEdit;

	QVBoxLayout *lleVbox = new QVBoxLayout;
	lleVbox->addWidget(automaticRB);
	lleVbox->addWidget(manualRB);
	lleVbox->addWidget(automanualRB);
	lleVbox->addWidget(liblv2RB);
	lleVbox->addSpacing(5);
	lleVbox->addWidget(lleList);
	lleVbox->addWidget(searchBox);
	lle->setLayout(lleVbox);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(ppuDecoder);
	vbox->addWidget(spuDecoder);
	vbox->addWidget(hookStFunc);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addWidget(lle);
	setLayout(hbox);

	// Events
	connect(hookStFunc, SIGNAL(buttonToggled(int)), this, SLOT(OnHookButtonToggled(int)));
	connect(ppuDecoder, SIGNAL(buttonToggled(int)), this, SLOT(OnPPUDecoderToggled(int)));
	connect(spuDecoder, SIGNAL(buttonToggled(int)), this, SLOT(OnSPUDecoderToggled(int)));
	connect(libModeBG, SIGNAL(buttonToggled(int)), this, SLOT(OnLibModeToggled(int)));
	connect(searchBox, &QLineEdit::textChanged, this, &CoreTab::OnSearchBoxTextChanged);
}

void CoreTab::OnSearchBoxTextChanged()
{
	if (searchBox->text().isEmpty())

	lleList->clear();

	QString searchTerm = searchBox->text().toLower();
}

void CoreTab::OnHookButtonToggled()
{

}

void CoreTab::OnPPUDecoderToggled()
{

}

void CoreTab::OnSPUDecoderToggled()
{

}

void CoreTab::OnLibButtonToggled()
{
	
}
