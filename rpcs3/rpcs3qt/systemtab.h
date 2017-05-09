#ifndef SYSTEMTAB_H
#define SYSTEMTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <memory>

class SystemTab : public QWidget
{
	Q_OBJECT

public:
	explicit SystemTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);

signals:

public slots:
};

#endif // SYSTEMTAB_H
