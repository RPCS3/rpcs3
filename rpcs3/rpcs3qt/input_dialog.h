#pragma once

#include <QDialog>
#include <QLabel>

class input_dialog : public QDialog
{
	Q_OBJECT

public:
	input_dialog(int max_length, const QString& text, const QString& title, const QString& label, const QString& placeholder, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~input_dialog();

	void set_label_text(const QString& text);

private:
	QLabel* m_label = nullptr;
	QString m_text;

Q_SIGNALS:
	void text_changed(const QString& text);
};
