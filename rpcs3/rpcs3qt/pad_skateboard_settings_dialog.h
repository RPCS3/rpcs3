#pragma once

#include <QDialog>
#include <QPushButton>

#include <memory>

namespace Ui
{
	class pad_skateboard_settings_dialog;
}

class pad_skateboard_settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit pad_skateboard_settings_dialog(QWidget* parent);
	~pad_skateboard_settings_dialog();

	QPushButton* ir_nose_button() const;
	QPushButton* ir_tail_button() const;
	QPushButton* ir_left_button() const;
	QPushButton* ir_right_button() const;
	QPushButton* tilt_left_button() const;
	QPushButton* tilt_right_button() const;

private:
	std::unique_ptr<Ui::pad_skateboard_settings_dialog> ui;
};
