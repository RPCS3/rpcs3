#ifndef SYSTEMTAB_H
#define SYSTEMTAB_H

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class system_tab : public QWidget
{
	Q_OBJECT

public:
	explicit system_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);

signals:

public slots:
};

#endif // SYSTEMTAB_H
