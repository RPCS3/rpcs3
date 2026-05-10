#pragma once

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>

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
		static constexpr int margin_adjustement = 4;

		const int margin = (option.rect.height() - opt.fontMetrics.height() - margin_adjustement);
		QRect text_rect  = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, nullptr);

		if (index.column() != 0)
		{
			text_rect.adjust(5, 0, 0, 0);
		}

		text_rect.setTop(text_rect.top() + margin);

		painter->translate(text_rect.topLeft());
		painter->setClipRect(text_rect.translated(-text_rect.topLeft()));

		// Create a context for our painter
		QAbstractTextDocumentLayout::PaintContext context;

		// Apply the styled palette
		context.palette = opt.palette;

		// Draw the text
		m_document->documentLayout()->draw(painter, context);

		painter->restore();
	}
};
