#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include "core_tab.h"
#include "graphics_tab.h"
#include "audio_tab.h"
#include "input_tab.h"
#include "misc_tab.h"
#include "gui_tab.h"
#include "networking_tab.h"
#include "system_tab.h"
#include "settings_dialog.h"
#include "emu_settings.h"

inline QString qstr(const std::string& _in) { return QString::fromUtf8(_in.data(), _in.size()); }

settings_dialog::settings_dialog(std::shared_ptr<gui_settings> xgui_settings, QWidget *parent, GameInfo* game) : QDialog(parent)
{
	std::shared_ptr<emu_settings> xemu_settings;
	if (game)
	{
		xemu_settings.reset(new emu_settings("data/" + game->serial));
		setWindowTitle(tr("Settings: [") + qstr(game->serial) + "] " + qstr(game->name));
	}
	else
	{
		xemu_settings.reset(new emu_settings(""));
		setWindowTitle(tr("Settings"));
	}
	
	core_tab* coreTab = new core_tab(xemu_settings, this);

	QPushButton *okButton = new QPushButton(tr("OK"));
	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setDefault(true);

	tabWidget = new QTabWidget;
	tabWidget->setUsesScrollButtons(false);
	tabWidget->addTab(coreTab, tr("Core"));
	tabWidget->addTab(new graphics_tab(xemu_settings, this), tr("Graphics"));
	tabWidget->addTab(new audio_tab(xemu_settings, this), tr("Audio"));
	tabWidget->addTab(new input_tab(xemu_settings, this), tr("Input / Output"));
	tabWidget->addTab(new misc_tab(xemu_settings, this), tr("Misc"));
	tabWidget->addTab(new networking_tab(xemu_settings, this), tr("Networking"));
	tabWidget->addTab(new system_tab(xemu_settings, this), tr("System"));
	
	if (!game)
	{	// Don't add gui tab to game settings.
		gui_tab* guiTab = new gui_tab(xgui_settings, this);
		tabWidget->addTab(guiTab, tr("Gui"));
		connect(guiTab, &gui_tab::GuiSettingsSyncRequest, this, &settings_dialog::GuiSettingsSyncRequest);
		connect(guiTab, &gui_tab::GuiSettingsSaveRequest, this, &settings_dialog::GuiSettingsSaveRequest);
		connect(guiTab, &gui_tab::GuiStylesheetRequest, this, &settings_dialog::GuiStylesheetRequest);
		connect(okButton, &QAbstractButton::clicked, guiTab, &gui_tab::Accept);
	}

	// Various connects
	connect(okButton, &QAbstractButton::clicked, coreTab, &core_tab::SaveSelectedLibraries);
	connect(okButton, &QAbstractButton::clicked, xemu_settings.get(), &emu_settings::SaveSettings);
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
}
