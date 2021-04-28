#pragma once

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QTextDocument>

class richtext_item_delegate : public QStyledItemDelegate
{
private:
	QTextDocument* m_document;

public:
	explicit richtext_item_delegate(QObject* parent = nullptr)
		: QStyledItemDelegate(parent)
		, m_document(new QTextDocument(this))
	{
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
	{
		painter->save();

		// Initialize style option
		QStyleOptionViewItem opt = option;
		initStyleOption(&opt, index);

		// Setup the QTextDocument with our HTML
		m_document->setHtml(opt.text);
		opt.text.clear();

		// Get the available style
		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

		// Draw our widget if available
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

		// Adjust our painter parameters with some magic that looks good.
		// This is necessary so that we don't draw on top of the optional widget.
		const int margin = (option.rect.height() - opt.fontMetrics.height() - 4);
		QRect text_rect  = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, nullptr);

		if (index.column() != 0)
		{
			text_rect.adjust(5, 0, 0, 0);
		}

		text_rect.setTop(text_rect.top() + margin);

		painter->translate(text_rect.topLeft());
		painter->setClipRect(text_rect.translated(-text_rect.topLeft()));

		// Create a context for our painter
		const QAbstractTextDocumentLayout::PaintContext context;

		// We can change the text color based on state (Unused because it didn't look good)
		//context.palette.setColor(QPalette::Text, option.palette.color(QPalette::Active, (option.state & QStyle::State_Selected) ? QPalette::HighlightedText : QPalette::Text));

		// Draw the text
		m_document->documentLayout()->draw(painter, context);

		painter->restore();

		//QStyledItemDelegate::paint(painter, opt, index); // Unused
	}
};
