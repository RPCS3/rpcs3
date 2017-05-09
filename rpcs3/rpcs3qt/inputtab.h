#ifndef INPUTTAB_H
#define INPUTTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <memory>

class InputTab : public QWidget
{
	Q_OBJECT

public:
	explicit InputTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);

signals:

public slots:
};

#endif // INPUTTAB_H
