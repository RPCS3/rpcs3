#pragma once

#include "gui_settings.h"
#include "emu_settings.h"

#include "Emu/GameInfo.h"

#include <QDialog>
#include <QLabel>
#include <QTabWidget>

#include <memory>

namespace Ui
{
	class settings_dialog;
}

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, const int& tabIndex = 0, QWidget *parent = 0, const GameInfo *game = nullptr);
	~settings_dialog();
	int exec() override;
Q_SIGNALS:
	void GuiSettingsSyncRequest(bool configure_all);
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
	void GuiRepaintRequest();
private Q_SLOTS:
	void OnBackupCurrentConfig();
	void OnApplyConfig();
	void OnApplyStylesheet();
private:
	void EnhanceSlider(emu_settings::SettingsType settings_type, QSlider* slider, QLabel* label, const QString& label_text);

	// Emulator tab
	void AddConfigs();
	void AddStylesheets();
	QString m_currentStylesheet;
	QString m_currentConfig;
	// Gpu tab
	QString m_oldRender = "";
	// Audio tab
	QComboBox *mics_combo[4];

	int m_tab_Index;
	Ui::settings_dialog *ui;
	std::shared_ptr<gui_settings> xgui_settings;
	std::shared_ptr<emu_settings> xemu_settings;

	// Discord
	bool m_use_discord;
	QString m_discord_state;

	// Descriptions
	QList<QPair<QLabel*, QString>> m_description_labels;
	QHash<QObject*, QString> m_descriptions;
	void SubscribeDescription(QLabel* description);
	void SubscribeTooltip(QObject* object, const QString& tooltip);
	void SubscribeTooltip(QList<QObject*> objects, const QString& tooltip);
	bool eventFilter(QObject* object, QEvent* event) override;
};
