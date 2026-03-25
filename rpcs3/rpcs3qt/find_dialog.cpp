#include "stdafx.h"
#include "find_dialog.h"

#include <QPushButton>
#include <QVBoxLayout>
#include <QTextBlock>
#include <QCheckBox>

find_dialog::find_dialog(QPlainTextEdit* edit, QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), m_text_edit(edit)
{
	setWindowTitle(tr("Find string"));

	m_find_bar = new QLineEdit();
	m_find_bar->setPlaceholderText(tr("Search..."));

	QCheckBox* cb_case_sensitive = new QCheckBox(tr("Case sensitive"));
	cb_case_sensitive->setChecked(m_case_sensitive);
	connect(cb_case_sensitive, &QCheckBox::toggled, this, [=](bool checked)
	{
		m_case_sensitive = checked;
	});

	m_label_count_lines = new QLabel(tr("Line 0/0"));
	m_label_count_total = new QLabel(tr("Word 0/0"));

	QPushButton* find_first = new QPushButton(tr("First"));
	QPushButton* find_last = new QPushButton(tr("Last"));
	QPushButton* find_next = new QPushButton(tr("Next"));
	QPushButton* find_previous = new QPushButton(tr("Previous"));

	QHBoxLayout* find_layout = new QHBoxLayout();
	find_layout->addWidget(m_find_bar);
	find_layout->addWidget(cb_case_sensitive);

	QHBoxLayout* count_layout = new QHBoxLayout();
	count_layout->addWidget(m_label_count_lines);
	count_layout->addWidget(m_label_count_total);

	QHBoxLayout* button_layout = new QHBoxLayout();
	button_layout->addWidget(find_first);
	button_layout->addWidget(find_last);
	button_layout->addWidget(find_previous);
	button_layout->addWidget(find_next);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addLayout(find_layout);
	layout->addLayout(count_layout);
	layout->addLayout(button_layout);
	setLayout(layout);

	connect(find_first, &QPushButton::clicked, this, [this](){ find(find_type::first); });
	connect(find_last, &QPushButton::clicked, this, [this](){ find(find_type::last); });
	connect(find_next, &QPushButton::clicked, this, [this](){ find(find_type::next); });
	connect(find_previous, &QPushButton::clicked, this, [this](){ find(find_type::prev); });

	find_next->setDefault(true);

	show();
}

void find_dialog::find(find_type type)
{
	m_count_lines = 0;
	m_count_total = 0;
	m_current_line = 0;
	m_current_index = 0;

	if (!m_text_edit || m_find_bar->text().isEmpty())
	{
		show_count();
		return;
	}

	const QTextDocument::FindFlags flags = m_case_sensitive ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlag{};

	std::map<int /*block*/, std::map<int /*line*/, std::vector<int> /*pos*/>> block_indices;

	const QTextCursor old_cursor = m_text_edit->textCursor();
	m_text_edit->moveCursor(QTextCursor::Start);

	const QString text = m_find_bar->text();

	while (m_text_edit->find(text, flags))
	{
		m_count_total++;

		const QTextCursor cursor = m_text_edit->textCursor();
		const QTextBlock block = cursor.block();
		const QTextLayout* layout = block.layout();

		const int relative_pos = cursor.position() - block.position();
		const QTextLine line = layout->lineForTextPosition(relative_pos);
		const int pos_in_line = relative_pos - line.textStart();

		block_indices[cursor.blockNumber()][line.lineNumber()].push_back(pos_in_line);
	}

	switch (type)
	{
	case find_type::first:
	{
		m_text_edit->moveCursor(QTextCursor::Start);
		m_text_edit->find(text, flags);
		break;
	}
	case find_type::last:
	{
		m_text_edit->moveCursor(QTextCursor::End);
		m_text_edit->find(text, flags | QTextDocument::FindBackward);
		break;
	}
	case find_type::next:
	{
		m_text_edit->setTextCursor(old_cursor);
		m_text_edit->find(text, flags);
		break;
	}
	case find_type::prev:
	{
		m_text_edit->setTextCursor(old_cursor);
		m_text_edit->find(text, flags | QTextDocument::FindBackward);
		break;
	}
	}

	const QTextCursor cursor = m_text_edit->textCursor();
	const QTextBlock block = cursor.block();
	const QTextLayout* layout = block.layout();

	const int relative_pos = cursor.position() - block.position();
	const QTextLine line = layout->lineForTextPosition(relative_pos);
	const int pos_in_line = relative_pos - line.textStart();

	const int current_line = line.lineNumber();
	const int current_pos = pos_in_line;

	int word_count = 0;
	for (const auto& [block, lines] : block_indices)
	{
		const bool is_current_block = block == cursor.blockNumber();

		for (const auto& [line, positions] : lines)
		{
			const bool is_current_line = line == current_line;

			m_count_lines++;

			int pos_count = 0;

			for (int pos : positions)
			{
				pos_count++;
				word_count++;

				if (is_current_block && is_current_line && pos == current_pos)
				{
					m_current_line = m_count_lines;
					m_current_index = word_count;
				}
			}
		}
	}

	show_count();
}

void find_dialog::show_count() const
{
	m_label_count_lines->setText(tr("Line %0/%1").arg(m_current_line).arg(m_count_lines));
	m_label_count_total->setText(tr("Word %0/%1").arg(m_current_index).arg(m_count_total));
}
