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
	explicit settings_dialog(std::shared_ptr<gui_settings> gui_settings, std::shared_ptr<emu_settings> emu_settings, const int& tab_index = 0, QWidget *parent = nullptr, const GameInfo *game = nullptr);
	~settings_dialog();
	int exec() override;
Q_SIGNALS:
	void GuiStylesheetRequest();
	void GuiRepaintRequest();
	void EmuSettingsApplied();
private:
	void EnhanceSlider(emu_settings_type settings_type, QSlider* slider, QLabel* label, const QString& label_text) const;

	// Snapping of sliders when moved with mouse
	void SnapSlider(QSlider* slider, int interval);
	QSlider* m_current_slider = nullptr;

	// Gui tab
	void AddStylesheets();
	void ApplyStylesheet(bool reset);
	QString m_current_stylesheet;
	// Gpu tab
	QString m_old_renderer;
	// Audio tab
	QComboBox *m_mics_combo[4];

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
