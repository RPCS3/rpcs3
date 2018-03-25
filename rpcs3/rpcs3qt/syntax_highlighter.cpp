#include "syntax_highlighter.h"

syntax_highlighter::syntax_highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
}

void syntax_highlighter::AddSimpleRule(SimpleRule rule)
{
	rule.expressions.erase(std::remove_if(rule.expressions.begin(), rule.expressions.end(), [&](const auto &e){
		return IsInvalidExpression(e) || e.captureCount() > 1;
	}), rule.expressions.end());

	if (rule.expressions.isEmpty())
	{
		return;
	}
	m_rules.append(rule);
}

void syntax_highlighter::AddSimpleRule(const QStringList& expressions, const QColor& color)
{
	QTextCharFormat format;
	format.setForeground(color);
	QVector<QRegularExpression> regexs;
	for (const auto& expr : expressions)
	{
		regexs.append(QRegularExpression(expr));
	}
	AddSimpleRule(SimpleRule(regexs, format));
}

void syntax_highlighter::AddWordRule(const QStringList& words, const QColor& color)
{
	QTextCharFormat format;
	format.setForeground(color);
	QVector<QRegularExpression> regexs;
	for (const auto& word : words)
	{
		regexs.append(QRegularExpression("\\b" + word + "\\b"));
	}
	AddSimpleRule(SimpleRule(regexs, format));
}

void syntax_highlighter::AddMultiRule(const MultiRule& rule)
{
	if (IsInvalidExpression(rule.expression) || rule.formats.length() <= rule.expression.captureCount())
	{
		return;
	}

	m_multi_rules.append(rule);
}
void syntax_highlighter::AddMultiRule(const QString& expression, const QVector<QColor>& colors)
{
	QVector<QTextCharFormat> formats;
	for (const auto& color : colors)
	{
		QTextCharFormat format;
		format.setForeground(color);
		formats.append(format);
	}
	AddMultiRule(MultiRule(QRegularExpression(expression), formats));
}

void syntax_highlighter::AddCommentRule(const CommentRule& rule)
{
	if (IsInvalidExpression(rule.start_expression) || (rule.multi_line && IsInvalidExpression(rule.end_expression)))
	{
		return;
	}
	m_comment_rules.append(rule);
}

void syntax_highlighter::AddCommentRule(const QString& start, const QColor& color, bool multi_line, const QString& end)
{
	QTextCharFormat format;
	format.setForeground(color);
	AddCommentRule(CommentRule(QRegularExpression(start), QRegularExpression(end), format, multi_line));
}

void syntax_highlighter::highlightBlock(const QString &text)
{
	m_current_block = text;

	for (const auto& rule : m_multi_rules)
	{
		// Search for all the matching strings
		QRegularExpressionMatchIterator iter = rule.expression.globalMatch(m_current_block);

		// Iterate through the matching strings
		while (iter.hasNext())
		{
			// Apply formats to their respective found groups
			QRegularExpressionMatch match = iter.next();

			for (int i = 0; i <= match.lastCapturedIndex(); i++)
			{
				setFormat(match.capturedStart(i), match.capturedLength(i), rule.formats[i]);
			}
		}
	}

	for (const auto& rule : m_rules)
	{
		for (const auto& expression : rule.expressions)
		{
			// Search for all the matching strings
			QRegularExpressionMatchIterator iter = expression.globalMatch(m_current_block);
			bool contains_group = expression.captureCount() > 0;

			// Iterate through the matching strings
			while (iter.hasNext())
			{
				// Apply format to the matching string
				QRegularExpressionMatch match = iter.next();
				if (contains_group)
				{
					setFormat(match.capturedStart(1), match.capturedLength(1), rule.format);
				}
				else
				{
					setFormat(match.capturedStart(), match.capturedLength(), rule.format);
				}
			}
		}
	}

	for (const auto& rule : m_comment_rules)
	{
		int comment_start  = 0; // Current comment's start position in the text block
		int comment_end    = 0; // Current comment's end position in the text block
		int comment_length = 0; // Current comment length

		// We assume we end outside a comment until we know better
		setCurrentBlockState(block_state::ended_outside_comment);

		// Search for the first comment in this block if we start outside or don't want to search for multiline comments
		if (!rule.multi_line || previousBlockState() != block_state::ended_inside_comment)
		{
			comment_start = m_current_block.indexOf(rule.start_expression);
		}

		// Set format for the rest of this block/line
		if (!rule.multi_line)
		{
			comment_length = m_current_block.length() - comment_start;
			setFormat(comment_start, comment_length, rule.format);
			break;
		}

		// Format all comments in this block (if they exist)
		while (comment_start >= 0)
		{
			// Search for end of comment in the remaining text
			QRegularExpressionMatch match = rule.end_expression.match(m_current_block, comment_start);
			comment_end = match.capturedStart();
			match.captured(1);

			if (comment_end == -1)
			{
				// We end inside a comment and want to format the entire remaining text
				setCurrentBlockState(block_state::ended_inside_comment);
				comment_length = m_current_block.length() - comment_start;
			}
			else
			{
				// We found the end of the comment so we need to go another round
				comment_length = comment_end - comment_start + match.capturedLength();
			}

			// Set format for this text segment
			setFormat(comment_start, comment_length, rule.format);

			// Search for the next comment
			comment_start = m_current_block.indexOf(rule.start_expression, comment_start + comment_length);
		}
	}
}

bool syntax_highlighter::IsInvalidExpression(const QRegularExpression& expression)
{
	return !expression.isValid() || expression.pattern().isEmpty();
}
