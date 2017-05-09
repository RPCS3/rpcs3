#ifndef NETWORKINGTAB_H
#define NETWORKINGTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <Memory>

class NetworkingTab : public QWidget
{
	Q_OBJECT

public:
	explicit NetworkingTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);

signals:

public slots:
};

#endif // NETWORKINGTAB_H
