#include "find_dialog.h"

#include <QVBoxLayout>

find_dialog::find_dialog(QTextEdit* edit, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), m_text_edit(edit)
{
	setWindowTitle(tr("Find string"));

	m_find_bar = new QLineEdit();
	m_find_bar->setPlaceholderText(tr("Search..."));

	m_find_first = new QPushButton(tr("First"));
	m_find_last = new QPushButton(tr("Last"));
	m_find_next = new QPushButton(tr("Next"));
	m_find_previous = new QPushButton(tr("Previous"));

	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(m_find_first);
	button_layout->addWidget(m_find_last);
	button_layout->addWidget(m_find_next);
	button_layout->addWidget(m_find_previous);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(m_find_bar);
	layout->addLayout(button_layout);
	setLayout(layout);

	connect(m_find_first, &QPushButton::pressed, this, &find_dialog::find_first);
	connect(m_find_last, &QPushButton::pressed, this, &find_dialog::find_last);
	connect(m_find_next, &QPushButton::pressed, this, &find_dialog::find_next);
	connect(m_find_previous, &QPushButton::pressed, this, &find_dialog::find_previous);

	show();
}

find_dialog::~find_dialog()
{
}

void find_dialog::find_first()
{
	if (!m_text_edit || m_find_bar->text().isEmpty())
		return;

	m_text_edit->moveCursor(QTextCursor::Start);
	m_text_edit->find(m_find_bar->text());
}

void find_dialog::find_last()
{
	if (!m_text_edit || m_find_bar->text().isEmpty())
		return;

	m_text_edit->moveCursor(QTextCursor::End);
	m_text_edit->find(m_find_bar->text(), QTextDocument::FindBackward);
}

void find_dialog::find_next()
{
	if (!m_text_edit || m_find_bar->text().isEmpty())
		return;

	m_text_edit->find(m_find_bar->text());
}

void find_dialog::find_previous()
{
	if (!m_text_edit || m_find_bar->text().isEmpty())
		return;

	m_text_edit->find(m_find_bar->text(), QTextDocument::FindBackward);
}
