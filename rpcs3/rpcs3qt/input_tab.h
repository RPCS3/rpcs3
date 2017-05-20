#ifndef INPUTTAB_H
#define INPUTTAB_H

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class input_tab : public QWidget
{
	Q_OBJECT

public:
	explicit input_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);

signals:

public slots:
};

#endif // INPUTTAB_H
