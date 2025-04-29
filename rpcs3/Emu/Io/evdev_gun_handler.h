#pragma once
#ifdef HAVE_LIBEVDEV

#include <map>
#include "Utilities/mutex.h"

enum class gun_button
{
	btn_left,
	btn_right,
	btn_middle,
	btn_1,
	btn_2,
	btn_3,
	btn_4,
	btn_5,
	btn_6,
	btn_7,
	btn_8
};

class evdev_gun_handler
{
public:
	evdev_gun_handler();
	~evdev_gun_handler();

	bool init();

	bool is_init() const;
	u32 get_num_guns() const;
	int get_button(u32 gunno, gun_button button) const;
	int get_axis_x(u32 gunno) const;
	int get_axis_y(u32 gunno) const;
	int get_axis_x_max(u32 gunno) const;
	int get_axis_y_max(u32 gunno) const;

	void poll(u32 index);

	shared_mutex mutex;

private:
	atomic_t<bool> m_is_init{false};
	struct udev* m_udev = nullptr;

	struct evdev_axis
	{
		int value = 0;
		int min = 0;
		int max = 0;
	};

	struct evdev_gun
	{
		struct libevdev* device = nullptr;
		std::map<int, int> buttons;
		std::map<int, evdev_axis> axis;
	};

	std::vector<evdev_gun> m_devices;
};

#endif
