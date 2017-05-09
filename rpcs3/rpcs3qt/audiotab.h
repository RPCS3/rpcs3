#ifndef AUDIOTAB_H
#define AUDIOTAB_H

#include "EmuSettings.h"

#include <QWidget>

#include <memory>

class AudioTab : public QWidget
{
	Q_OBJECT

public:
	explicit AudioTab(std::shared_ptr<EmuSettings> xEmuSettings, QWidget *parent = 0);

signals:

public slots:
};

#endif // AUDIOTAB_H
