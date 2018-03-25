#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class syntax_highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

	enum block_state
	{
		ended_outside_comment,
		ended_inside_comment
	};

public:
	syntax_highlighter(QTextDocument *parent = 0);

	struct SimpleRule
	{
		QVector<QRegularExpression> expressions;
		QTextCharFormat format;
		SimpleRule(){}
		SimpleRule(const QVector<QRegularExpression>& expressions, const QTextCharFormat& format)
			: expressions(expressions), format(format) {}
	};
	struct MultiRule
	{
		QRegularExpression expression;
		QVector<QTextCharFormat> formats;
		MultiRule(){}
		MultiRule(const QRegularExpression& expr, const QVector<QTextCharFormat>& formats)
			: expression(expr), formats(formats) {}
	};
	struct CommentRule
	{
		QRegularExpression start_expression;
		QRegularExpression end_expression;
		QTextCharFormat format;
		bool multi_line = false;
		CommentRule(){}
		CommentRule(const QRegularExpression& start, const QRegularExpression& end, const QTextCharFormat& format, bool multi_line)
			: start_expression(start), end_expression(end), format(format), multi_line(multi_line) {}
	};

	/**
		Add a simple highlighting rule that applies the given format to all given expressions.
		You can add up to one Group to the expression. The full match group will be ignored
	*/
	void AddSimpleRule(SimpleRule rule);
	void AddSimpleRule(const QStringList& expressions, const QColor& color);
	/** Add a simple highlighting rule for words. Not supposed to be used with any other expression */
	void AddWordRule(const QStringList& words, const QColor& color);

	/**
		Add a complex highlighting rule that applies different formats to the expression's groups.
		Make sure you don't have more groups in your expression than formats !!!
		Group 0 is always the full match, so the first color has to be for that !!!
		Example expression for string "rdch $4,$6,$8,$5" with 6 groups (5 + full match):
		"^(?<group1>[^\s]+) (?<group2>[^,]+),(?<group3>[^,]+),(?<group4>[^,]+),(?<group5>[^,]+)$"
	*/
	void AddMultiRule(const MultiRule& rule);
	void AddMultiRule(const QString& expression, const QVector<QColor>& colors);

	/**
		Add a comment highlighting rule. Add them in ascending priority.
		We only need rule.end_expression in case of rule.multi_line.
		A block ends at the end of a line anyway.
	*/
	void AddCommentRule(const CommentRule& rule);
	void AddCommentRule(const QString& start, const QColor& color, bool multi_line = false, const QString& end = "");

protected:
	void highlightBlock(const QString &text) override;

private:
	/** Checks if an expression is invalid or empty */
	static bool IsInvalidExpression(const QRegularExpression& expression);

	QVector<SimpleRule> m_rules;
	QVector<CommentRule> m_comment_rules;
	QVector<MultiRule> m_multi_rules;

	QString m_current_block;
};
