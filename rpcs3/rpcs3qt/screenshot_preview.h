#pragma once

#include <QImage>
#include <QLabel>

class screenshot_preview : public QLabel
{
	Q_OBJECT

public:
	screenshot_preview(const QString& filepath, QWidget* parent = nullptr);

protected:
	void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
	void show_context_menu(const QPoint& pos);

private:
	void scale(const QSize& size);

	QString m_filepath;
	QImage m_image;
	double m_factor = 1.0;
	bool m_stretch = false;
};
