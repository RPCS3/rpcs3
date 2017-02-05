#ifndef CORETAB_H
#define CORETAB_H

#include <QLineEdit>
#include <QListWidget>
#include <QWidget>

class CoreTab : public QWidget
{
	Q_OBJECT

public:
	explicit CoreTab(QWidget *parent = 0);

private slots:
    void OnSearchBoxTextChanged();

private:
    QListWidget *lleList;
    QLineEdit *searchBox;
};

#endif // CORETAB_H
