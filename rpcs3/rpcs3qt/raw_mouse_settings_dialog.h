#pragma once

#include "util/types.hpp"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>

#include <vector>
#include <unordered_map>

class raw_mouse_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	raw_mouse_settings_dialog(QWidget* parent = nullptr);
	virtual ~raw_mouse_settings_dialog();

private:
	void update_combo_box(usz player);
	void add_tabs(QTabWidget* tabs);
	void on_enumeration();
	void reset_config();
	void on_button_click(int id);
	void mouse_press(const std::string& device_name, s32 button_code, bool pressed);
	void key_press(const std::string& device_name, s32 scan_code, bool pressed);
	void handle_device_change(const std::string& device_name);
	bool is_device_active(const std::string& device_name);
	std::string get_current_device_name(int player);
	void reactivate_buttons();

	QTabWidget* m_tab_widget = nullptr;
	std::vector<QComboBox*> m_device_combos;
	std::vector<QDoubleSpinBox*> m_accel_spin_boxes;

	// Buttons
	QDialogButtonBox* m_button_box = nullptr;
	QButtonGroup* m_buttons = nullptr;
	std::vector<std::unordered_map<int, QPushButton*>> m_push_buttons;
	int m_button_id = -1;

	// Backup for standard button palette
	QPalette m_palette;

	// Remap Timer
	static constexpr int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_remap_timer;
	QTimer m_mouse_release_timer;
	bool m_disable_mouse_release_event = false;

	// Update Timer
	QTimer m_update_timer;

protected:
	bool eventFilter(QObject* object, QEvent* event) override;
};
