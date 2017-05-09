#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "coretab.h"
#include "graphicstab.h"
#include "audiotab.h"
#include "inputtab.h"
#include "misctab.h"
#include "GuiTab.h"
#include "networkingtab.h"
#include "systemtab.h"
#include "settingsdialog.h"
#include "EmuSettings.h"

SettingsDialog::SettingsDialog(std::shared_ptr<GuiSettings> xGuiSettings, QWidget *parent, const std::string& path) : QDialog(parent)
{
	std::shared_ptr<EmuSettings> xEmuSettings;
	xEmuSettings.reset(new EmuSettings(path));

	CoreTab* coreTab = new CoreTab(xEmuSettings, this);

	QPushButton *okButton = new QPushButton(tr("OK"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setDefault(true);

	tabWidget = new QTabWidget;
	tabWidget->setUsesScrollButtons(false);
	tabWidget->addTab(coreTab, tr("Core"));
	tabWidget->addTab(new GraphicsTab(xEmuSettings, this), tr("Graphics"));
	tabWidget->addTab(new AudioTab(xEmuSettings, this), tr("Audio"));
	tabWidget->addTab(new InputTab(xEmuSettings, this), tr("Input / Output"));
	tabWidget->addTab(new MiscTab(xEmuSettings, this), tr("Misc"));
	tabWidget->addTab(new NetworkingTab(xEmuSettings, this), tr("Networking"));
	tabWidget->addTab(new SystemTab(xEmuSettings, this), tr("System"));
	if (path == "")
	{ // Don't add gui tab to game settings.
		GuiTab* guiTab = new GuiTab(xGuiSettings, this);
		tabWidget->addTab(guiTab, tr("Gui"));
		connect(guiTab, &GuiTab::GuiSettingsSyncRequest, this, &SettingsDialog::GuiSettingsSyncRequest);
		connect(guiTab, &GuiTab::GuiSettingsSaveRequest, this, &SettingsDialog::GuiSettingsSaveRequest);
		connect(guiTab, &GuiTab::GuiStylesheetRequest, this, &SettingsDialog::GuiStylesheetRequest);
		connect(okButton, &QAbstractButton::clicked, guiTab, &GuiTab::Accept);
	}

	// Various connects
	connect(okButton, &QAbstractButton::clicked, coreTab, &CoreTab::SaveSelectedLibraries);
	connect(okButton, &QAbstractButton::clicked, xEmuSettings.get(), &EmuSettings::SaveSettings);
	connect(okButton, &QAbstractButton::clicked, this, &QDialog::accept);
	connect(cancelButton, &QAbstractButton::clicked, this, &QWidget::close);
	connect(tabWidget, &QTabWidget::currentChanged, [=]() {cancelButton->setFocus(); });

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
	buttonsLayout->addStretch();
	buttonsLayout->addWidget(okButton);
	buttonsLayout->addWidget(cancelButton);

	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addWidget(tabWidget);
	mainLayout->addLayout(buttonsLayout);
	setLayout(mainLayout);

	setWindowTitle(tr("Settings"));
}
