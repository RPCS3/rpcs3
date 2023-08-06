#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class find_dialog : public QDialog
{
	Q_OBJECT

public:
	find_dialog(QPlainTextEdit* edit, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

private:
	int m_count_lines = 0;
	int m_count_total = 0;
	QLabel* m_label_count_lines;
	QLabel* m_label_count_total;
	QPlainTextEdit* m_text_edit;
	QLineEdit* m_find_bar;
	QPushButton* m_find_first;
	QPushButton* m_find_last;
	QPushButton* m_find_next;
	QPushButton* m_find_previous;

private Q_SLOTS:
	int count_all();
	void find_first();
	void find_last();
	void find_next();
	void find_previous();
	void show_count() const;
};
