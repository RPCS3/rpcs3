// This makes debugging on windows less painful
//#define HAVE_LIBEVDEV

#ifdef HAVE_LIBEVDEV

#include "evdev_gun_handler.h"
#include <libudev.h>
#include <fcntl.h>
#include <linux/input.h>

LOG_CHANNEL(evdev_log, "evdev");

evdev_gun_handler* evdev_gun_handler_instance = nullptr;

evdev_gun_handler* evdev_gun_handler::getInstance()
{
	if (evdev_gun_handler_instance != nullptr)
		return evdev_gun_handler_instance;
	evdev_gun_handler_instance = new evdev_gun_handler();
	evdev_gun_handler_instance->init();
	return evdev_gun_handler_instance;
}

struct event_udev_entry
{
	const char* devnode;
	struct udev_list_entry* item;
};

int event_isNumber(const char* s)
{
	size_t n;

	if (strlen(s) == 0)
	{
		return 0;
	}

	for (n = 0; n < strlen(s); n++)
	{
		if (!(s[n] == '0' || s[n] == '1' || s[n] == '2' || s[n] == '3' || s[n] == '4' ||
				s[n] == '5' || s[n] == '6' || s[n] == '7' || s[n] == '8' || s[n] == '9'))
			return 0;
	}
	return 1;
}

// compare /dev/input/eventX and /dev/input/eventY where X and Y are numbers
int event_strcmp_events(const char* x, const char* y)
{

	// find a common string
	int n, common, is_number;
	int a, b;

	n = 0;
	while (x[n] == y[n] && x[n] != '\0' && y[n] != '\0')
	{
		n++;
	}
	common = n;

	// check if remaining string is a number
	is_number = 1;
	if (event_isNumber(x + common) == 0)
		is_number = 0;
	if (event_isNumber(y + common) == 0)
		is_number = 0;

	if (is_number == 1)
	{
		a = atoi(x + common);
		b = atoi(y + common);

		if (a == b)
			return 0;
		if (a < b)
			return -1;
		return 1;
	}
	else
	{
		return strcmp(x, y);
	}
}

/* Used for sorting devnodes to appear in the correct order */
static int sort_devnodes(const void* a, const void* b)
{
	const struct event_udev_entry* aa = static_cast<const struct event_udev_entry*>(a);
	const struct event_udev_entry* bb = static_cast<const struct event_udev_entry*>(b);
	return event_strcmp_events(aa->devnode, bb->devnode);
}

evdev_gun_handler::evdev_gun_handler()
{
	m_is_init = false;
	m_ndevices = 0;
}

evdev_gun_handler::~evdev_gun_handler()
{
	for (int i = 0; i < m_ndevices; i++)
	{
		close(m_devices[i]);
	}
	if (m_udev != nullptr)
		udev_unref(m_udev);
	m_ndevices = 0;
	m_is_init = false;
	evdev_log.notice("Lightgun: Shutdown udev initialization");
}

int evdev_gun_handler::getButton(int gunno, int button)
{
	return m_devices_buttons[gunno][button];
}

int evdev_gun_handler::getAxisX(int gunno)
{
	return m_devices_axis[gunno][0][EVDEV_GUN_AXIS_VALS_CURRENT] - m_devices_axis[gunno][0][EVDEV_GUN_AXIS_VALS_MIN];
}

int evdev_gun_handler::getAxisY(int gunno)
{
	return m_devices_axis[gunno][1][EVDEV_GUN_AXIS_VALS_CURRENT] - m_devices_axis[gunno][1][EVDEV_GUN_AXIS_VALS_MIN];
}

int evdev_gun_handler::getAxisXMax(int gunno)
{
	return m_devices_axis[gunno][0][EVDEV_GUN_AXIS_VALS_MAX] - m_devices_axis[gunno][0][EVDEV_GUN_AXIS_VALS_MIN];
}

int evdev_gun_handler::getAxisYMax(int gunno)
{
	return m_devices_axis[gunno][1][EVDEV_GUN_AXIS_VALS_MAX] - m_devices_axis[gunno][1][EVDEV_GUN_AXIS_VALS_MIN];
}

void evdev_gun_handler::pool()
{
	struct input_event input_events[32];
	int j, len;

	for (int i = 0; i < m_ndevices; i++)
	{
		while ((len = read(m_devices[i], input_events, sizeof(input_events))) > 0)
		{
			len /= sizeof(*input_events);
			for (j = 0; j < len; j++)
			{
				if (input_events[j].type == EV_KEY)
				{
					switch (input_events[j].code)
					{
					case BTN_LEFT:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_LEFT] = input_events[j].value;
						break;
					case BTN_RIGHT:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_RIGHT] = input_events[j].value;
						break;
					case BTN_MIDDLE:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_MIDDLE] = input_events[j].value;
						break;
					case BTN_1:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_BTN1] = input_events[j].value;
						break;
					case BTN_2:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_BTN2] = input_events[j].value;
						break;
					case BTN_3:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_BTN3] = input_events[j].value;
						break;
					case BTN_4:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_BTN4] = input_events[j].value;
						break;
					case BTN_5:
						m_devices_buttons[i][EVDEV_GUN_BUTTON_BTN5] = input_events[j].value;
						break;
					}
				}
				else if (input_events[j].type == EV_ABS)
				{
					if (input_events[j].code == ABS_X)
					{
						m_devices_axis[i][0][EVDEV_GUN_AXIS_VALS_CURRENT] = input_events[j].value;
					}
					else if (input_events[j].code == ABS_Y)
					{
						m_devices_axis[i][1][EVDEV_GUN_AXIS_VALS_CURRENT] = input_events[j].value;
					}
				}
			}
		}
	}
}

