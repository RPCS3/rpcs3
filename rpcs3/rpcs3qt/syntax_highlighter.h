#pragma once

#include <QSyntaxHighlighter>
#include <QRegularExpression>

// Inspired by https://doc.qt.io/qt-5/qtwidgets-richtext-syntaxhighlighter-example.html

class Highlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	Highlighter(QTextDocument *parent = 0);

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

class AsmHighlighter : public Highlighter
{
	Q_OBJECT

public:
	AsmHighlighter(QTextDocument *parent = 0);
};

class GlslHighlighter : public Highlighter
{
	Q_OBJECT

public:
	GlslHighlighter(QTextDocument *parent = 0);
};
