#pragma once
#ifdef HAVE_LIBEVDEV

#include "util/types.hpp"
#include "util/logs.hpp"

#define EVDEV_GUN_MAX_DEVICES 8

enum evdev_gun_buttons
{
	EVDEV_GUN_BUTTON_LEFT,
	EVDEV_GUN_BUTTON_RIGHT,
	EVDEV_GUN_BUTTON_MIDDLE,
	EVDEV_GUN_BUTTON_BTN1,
	EVDEV_GUN_BUTTON_BTN2,
	EVDEV_GUN_BUTTON_BTN3,
	EVDEV_GUN_BUTTON_BTN4,
	EVDEV_GUN_BUTTON_BTN5,
	EVDEV_GUN_BUTTON_MAX,
};

enum evdev_gun_axis_vals
{
	EVDEV_GUN_AXIS_VALS_MIN,
	EVDEV_GUN_AXIS_VALS_CURRENT,
	EVDEV_GUN_AXIS_VALS_MAX,
};

#define EVDEV_GUN_BUTTON_LEFT 1

class evdev_gun_handler
{
public:
	evdev_gun_handler();
	~evdev_gun_handler();

	static evdev_gun_handler* getInstance();
	static void shutdown();

	bool init();

	int getNumGuns();
	int getButton(int gunno, int button);
	int getAxisX(int gunno);
	int getAxisY(int gunno);
	int getAxisXMax(int gunno);
	int getAxisYMax(int gunno);

	void pool();

private:
	bool m_is_init;
	struct udev* m_udev = nullptr;
	int m_devices[EVDEV_GUN_MAX_DEVICES];
	int m_devices_buttons[EVDEV_GUN_MAX_DEVICES][EVDEV_GUN_BUTTON_MAX];
	int m_devices_axis[EVDEV_GUN_MAX_DEVICES][2][3];
	int m_ndevices;
};

#endif
