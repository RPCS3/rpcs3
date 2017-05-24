#ifndef NETWORKINGTAB_H
#define NETWORKINGTAB_H

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class networking_tab : public QWidget
{
	Q_OBJECT

public:
	explicit networking_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);

signals:

public slots:
};

#endif // NETWORKINGTAB_H
