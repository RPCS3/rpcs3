#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>

// Inspired by https://doc.qt.io/qt-5/qtwidgets-richtext-syntaxhighlighter-example.html

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit Highlighter(QTextDocument *parent = nullptr);

protected:
	void highlightBlock(const QString &text) override;
	void addRule(const QString &pattern, const QBrush &brush);

	struct HighlightingRule
	{
		QRegularExpression pattern;
		QTextCharFormat format;
	};
	QVector<HighlightingRule> highlightingRules;

	QRegularExpression commentStartExpression;
	QRegularExpression commentEndExpression;

	QTextCharFormat multiLineCommentFormat;
};

class LogHighlighter : public Highlighter
{
	Q_OBJECT

public:
	explicit LogHighlighter(QTextDocument* parent = nullptr);
};

class AsmHighlighter : public Highlighter
{
	Q_OBJECT

public:
	explicit AsmHighlighter(QTextDocument *parent = nullptr);
};

class GlslHighlighter : public Highlighter
{
	Q_OBJECT

public:
	explicit GlslHighlighter(QTextDocument *parent = nullptr);
};
