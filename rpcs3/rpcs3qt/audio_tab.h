#ifndef AUDIOTAB_H
#define AUDIOTAB_H

#include "emu_settings.h"

#include <QWidget>

#include <memory>

class audio_tab : public QWidget
{
	Q_OBJECT

public:
	explicit audio_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);

signals:

public slots:
};

#endif // AUDIOTAB_H
