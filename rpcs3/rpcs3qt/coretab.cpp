#ifdef QT_UI

#include <QCheckBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>

#include "coretab.h"

CoreTab::CoreTab(QWidget *parent) : QWidget(parent)
{
    // PPU Decoder
    QGroupBox *ppuDecoder = new QGroupBox(tr("PPU Decoder"), this);

    QRadioButton *ppuRadio1 = new QRadioButton(tr("Interpreter (precise)"), this);
    QRadioButton *ppuRadio2 = new QRadioButton(tr("Interpreter (fast)"), this);
    QRadioButton *ppuRadio3 = new QRadioButton(tr("Recompiler (LLVM)"), this);

    QVBoxLayout *ppuVbox = new QVBoxLayout(this);
    ppuVbox->addWidget(ppuRadio1);
    ppuVbox->addWidget(ppuRadio2);
    ppuVbox->addWidget(ppuRadio3);
    ppuDecoder->setLayout(ppuVbox);

    // SPU Decoder
    QGroupBox *spuDecoder = new QGroupBox(tr("SPU Decoder"), this);

    QRadioButton *spuRadio1 = new QRadioButton(tr("Interpreter (precise)"), this);
    QRadioButton *spuRadio2 = new QRadioButton(tr("Interpreter (fast)"), this);
    QRadioButton *spuRadio3 = new QRadioButton(tr("Recompiler (ASMJIT)"), this);
    QRadioButton *spuRadio4 = new QRadioButton(tr("Recompiler (LLVM)"), this);
    spuRadio4->setEnabled(false); // TODO

    QVBoxLayout *spuVbox = new QVBoxLayout(this);
    spuVbox->addWidget(spuRadio1);
    spuVbox->addWidget(spuRadio2);
    spuVbox->addWidget(spuRadio3);
    spuVbox->addWidget(spuRadio4);
    spuDecoder->setLayout(spuVbox);

    // Checkboxes
    QCheckBox *hookStFunc = new QCheckBox(tr("Hook static functions"), this);
    QCheckBox *loadLiblv2 = new QCheckBox(tr("Load liblv2.sprx only"), this);

    // Load libraries
    QGroupBox *coreLle = new QGroupBox(tr("Load libraries"), this);

    QListWidget *lleList = new QListWidget(this);
    searchBox = new QLineEdit(this);
    connect(searchBox, &QLineEdit::textChanged, this, &CoreTab::OnSearchBoxTextChanged);

    QVBoxLayout *lleVbox = new QVBoxLayout;
    lleVbox->addWidget(lleList);
    lleVbox->addWidget(searchBox);
    coreLle->setLayout(lleVbox);

    // Main layout
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(ppuDecoder);
    vbox->addWidget(spuDecoder);
    vbox->addWidget(hookStFunc);
    vbox->addWidget(loadLiblv2);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->addLayout(vbox);
    hbox->addWidget(coreLle);
    setLayout(hbox);
}

void CoreTab::OnSearchBoxTextChanged()
{

}

#endif // QT_UI
