#pragma once

#include <QDialog>
#include <QDialogButtonBox>

class QLabel;
class QLineEdit;
class QValidator;

class input_dialog : public QDialog
{
	Q_OBJECT

public:
	input_dialog(int max_length, const QString& text, const QString& title, const QString& label, const QString& placeholder, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~input_dialog();

	void set_label_text(const QString& text);
	void set_validator(const QValidator* validator);
	void set_clear_button_enabled(bool enable);
	void set_input_font(const QFont& font, bool fix_width, char sample = '\0');
	void set_button_enabled(QDialogButtonBox::StandardButton id, bool enabled);
	QString get_input_text() const;

private:
	QLabel* m_label = nullptr;
	QLineEdit* m_input = nullptr;
	QDialogButtonBox* m_button_box = nullptr;
	QString m_text;

Q_SIGNALS:
	void text_changed(const QString& text);
};
