#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

class find_dialog : public QDialog
{
public:
	find_dialog(QTextEdit* edit, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
	~find_dialog();

private:
	QTextEdit* m_text_edit;
	QLineEdit* m_find_bar;
	QPushButton* m_find_first;
	QPushButton* m_find_last;
	QPushButton* m_find_next;
	QPushButton* m_find_previous;

private Q_SLOTS:
	void find_first();
	void find_last();
	void find_next();
	void find_previous();
};
