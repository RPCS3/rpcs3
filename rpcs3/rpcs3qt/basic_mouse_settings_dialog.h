#pragma once

#include <QButtonGroup>
#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMouseEvent>
#include <QTimer>

#include <unordered_map>

class basic_mouse_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	basic_mouse_settings_dialog(QWidget* parent = nullptr);

private:
	void reset_config();

	void on_button_click(int id);
	void reactivate_buttons();

	// Buttons
	QDialogButtonBox* m_button_box = nullptr;
	QButtonGroup* m_buttons = nullptr;
	std::unordered_map<int, QPushButton*> m_push_buttons;
	int m_button_id = -1;

	// Backup for standard button palette
	QPalette m_palette;

	// Remap Timer
	static constexpr int MAX_SECONDS = 5;
	int m_seconds = MAX_SECONDS;
	QTimer m_remap_timer;

protected:
	void keyPressEvent(QKeyEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	bool eventFilter(QObject* object, QEvent* event) override;
};
