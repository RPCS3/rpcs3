#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "coretab.h"
#include "graphicstab.h"
#include "audiotab.h"
#include "inputtab.h"
#include "misctab.h"
#include "networkingtab.h"
#include "systemtab.h"
#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    tabWidget = new QTabWidget;
    tabWidget->setFixedSize(450, 400);
    tabWidget->addTab(new CoreTab(this), tr("Core"));
    tabWidget->addTab(new GraphicsTab(this), tr("Graphics"));
    tabWidget->addTab(new AudioTab(this), tr("Audio"));
    tabWidget->addTab(new InputTab(this), tr("Input / Output"));
    tabWidget->addTab(new MiscTab(this), tr("Misc"));
    tabWidget->addTab(new NetworkingTab(this), tr("Networking"));
    tabWidget->addTab(new SystemTab(this), tr("System"));

    QPushButton *okButton = new QPushButton(tr("OK"));

    QPushButton *closeButton = new QPushButton(tr("Close"));
    connect(closeButton, &QAbstractButton::clicked, this, &QWidget::close);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Settings"));
}
