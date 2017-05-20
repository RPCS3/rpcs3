#ifndef GRAPHICSTAB_H
#define GRAPHICSTAB_H

#include "emu_settings.h"

#include <QWidget>

#include <memory>
class graphics_tab : public QWidget
{
	Q_OBJECT

public:
	explicit graphics_tab(std::shared_ptr<emu_settings> xemu_settings, QWidget *parent = 0);

signals:

private:
	std::shared_ptr<emu_settings> xemu_settings;
};

#endif // GRAPHICSTAB_H
