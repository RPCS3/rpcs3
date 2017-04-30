#ifndef MISCTAB_H
#define MISCTAB_H

#include <QWidget>

class MiscTab : public QWidget
{
	Q_OBJECT

public:
	explicit MiscTab(QWidget *parent = 0);

signals:

public slots:
	void OnApply();
	void OnReset();
	void OnStylesheetChanged();
};

#endif // MISCTAB_H
