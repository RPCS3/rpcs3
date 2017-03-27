#ifdef QT_UI

#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "coretab.h"

CoreTab::CoreTab(QWidget *parent) : QWidget(parent)
{
	// PPU Decoder
	QGroupBox *ppuDecoder = new QGroupBox(tr("PPU Decoder"));

	QRadioButton *ppuRadio1 = new QRadioButton(tr("Interpreter (precise)"));
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
	QCheckBox *loadLiblv2 = new QCheckBox(tr("Load liblv2.sprx only"));

	// Load libraries
	QGroupBox *lle = new QGroupBox(tr("Load libraries"));

	lleList = new QListWidget;
	searchBox = new QLineEdit;
	connect(searchBox, &QLineEdit::textChanged, this, &CoreTab::OnSearchBoxTextChanged);

	QVBoxLayout *lleVbox = new QVBoxLayout;
	lleVbox->addWidget(lleList);
	lleVbox->addWidget(searchBox);
	lle->setLayout(lleVbox);

	// Main layout
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(ppuDecoder);
	vbox->addWidget(spuDecoder);
	vbox->addWidget(hookStFunc);
	vbox->addWidget(loadLiblv2);
	vbox->addStretch();

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addLayout(vbox);
	hbox->addWidget(lle);
	setLayout(hbox);
}

void CoreTab::OnSearchBoxTextChanged()
{
	if (searchBox->text().isEmpty())
		qDebug() << "Empty!";

	lleList->clear();

	QString searchTerm = searchBox->text().toLower();
}

#endif // QT_UI