int evdev_gun_handler::getNumGuns()
{
	return m_ndevices;
}

void evdev_gun_handler::shutdown()
{
	if (evdev_gun_handler_instance != nullptr)
	{
		delete evdev_gun_handler_instance;
		evdev_gun_handler_instance = nullptr;
	}
}

bool evdev_gun_handler::init()
{
	if (m_is_init)
		return true;

	struct udev_enumerate* enumerate;
	struct udev_list_entry* devs = nullptr;
	struct udev_list_entry* item = nullptr;
	unsigned sorted_count = 0;
	struct event_udev_entry sorted[8]; // max devices
	unsigned int i;

	evdev_log.notice("Lightgun: Begin udev initialization");

	m_udev = udev_new();
	if (m_udev == nullptr)
		return false;

	enumerate = udev_enumerate_new(m_udev);

	if (enumerate != nullptr)
	{
		udev_enumerate_add_match_property(enumerate, "ID_INPUT_MOUSE", "1");
		udev_enumerate_add_match_subsystem(enumerate, "input");
		udev_enumerate_scan_devices(enumerate);
		devs = udev_enumerate_get_list_entry(enumerate);

		for (item = devs; item; item = udev_list_entry_get_next(item))
		{
			const char* name = udev_list_entry_get_name(item);
			struct udev_device* dev = udev_device_new_from_syspath(m_udev, name);
			const char* devnode = udev_device_get_devnode(dev);

			if (devnode != nullptr && sorted_count < 8)
			{
				sorted[sorted_count].devnode = devnode;
				sorted[sorted_count].item = item;
				sorted_count++;
			}
			else
			{
				udev_device_unref(dev);
			}
		}

		/* Sort the udev entries by devnode name so that they are
		 * created in the proper order */
		qsort(sorted, sorted_count,
			sizeof(struct event_udev_entry), sort_devnodes);

		for (i = 0; i < sorted_count; i++)
		{
			if (m_ndevices >= EVDEV_GUN_MAX_DEVICES)
				break;

			const char* name = udev_list_entry_get_name(sorted[i].item);
			/* Get the filename of the /sys entry for the device
			 * and create a udev_device object (dev) representing it. */
			struct udev_device* dev = udev_device_new_from_syspath(m_udev, name);
			evdev_log.notice("Lightgun: found device %s", name);
			const char* devnode = udev_device_get_devnode(dev);

			if (devnode)
			{
				struct input_absinfo absx, absy;
				int valid = 0;
				int fd = open(devnode, O_RDONLY | O_NONBLOCK);
				if (fd != -1)
				{
					for (int b = 0; b < EVDEV_GUN_BUTTON_MAX; b++)
					{
						m_devices_buttons[m_ndevices][b] = 0;
					}
					for (int a = 0; a < 3; a++)
					{
						m_devices_axis[m_ndevices][0][a] = 0;
						m_devices_axis[m_ndevices][1][a] = 0;
					}
					if (ioctl(fd, EVIOCGABS(ABS_X), &absx) >= 0)
					{
						if (ioctl(fd, EVIOCGABS(ABS_Y), &absy) >= 0)
						{
							evdev_log.notice("Lightgun: device %s, absx(%i, %i), absy(%i, %i)", name, absx.minimum, absx.maximum, absy.minimum, absy.maximum);

							m_devices_axis[m_ndevices][0][EVDEV_GUN_AXIS_VALS_MIN] = absx.minimum;
							m_devices_axis[m_ndevices][0][EVDEV_GUN_AXIS_VALS_MAX] = absx.maximum;
							m_devices_axis[m_ndevices][1][EVDEV_GUN_AXIS_VALS_MIN] = absy.minimum;
							m_devices_axis[m_ndevices][1][EVDEV_GUN_AXIS_VALS_MAX] = absy.maximum;
							valid = 1;
						}
					}

					if (valid == 1)
					{
						evdev_log.notice("Lightgun: device %s set as gun %i", name, m_ndevices);
						m_devices[m_ndevices++] = fd;
					}
					else
					{
						evdev_log.notice("Lightgun: device %s not valid. abs_x and abs_y not found", name);
						close(fd);
					}
				}
			}
			udev_device_unref(dev);
		}
		udev_enumerate_unref(enumerate);
		return true;
	}

	m_is_init = true;
	return true;
}

#endif
