#pragma once

#include <QDockWidget>
#include <QStyleOption>
#include <QPainter>

class custom_dock_widget : public QDockWidget
{
public:
	explicit custom_dock_widget(const QString &title, QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags())
		: QDockWidget(title, parent, flags)
	{
		connect(this, &QDockWidget::topLevelChanged, [this](bool/* topLevel*/)
		{
			style()->unpolish(this);
			style()->polish(this);
		});
	};

protected:
	void paintEvent(QPaintEvent* event) override
	{
		// We need to repaint the dock widgets as plain widgets in floating mode.
		// Source: https://stackoverflow.com/questions/10272091/cannot-add-a-background-image-to-a-qdockwidget
		if (isFloating())
		{
			QStyleOption opt;
			opt.init(this);
			QPainter p(this);
			style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
			return;
		}

		// Use inherited method for docked mode because otherwise the dock would lose the title etc.
		QDockWidget::paintEvent(event);
	}
};
