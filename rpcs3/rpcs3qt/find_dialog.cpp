#include "find_dialog.h"

#include <QVBoxLayout>

find_dialog::find_dialog(QPlainTextEdit* edit, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), m_text_edit(edit)
{
	setWindowTitle(tr("Find string"));

	m_find_bar = new QLineEdit();
	m_find_bar->setPlaceholderText(tr("Search..."));

	m_label_count_lines = new QLabel(tr("Counted in lines: -"));
	m_label_count_total = new QLabel(tr("Counted in total: -"));

	m_find_first = new QPushButton(tr("First"));
	m_find_last = new QPushButton(tr("Last"));
	m_find_next = new QPushButton(tr("Next"));
	m_find_previous = new QPushButton(tr("Previous"));

	QHBoxLayout* count_layout = new QHBoxLayout();
	count_layout->addWidget(m_label_count_lines);
	count_layout->addWidget(m_label_count_total);

	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(m_find_first);
	button_layout->addWidget(m_find_last);
	button_layout->addWidget(m_find_previous);
	button_layout->addWidget(m_find_next);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_find_bar);
	layout->addLayout(count_layout);
	layout->addLayout(button_layout);
	setLayout(layout);

	connect(m_find_first, &QPushButton::clicked, this, &find_dialog::find_first);
	connect(m_find_last, &QPushButton::clicked, this, &find_dialog::find_last);
	connect(m_find_next, &QPushButton::clicked, this, &find_dialog::find_next);
	connect(m_find_previous, &QPushButton::clicked, this, &find_dialog::find_previous);

	m_find_next->setDefault(true);

	show();
}

int find_dialog::count_all()
{
	m_count_lines = 0;
	m_count_total = 0;

	if (!m_text_edit || m_find_bar->text().isEmpty())
	{
		show_count();
		return 0;
	}

	const QTextCursor old_cursor = m_text_edit->textCursor();
	m_text_edit->moveCursor(QTextCursor::Start);

	int old_line_index = -1;

	while (m_text_edit->find(m_find_bar->text()))
	{
		m_count_total++;
		const int new_line_index = m_text_edit->textCursor().blockNumber();

		if (new_line_index != old_line_index)
		{
			m_count_lines++;
			old_line_index = new_line_index;
		}
	}

	m_text_edit->setTextCursor(old_cursor);
	show_count();
	return m_count_total;
}

void find_dialog::find_first()
{
	if (count_all() <= 0)
		return;

	m_text_edit->moveCursor(QTextCursor::Start);
	m_text_edit->find(m_find_bar->text());
}

void find_dialog::find_last()
{
	if (count_all() <= 0)
		return;

	m_text_edit->moveCursor(QTextCursor::End);
	m_text_edit->find(m_find_bar->text(), QTextDocument::FindBackward);
}

void find_dialog::find_next()
{
	if (count_all() <= 0)
		return;

	m_text_edit->find(m_find_bar->text());
}

void find_dialog::find_previous()
{
	if (count_all() <= 0)
		return;

	m_text_edit->find(m_find_bar->text(), QTextDocument::FindBackward);
}

void find_dialog::show_count() const
{
	m_label_count_lines->setText(tr("Counted in lines: %0").arg(m_count_lines));
	m_label_count_total->setText(tr("Counted in total: %0").arg(m_count_total));
}
