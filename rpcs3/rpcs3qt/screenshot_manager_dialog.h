#pragma once

#include <QDialog>
#include <QFutureWatcher>
#include <QIcon>
#include <QSize>

class QListWidget;
class QListWidgetItem;

class screenshot_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	screenshot_manager_dialog(QWidget* parent = nullptr);
	~screenshot_manager_dialog();

protected:
	void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
	void update_icon(int index);

Q_SIGNALS:
	void signal_icon_change(int index, const QString& path);

private:
	void show_preview(QListWidgetItem* item);
	void update_icons(int value);

	enum item_role
	{
		source = Qt::UserRole,
		loaded = Qt::UserRole + 1,
	};

	struct thumbnail
	{
		QIcon icon;
		QString path;
		int index = 0;
	};

	QListWidget* m_grid = nullptr;

	QFutureWatcher<thumbnail>* m_icon_loader;

	QSize m_icon_size;
	QIcon m_placeholder;

	int m_scrollbar_value = 0;
};
