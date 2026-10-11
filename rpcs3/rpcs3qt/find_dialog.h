#pragma once

#include <QDialog>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>

class find_dialog : public QDialog
{
	Q_OBJECT

public:
	find_dialog(QPlainTextEdit* edit, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());

private:
	enum class find_type
	{
		first,
		last,
		next,
		prev
	};

	void find(find_type type);
	void show_count() const;

	int m_count_lines = 0;
	int m_count_total = 0;
	int m_current_line = 0;
	int m_current_index = 0;

	bool m_case_sensitive = false;

	QLabel* m_label_count_lines;
	QLabel* m_label_count_total;
	QPlainTextEdit* m_text_edit;
	QLineEdit* m_find_bar;
};
