#ifndef MISCTAB_H
#define MISCTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <memory>

class MiscTab : public QWidget
{
	Q_OBJECT

public:
	explicit MiscTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);
};

#endif // MISCTAB_H
