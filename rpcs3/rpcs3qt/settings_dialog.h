#pragma once

#include "emu_settings.h"

#include <QDialog>
#include <QLabel>
#include <QSlider>

#include <memory>

class gui_settings;
struct GameInfo;

namespace Ui
{
	class settings_dialog;
}

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, const int& tab_index = 0, QWidget *parent = 0, const GameInfo *game = nullptr);
	~settings_dialog();
	int exec() override;
Q_SIGNALS:
	void GuiSettingsSyncRequest(bool configure_all);
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
	void GuiRepaintRequest();
	void EmuSettingsApplied();
private Q_SLOTS:
	void OnBackupCurrentConfig();
	void OnApplyConfig();
	void OnApplyStylesheet();
private:
	void EnhanceSlider(emu_settings::SettingsType settings_type, QSlider* slider, QLabel* label, const QString& label_text);

	// Snapping of sliders when moved with mouse
	void SnapSlider(QSlider* slider, int interval);
	QSlider* m_current_slider = nullptr;

	// Emulator tab
	void AddConfigs();
	void AddStylesheets();
	QString m_current_stylesheet;
	QString m_current_gui_config;
	// Gpu tab
	QString m_old_renderer = "";
	// Audio tab
	QComboBox *mics_combo[4];

	int m_tab_index;
	Ui::settings_dialog *ui;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;

	// Discord
	bool m_use_discord;
	QString m_discord_state;

	// Descriptions
	QList<QPair<QLabel*, QString>> m_description_labels;
	QHash<QObject*, QString> m_descriptions;
	void SubscribeDescription(QLabel* description);
	void SubscribeTooltip(QObject* object, const QString& tooltip);
	bool eventFilter(QObject* object, QEvent* event) override;
};
